#![cfg(feature = "rtrb")]

use std::time::{Duration, Instant};

use matcher::{
    handler::{Event, Handler, RtrbHandler},
    Matcher, Order, Symbol, SymbolMetadata,
};
use once_cell::sync::Lazy;
use orderbook::EventHandlerBuilder;
use rand::Rng;
use rtrb::RingBuffer;

const NUM_ORDERS: usize = 100000;

static mut TIMETABLE: Lazy<Vec<Option<Instant>>> = Lazy::new(|| vec![None; NUM_ORDERS]);
static mut LATENCIES: Lazy<Vec<u128>> = Lazy::new(|| vec![0; NUM_ORDERS]);

struct Context {}

fn event_handler(_ctx: &mut Context, event: Event) {
    match event {
        Event::Order { event, .. } => match event.status {
            orderbook::OrderStatus::Created => unsafe {
                match &mut TIMETABLE[event.order_id as usize - 1] {
                    Some(instant) => {
                        LATENCIES[event.order_id as usize - 1] = instant.elapsed().as_nanos();
                    }
                    None => {}
                }
            },
            _ => {}
        },
        Event::Trade { .. } => {}
    }
}

fn main() {
    let (tx, rx) = RingBuffer::new(NUM_ORDERS.next_power_of_two());

    // Spawn a dedicated thread to handle the events from the maching engine
    let handle = RtrbHandler::spawn(rx, Context {}, Some(1), event_handler);
    std::thread::sleep(Duration::from_millis(1000)); // wait till thread is spawned

    // Make a handler for the events
    let mut handler = EventHandlerBuilder::with_context(RtrbHandler::new(tx))
        .on_order(RtrbHandler::on_order)
        .on_trade(RtrbHandler::on_trade)
        .build();

    // Create the matching engine
    let mut matcher = Matcher::new();

    // Initialise the orderbook for all symbols
    for symbol in [Symbol::BTCUSDT, Symbol::ETHUSDT, Symbol::ADAUSDT] {
        matcher.add_symbol(
            symbol,
            SymbolMetadata {
                symbol: symbol.to_string(),
                price_precision: 2,
                size_precision: 3,
            },
            &mut handler,
        );
    }

    // Make random orders
    let mut rng = rand::thread_rng();
    for i in 0..NUM_ORDERS {
        let (symbol, order) = (
            rng.gen::<Symbol>(),
            Order::random(62500.0..=62500.5, 0.001..=1.0),
        );
        unsafe { TIMETABLE[i as usize] = Some(Instant::now()) };
        let _ = matcher.order(symbol, order).inspect_err(|e| match e {
            matcher::MatcherError::SymbolNotFound(e) => eprintln!("{}", e),
            _ => {}
        });
    }

    // Wait for awhile to let all events to be handled
    std::thread::sleep(Duration::from_millis(1000));
    handle.shutdown().unwrap();

    unsafe {
        for latency in LATENCIES.iter() {
            println!("{}", latency);
        }
        let mean = LATENCIES.iter().fold(0, |acc, l| acc + l) / LATENCIES.len() as u128;
        let min = LATENCIES
            .iter()
            .fold(u128::MAX, |acc, l| if *l < acc { *l } else { acc });
        let max = LATENCIES
            .iter()
            .fold(u128::MIN, |acc, l| if *l > acc { *l } else { acc });
        println!("min: {}ns", min);
        println!("max: {}ns", max);
        println!("mean: {}ns", mean);
    }
}
