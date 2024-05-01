project('orderbook', 'c', default_options: ['c_std=gnu2x', 'optimization=3'])

incdir = include_directories('include')
src = [
    'src/orderbook.c', 
    'src/limit.c', 
    'src/limit_tree.c',
    'src/price_limit_map.c', 
    'src/order_metadata_map.c', 
]
test_src = [
    'tests/orderbook_test.c', 
    'tests/limit_tree_test.c',
    'tests/price_limit_map_test.c', 
    'tests/order_metadata_map_test.c', 
]

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
