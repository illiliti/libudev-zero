# SPDX-License-Identifier: ISC

project(
    'libudev-zero',
    'c',
    version: '1.0.4',
    license: 'ISC',
    meson_version: '>=0.60.0',
    default_options: [
        'c_std=c99',
        'warning_level=2',
        'default_library=both',
    ],
)

cc = meson.get_compiler('c')

usb_ids_path = get_option('usb-ids-path')
if usb_ids_path == ''
    usb_ids_path = get_option('prefix') / get_option('datadir') / 'hwdata' / 'usb.ids'
endif

add_project_arguments(
    '-D_XOPEN_SOURCE=700',
    '-DUSB_IDS_PATH="@0@"'.format(usb_ids_path),
    cc.get_supported_arguments(
        '-Wpedantic',
        '-Wmissing-prototypes',
        '-Wstrict-prototypes',
        '-Wno-unused-parameter',
    ),
    language: 'c',
)

symbol_versioning = get_option('symbol-versioning')
libudev_link_args = []
libudev_link_depends = []
if symbol_versioning.allowed()
    version_script = '-Wl,--version-script,@0@'.format(
        meson.current_source_dir() / 'libudev.sym',
    )
    if cc.has_link_argument(version_script)
        libudev_link_args += version_script
        libudev_link_depends += files('libudev.sym')
    elif symbol_versioning.enabled()
        error('symbol-versioning was enabled, but the linker does not support --version-script')
    endif
endif

libudev = library(
    'udev',
    files(
        'udev.c',
        'udev_device.c',
        'udev_enumerate.c',
        'udev_list.c',
        'udev_monitor.c',
    ),
    soversion: '1',
    link_args: libudev_link_args,
    link_depends: libudev_link_depends,
    install: true,
)

install_headers(
    configure_file(
        input: 'udev.h',
        output: 'libudev.h',
        copy: true,
    ),
)

pkg = import('pkgconfig')
pkg.generate(
    libudev,
    name: 'libudev',
    filebase: 'libudev',
    description: 'Daemonless replacement for libudev',
    url: 'https://github.com/illiliti/libudev-zero',
    version: '251',
)
