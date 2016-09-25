project ('libmo', 'c',
         default_options : ['c_std=c99'])

gnome = import ('gnome')

cc = meson.get_compiler('c')

mo_version_major = '0'
mo_version_minor = '1'
mo_version_micro = '0'
mo_version = '@0@.@1@.@2@'.format (mo_version_major,
                                   mo_version_minor,
                                   mo_version_micro)

mo_api_version_major = '0'
mo_api_version_minor = '1'
mo_api_version = '@0@.@1@'.format (mo_api_version_major,
                                   mo_api_version_minor)

lt_current  = 0
lt_revision = 0
lt_age      = 0
lt_version = '@0@.@1@.@2@'.format (lt_current,
                                   lt_revision,
                                   lt_age)

c_args = []

foreach argument : ['-fno-strict-aliasing',
                    '-pedantic',
                    '-Wall',
                    '-Werror',
                    '-Wextra',
                    '-Warray-bounds',
                    '-Wcast-align',
                    '-Wdeclaration-after-statement',
                    '-Werror=implicit-function-declaration',
                    '-Wformat=2',
                    '-Wformat-nonliteral',
                    '-Wformat-security',
                    '-Wimplicit-function-declaration',
                    '-Winit-self',
                    '-Winline',
                    '-Wmissing-declarations',
                    '-Wmissing-format-attribute',
                    '-Wmissing-include-dirs',
                    '-Wmissing-noreturn',
                    '-Wmissing-prototypes',
                    '-Wnested-externs',
                    '-Wno-format-extra-args',
                    '-Wno-missing-field-initializers',
                    '-Wno-unused-parameter',
                    '-Wold-style-definition',
                    '-Wpacked',
                    '-Wpointer-arith',
                    '-Wredundant-decls',
                    '-Wreturn-type',
                    '-Wshadow',
                    '-Wsign-compare',
                    '-Wstrict-aliasing',
                    '-Wstrict-overflow',
                    '-Wstrict-prototypes',
                    '-Wswitch-default',
                    '-Wswitch-enum',
                    '-Wundef',
                    '-Wunused-but-set-variable',
                    '-Wwrite-strings',
                    '-Wno-suggest-attribute=format',
                    '-Wno-error=format-extra-args',
                    '-Wno-error=missing-field-initializers',
                    '-Wno-error=pedantic',
                    '-Wno-error=unused-parameter']
        if cc.has_argument (argument)
                c_args += [argument]
        endif
endforeach

c_args += ['-DMO_COMPILATION']

link_args = ['-Wl,--fatal-warnings']

glib_required_version = '2.43.4'

gobject = dependency ('gobject-2.0', version : '>= @0@'.format(glib_required_version))
glib    = dependency ('glib-2.0',    version : '>= @0@'.format(glib_required_version))
gio     = dependency ('gio-2.0',     version : '>= @0@'.format(glib_required_version))

# the main library

libmo_headers = ['libmo/mo.h', 'libmo/mofile.h']
install_headers (libmo_headers,
                 subdir : 'libmo')

libmo_sources = ['libmo/mofile.c']
deps = [gobject, glib, gio]
mapfile = 'libmo/mo.map'

vflag = '-Wl,--version-script,@0@/@1@'.format (meson.current_source_dir (), mapfile)
libmo = library ('mo',
                 libmo_sources,
                 soversion : lt_current - lt_age,
                 version : lt_version,
                 dependencies : deps,
                 include_directories : include_directories (meson.current_source_dir()),
                 link_args : ['-Wl,--no-undefined', vflag] + link_args,
                 link_depends : mapfile,
                 c_args : c_args,
                 install : true)

# the example program

example = executable ('sample-query',
                      'example/sample-query.c',
                      include_directories : include_directories (meson.current_source_dir()),
                      dependencies : deps,
                      c_args : c_args,
                      link_args : link_args,
                      link_with : libmo)

# the introspection files
girscanner = find_program ('g-ir-scanner',
                           required: false)

if girscanner.found ()
        introspection_sources = libmo_headers + libmo_sources

        gnome.generate_gir (libmo,
                            sources : introspection_sources,
                            nsversion : mo_api_version,
                            namespace : 'Mo',
                            symbol_prefix : 'mo_',
                            identifier_prefix : 'Mo',
                            export_packages : 'libmo',
                            includes : ['GObject-2.0', 'Gio-2.0', 'GLib-2.0'],
                            install : true
        )
endif

gtkdoc = find_program ('gtkdoc-scan',
                       required : false)

if gtkdoc.found ()
        subdir ('docs/reference/libmo')
endif
