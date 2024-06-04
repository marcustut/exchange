#![cfg(feature = "disruptor")]

use std::time::{Duration, Instant};

use disruptor::{BusySpin, ProcessorSettings, Sequence};
use matcher::{
    handler::{DisruptorHandler, Event, Handler},
    Matcher, Order, Side, Symbol, SymbolMetadata,
};
use once_cell::sync::Lazy;
use orderbook::{EventHandlerBuilder, TradeEvent};
use rand::Rng;
use strum::IntoEnumIterator;

const NUM_ORDERS: usize = 100000;

static mut TIMETABLE: Lazy<Vec<Option<Instant>>> = Lazy::new(|| vec![None; NUM_ORDERS]);
static mut LATENCIES: Lazy<Vec<u128>> = Lazy::new(|| vec![0; NUM_ORDERS]);

struct Context {}

fn event_handler(_ctx: &mut Context, event: &Event, _seq: Sequence, _end_of_batch: bool) {
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
    let factory = || Event::Trade {
        ob_id: 0,
        trade_id: 0,
        event: TradeEvent {
            size: 0,
            price: 0,
            side: Side::Bid,
            buyer_order_id: 0,
            seller_order_id: 0,
        },
        timestamp: chrono::Utc::now(),
    };
    let producer =
        disruptor::build_single_producer(NUM_ORDERS.next_power_of_two(), factory, BusySpin)
            .pined_at_core(1)
            .handle_events_with(DisruptorHandler::handle(Context {}, event_handler))
            .build();
    std::thread::sleep(Duration::from_millis(1000)); // wait till thread is spawned

    // Make a handler for the events
    let mut handler = EventHandlerBuilder::with_context(DisruptorHandler::new(producer))
        .on_order(DisruptorHandler::on_order)
        .on_trade(DisruptorHandler::on_trade)
        .build();

    // Create the matching engine
    let mut matcher = Matcher::new(&mut handler);

    // Initialise the orderbook for all symbols
    for symbol in Symbol::iter() {
        matcher.add_symbol(
            symbol,
            SymbolMetadata {
                symbol: symbol.to_string(),
                price_precision: 2,
                size_precision: 3,
            },
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
