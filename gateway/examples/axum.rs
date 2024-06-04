use std::time::Duration;

use matcher::{
    handler::{BroadcastHandler, Handler},
    EventHandlerBuilder, Matcher, Order, Symbol, SymbolMetadata,
};

fn main() {
    let rt = tokio::runtime::Builder::new_multi_thread()
        .enable_io()
        .enable_time()
        .build()
        .unwrap();

    let (tx, rx) = tokio::sync::broadcast::channel(u16::MAX.into());

    let mut handler = EventHandlerBuilder::with_context(BroadcastHandler::new(tx))
        .on_order(BroadcastHandler::on_order)
        .on_trade(BroadcastHandler::on_trade)
        .build();

    let mut matcher = Matcher::new(&mut handler);

    matcher.add_symbol(
        Symbol::BTCUSDT,
        SymbolMetadata {
            symbol: Symbol::BTCUSDT.to_string(),
            price_precision: 2,
            size_precision: 3,
        },
    );

    std::thread::spawn(move || {
        rt.block_on(gateway::axum::spawn("127.0.0.1:8080", rx))
            .unwrap();
    });

    loop {
        std::thread::sleep(Duration::from_millis(10));
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
