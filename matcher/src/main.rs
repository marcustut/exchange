use std::sync::Arc;

use matcher::{
    handler::{Handler, StdOutHandler},
    Matcher, Order, Symbol, SymbolMetadata,
};
use orderbook::EventHandlerBuilder;
use rand::Rng;
use strum::IntoEnumIterator;

fn main() {
    // Make a handler for the events
    let mut handler = EventHandlerBuilder::with_context(StdOutHandler::new())
        .on_order(Arc::new(StdOutHandler::on_order))
        .on_trade(Arc::new(StdOutHandler::on_trade))
        .build();

    // Create the matching engine
    let mut matcher = Matcher::new(&mut handler);

    // Initialise the orderbook for all symbols
    for symbol in Symbol::iter() {
        matcher.add_symbol(
            symbol,
            SymbolMetadata {
                price_precision: 2,
                size_precision: 3,
            },
        );
    }

    println!("{}", matcher.get_orderbook(Symbol::BTCUSDT).unwrap());

    // Make 1000 random orders
    let mut orders = vec![];
    let mut rng = rand::thread_rng();
    for _ in 0..1000 {
        orders.push((
            rng.gen::<Symbol>(),
            Order::random(62500.0..=62500.5, 0.001..=5.0),
        ));
    }

    for (symbol, order) in orders {
        let _ = matcher.order(symbol, order).inspect_err(|e| match e {
            matcher::MatcherError::SymbolNotFound(e) => eprintln!("{}", e),
        });
    }
}
