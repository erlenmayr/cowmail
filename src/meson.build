cowmail_sources = [
  'main.c',
  'cowmail-window.c',
  'cowmail-write-window.c',
  'cowmail-contact-window.c',
  'cowmail-msg-row.c',
  'cowmail-contact-row.c',
  'libcowmail.c',
]

cowmail_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('gtk+-3.0', version: '>= 3.22'),
  dependency('gnutls', version: '>= 3.6'),
  dependency('nettle', version: '>= 3.6'),
  dependency('hogweed', version: '>= 3.6'),
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
