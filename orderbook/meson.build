project('orderbook', 'c', default_options: ['c_std=gnu2x', 'optimization=3'])

incdir = include_directories('include')
src = ['src/orderbook.c', 'src/price_limit_map.c', 'src/limit.c']
test_src = ['tests/price_limit_map_test.c']

criterion = dependency('criterion')

executable(
    'demo',
    src + 'main.c',
    include_directories: incdir,
)
executable(
    'unit_test',
    src + test_src,
    include_directories: incdir,
    dependencies: [criterion],
)
