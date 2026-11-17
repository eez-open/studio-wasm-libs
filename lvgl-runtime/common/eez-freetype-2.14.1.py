# Based on Emscripten's FreeType port (Copyright 2015 The Emscripten Authors).
# Emscripten is available under the MIT and University of Illinois/NCSA licenses.
# Keep its full module set and zlib configuration, but pin the upstream version
# already used by studio-wasm-libs instead of selecting the SDK's older version.

import os

VERSION = '2.14.1'
PORT_NAME = 'eez-freetype-' + VERSION
URL = 'https://download.savannah.gnu.org/releases/freetype/freetype-' + VERSION + '.tar.gz'
SHA512 = '1a1db7baa9979b55e663e210433ea53818d39ca25793242a6b3a7edbdd68ea1c386a72aeaa30cea9a9bee5dd0ffd42e8f2785e58bcc543c3452ac5c5c0dc7e2d'
deps = ['zlib']


def get_lib_name(settings):
    suffix = '-legacysjlj' if settings.SUPPORT_LONGJMP == 'wasm' else ''
    return 'lib' + PORT_NAME + suffix + '.a'


def get(ports, settings, shared):
    ports.fetch_project(PORT_NAME, URL, sha512hash=SHA512)

    def create(final):
        source_path = ports.get_dir(PORT_NAME, 'freetype-' + VERSION)
        ports.install_header_dir(os.path.join(source_path, 'include'), target=PORT_NAME)
        srcs = [
            'builds/unix/ftsystem.c',
            'src/autofit/autofit.c',
            'src/base/ftbase.c',
            'src/base/ftbbox.c',
            'src/base/ftbdf.c',
            'src/base/ftbitmap.c',
            'src/base/ftcid.c',
            'src/base/ftdebug.c',
            'src/base/ftfstype.c',
            'src/base/ftgasp.c',
            'src/base/ftglyph.c',
            'src/base/ftgxval.c',
            'src/base/ftinit.c',
            'src/base/ftmm.c',
            'src/base/ftotval.c',
            'src/base/ftpatent.c',
            'src/base/ftpfr.c',
            'src/base/ftstroke.c',
            'src/base/ftsynth.c',
            'src/base/fttype1.c',
            'src/base/ftwinfnt.c',
            'src/bdf/bdf.c',
            'src/bzip2/ftbzip2.c',
            'src/cache/ftcache.c',
            'src/cff/cff.c',
            'src/cid/type1cid.c',
            'src/gzip/ftgzip.c',
            'src/lzw/ftlzw.c',
            'src/pcf/pcf.c',
            'src/pfr/pfr.c',
            'src/psaux/psaux.c',
            'src/pshinter/pshinter.c',
            'src/psnames/psnames.c',
            'src/raster/raster.c',
            'src/sdf/sdf.c',
            'src/sfnt/sfnt.c',
            'src/smooth/smooth.c',
            'src/svg/svg.c',
            'src/truetype/truetype.c',
            'src/type1/type1.c',
            'src/type42/type42.c',
            'src/winfonts/winfnt.c',
        ]
        flags = [
            '-DFT2_BUILD_LIBRARY',
            '-DFT_CONFIG_OPTION_SYSTEM_ZLIB',
            '-DHAVE_UNISTD_H',
            '-DHAVE_FCNTL_H',
            '-I' + os.path.join(source_path, 'include'),
            '-pthread',
        ]
        if settings.SUPPORT_LONGJMP == 'wasm':
            flags.append('-sSUPPORT_LONGJMP=wasm')
        ports.build_port(source_path, final, PORT_NAME, flags=flags, srcs=srcs)

    return [shared.cache.get_lib(get_lib_name(settings), create, what='port')]


def clear(ports, settings, shared):
    shared.cache.erase_lib(get_lib_name(settings))


def process_args(ports):
    return ['-isystem', ports.get_include_dir(PORT_NAME)]


def show():
    return 'EEZ FreeType ' + VERSION + ' (FreeType license)'
