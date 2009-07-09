// Consistent hashing PHP class extension
// DM/PYKE - 05/2009

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"

#define CHASH_REPLICAS  (64)

// CHash composite object definition, including module private storage data
typedef struct
{
    zend_object zo;
    u_int32_t   targets_count;
    char        **targets;
    u_int32_t   entries_count;
    u_int32_t   *entries;
    u_int32_t   *selected;
} chash_object;

// CHash class entry
zend_class_entry *chash_class_entry;

// MurmurHash2 light implementation
static u_int32_t mmhash2(const void *key, int key_size)
{
    const u_int32_t magic  = 0x5bd1e995;
    const int       rotate = 24;
    const u_char    *data  = (const u_char *)key;
    u_int32_t       hash   = 0x4d4d4832 ^ key_size;

    if (key_size < 0)
    {
        key_size = strlen(data);
    }
    while (key_size >= 4)
    {
        u_int32_t k = *(u_int32_t *)data;

        k        *= magic;
        k        ^= k >> rotate;
        k        *= magic;
        hash     *= magic;
        hash     ^= k;
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

// Integer-key array sort
static int chash_sort(const void *element1, const void *element2)
{
    return (*(u_int32_t *)element1 > *(u_int32_t *)element2) ? 1 : -1;
}

// CHash method setTargets(array('name1' => weight1 [, ...])) -> long
PHP_METHOD(CHash, setTargets)
{
    HashPosition position;
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    zval         *targets, **value;
    uint         length, index, entries_index = 0, targets_index = 0;
    long         unused, weight;
    char         *target, name[128];

    if (instance->targets_count)
    {
        RETURN_LONG(instance->entries_count)
    }
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &targets) == SUCCESS)
    {
        for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(targets), &position);
             zend_hash_get_current_key_ex(Z_ARRVAL_P(targets), &target, &length, &unused, 0, &position) == HASH_KEY_IS_STRING &&
             zend_hash_get_current_data_ex(Z_ARRVAL_P(targets), (void **) &value, &position) == SUCCESS &&
             Z_TYPE_PP(value) == IS_LONG;
             zend_hash_move_forward_ex(Z_ARRVAL_P(targets), &position))
        {
            weight = Z_LVAL_PP(value);
            weight = weight < 1 ? 1 : weight;
            weight = weight > 10 ? 10 : weight;
            instance->entries_count += weight * CHASH_REPLICAS;
            instance->targets_count ++;
        }
        instance->targets  = (char **)ecalloc(instance->targets_count, sizeof(char *));
        instance->selected = (u_int32_t *)ecalloc(instance->targets_count * 2, sizeof(u_int32_t));
        instance->entries  = (u_int32_t *)ecalloc(instance->entries_count * 2, sizeof(u_int32_t));
        for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(targets), &position);
             zend_hash_get_current_key_ex(Z_ARRVAL_P(targets), &target, &length, &unused, 0, &position) == HASH_KEY_IS_STRING &&
             zend_hash_get_current_data_ex(Z_ARRVAL_P(targets), (void **) &value, &position) == SUCCESS &&
             Z_TYPE_PP(value) == IS_LONG;
             zend_hash_move_forward_ex(Z_ARRVAL_P(targets), &position))
        {
            instance->targets[targets_index] = estrdup(target);
            weight = Z_LVAL_PP(value);
            weight = weight < 1 ? 1 : weight;
            weight = weight > 10 ? 10 : weight;
            for (index = 0; index < weight * CHASH_REPLICAS; index ++)
            {
                snprintf(name, sizeof(name) - 1, "%s_%03d", target, index + 1);
                instance->entries[entries_index]     = mmhash2(name, -1);
                instance->entries[entries_index + 1] = targets_index;
                entries_index += 2;
            }
            targets_index ++;
        }
        qsort(instance->entries, instance->entries_count, sizeof(u_int32_t) * 2, chash_sort);
    }
    RETURN_LONG(instance->entries_count)
}

// Actually perform the search
static int chash_search(chash_object *instance, char *value, u_int32_t count)
{
    u_int32_t hash, start_index = 0, index, rank = 1;

    hash = mmhash2(value, -1);
    if (hash > instance->entries[0] && hash <= instance->entries[(instance->entries_count - 1) * 2])
    {
        for (start_index = 0; start_index < instance->entries_count - 1; start_index ++)
        {
            if (hash > instance->entries[start_index * 2] && hash <= instance->entries[(start_index + 1) * 2])
            {
                break;
            }
        }
    }
    memset(instance->selected, 0, instance->targets_count * sizeof(u_int32_t) * 2);
    instance->selected[instance->entries[(start_index * 2) + 1] * 2] = rank ++;
    count = (count < 1) ? 1 : count;
    count = (count > instance->targets_count) ? instance->targets_count : count;;
    while (rank <= count && start_index < instance->entries_count)
    {
        if (! instance->selected[instance->entries[(start_index * 2) + 1] * 2])
        {
            instance->selected[instance->entries[(start_index * 2) + 1] * 2] = rank ++;
        }
        start_index ++;
    }
    for (index = 0; index < instance->targets_count; index ++)
    {
        instance->selected[(index * 2) + 1] = index;
    }
    qsort(instance->selected, instance->targets_count, sizeof(u_int32_t) * 2, chash_sort);
    return count;
}

// CHash method lookupList(name [, count]) -> array
PHP_METHOD(CHash, lookupList)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    u_int32_t    hash, count = 1, length, start_index = 0, index, rank = 1;
    char         *value;

    array_init(return_value);
    if (! instance->targets_count)
    {
        return;
    }
    if (instance->targets_count == 1 || zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &value, &length, &count) != SUCCESS || length == 0)
    {
        add_next_index_string(return_value, instance->targets[0], 1);
        return;
    }
    count = chash_search(instance, value, count);
    for (index = instance->targets_count - count; index < instance->targets_count; index ++)
    {
        add_next_index_string(return_value, instance->targets[instance->selected[(index * 2) + 1]], 1);
    }
}

// CHash method lookupBalance(name [, count]) -> string
PHP_METHOD(CHash, lookupBalance)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    u_int32_t    count = 1, length;
    char         *value;

    if (! instance->targets_count)
    {
        RETURN_STRING("", 1)
    }
    if (instance->targets_count == 1 || zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &value, &length, &count) != SUCCESS || length == 0)
    {
        RETURN_STRING(instance->targets[0], 1)
    }
    count = (count < 1) ? 1 : count;
    count = (count > instance->targets_count) ? instance->targets_count : count;
    count = (rand() % count) + 1;
    count = chash_search(instance, value, count);
    RETURN_STRING(instance->targets[instance->selected[((instance->targets_count - 1) * 2) + 1]], 1)
}

// CHash module v-table
static function_entry chash_class_methods[] =
{
    PHP_ME(CHash, setTargets, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, lookupList, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, lookupBalance, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

// CHash extended object destructor
static void chash_free(chash_object *instance TSRMLS_DC)
{
    int index;

    zend_object_std_dtor(&(instance->zo) TSRMLS_CC);
    if (instance->targets_count)
    {
        for (index = 0; index < instance->targets_count; index ++)
        {
            efree(instance->targets[index]);
        }
        efree(instance->targets);
        instance->targets        = NULL;
        instance->targets_count = 0;
    }
    if (instance->entries_count)
    {
        efree(instance->entries);
        instance->entries       = NULL;
        instance->entries_count = 0;
    }
    if (instance->selected)
    {
        efree(instance->selected);
        instance->selected = NULL;
    }
    efree(instance);
    instance = NULL;
}

// CHash extended object constructor
zend_object_value chash_allocate(zend_class_entry *class_entry TSRMLS_DC)
{
    zend_object_value value;
    chash_object      *instance;

    instance = ecalloc(1, sizeof(chash_object));
    zend_object_std_init(&(instance->zo), class_entry TSRMLS_CC);
    value.handle = zend_objects_store_put(instance, (zend_objects_store_dtor_t)zend_objects_destroy_object,
                                                    (zend_objects_free_object_storage_t)chash_free, NULL TSRMLS_CC);
    value.handlers = zend_get_std_object_handlers();
    return value;
}

// CHash module global initialization
PHP_MINIT_FUNCTION(chash)
{
    zend_class_entry class_entry;

    srand(getpid() + time(NULL));
    INIT_CLASS_ENTRY(class_entry, "CHash", chash_class_methods);
    chash_class_entry = zend_register_internal_class(&class_entry TSRMLS_CC);
    chash_class_entry->create_object = chash_allocate;
    return SUCCESS;
}

// CHash module definition
zend_module_entry chash_module_entry =
{
    STANDARD_MODULE_HEADER,
    "chash",
    NULL,
    PHP_MINIT(chash),
    NULL,
    NULL,
    NULL,
    NULL,
    "1.0",
    STANDARD_MODULE_PROPERTIES
};
#ifdef COMPILE_DL_CHASH
ZEND_GET_MODULE(chash)
#endif
