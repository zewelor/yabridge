project(
  'yabridge',
  'cpp',
  version : '3.8.1',
  meson_version : '>=0.56',
  default_options : [
    'warning_level=3',
    'cpp_std=c++2a',
    # Even though Meson will complain that this option does not exist, without
    # this Meson will not apply the above option to native targets
    'build.cpp_std=c++2a',
  ],
)

#
# Build options
#

# In theory yabridge should compile fine on a 32-bit system, but you will always
# need to pass `-Dbitbridge=true`. We just make sure that we won't build
# any 64-bit binaries in that situation.
is_64bit_system = build_machine.cpu_family() not in ['x86', 'arm']
with_32bit_libraries = (not is_64bit_system) or get_option('build.cpp_args').contains('-m32')
with_bitbridge = get_option('bitbridge')
with_winedbg = get_option('winedbg')
with_vst3 = get_option('vst3')

#
# Compiler flags
#

# Depending on the `bitbridge` flag we'll enable building secondary 32-bit
# host applications that can act as a bit bridge for using 32-bit Windows
# plugins in 64-bit Linux VST hosts. The plugin will determine which host
# application to use based on the `.dll` file it's trying to load. This setup is
# necessary until Meson provides a way to have multiple cross-builds for a
# single build directory: https://github.com/mesonbuild/meson/issues/5125

# These variables are used to generate a `config.h` file. The library names will
# be prefixed with `lib` and suffixed with `.so`, and the host names will be
# suffixed with `.exe`.
vst2_plugin_name = 'yabridge-vst2'
vst3_plugin_name = 'yabridge-vst3'
host_name_64bit = 'yabridge-host'
host_name_32bit = 'yabridge-host-32'

compiler_options = [
  '-fvisibility=hidden',
  '-fvisibility-inlines-hidden',
  # We use an intrinsic to force flush-to-zero. SSE2 is always enabled in x86_64
  # CPUs, but when we're compiling the 32-bit bitbridge we need to manually add
  # this flag.
  '-msse2',
]

chainloader_compiler_options = [
  # We use our process library for sending notifications from the chainloaders,
  # but we don't need the Asio pipe support there
  '-DWITHOUT_ASIO',
]

# HACK: Some stuff from `windows.h` that we don't need results in conflicting
#       definitions, so we'll try to exclude those bits
wine_compiler_options = [
  '-DNOMINMAX',
  # Winsock conflicts with the Posix sockets API. Before Wine 6.8 there was a
  # `WINE_NOWINSOCK` that would exclude just `winsock.h` from `windows.h`, but
  # they got rid of that so we now need to explicitly define the ifdef guards
  '-D__WINE_WINSOCKAPI_STDLIB_H',
  '-D_WINSOCKAPI_',
  # This is only relevant for Wine 6.2, but commit
  # `0c19e2e487d36a89531daf4897c0b6390d82a843`, broke compilation of
  # `shobjidl.h` under C++.
  #
  # https://bugs.winehq.org/show_bug.cgi?id=50670
  '-D__IFileOperation_INTERFACE_DEFINED__',
  # This Wine 6.20 commit `dfdf56fbe47f8ff50ebe533e6d73f2de6546f008` added a
  # bunch of new SAL includes to `windows.h`, which include things like `__in`
  # and `__out`. This breaks libstdc++ compilation since they often use those
  # names for function parameters.
  #
  # https://bugs.winehq.org/show_bug.cgi?id=51919
  '-D__WINE_SAL_H__',
]

# NOTE: GCC doesn't 8-byte align doubles in structs on x86 for ABI-compatibilty
#       reasons, but MSVC++ does. We need to force this same alignment to be
#       ABI-compatible with 32-bit binaries created with MSVC++ on Windows.
wine_32bit_compiler_options = wine_compiler_options + ['-m32', '-malign-double']
wine_64bit_compiler_options = wine_compiler_options + ['-m64']

# Enable addition assertions on the STL containers during debug builds. Meson
# has a `cpp_debugstl` option, but it's nicer having this automatically tied to
# debug builds.
if get_option('buildtype') == 'debug'
  compiler_options += ['-D_GLIBCXX_DEBUG']
endif

if with_bitbridge
  compiler_options += '-DWITH_BITBRIDGE'
endif

# This provides an easy way to start the Wine VST host using winedbg since it
# can be quite a pain to set up
if with_winedbg
  compiler_options += '-DWITH_WINEDBG'
endif

if with_vst3
  compiler_options += '-DWITH_VST3'
endif

#
# Wine checks
#

# Meson does not let us set a default cross compiler, which makes sense, but it
# also means that it's easy to forget. This will cause the setup process to
# abort if no cross compiler has been set up.
winelib_check = '''#ifndef __WINE__
#error 1
#endif'''
if not meson.get_compiler('cpp').compiles(winelib_check)
  error('You need to set up a cross compiler, check the README for compilation instructions.')
endif

# Wine versions after Wine 5.6 and before 6.0 require a `__cdecl` calling
# convention to be specified on the `main()` functions or else `argc` and `argv`
# will point to the wrong memory. Similarly, with other versions of Wine this
# should _not_ be specified for the same reason. We'll try to figure out the
# current Wine version and add this calling convention based on that. Also,
# printing the configure-time Wine version might be useful in diagnosing build
# issues so we'll do just that.
#
# https://bugs.winehq.org/show_bug.cgi?id=49138
wine_version = run_command(
  'sh', '-c', '''wine --version | grep --only-matching -P '[0-9]+\.[0-9]+' | head -n1''',
  check : false
)
if wine_version.returncode() == 0
  message('Targetting Wine @0@'.format(wine_version.stdout()))
  if wine_version.stdout().version_compare('>=5.7') and \
     wine_version.stdout().version_compare('<6.0')
    message('- Using the cdecl calling convention')
    compiler_options += '-DWINE_USE_CDECL'
  endif
  if wine_version.stdout().version_compare('<6.23') and with_winedbg
    message('- Using legacy winedbg argument quoting')
    compiler_options += '-DWINEDBG_LEGACY_ARGUMENT_QUOTING'
  endif
else
  warning('Unable to determine the current Wine version')
endif

# Wine versions below 5.7 will segfault in `CoCreateGuid` which gets called
# during static initialization. I'm not exactly sure why this is happening, but
# to prevent this from causing more headaches and confusion in the future we
# should just immediately error out when building yabridge's VST3 support with
# these older Wine versions.
if wine_version.returncode() == 0 and \
   wine_version.stdout().version_compare('<5.7') and \
   with_vst3
  error('Because of a bug in Wine < 5.7\n' +
        'you cannot build yabridge with VST3 support using these older Wine versions.\n' +
        'Use the \'-Dvst3=false\' build option to disable VST3 support.\n\n' +
        'https://github.com/robbert-vdh/yabridge/issues/63#issuecomment-757369645')
endif

#
# Dependencies
#

include_dir = include_directories('src/include')

# These dependencies require separate linking flags for the 32-bit and 64-bit
# versions

# I honestly have no idea what the correct way is to have `dependency()` or
# `compiler.find_dependency()` search for 32-bit versions of libraries when
# cross-compiling. Meson also doesn't seem to respect the default linker
# search path set by the system in `find_library()`. If anyone does know how
# to properly do this, please let me know!
winegcc = meson.get_compiler('cpp', native : false)

if is_64bit_system
  xcb_64bit_dep = dependency('xcb')
endif
if with_32bit_libraries or with_bitbridge
  xcb_32bit_dep = winegcc.find_library('xcb')
endif

# These are all headers-only libraries, and thus won't require separate 32-bit
# and 64-bit versions

asio_dep = dependency('asio', version : '>=1.22.0')
if meson.version().version_compare('>=0.60')
  # Bitsery's CMake build definition is capitalized for some reason
  bitsery_dep = dependency('bitsery', 'Bitsery', version : '>=5.2.0')
else
  # Mmeson <=0.6.0 didn't support multiple names for a dependency, and since at
  # the moment this is only relevant for packing on Arch btw, it's probably
  # better to remove this conditional later than it is to bump the minimum Meson
  # version now.
  bitsery_dep = dependency('bitsery', version : '>=5.2.0')
endif
function2_dep = dependency('function2', version : '>=4.0.0')
ghc_filesystem_dep = dependency('ghc_filesystem', version : '>=1.5.0')
threads_dep = dependency('threads')
tomlplusplus_dep = dependency('tomlplusplus', version : '>=2.0.0')

dl_dep = declare_dependency(link_args : '-ldl')
rt_dep = declare_dependency(link_args : '-lrt')

wine_ole32_dep = declare_dependency(link_args : '-lole32')
# The SDK includes a comment pragma that would link to this on MSVC
wine_shell32_dep = declare_dependency(link_args : '-lshell32')
# The built in threads dependency does not know how to handle winegcc
wine_threads_dep = declare_dependency(link_args : '-lpthread')
wine_uuid_dep = declare_dependency(link_args : '-luuid')

# We need to build the VST3 SDK dependencies in tree because Meson won't let us
# build both native, 32-bit cross compiled and 64-bit cross compiled
# dependencies from a (CMake) subproject
if with_vst3
  subdir('src/common/vst3')
endif

#
# Binaries
#
# The application consists of a plugin (`libyabridge-{vst2,vst3}.so`) that calls
# a Winelib application (`yabridge-host{,-32}.exe`) that can host Windows VST2
# and VST3 plugins. These plugins can in turn be loaded from small stub
# libraries dubbed chainloaders to avoid having to copy large plugin libraries
# around. More information about the way these two components work together can
# be found in `docs/architecture.md`.
#

# Generate header files for configuration variables such as the current git tag
# and the name of the host binary
subdir('src/common/config')

# These only contain the definitions for sources and dependencies. It would be
# nice to define the libraries and executables inside of these meson.build
# files, but that will also scatter the build artifacts around in the `build/`
# directory and it's much more convenient having all of the important files
# directory under `build/`.
# https://github.com/mesonbuild/meson/pull/4037
subdir('src/chainloader')
subdir('src/plugin')
subdir('src/wine-host')

shared_library(
  vst2_plugin_name,
  vst2_plugin_sources,
  native : true,
  include_directories : include_dir,
  dependencies : vst2_plugin_deps,
  # NOTE: LTO does not support Winelibs, and it seems to break
  #       `libyabridge-vst2.so` in Bitwig for some reason. It should be left
  #       turned off for the time being except for on the chainloader
  #       libraries.
  cpp_args : compiler_options,
)
shared_library(
  'yabridge-chainloader-vst2',
  vst2_chainloader_sources,
  native : true,
  dependencies : chainloader_deps,
  cpp_args : compiler_options + chainloader_compiler_options,
  # LTO is useful here to get rid of unused code
  override_options : ['b_lto=true'],
)

if with_vst3
  # This is the VST3 equivalent of `libyabridge-vst2.so`. The Wine host
  # applications can handle both VST2 and VST3 plugins.
  shared_library(
    vst3_plugin_name,
    vst3_plugin_sources,
    native : true,
    include_directories : include_dir,
    dependencies : vst3_plugin_deps,
    cpp_args : compiler_options,
  )
  shared_library(
    'yabridge-chainloader-vst3',
    vst3_chainloader_sources,
    native : true,
    dependencies : chainloader_deps,
    cpp_args : compiler_options + chainloader_compiler_options,
    # See above
    override_options : ['b_lto=true'],
  )
endif

if is_64bit_system
  executable(
    host_name_64bit,
    host_sources,
    native : false,
    include_directories : include_dir,
    dependencies : host_64bit_deps,
    cpp_args : compiler_options + wine_64bit_compiler_options,
    link_args : ['-m64'],
  )
endif

if with_bitbridge
  executable(
    host_name_32bit,
    host_sources,
    native : false,
    include_directories : include_dir,
    dependencies : host_32bit_deps,
    cpp_args : compiler_options + wine_32bit_compiler_options,
    link_args : ['-m32'],
  )
endif
