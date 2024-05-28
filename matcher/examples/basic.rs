use std::sync::Arc;

use matcher::{Matcher, Order, Side, Symbol, SymbolMetadata};
use orderbook::EventHandlerBuilder;

fn main() {
    let mut handler = EventHandlerBuilder::with_context(())
        .on_order(Arc::new(|_ctx, ob_id, event| {
            println!("[{:?}]: {:?}\n", ob_id, event);
        }))
        .on_trade(Arc::new(|_ctx, ob_id, event| {
            println!("[{:?}]: {:?}\n", ob_id, event);
        }))
        .build();

    let mut matcher = Matcher::new(&mut handler);
    matcher.add_symbol(
        Symbol::BTCUSDT,
        SymbolMetadata {
            price_precision: 2,
            size_precision: 3,
        },
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
