// Consistent hashing library
// pyke@dailymotion.com - 05/2009

// Mandatory includes
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include "libchash.h"

// Defines
#define TARGETS     (100)
#define CANDIDATES  (200000)
#define FREEZEPATH  "/tmp/chash.freeze"

// Helper functions
static struct timeval time_start, time_end;
static int            test_steps = 0;
static int            test_status = 0;
static char           test_message[1024];

static void test_start(char *title)
{
    int index;

    printf("%s ", title);
    for (index = 0; index < 50 - strlen(title); index++)
    {
        printf(".");
    }
    printf(" ");
    fflush(stdout);
    test_steps    = 0;
    test_status   = 0;
    *test_message = 0;
    gettimeofday(&time_start, NULL);
}
static void test_step(int status, char *format, ...)
{
    va_list arguments;

    test_steps ++;
    if (! test_status && status)
    {
        test_status = status;
        if (format)
        {
            va_start(arguments, format);
            vsnprintf(test_message, sizeof(test_message) - 1, format, arguments);
            va_end(arguments);
        }
    }
}
static void test_end(char *format, ...)
{
    va_list arguments;
    double  time_spent;

    gettimeofday(&time_end, NULL);
    time_spent = (((double)(time_end.tv_sec - time_start.tv_sec)) * 1000) +
                 (((double)(time_end.tv_usec - time_start.tv_usec)) / 1000);
    if (test_status)
    {
        printf("fail (%d", test_status);
    }
    else
    {
        printf("ok (%.3fms", time_spent);
        if (test_steps > 1)
        {
            printf(" - %.3fms/step", time_spent / test_steps);
        }
    }
    if (! test_status && ! *test_message && format)
    {
        va_start(arguments, format);
        vsnprintf(test_message, sizeof(test_message) - 1, format, arguments);
        va_end(arguments);
    }
    if (*test_message)
    {
        printf(" - %s", test_message);
    }
    printf(")\n");
}

// Main program
int main(int argc, char **argv)
{
    CHASH_CONTEXT context;
    double        mean, deviation;
    int           index, status, count, size1, size2, target, lookups[TARGETS];
    u_char        *serialized1, *serialized2;
    char          buffer[32], **lookup, *balance;

    printf("\n");

    test_start("initialize");
    test_step(chash_initialize(&context, 0), NULL);
    test_end(NULL);

    test_start("add_target");
    for (index = 1; index <= TARGETS; index ++)
    {
        sprintf(buffer, "target%03d", index);
        test_step(chash_add_target(&context, buffer, 1), NULL);
    }
    test_step(chash_targets_count(&context) == TARGETS ? 0 : -1, "invalid targets count %d", chash_targets_count(&context));
    test_end("added %d targets", chash_targets_count(&context));

    test_start("remove_target");
    for (index = 10; index < 30; index ++)
    {
        sprintf(buffer, "target%03d", index);
        test_step(chash_remove_target(&context, buffer), NULL);
    }
    test_step(chash_targets_count(&context) == (TARGETS - 20) ? 0 : -1,
              "invalid targets count %d", chash_targets_count(&context));
    test_end("targets count is now %d", chash_targets_count(&context));

    test_start("clean_target");
    test_step(chash_clear_targets(&context), "targets count is still %d", chash_targets_count(&context));
    test_end(NULL);
    for (index = 1; index <= TARGETS; index ++)
    {
        sprintf(buffer, "target%03d", index);
        chash_add_target(&context, buffer, 1);
    }

    test_start("freeze");
    test_step((status = chash_freeze(&context)) < 0 ? status : 0, NULL);
    test_end("continuum count is %d", chash_freeze(&context));

    test_start("unfreeze");
    test_step((status = chash_unfreeze(&context)) < 0 ? status : 0, NULL);
    test_end(NULL);

    test_start("serialize");
    test_step((size1 = chash_serialize(&context, &serialized1)) < 0 ? size1 : 0, NULL);
    test_end("serialized size is %d bytes", size1);

    chash_initialize(&context, 1);
    test_start("unserialize");
    test_step((count = chash_unserialize(&context, serialized1, size1)) < 0 ? count : 0, NULL);
    test_end("continuum count is %d", count);

    test_start("serialize coherency");
    test_step((size2 = chash_serialize(&context, &serialized2)) < 0 ? size2 : 0, NULL);
    test_step(size1 < 0 || size2 < 0 || size1 != size2 || memcmp(serialized1, serialized2, size1) ? -1 : 0, NULL);
    test_end(NULL);

    test_start("file_serialize");
    unlink(FREEZEPATH);
    test_step((size1 = chash_file_serialize(&context, FREEZEPATH)) < 0 ? size1 : 0, NULL);
    test_end("serialized size is %d bytes", size1);

    chash_initialize(&context, 1);
    test_start("file_unserialize");
    test_step((count = chash_file_unserialize(&context, FREEZEPATH)) < 0 ? count : 0, NULL);
    test_end("continuum count is %d", count);

    test_start("file serialize coherency");
    test_step((size2 = chash_serialize(&context, &serialized2)) < 0 ? size2 : 0, NULL);
    test_step(size1 < 0 || size2 < 0 || size1 != size2 || memcmp(serialized1, serialized2, size1) ? -1 : 0, NULL);
    test_end(NULL);

    test_start("lookup");
    memset(lookups, 0, sizeof(lookups));
    for (index = 0; index < CANDIDATES; index ++)
    {
        sprintf(buffer, "candidate%07d", index);
        test_step((count = chash_lookup(&context, buffer, 1, &lookup)) <= 0 ? count : 0, NULL);
        if (count >= 1 && sscanf(lookup[0], "target%d", &target) == 1)
        {
            lookups[target - 1] ++;
        }
    }
    count = 0; deviation = 0;
    for (index = 0; index < TARGETS; index ++)
    {
        count += lookups[index];
    }
    mean = (double)count / TARGETS;
    for (index = 0; index < TARGETS; index ++)
    {
        deviation += pow((double)lookups[index] - mean, 2);
    }
    test_end("deviation is %.2f", sqrt(deviation / TARGETS));

    test_start("lookup_balance");
    memset(lookups, 0, sizeof(lookups));
    for (index = 0; index < CANDIDATES; index ++)
    {
        test_step((status = chash_lookup_balance(&context, "candidate001", 10, &balance)), NULL);
        if (! status && sscanf(balance, "target%d", &target) == 1)
        {
            lookups[target - 1] ++;
        }
    }
    count = 0; deviation = 0;
    for (index = 0; index < TARGETS; index ++)
    {
        count += lookups[index];
    }
    mean = (double)count / 10;
    for (index = 0; index < TARGETS; index ++)
    {
        if (lookups[index])
        {
            deviation += pow((double)lookups[index] - mean, 2);
        }
    }
    test_end("deviation is %.2f", sqrt(deviation / 10));

    test_start("terminate");
    test_step(chash_terminate(&context, 0), NULL);
    test_end(NULL);

    printf("\n");

    return 0;
}
