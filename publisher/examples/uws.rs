use std::time::Duration;

use matcher::{
    handler::{Handler, RtrbHandler},
    EventHandlerBuilder, Matcher, Order, Symbol, SymbolMetadata,
};
use publisher::uws;
use rtrb::RingBuffer;
use uwebsockets_rs::us_socket_context_options::UsSocketContextOptions;

fn main() {
    let (tx, rx) = RingBuffer::new(1000u64.next_power_of_two() as usize);
    uws::spawn(
        3001,
        UsSocketContextOptions {
            key_file_name: None,
            cert_file_name: None,
            passphrase: None,
            dh_params_file_name: None,
            ca_file_name: None,
            ssl_ciphers: None,
            ssl_prefer_low_memory_usage: None,
        },
    );

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
