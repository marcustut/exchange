project('orderbook', 'c', default_options: ['c_std=gnu2x', 'optimization=3', 'default_library=both'])

incdir = include_directories('include')
src = [
    'src/event_handler.c', 
    'src/orderbook.c', 
    'src/limit.c', 
    'src/limit_tree.c',
    'src/uint64_hashmap.c', 
]
test_src = [
    'tests/orderbook_test.c', 
    'tests/limit_tree_test.c',
    'tests/uint64_hashmap_test.c', 
]

criterion = dependency('criterion')
cjson = dependency('libcjson')

lib = library('orderbook', src, include_directories: incdir)
executable(
    'bench',
    'benchmark.c',
    include_directories: [
        incdir, 
        include_directories('vendor/ubench'), 
        include_directories('tests'),
    ],
    link_args: ['-lm'],
    link_with: lib,
    dependencies: [cjson]
)
executable(
    'raw_bench',
    'raw_benchmark.c',
    include_directories: [
        incdir, 
        include_directories('tests'),
    ],
    link_args: ['-lm'],
    link_with: lib,
    dependencies: [cjson]
)
executable(
    'unit_test',
    test_src,
    include_directories: [incdir, include_directories('tests')],
    link_with: lib,
    dependencies: [criterion, cjson],
)
