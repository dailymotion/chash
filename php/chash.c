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
zend_class_entry *chash_class_entry;

// CHash errors/exceptions management
static zend_class_entry *chash_memory_exception,
                        *chash_io_exception,
                        *chash_invalid_parameter_exception,
                        *chash_not_found_exception;

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
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
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
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    char         *target;
    int          length;
    long         weight = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &target, &length, &weight) != SUCCESS || length == 0)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER))
    }
    RETVAL_LONG(chash_return(instance, chash_add_target(&(instance->context), target, weight)));
}

// CHash method removeTarget(<target>) -> long
PHP_METHOD(CHash, removeTarget)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    char         *target;
    int          length;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &target, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER))
    }
    RETURN_LONG(chash_return(instance, chash_remove_target(&(instance->context), target)))
}

// CHash method setTargets(array(<target> => <weight> [, ...])) -> long
PHP_METHOD(CHash, setTargets)
{
    HashPosition position;
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    zval         *targets, **weight;
    char         *target;
    ulong        unused;
    uint         length;
    int          status;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &targets) != SUCCESS)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER))
    }
    if ((status = chash_clear_targets(&(instance->context))) < 0)
    {
        RETURN_LONG(chash_return(instance, status))
    }
    for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(targets), &position);
         zend_hash_get_current_key_ex(Z_ARRVAL_P(targets), &target, &length, &unused, 0, &position) == HASH_KEY_IS_STRING &&
         zend_hash_get_current_data_ex(Z_ARRVAL_P(targets), (void **)&weight, &position) == SUCCESS &&
         Z_TYPE_PP(weight) == IS_LONG;
         zend_hash_move_forward_ex(Z_ARRVAL_P(targets), &position))
    {
        if ((status = chash_add_target(&(instance->context), target, Z_LVAL_PP(weight))) < 0)
        {
            RETURN_LONG(chash_return(instance, status))
        }
    }
    RETURN_LONG(chash_return(instance, chash_targets_count(&(instance->context))))
}

// CHash method clearTargets() -> long
PHP_METHOD(CHash, clearTargets)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_LONG(chash_return(instance, chash_clear_targets(&(instance->context))))
}

// CHash method getTargetsCount() -> long
PHP_METHOD(CHash, getTargetsCount)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_LONG(chash_return(instance, chash_targets_count(&(instance->context))))
}

// CHash method serialize() -> string
PHP_METHOD(CHash, serialize)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    u_char       *serialized;
    int          size;

    if ((size = chash_serialize(&(instance->context), &serialized)) < 0)
    {
        chash_return(instance, size);
        RETURN_STRING("", 1)
    }
    RETVAL_STRINGL((char *)serialized, size, 1);
    free(serialized);
}

// CHash method unserialize(<serialized>) -> long
PHP_METHOD(CHash, unserialize)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    u_char       *serialized;
    int          length;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &serialized, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER))
    }
    RETURN_LONG(chash_return(instance, chash_unserialize(&(instance->context), serialized, length)))
}

// CHash method serializeToFile(<path>) -> long
PHP_METHOD(CHash, serializeToFile)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    char         *path;
    int          length;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER))
    }
    RETURN_LONG(chash_return(instance, chash_file_serialize(&(instance->context), path)))
}

// CHash method unserializeFromFile(<path>) -> long
PHP_METHOD(CHash, unserializeFromFile)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    char         *path;
    int          length;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(chash_return(instance, CHASH_ERROR_INVALID_PARAMETER))
    }
    RETURN_LONG(chash_return(instance, chash_file_unserialize(&(instance->context), path)))
}

// CHash method lookupList(<candidate>[, <count>]) -> array
PHP_METHOD(CHash, lookupList)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    char         *candidate, **targets;
    int          length, index, status;
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
        add_next_index_string(return_value, targets[index], 1);
    }
}

// CHash method lookupBalance(<name>[, <count>]) -> string
PHP_METHOD(CHash, lookupBalance)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    char         *candidate, *target;
    int          length, status;
    long         count = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &candidate, &length, &count) != SUCCESS || length == 0 || count < 1)
    {
        chash_return(instance, CHASH_ERROR_INVALID_PARAMETER);
        RETURN_STRING("", 1)
    }
    if ((status = chash_lookup_balance(&(instance->context), candidate, count, &target)) < 0)
    {
        chash_return(instance, status);
        RETURN_STRING("", 1)
    }
    RETURN_STRING(target, 1)
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
static void chash_free(chash_object *instance TSRMLS_DC)
{
    zend_object_std_dtor(&(instance->zo) TSRMLS_CC);
    chash_terminate(&(instance->context), 0);
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
    chash_initialize(&(instance->context), 0);
    instance->use_exceptions = 1;
    value.handle = zend_objects_store_put(instance, (zend_objects_store_dtor_t)zend_objects_destroy_object,
                                                    (zend_objects_free_object_storage_t)chash_free, NULL TSRMLS_CC);
    value.handlers = zend_get_std_object_handlers();
    return value;
}

// CHash module global initialization
PHP_MINIT_FUNCTION(chash)
{
    zend_class_entry class_entry;

    INIT_CLASS_ENTRY(class_entry, "CHash", chash_class_methods);
    chash_class_entry = zend_register_internal_class(&class_entry TSRMLS_CC);
    chash_class_entry->create_object = chash_allocate;

    REGISTER_LONG_CONSTANT("CHASH_ERROR_DONE", CHASH_ERROR_DONE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CHASH_ERROR_MEMORY", CHASH_ERROR_MEMORY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CHASH_ERROR_IO", CHASH_ERROR_IO, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CHASH_ERROR_INVALID_PARAMETER", CHASH_ERROR_INVALID_PARAMETER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CHASH_ERROR_NOT_FOUND", CHASH_ERROR_NOT_FOUND, CONST_CS | CONST_PERSISTENT);

    INIT_CLASS_ENTRY(class_entry, "CHashMemoryException", NULL);
    chash_memory_exception = zend_register_internal_class_ex(&class_entry, zend_exception_get_default(), NULL);
    INIT_CLASS_ENTRY(class_entry, "CHashIOException", NULL);
    chash_io_exception = zend_register_internal_class_ex(&class_entry, zend_exception_get_default(), NULL);
    INIT_CLASS_ENTRY(class_entry, "CHashInvalidParameterException", NULL);
    chash_invalid_parameter_exception = zend_register_internal_class_ex(&class_entry, zend_exception_get_default(), NULL);
    INIT_CLASS_ENTRY(class_entry, "CHashNotFoundException", NULL);
    chash_not_found_exception = zend_register_internal_class_ex(&class_entry, zend_exception_get_default(), NULL);

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
