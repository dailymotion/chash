// Consistent hashing library PHP extension
// pyke@dailymotion.com - 05/2009

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include "libchash.h"

// CHash composite object definition
typedef struct
{
    zend_object   zo;
    CHASH_CONTEXT context;
} chash_object;

// CHash class entry
zend_class_entry *chash_class_entry;

// CHash method addTarget(<target>[, <weight>]) -> long
PHP_METHOD(CHash, addTarget)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    u_int32_t    weight = 1, length;
    char         *target;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &target, &length, &weight) != SUCCESS || length == 0)
    {
        RETURN_LONG(CHASH_ERROR_INVALID_PARAMETER);
    }
    RETURN_LONG(chash_add_target(&(instance->context), target, weight))
}

// CHash method removeTarget(<target>) -> long
PHP_METHOD(CHash, removeTarget)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    u_int32_t    length;
    char         *target;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &target, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(CHASH_ERROR_INVALID_PARAMETER);
    }
    RETURN_LONG(chash_remove_target(&(instance->context), target))
}

// CHash method setTargets(array(<target> => <weight> [, ...])) -> long
PHP_METHOD(CHash, setTargets)
{
    HashPosition position;
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    zval         *targets, **weight;
    uint         length;
    ulong        unused;
    int          status;
    char         *target;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &targets) != SUCCESS)
    {
        RETURN_LONG(CHASH_ERROR_INVALID_PARAMETER)
    }
    if ((status = chash_clear_targets(&(instance->context))) < 0)
    {
        RETURN_LONG(status)
    }
    for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(targets), &position);
         zend_hash_get_current_key_ex(Z_ARRVAL_P(targets), &target, &length, &unused, 0, &position) == HASH_KEY_IS_STRING &&
         zend_hash_get_current_data_ex(Z_ARRVAL_P(targets), (void **)&weight, &position) == SUCCESS &&
         Z_TYPE_PP(weight) == IS_LONG;
         zend_hash_move_forward_ex(Z_ARRVAL_P(targets), &position))
    {
        if ((status = chash_add_target(&(instance->context), target, Z_LVAL_PP(weight))) < 0)
        {
            RETURN_LONG(status)
        }
    }
    RETURN_LONG(chash_targets_count(&(instance->context)))
}

// CHash method clearTargets() -> long
PHP_METHOD(CHash, clearTargets)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_LONG(chash_clear_targets(&(instance->context)))
}

// CHash method getTargetsCount() -> long
PHP_METHOD(CHash, getTargetsCount)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_LONG(chash_targets_count(&(instance->context)))
}

// CHash method freeze() -> long
PHP_METHOD(CHash, freeze)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_LONG(chash_freeze(&(instance->context)))
}

// CHash method unfreeze() -> long
PHP_METHOD(CHash, unfreeze)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    RETURN_LONG(chash_unfreeze(&(instance->context)))
}

// CHash method serialize() -> string
PHP_METHOD(CHash, serialize)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    u_char       *serialized;
    int          size;

    if ((size = chash_serialize(&(instance->context), &serialized)) < 0)
    {
        RETURN_STRING("", 1)
    }
    RETVAL_STRINGL((char *)serialized, size, 1);
    free(serialized);
}

// CHash method unserialize(<serialized>) -> long
PHP_METHOD(CHash, unserialize)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    uint         length;
    u_char       *serialized;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &serialized, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(CHASH_ERROR_INVALID_PARAMETER);
    }
    RETURN_LONG(chash_unserialize(&(instance->context), serialized, length))
}

// CHash method serializeToFile(<path>) -> long
PHP_METHOD(CHash, serializeToFile)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    uint         length;
    char         *path;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(CHASH_ERROR_INVALID_PARAMETER);
    }
    RETURN_LONG(chash_file_serialize(&(instance->context), path))
}

// CHash method unserializeFromFile(<path>) -> long
PHP_METHOD(CHash, unserializeFromFile)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    uint         length;
    char         *path;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &length) != SUCCESS || length == 0)
    {
        RETURN_LONG(CHASH_ERROR_INVALID_PARAMETER);
    }
    RETURN_LONG(chash_file_unserialize(&(instance->context), path))
}

// CHash method lookupList(<candidate>[, <count>]) -> array
PHP_METHOD(CHash, lookupList)
{
    chash_object *instance = (chash_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    uint         count = 1, length, index;
    int          status;
    char         *candidate, **targets;

    array_init(return_value);
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &candidate, &length, &count) != SUCCESS || length == 0)
    {
        return;
    }
    if ((status = chash_lookup(&(instance->context), candidate, count, &targets)) < 0)
    {
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
    uint         count = 1, length;
    char         *candidate, *target;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &candidate, &length, &count) != SUCCESS || length == 0)
    {
        RETURN_STRING("", 1)
    }
    if (chash_lookup_balance(&(instance->context), candidate, count, &target) < 0)
    {
        RETURN_STRING("", 1)
    }
    RETURN_STRING(target, 1)
}

// CHash module v-table
static function_entry chash_class_methods[] =
{
    PHP_ME(CHash, addTarget, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, removeTarget, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, setTargets, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, clearTargets, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, getTargetsCount, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, freeze, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(CHash, unfreeze, NULL, ZEND_ACC_PUBLIC)
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
    REGISTER_LONG_CONSTANT("CHASH_ERROR_ALREADY_FROZEN", CHASH_ERROR_ALREADY_FROZEN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CHASH_ERROR_NOT_FROZEN", CHASH_ERROR_NOT_FROZEN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CHASH_ERROR_NOT_FOUND", CHASH_ERROR_NOT_FOUND, CONST_CS | CONST_PERSISTENT);
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
