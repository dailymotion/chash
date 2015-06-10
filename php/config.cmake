project(chash)

find_package(PkgConfig)
pkg_check_modules(LIBCHASH REQUIRED libchash)

include_directories(${LIBCHASH_INCLUDE_DIR})
link_directories(${LIBCHASH_LIBDIR})

HHVM_COMPAT_EXTENSION(hhchash chash.c)
HHVM_SYSTEMLIB(hhchash ext_chash.php)

target_link_libraries(hhchash ${LIBCHASH_LIBRARIES})