# handle the gtk-doc documentation

r = run_command ('/bin/cp', 'libmo-docs.xml', meson.current_build_dir ())

subdir ('xml')

gnome.gtkdoc ('libmo',
              src_dir : '@0@/src'.format (meson.source_root ()),
              main_xml : 'libmo-docs.xml',
              scan_args : '--rebuild-types',
              install: true)
