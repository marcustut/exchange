use std::time::Duration;

use matcher::{
    handler::{Handler, RtrbHandler},
    EventHandlerBuilder, Matcher, Order, Symbol, SymbolMetadata,
};
use publisher::TCPPublisher;
use rtrb::RingBuffer;

fn main() {
    let (tx, rx) = RingBuffer::new(1000u64.next_power_of_two() as usize);
    let mut publisher = TCPPublisher {};
    publisher.spawn(rx);

    let mut handler = EventHandlerBuilder::with_context(RtrbHandler::new(tx))
        .on_order(RtrbHandler::on_order)
        .on_trade(RtrbHandler::on_trade)
        .build();

    let mut matcher = Matcher::new(&mut handler);

    // Initialise the orderbook for all symbols
    matcher.add_symbol(
        Symbol::BTCUSDT,
        SymbolMetadata {
            symbol: Symbol::BTCUSDT.to_string(),
            price_precision: 2,
            size_precision: 3,
        },
    );

    //// Make random orders
    //for _ in 0..1000 {
    //    let (symbol, order) = (
    //        Symbol::BTCUSDT,
    //        Order::random(62500.0..=62500.5, 0.001..=1.0),
    //    );
    //    let _ = matcher.order(symbol, order).inspect_err(|e| match e {
    //        matcher::MatcherError::SymbolNotFound(e) => eprintln!("{}", e),
    //        _ => {}
    //    });
    //}

    loop {
        std::thread::sleep(Duration::from_millis(1000));
        let (symbol, order) = (
            Symbol::BTCUSDT,
            Order::random(62500.0..=62500.5, 0.001..=1.0),
        );
        let _ = matcher.order(symbol, order).inspect_err(|e| match e {
            matcher::MatcherError::SymbolNotFound(e) => eprintln!("{}", e),
            _ => {}
        });
    }
}
