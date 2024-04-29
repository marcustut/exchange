project('orderbook', 'c', default_options: ['c_std=gnu2x'])

incdir = include_directories('include')

src = ['src/orderbook.c', 'src/price_limit_map.c']

executable('demo', src + 'main.c', include_directories: incdir)
