resources = gnome.compile_resources ('resources', 'gnome-mines.gresource.xml',
                                     source_dir: '.',
                                     c_name: 'resources')

gnome_mines = executable ('solvable-mines',
                          [ 'config.vapi',
                            'gnome-mines.vala',
                            'minefield.vala',
                            'minefield-view.vala',
                            'tile.vala',
                            'solvable.c',
                            'theme-selector-dialog.vala'] + resources,
                          dependencies: [ glib_dep,
                                          gee_dep,
                                          gtk_dep,
                                          librsvg_dep,
                                          libmath_dep,
                                          libgnome_games_support_dep ],
                          vala_args: [ '--pkg=posix' ],
                          c_args: [ '-DVERSION="@0@"'.format (meson.project_version ()),
                                    '-DGETTEXT_PACKAGE="solvable-mines"',
                                    '-O3',
                                    '-DLOCALEDIR="@0@"'.format (localedir),
                                    '-DDATA_DIRECTORY="@0@"'.format (pkgdatadir) ],
                          install: true )
