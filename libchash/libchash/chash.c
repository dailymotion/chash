// Consistent hashing library
// pyke@dailymotion.com - 05/2009

// Mandatory includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "chash.h"

// Private defines
#define CHASH_MAGIC     (0x48414843)
#define CHASH_REPLICAS  (128)

// Static variables
static u_char chash_rand_initialized = 0;

// MurmurHash2 light implementation
static u_int32_t chash_mmhash2(const void *key, int key_size)
{
    const u_int32_t magic  = 0x5bd1e995;
    const int       rotate = 24;
    const u_char    *data  = (const u_char *)key;
    u_int32_t       hash   = 0x4d4d4832 ^ key_size;

    if (key_size < 0)
    {
        key_size = strlen((char *)data);
    }
    while (key_size >= 4)
    {
        u_int32_t value = *(u_int32_t *)data;
        value    *= magic;
        value    ^= value >> rotate;
        value    *= magic;
        hash     *= magic;
        hash     ^= value;
        data     += 4;
        key_size -= 4;
    }
    switch (key_size)
    {
        case 3: hash ^= data[2] << 16;
        case 2: hash ^= data[1] << 8;
        case 1: hash ^= data[0];
                hash *= magic;
    }
    hash ^= hash >> 13;
    hash *= magic;
    hash ^= hash >> 15;
    return hash;
}

// Compute continuum and block future modifications
static int chash_sort32(const void *element1, const void *element2)
{
    return (*(u_int32_t *)element1 > *(u_int32_t *)element2) ? 1 : -1;
}
static int chash_freeze(CHASH_CONTEXT *context)
{
    u_char weight, replica;
    char   target[128];
    int    index, position = 0;

    if (! context)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if (context->magic != CHASH_MAGIC)
    {
        return CHASH_ERROR_NOT_INITIALIZED;
    }
    if (context->frozen)
    {
        return context->items_count;
    }
    if (! context->targets_count)
    {
        return CHASH_ERROR_NOT_FOUND;
    }
    if (context->continuum)
    {
        free(context->continuum);
        context->continuum = NULL;
    }
    context->items_count = 0;
    for (index = 0; index < context->targets_count; index ++)
    {
        context->items_count += (context->targets[index].weight * CHASH_REPLICAS);
    }
    if (! (context->continuum = (CHASH_ITEM *)calloc(context->items_count, sizeof(CHASH_ITEM))))
    {
        return CHASH_ERROR_MEMORY;
    }
    for (index = 0; index < context->targets_count; index ++)
    {
        for (weight = 0; weight < context->targets[index].weight; weight ++)
        {
            for (replica = 0; replica < CHASH_REPLICAS; replica ++)
            {
                snprintf(target, sizeof(target) - 1, "%s%d%d", context->targets[index].name, weight, replica);
                context->continuum[position].hash   = chash_mmhash2(target, -1);
                context->continuum[position].target  = index;
                position ++;
            }
        }
    }
    qsort(context->continuum, context->items_count, sizeof(CHASH_ITEM), chash_sort32);
    context->frozen = 1;
    return context->items_count;
}

// Discard continuum and allow modifications back
static int chash_unfreeze(CHASH_CONTEXT *context)
{
    if (! context)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if (context->magic != CHASH_MAGIC)
    {
        return CHASH_ERROR_NOT_INITIALIZED;
    }
    context->frozen = 0;
    return CHASH_ERROR_DONE;
}

// Initialize context
int chash_initialize(CHASH_CONTEXT *context, u_char force)
{
    if (! context)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if (context->magic == CHASH_MAGIC && ! force)
    {
        return CHASH_ERROR_ALREADY_INITIALIZED;
    }
    memset(context, 0, sizeof(CHASH_CONTEXT));
    context->magic = CHASH_MAGIC;
    return CHASH_ERROR_DONE;
}

// Terminate context (free memory)
int chash_terminate(CHASH_CONTEXT *context, u_char force)
{
    u_int16_t index;

    if (! context)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if (context->magic != CHASH_MAGIC && ! force)
    {
        return CHASH_ERROR_NOT_INITIALIZED;
    }
    if (context->targets)
    {
        for (index = 0; index < context->targets_count; index ++)
        {
            free(context->targets[index].name);
        }
        free(context->targets);
    }
    if (context->continuum)
    {
        free(context->continuum);
    }
    if (context->lookups)
    {
        free(context->lookups);
    }
    if (context->lookup)
    {
        free(context->lookup);
    }
    memset(context, 0, sizeof(CHASH_CONTEXT));
    return CHASH_ERROR_DONE;
}

// Add target to context
int chash_add_target(CHASH_CONTEXT *context, const char *target, u_char weight)
{
    u_int16_t index = 0;
    int       status;

    if (! target)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if ((status = chash_unfreeze(context)) < 0)
    {
        return status;
    }
    weight = weight < 1 ? 1 : weight;
    weight = weight > 10 ? 10 : weight;
    if (context->targets)
    {
        for (index = 0; index < context->targets_count; index ++)
        {
            if (! strcmp(target, context->targets[index].name))
            {
                context->targets[index].weight = weight;
                break;
            }
        }
    }
    if (index == context->targets_count)
    {
        if (! (context->targets = (CHASH_TARGET *)realloc(context->targets,
                                                          (context->targets_count + 1) * sizeof(CHASH_TARGET))))
        {
            context->targets_count = 0;
            return CHASH_ERROR_MEMORY;
        }
        if (! (context->targets[context->targets_count].name = strdup(target)))
        {
            return CHASH_ERROR_MEMORY;
        }
        context->targets[context->targets_count].weight = weight;
        context->targets_count ++;
    }
    return CHASH_ERROR_DONE;
}

// Remove target from context
int chash_remove_target(CHASH_CONTEXT *context, const char *target)
{
    u_int16_t index;
    int       status;

    if (! target)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if ((status = chash_unfreeze(context)) < 0)
    {
        return status;
    }
    if (context->targets)
    {
        for (index = 0; index < context->targets_count; index ++)
        {
            if (! strcmp(target, context->targets[index].name))
            {
                memmove(&(context->targets[index]), &(context->targets[index + 1]),
                        sizeof(CHASH_TARGET) * (context->targets_count - index - 1));
                context->targets_count --;
                return CHASH_ERROR_DONE;
            }
        }
    }
    return CHASH_ERROR_NOT_FOUND;
}

// Clear all targets from context
int chash_clear_targets(CHASH_CONTEXT *context)
{
    u_int16_t index;
    int       status;

    if ((status = chash_unfreeze(context)) < 0)
    {
        return status;
    }
    if (context->targets)
    {
        for (index = 0; index < context->targets_count; index ++)
        {
            free(context->targets[index].name);
        }
        free(context->targets);
        context->targets       = NULL;
        context->targets_count = 0;
    }
    return CHASH_ERROR_DONE;
}

// Return targets count within context
int chash_targets_count(CHASH_CONTEXT *context)
{
    if (! context)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if (context->magic != CHASH_MAGIC)
    {
        return CHASH_ERROR_NOT_INITIALIZED;
    }
    return context->targets_count;
}

// Save context into a memory chunk (implicit freeze)
int chash_serialize(CHASH_CONTEXT *context, u_char **output)
{
    int status, index, size, position = 0, length;

    if (! context || ! output)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if ((status = chash_freeze(context)) < 0)
    {
        return status;
    }
    size = (2 * sizeof(u_int32_t)) + sizeof(u_int16_t);
    for (index = 0; index < context->targets_count; index ++)
    {
        size += strlen(context->targets[index].name) + 2;
    }
    size += sizeof(u_int32_t) + (context->items_count * sizeof(CHASH_ITEM));
    if (! (*output = calloc(1, size)))
    {
        return CHASH_ERROR_MEMORY;
    }
    *(u_int32_t *)((*output) + position) = size; position += sizeof(u_int32_t);
    *(u_int32_t *)((*output) + position) = CHASH_MAGIC; position += sizeof(u_int32_t);
    *(u_int16_t *)((*output) + position) = context->targets_count; position += sizeof(u_int16_t);
    for (index = 0; index < context->targets_count; index ++)
    {
        *((*output) + position) = context->targets[index].weight; position += sizeof(u_char);
        length = strlen(context->targets[index].name) + 1;
        memcpy((*output) + position, context->targets[index].name, length); position += length;
    }
    *(u_int32_t *)((*output) + position) = context->items_count; position += sizeof(u_int32_t);
    memcpy((*output) + position, context->continuum, context->items_count * sizeof(CHASH_ITEM));
    return size;
}

// Restore context from a memory chunk (implicit freeze)
int chash_unserialize(CHASH_CONTEXT *context, const u_char *input, u_int32_t size)
{
    int index, position = (2 * sizeof(u_int32_t)) + sizeof(u_int16_t);

    if (! context || ! input || size < (3 * sizeof(u_int32_t)) + sizeof(u_int16_t))
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if (*(u_int32_t *)input != size || *(u_int32_t *)(input + sizeof(u_int32_t)) != CHASH_MAGIC)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    chash_terminate(context, 0);
    context->targets_count = *(u_int16_t *)(input + (2 * sizeof(u_int32_t)));
    if (! context->targets_count)
    {
        return CHASH_ERROR_NOT_FOUND;
    }
    if (! (context->targets = (CHASH_TARGET *)malloc(context->targets_count * sizeof(CHASH_TARGET))))
    {
        context->targets_count = 0;
        return CHASH_ERROR_MEMORY;
    }
    for (index = 0; index < context->targets_count; index ++)
    {
        context->targets[index].weight = *(input + position);
        context->targets[index].name   = strdup((const char *)(input + position + 1));
        position += sizeof(u_char) + strlen((const char *)(input + position + 1)) + 1;
    }
    context->items_count = *(u_int32_t *)(input + position);
    if (! (context->continuum = (CHASH_ITEM *)malloc(context->items_count * sizeof(CHASH_ITEM))))
    {
        context->items_count = 0;
        return CHASH_ERROR_MEMORY;
    }
    memcpy(context->continuum, input + position + sizeof(u_int32_t), context->items_count * sizeof(CHASH_ITEM));
    context->magic  = CHASH_MAGIC;
    context->frozen = 1;
    return context->items_count;
}

// Save context into a file (implicit freeze)
int chash_file_serialize(CHASH_CONTEXT *context, const char *path)
{
    u_char *serialized;
    int    status, output;

    if (! context || ! path || ! *path)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if ((status = chash_serialize(context, &serialized)) < 0)
    {
        return status;
    }
    if ((output = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0755)) < 0)
    {
        free(serialized);
        return CHASH_ERROR_IO;
    }
    if (write(output, serialized, status) != status)
    {
        close(output);
        free(serialized);
        return CHASH_ERROR_IO;
    }
    close(output);
    free(serialized);
    return status;
}

// Restore context from a file (implicit freeze)
int chash_file_unserialize(CHASH_CONTEXT *context, const char *path)
{
    struct stat info;
    u_char      *serialized;
    int         status, input;

    if (! context || ! path || ! *path)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if (stat(path, &info) < 0 || (input = open(path, O_RDONLY)) < 0)
    {
        return CHASH_ERROR_IO;
    }
    if (! (serialized = mmap(NULL, info.st_size, PROT_READ, MAP_PRIVATE, input, 0)))
    {
        close(input);
        return CHASH_ERROR_IO;
    }
    status = chash_unserialize(context, serialized, info.st_size);
    munmap(serialized, info.st_size);
    close(input);
    return status;
}

// Perform a lookup (implicit freeze)
static int chash_sort16(const void *element1, const void *element2)
{
    return (*(u_int16_t *)element1 > *(u_int16_t *)element2) ? 1 : -1;
}
int chash_lookup(CHASH_CONTEXT *context, const char *candidate, u_int16_t count, char ***output)
{
    u_int32_t hash, start = 0, range;
    u_int16_t rank = 1, index;
    int       status;

    if (! context || ! candidate || ! *candidate)
    {
        return CHASH_ERROR_INVALID_PARAMETER;
    }
    if ((status = chash_freeze(context)) < 0)
    {
        return status;
    }
    if (! (context->lookups = (CHASH_LOOKUP *)realloc(context->lookups, context->targets_count * sizeof(CHASH_LOOKUP))))
    {
        return CHASH_ERROR_MEMORY;
    }
    if (! (context->lookup = (char **)realloc(context->lookup, context->targets_count * sizeof(char *))))
    {
        return CHASH_ERROR_MEMORY;
    }
    memset(context->lookups, 0, context->targets_count * sizeof(CHASH_LOOKUP));
    memset(context->lookup, 0, context->targets_count * sizeof(char *));
    hash = chash_mmhash2(candidate, -1);
    if (hash > context->continuum[0].hash && hash <= context->continuum[context->items_count - 1].hash)
    {
        start = context->items_count / 2;
        range = start / 2;
        while (1)
        {
            if (hash > context->continuum[start].hash && hash <= context->continuum[start + 1].hash)
            {
                break;
            }
            start += (hash > context->continuum[start].hash) ? range : -range;
            range /= 2;
            range  = range < 1 ? 1 : range;
        }
    }
    count = (count < 1) ? 1 : count;
    count = (count > context->targets_count) ? context->targets_count : count;
    while (rank <= count && start < context->items_count)
    {
        if (! context->lookups[context->continuum[start].target].rank)
        {
            context->lookups[context->continuum[start].target].rank   = rank ++;
            context->lookups[context->continuum[start].target].target = context->continuum[start].target;
        }
        start ++;
    }
    qsort(context->lookups, context->targets_count, sizeof(CHASH_LOOKUP), chash_sort16);
    for (index = context->targets_count - count; index < context->targets_count; index ++)
    {
        context->lookup[index - (context->targets_count - count)] = context->targets[context->lookups[index].target].name;
    }
    if (output)
    {
        *output = context->lookup;
    }
    return count;
}

// Perform a lookup and randomly balance among results
int chash_lookup_balance(CHASH_CONTEXT *context, const char *candidate, u_int16_t count, char **output)
{
    int status;

    if ((status = chash_lookup(context, candidate, count, NULL)) < 0)
    {
        return status;
    }
    if (! chash_rand_initialized)
    {
        srand(getpid() + time(NULL));
        chash_rand_initialized = 1;
    }
    if (output)
    {
        *output = context->lookup[rand() % status];
    }
    return CHASH_ERROR_DONE;
}
