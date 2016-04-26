// Consistent hashing library PHP extension
// pyke@dailymotion.com - 05/2009

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include "zend_exceptions.h"
#include "chash.h"

// CHash composite object definition
typedef struct
{
    zend_object   zo;
    u_char        use_exceptions;
    CHASH_CONTEXT context;
} chash_object;

// CHash class entry
zend_class_entry *chash_ce;

static zend_object_handlers chash_object_handlers;

// CHash errors/exceptions management
static zend_class_entry *chash_memory_exception,
                        *chash_io_exception,
                        *chash_invalid_parameter_exception,
                        *chash_not_found_exception;

static inline chash_object *php_chash_fetch_object(zend_object *obj) {
    return (chash_object *)((char *)obj - XtOffsetOf(chash_object, zo));
}
#define Z_CHASH_OBJ_P(zv) php_chash_fetch_object(Z_OBJ_P(getThis()));

static int chash_return(chash_object *instance, int status)
{
    if (status < 0 && instance->use_exceptions)
    {
        switch (status)
        {
            case CHASH_ERROR_MEMORY:
                 zend_throw_exception(chash_memory_exception, "Memory allocation error", CHASH_ERROR_MEMORY);
                 break;

            case CHASH_ERROR_IO:
                 zend_throw_exception(chash_io_exception, "File I/O error", CHASH_ERROR_IO);
                 break;

            case CHASH_ERROR_INVALID_PARAMETER:
                 zend_throw_exception(chash_invalid_parameter_exception, "Invalid parameter", CHASH_ERROR_INVALID_PARAMETER);
                 break;

            case CHASH_ERROR_NOT_FOUND:
                 zend_throw_exception(chash_not_found_exception, "No element found", CHASH_ERROR_NOT_FOUND);
                 break;

            default:
                 zend_throw_exception(NULL, "Unknown CHash exception", 0);
        }
    }
    return status;
}

// CHash method useExceptions(bool) -> bool
PHP_METHOD(CHash, useExceptions)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    u_char       use;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b", &use) == SUCCESS)
    {
        instance->use_exceptions = use;
    }
    RETVAL_BOOL(instance->use_exceptions);
}

// CHash method addTarget(<target>[, <weight>]) -> long
PHP_METHOD(CHash, addTarget)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    char         *target;
    size_t       length;
    long         weight = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &target, &length, &weight) != SUCCESS || length == 0)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER));
    }
    RETVAL_LONG(chash_return(instance, chash_add_target(&(instance->context), target, weight)));
}

// CHash method removeTarget(<target>) -> long
PHP_METHOD(CHash, removeTarget)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    char  *target;
    size_t  length;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &target, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER));
    }
    RETURN_LONG(chash_return(instance, chash_remove_target(&(instance->context), target)));
}

// CHash method setTargets(array(<target> => <weight> [, ...])) -> long
PHP_METHOD(CHash, setTargets)
{
    chash_object *instance;
    instance = Z_CHASH_OBJ_P();
    zval         *targets, *weight;
    zend_string  *target;
    ulong        unused;
    uint         length;
    int          status;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &targets) != SUCCESS)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER));
    }
    if ((status = chash_clear_targets(&(instance->context))) < 0)
    {
        RETURN_LONG(chash_return(instance, status));
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(targets), target, weight) {
        if (Z_TYPE_P(weight) == IS_LONG && (status = chash_add_target(&(instance->context), target->val, Z_LVAL_P(weight))) < 0)
        {
            zend_string_release(target);
            RETURN_LONG(chash_return(instance, status));
        }
    } ZEND_HASH_FOREACH_END();
    zend_string_release(target);
    RETURN_LONG(chash_return(instance, chash_targets_count(&(instance->context))));
}

// CHash method clearTargets() -> long
PHP_METHOD(CHash, clearTargets)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    RETURN_LONG(chash_return(instance, chash_clear_targets(&(instance->context))));
}

// CHash method getTargetsCount() -> long
PHP_METHOD(CHash, getTargetsCount)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    RETURN_LONG(chash_return(instance, chash_targets_count(&(instance->context))));
}

// CHash method serialize() -> string
PHP_METHOD(CHash, serialize)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    u_char       *serialized;
    int          size;

    if ((size = chash_serialize(&(instance->context), &serialized)) < 0)
    {
        chash_return(instance, size);
        RETURN_STRING("");
    }
    free(serialized);
    RETVAL_STRINGL((char *)serialized, size);
}

// CHash method unserialize(<serialized>) -> long
PHP_METHOD(CHash, unserialize)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    u_char *serialized;
    size_t length;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &serialized, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER));
    }
    RETURN_LONG(chash_return(instance, chash_unserialize(&(instance->context), serialized, length)));
}

// CHash method serializeToFile(<path>) -> long
PHP_METHOD(CHash, serializeToFile)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    char *path;
    size_t length;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER));
    }
    RETURN_LONG(chash_return(instance, chash_file_serialize(&(instance->context), path)));
}

// CHash method unserializeFromFile(<path>) -> long
PHP_METHOD(CHash, unserializeFromFile)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    char *path;
    size_t length;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER));
    }
    RETURN_LONG(chash_return(instance, chash_file_unserialize(&(instance->context), path)));
}

// CHash method lookupList(<candidate>[, <count>]) -> array
PHP_METHOD(CHash, lookupList)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    char         *candidate, **targets;
    size_t       length;
    int          index, status;
    long         count = 1;


    array_init(return_value);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &candidate, &length, &count) != SUCCESS || length == 0 || count < 1)
    {
        chash_return(instance, CHASH_ERROR_INVALID_PARAMETER);
        return;
    }
    if ((status = chash_lookup(&(instance->context), candidate, count, &targets)) < 0)
    {
        chash_return(instance, status);
        return;
    }
    for (index = 0; index < status; index ++)
    {
        add_next_index_string(return_value, targets[index]);
    }
}

// CHash method lookupBalance(<name>[, <count>]) -> string
PHP_METHOD(CHash, lookupBalance)
{
    chash_object* instance = Z_CHASH_OBJ_P();
    char         *candidate, *target;
    size_t       length;
    int          status;
    long         count = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &candidate, &length, &count) != SUCCESS || length == 0 || count < 1)
    {
        chash_return(instance, CHASH_ERROR_INVALID_PARAMETER);
        RETURN_STRING("");
    }
    if ((status = chash_lookup_balance(&(instance->context), candidate, count, &target)) < 0)
    {
        chash_return(instance, status);
        RETURN_STRING("");
    }
    RETURN_STRING(target);
}

// CHash module v-table
static zend_function_entry chash_class_methods[] =
{
    PHP_ME(CHash, useExceptions, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, addTarget, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, removeTarget, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, setTargets, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, clearTargets, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, getTargetsCount, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, serialize, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, unserialize, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, serializeToFile, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, unserializeFromFile, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, lookupList, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, lookupBalance, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

// CHash extended object destructor
static void chash_free(zend_object *obj)
{
    chash_object *i_obj = php_chash_fetch_object(obj);
    zend_object_std_dtor(&(i_obj->zo));
    chash_terminate(&(i_obj->context), 0);
    i_obj = NULL;
    obj = NULL;
    efree(obj);
    efree(i_obj);
}

static zend_object *chash_allocate(zend_class_entry *ce)
{
    chash_object *instance = ecalloc(1, sizeof(chash_object));

    zend_object_std_init(&(instance->zo), ce);
    chash_initialize(&(instance->context), 0);
    instance->use_exceptions = 1;
    instance->zo.handlers = &chash_object_handlers;

    return &instance->zo;
}

// CHash module global initialization
PHP_MINIT_FUNCTION(chash)
{
    zend_class_entry ce;

    memcpy(&chash_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

    INIT_CLASS_ENTRY(ce, "CHash", chash_class_methods);
    ce.create_object = chash_allocate;

    chash_object_handlers.offset = XtOffsetOf(chash_object, zo);
    chash_object_handlers.clone_obj = NULL;
    chash_object_handlers.free_obj = chash_free;

    chash_ce = zend_register_internal_class(&ce);

    REGISTER_LONG_CONSTANT("CHASH_ERROR_DONE", CHASH_ERROR_DONE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CHASH_ERROR_MEMORY", CHASH_ERROR_MEMORY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CHASH_ERROR_IO", CHASH_ERROR_IO, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CHASH_ERROR_INVALID_PARAMETER", CHASH_ERROR_INVALID_PARAMETER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CHASH_ERROR_NOT_FOUND", CHASH_ERROR_NOT_FOUND, CONST_CS | CONST_PERSISTENT);

    INIT_CLASS_ENTRY(ce, "CHashMemoryException", NULL);
    chash_memory_exception = zend_register_internal_class_ex(&ce, zend_exception_get_default());
    INIT_CLASS_ENTRY(ce, "CHashIOException", NULL);
    chash_io_exception = zend_register_internal_class_ex(&ce, zend_exception_get_default());
    INIT_CLASS_ENTRY(ce, "CHashInvalidParameterException", NULL);
    chash_invalid_parameter_exception = zend_register_internal_class_ex(&ce, zend_exception_get_default());
    INIT_CLASS_ENTRY(ce, "CHashNotFoundException", NULL);
    chash_not_found_exception = zend_register_internal_class_ex(&ce, zend_exception_get_default());

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
