from distutils.core import setup, Extension

module = Extension('rendertext',
                   sources = ['rendertext.c'],
                   include_dirs = [
                       '/usr/local/opt/freetype/include/freetype2/',
                       '/usr/local/opt/icu4c/include/',
                   ],
                   library_dirs = [
                       '/usr/local/opt/icu4c/lib/',
                   ],
                   libraries = [
                       'freetype',
                       'icuuc',
                       'harfbuzz',
                   ])

setup (name = 'rendertext',
       version = '1.0',
       description = 'Render text with emoji',
       ext_modules = [module])
