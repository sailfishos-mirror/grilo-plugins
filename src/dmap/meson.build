#
# meson.build
#
# Author: Juan A. Suarez Romero <jasuarez@igalia.com>
#
# Copyright (C) 2016 Igalia S.L. All rights reserved.

daap_sources = [
    'grl-common.c',
    'grl-common.h',
    'grl-daap-db.c',
    'grl-daap-db.h',
    'grl-daap-record-factory.c',
    'grl-daap-record-factory.h',
    'grl-daap-record.c',
    'grl-daap-record.h',
    'grl-daap.c',
    'grl-daap.h',
]

dpap_sources = [
    'grl-common.c',
    'grl-common.h',
    'grl-dpap-db.c',
    'grl-dpap-db.h',
    'grl-dpap-record-factory.c',
    'grl-dpap-record-factory.h',
    'grl-dpap-record.c',
    'grl-dpap-record.h',
    'grl-dpap.c',
    'grl-dpap.h',
]

configure_file(output: 'config.h',
    configuration: cdata)

shared_library('grldaap',
    sources: daap_sources,
    install: true,
    install_dir: pluginsdir,
    dependencies: must_deps + plugins[dmap_idx][DEPS],
    c_args: [
        '-DDAAP_PLUGIN_ID=DMAP_PLUGIN_ID',
        '-DG_LOG_DOMAIN="GrlDmap"',
        '-DHAVE_CONFIG_H',
    ],
    link_args: [
        '-Wl,--no-undefined',
    ])

shared_library('grldpap',
    sources: dpap_sources,
    install: true,
    install_dir: pluginsdir,
    dependencies: must_deps + plugins[dmap_idx][DEPS],
    c_args: [
        '-DDPAP_PLUGIN_ID=DMAP_PLUGIN_ID',
        '-DG_LOG_DOMAIN="GrlDmap"',
        '-DHAVE_CONFIG_H',
    ],
    link_args: [
        '-Wl,--no-undefined',
    ])