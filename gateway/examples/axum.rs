use gateway::Request;
use std::time::Duration;

use disruptor::Producer;
use matcher::{
    ffi,
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

    let mut producer =
        disruptor::build_multi_producer(1024, || Request::default(), disruptor::BusySpin)
            .handle_events_and_state_with(
                |state: &mut (ffi::event_handler, Matcher), req, _seq, _end_of_batch| {
                    let eh = &mut state.0;
                    let matcher = &mut state.1;

                    match req {
                        Request::Order((symbol, order)) => {
                            let _ =
                                matcher
                                    .order(*symbol, order.clone())
                                    .inspect_err(|e| match e {
                                        matcher::MatcherError::SymbolNotFound(e) => {
                                            eprintln!("{}", e)
                                        }
                                        _ => {}
                                    });
                        }
                        Request::AddSymbol((symbol, metadata)) => {
                            matcher.add_symbol(*symbol, metadata.clone(), eh)
                        }
                        Request::None => return,
                    }
                },
                || {
                    let handler = EventHandlerBuilder::with_context(BroadcastHandler::new(tx))
                        .on_order(BroadcastHandler::on_order)
                        .on_trade(BroadcastHandler::on_trade)
                        .build();

                    (handler, Matcher::new())
                },
            )
            .build();

    let request_tx = producer.clone();
    std::thread::spawn(move || {
        rt.block_on(gateway::axum::spawn("127.0.0.1:8080", rx, request_tx))
            .unwrap();
    });

    producer.publish(|r| {
        *r = Request::AddSymbol((
            Symbol::BTCUSDT,
            SymbolMetadata {
                symbol: Symbol::BTCUSDT.to_string(),
                price_precision: 2,
                size_precision: 3,
            },
        ));
    });

    loop {
        std::thread::sleep(Duration::from_millis(10));
        let (symbol, order) = (
            Symbol::BTCUSDT,
            Order::random(62500.0..=62500.5, 0.001..=1.0),
        );
        producer.publish(|r| {
            *r = Request::Order((symbol, order));
        });
    }
}
