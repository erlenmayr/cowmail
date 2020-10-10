cowmail_sources = [
  'main.c',
  'cowmail-window.c',
  'write-window.c',
]

cowmail_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('gtk+-3.0', version: '>= 3.22'),
  dependency('libsodium', version: '>= 1.0.18'),
]

gnome = import('gnome')

cowmail_sources += gnome.compile_resources('cowmail-resources',
  'cowmail.gresource.xml',
  c_name: 'cowmail'
)

executable('cowmail', cowmail_sources,
  dependencies: cowmail_deps,
  install: true,
)
