use matcher::{Matcher, Order, Side, Symbol, SymbolMetadata};
use orderbook::EventHandlerBuilder;

fn main() {
    let mut handler = EventHandlerBuilder::with_context(())
        .on_order(|_ctx, ob_id, event| {
            println!("[{:?}]: {:?}\n", ob_id, event);
        })
        .on_trade(|_ctx, ob_id, event| {
            println!("[{:?}]: {:?}\n", ob_id, event);
        })
        .build();

    let mut matcher = Matcher::new();
    matcher.add_symbol(
        Symbol::BTCUSDT,
        SymbolMetadata {
            symbol: Symbol::BTCUSDT.to_string(),
            price_precision: 2,
            size_precision: 3,
        },
        &mut handler,
    );

    println!("{}", matcher.get_orderbook(Symbol::BTCUSDT).unwrap());

    let orders = vec![
        (
            Symbol::BTCUSDT,
            Order {
                price: 62805.05,
                size: 0.001,
                side: Side::Bid,
            },
        ),
        (
            Symbol::BTCUSDT,
            Order {
                price: 62805.05,
                size: 0.002,
                side: Side::Ask,
            },
        ),
    ];

    for (symbol, order) in orders {
        matcher.order(symbol, order).unwrap()
    }

    println!("{}", matcher.get_orderbook(Symbol::BTCUSDT).unwrap());
}
