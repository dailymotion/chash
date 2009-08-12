from distutils.core import setup, Extension

module = Extension('chash', 
                   sources = ['chash.c'],
                   include_dirs = ['../library'],
                   libraries = ['chash'],
                   library_dirs = ['../library'],
                   )

setup (name = 'CHash',
       version = '1.0.0',
       author = 'Sebastien Estienne',
       author_email = 'sebastien.estienne@dailymotion.com',
       description = 'Consistent hashing library Python extension',
       ext_modules = [module])
