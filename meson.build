project(
  'wolox',
  'c',
  version : '0.1.0',
  license : 'MIT',
  default_options: ['default_library=static', 'c_std=c99','warning_level=3'],
)

cc = meson.get_compiler('c')
if cc.get_id() == 'msvc'
  add_global_arguments('/D_CRT_SECURE_NO_WARNINGS', language : 'c')
endif

sdl2 = dependency('sdl2')
stc = dependency('stc')
enet = dependency('libenet')
flecs = dependency('flecs')
nuklear = dependency('nuklear')
chipmunk2d = dependency('chipmunk2d')
cmocka = dependency('cmocka')

m_dep = cc.find_library('m', required : false)

source_files = [
  'src/main.c',
]

executable('wolox', sources: source_files,
           # win_subsystem: 'windows',
           include_directories: include_directories(['src']),
           dependencies: [sdl2, flecs, nuklear, chipmunk2d, enet, stc, m_dep])

test('test1', executable('test1', 'test/test_1.c', dependencies: [cmocka]))
