import os
import io
import sys
import glob
import shutil
import hashlib
import inspect
import tarfile
import zipfile
import functools
import collections
import urllib.request

import logging
logger = logging.getLogger(__name__)

from invoke import Collection, Task, ctask

import mev_build_utils

THIS_DIR = os.path.abspath(os.path.dirname(__file__))


def SourceForgeURL(fname):
    url = 'http://downloads.sourceforge.net/project/{}?use_mirror=autoselect'
    return url.format(fname)

AVAILABLE_MODULES = collections.OrderedDict([
   ('unxutils', {
       'compression': 'zip',
       'url': SourceForgeURL('unxutils/unxutils/current/UnxUtils.zip'),
       'sha256': 'b8c694072723a417194022a2ba265750eec61e15d1725e39449df1763e224b45',
       'unpack_location': 'unxutils2',
       'rename': [('unxutils2/usr/local/wbin', 'unxutils'),
                  ('unxutils2/bin/sh.exe', 'unxutils/sh.exe'),
                  ('unxutils2', 'unxutils/misc'),
                 ],
   }),
   ('cmake', {
       'compression': 'zip',
       'url': 'http://www.cmake.org/files/v3.2/cmake-3.2.2-win32-x86.zip',
       'sha256': '8728119bbb48468ab45230a579463729e7b12d6babfa1ad77e771b239b5430db',
       'rename': [('cmake-3.2.2-win32-x86', 'cmake')],
       'provides': [('cmake', 'bin\\cmake.exe')],
   }),
   ('nasm', {
       'compression': 'zip',
       'url': SourceForgeURL('nasm/Win32 binaries/2.07/nasm-2.07-win32.zip'),
       'sha256': '3188d693619cf9cf646e4429329fccd4c9f1ba08fda14437de43e55452e352b8',
       'rename': [('nasm-2.07', 'nasm')],
       'provides': [('nasm', 'nasm.exe')],
   }),
   ('boost', {
       'compression': 'zip',
       'url': 'file:./boost-mini.zip',
       'sha256': '29bc6b06c00d6f9fb7a18b99508bda403c8adb0939aab32ee4a480b126ad5157',
       'unpack_location': 'boost',
       # Publish only the debug/non-debug DLLs, depending on build configuration.
       'build': [('del build.cfg.* 2>nul; echo 1 > build.cfg.{config}')],
       'publish': lambda:
                  glob.glob(os.path.join(THIS_DIR, 'boost\\lib32-msvc-10.0\\*-mt-1_58.dll'))
                  if os.path.exists(os.path.join(THIS_DIR, 'boost\\build.cfg.release')) else
                  glob.glob(os.path.join(THIS_DIR, 'boost\\lib32-msvc-10.0\\*-mt-gd-1_58.dll'))
   }),
   ('libjpeg-turbo', {
       'compression': 'tar',
       'url': SourceForgeURL('libjpeg-turbo/1.4.0/libjpeg-turbo-1.4.0.tar.gz'),
       'sha256': 'd93ad8546b510244f863b39b4c0da0fa4c0d53a77b61a8a3880f258c232bbbee',
       'rename': [('libjpeg-turbo-1.4.0', 'libjpeg-turbo')],
       'build': [('{cmake} -DNASM="{nasm}"  -DCMAKE_INSTALL_PREFIX=instdir '
                          '-G "{cmake_generator}"'),
                 ('{cmake} --build . --target install --config {cmake_config} -- /m')],
       'publish': ['instdir\\bin\\jpeg62.dll'],
   }),
   ('proj4', {
       'compression': 'tar',
       'url': 'http://download.osgeo.org/proj/proj-4.9.1.tar.gz',
       'sha256': 'fca0388f3f8bc5a1a803d2f6ff30017532367992b30cf144f2d39be88f36c319',
       'rename': [('proj-4.9.1', 'proj4')],
       'patches': ['proj4.diff'],
       'build': [('{cmake} -DBUILD_LIBPROJ_SHARED:BOOL="1" -DCMAKE_INSTALL_PREFIX=instdir '
                          '-DINCLUDEDIR=instdir\\include -DLIBDIR=instdir\\lib '
                          '-DPROJ_CORE_TARGET_OUTPUT_NAME=proj '
                          '-G "{cmake_generator}"'),
                 ('{cmake} --build . --target install --config {cmake_config} -- /m'),
                ],
       'publish': ['instdir\\bin\\proj.dll'],
   }),
   ('libtiff', {
       'compression': 'zip',
       'url': 'http://download.osgeo.org/libtiff/tiff-4.0.4beta.zip',
       'sha256': '612dada703c859ea0e024e12b1bdb10d65eaf6807e06a57ac9d3ae73afe39945',
       'rename': [('tiff-4.0.4beta', 'libtiff')],
       'patches': ['libtiff.diff'],
       'build': [('nmake /f Makefile.vc '
                  '"OPTFLAGS= -EHsc -W3 -D_CRT_SECURE_NO_DEPRECATE '
                  '-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE '
                  '-DZFILLODER_LSB2MSB {flags}" '
                  'JPEG_SUPPORT=1 '
                  'JPEGDIR=../../libjpeg-turbo/instdir '
                  'JPEG_INCLUDE=-I$(JPEGDIR)/include '
                  'JPEG_LIB=$(JPEGDIR)/lib/jpeg.lib '
                  '"LD=link /nologo /manifest" '
                  )],
       'publish': ['libtiff\\libtiff.dll'],
   }),
   ('libgeotiff', {
       'compression': 'zip',
       'url': 'ftp://ftp.remotesensing.org/pub/geotiff/libgeotiff/libgeotiff-1.4.1.zip',
       'sha256': '13e723ce4672d84f640eb3fc321e73a615bf5cec41760751af00a70168f25e0d',
       'rename': [('libgeotiff-1.4.1', 'libgeotiff')],
       'patches': ['libgeotiff.diff'],
       'build': [('copy /b geo_config.h.vc +,,'),
                 ('nmake /f Makefile.vc WANT_PROJ4=1 "OPTFLAGS= -nologo {flags}" geo_config.h'),
                 ('nmake /f Makefile.vc WANT_PROJ4=1 "OPTFLAGS= -nologo {flags}"'),
                ],
       'publish': ['geotiff.dll', 'csv'],
   }),
   ('geographiclib', {
       'compression': 'zip',
       'url': SourceForgeURL('geographiclib/distrib/GeographicLib-1.41.zip'),
       'sha256': 'fd48b6608fffa9419cb221c3f0663426900961ad5a02b8db3065101f53528bfa',
       'rename': [('GeographicLib-1.41', 'geographiclib')],
       'patches': ['geographiclib.diff'],
       'build': [('nmake /f Makefile.vc "OPTFLAGS= -nologo {flags}"'),],
   }),
   ('sip', {
       'compression': 'zip',
       'url': SourceForgeURL('pyqt/sip/sip-4.15.4/sip-4.15.4.zip'),
       'sha256': 'd4b522caf93620675a6f1a7f3ed6ddcb9533cce71482d77d5015f61b98e426d2',
       'rename': [('sip-4.15.4', 'sip'),],
       'build': [('python configure.py --platform=win32-msvc2010 '
                      '-e "{py_inc_dir}" INCDIR+="{py_inc_dir}" '
                      'LFLAGS+="/DEBUG /PDB:$(TARGET).pdb"'),
                 ('cd sipgen && nmake'),
                 # siplib is linked into our application, so control its compilation
                 # flags. We need to replace both CFLAGS and CXXFlags, so use this
                 # expansion strategy here.
                 ('cd siplib && nmake "CFLAGS=%(f)s" "CXXFLAGS=%(f)s /EHsc"' % { 'f':
                     ('-nologo -Zm200 /Zc:wchar_t /D WIN32 /D STRICT '
                      '/D NOMINMAX /D _DEBUG /D _WINDOWS /D _WINDLL '
                      '/D _UNICODE /D UNICODE /W0 /Oy- /Gm /GS '
                      '/fp:precise -nologo -W0 -w34100 -w34189 '
                      '{flags}')}),
                 ('nmake install'),
                ],
   }),
])

FLAGS = { 'debug': "-D_MT -MDd /Zi /RTC1 /Od",
          'release': "-D_MT -MD /Ox" }
CMAKE_CONFIG = { 'debug': 'Debug',
                 'release': 'RelWithDebInfo' }
CMAKE_GENERATOR = "Visual Studio 10 2010"
PY_INC_DIR = os.path.join(os.environ['VIRTUAL_ENV'], 'Include')


class ModuleValidationError(RuntimeError):
    pass


def extract_filename(url):
    """Extract a probable filename from an URL"""
    return os.path.basename(url.split('?')[0])

def unpack(url, sha256, compression, unpack_location='.'):
    """Fetch a remote archive, check its hash and decompress it

    Download the file ``url``, ensure its hash matches the ``sha256`` argument,
    then decompress it using ``compression`` method (either 'tar' or 'zip').
    The unpacked files will be written to ``unpack_location``.
    """

    print("Downloading", extract_filename(url))
    with mev_build_utils.temporary_chdir(THIS_DIR):
        # Chdir to make relative file:./z.zip URLs work.
        urlopener = urllib.request.urlopen(url)
    f = io.BytesIO(urlopener.read())
    sha256_found = hashlib.sha256(f.getvalue()).hexdigest()
    if sha256_found != sha256:
        raise ModuleValidationError('Failed to validate downloaded package',
                                    url, sha256_found, sha256)
    available_compressors = {
            'tar': lambda file, mode: tarfile.open(fileobj=file, mode=mode),
            'zip': lambda file, mode: zipfile.ZipFile(file=file, mode=mode),
            }
    compressor = available_compressors[compression]
    with compressor(f, 'r') as cf:
        cf.extractall(os.path.join(THIS_DIR, unpack_location))

def git_patch_path():
    git_dir = os.path.dirname(mev_build_utils.find_executable('git'))
    return os.path.join(git_dir, '..', 'bin', 'patch')

@functools.lru_cache()
def get_provides():
    """Return a dict mapping all provides to actual executables

    Collect all provides from AVAILABLE_MODULES and return them as `dict`
    mapping template name to executables.

    Executable names are absolute paths.
    """

    provides = {
        'patch': git_patch_path(),
    }
    for modulename, module in AVAILABLE_MODULES.items():
        for name, cmd in module.get('provides', []):
            provides[name] = os.path.join(THIS_DIR, modulename, cmd)
    return provides

def run_with_vs(ctx, directory, command, config):
    """Template-expand a command and run it under a VS environment

    Keyword-expand `command` (via `str.format()`), then execute it in
    `directory` with an active Visual Studio environment.

    Available keywords:

    * `config`: desired build configuration (release/debug)
    * `cmake_config`: cmake build configuration (RelWithDebInfo/Debug)
    * `cmake_generator`: cmake generator to use
    * `flags`: current build flags
    * `py_inc_dir`: include directory of the python distribution (or venv)
    * all expansions exported via the `'provides'` mechanism
    """

    provides_expands = get_provides()
    command = command.format(config=config,
                             flags=FLAGS[config.lower()],
                             cmake_config=CMAKE_CONFIG[config.lower()],
                             cmake_generator=CMAKE_GENERATOR,
                             py_inc_dir=PY_INC_DIR,
                             **provides_expands)
    return ctx.run('call "%VS100COMNTOOLS%\\vsvars32.bat" && {command}'.format(
                   directory=directory, command=command), cwd=directory)

def select_modules(modules):
    "Yield items from AVAILABLE_MODULES selected by the modules parameter"
    if modules is None or modules == 'all':
        return AVAILABLE_MODULES.items()
    return ((m, AVAILABLE_MODULES[m]) for m in modules.split(';'))

@ctask(help={'modules': 'Semicolon-separated list of modules to operate on (default: all)'})
def download(ctx, modules=None):
    "Download third-party modules required for building MapsEvolved"
    for modulename, module in select_modules(modules):
        args = set(inspect.getargspec(unpack).args)
        args = {k: v for k, v in module.items() if k in args}
        unpack(**args)
        for src, dest in module.get('rename', []):
            mev_build_utils.resilient_rename(os.path.join(THIS_DIR, src),
                                             os.path.join(THIS_DIR, dest))
        for diff in module.get('patches', []):
            cmd = '"{patch}" -p1 --forward --fuzz 0 < "{diff}"'
            ctx.run(cmd.format(modulename=modulename,
                               patch=get_provides()['patch'],
                               diff=os.path.join(THIS_DIR, diff)),
                    cwd=os.path.join(THIS_DIR, modulename))

@ctask(help={'config': 'Which configuration to build: debug/release',
             'modules': 'Semicolon-separated list of modules to operate on (default: all)'})
def build(ctx, config, modules=None):
    "Build third-party modules required for building MapsEvolved"
    for modulename, module in select_modules(modules):
        for command in module.get('build', []):
            run_with_vs(ctx, os.path.join(THIS_DIR, modulename), command, config)

@ctask(help={'modules': 'Semicolon-separated list of modules to operate on (default: all)'})
def distclean(ctx, modules=None):
    "Remove the downloaded modules completely"
    for modulename, module in select_modules(modules):
        path = os.path.join(THIS_DIR, modulename)
        if os.path.exists(path):
            shutil.rmtree(path)

@ctask(help={'modules': 'Semicolon-separated list of modules to operate on (default: all)',
             'targets': 'Semicolon-separated list of directories to copy the libraries into'})
def publish(ctx, targets, modules=None):
    "Publish built libraries to a target location"
    targets = targets.split(';')
    logger.debug("Copying publishable files to targets: %r", targets)
    for modulename, module in select_modules(modules):
        # Allow boost to update it's publish list while actually running, otherwise the
        # publish list is set on initialization and might be wrong if we built since then.
        published = module.get('publish', [])
        if callable(published):
            published = published()

        for src in published:
            src = os.path.join(THIS_DIR, modulename, src)
            if not os.path.exists(src):
                raise RuntimeError("Publishing source doesn't exist: %s" % src)
            logger.debug("Copying %s", src)
            for target in targets:
                mev_build_utils.copy_any(src, target, overwrite=True)

@ctask
def listmodules(ctx):
    "List available third-party modules"
    print('\n'.join(AVAILABLE_MODULES.keys()))

ns = Collection(*[obj for obj in vars().values() if isinstance(obj, Task)])
ns.configure({'run': { 'runner': mev_build_utils.LightInvokeRunner }})
