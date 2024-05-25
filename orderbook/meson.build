project('orderbook', 'c', default_options: ['c_std=gnu2x', 'optimization=3'])

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

executable(
    'bench',
    src + 'benchmark.c',
    include_directories: [
        incdir, 
        include_directories('vendor/ubench'), 
        include_directories('tests'),
    ],
    link_args: ['-lm'],
    dependencies: [cjson]
)
executable(
    'raw_bench',
    src + 'raw_benchmark.c',
    include_directories: [
        incdir, 
        include_directories('tests'),
    ],
    link_args: ['-lm'],
    dependencies: [cjson]
)
executable(
    'unit_test',
    src + test_src,
    include_directories: [incdir, include_directories('tests')],
    dependencies: [criterion, cjson],
)
