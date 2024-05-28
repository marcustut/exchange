use std::{
    ptr::null_mut,
    sync::{mpsc, Arc, Mutex},
    time::Duration,
};

use orderbook::{EventHandlerBuilder, OrderEvent, Orderbook, Side, TradeEvent};

#[derive(Debug)]
struct EventStore {
    order_events: Vec<OrderEvent>,
    trade_events: Vec<TradeEvent>,
}

#[derive(Debug)]
enum Message {
    Order(OrderEvent),
    Trade(TradeEvent),
}

struct Context {
    tx: mpsc::Sender<Message>,
}

fn main() {
    // A local event store
    let store = Arc::new(Mutex::new(EventStore {
        order_events: vec![],
        trade_events: vec![],
    }));

    // Channel for sending / receiving events
    let (tx, rx) = mpsc::channel();

    // Spawn a thread to consume the events from the orderbook's matching process
    let store_thread = store.clone();
    std::thread::spawn(move || {
        println!("[THREAD] Start listening for messages");
        loop {
            match rx.recv() {
                Ok(msg) => {
                    let mut store = store_thread.lock().unwrap();
                    match msg {
                        Message::Order(event) => store.order_events.push(event),
                        Message::Trade(event) => store.trade_events.push(event),
                    }
                    if store.order_events.len() + store.trade_events.len() >= 4 {
                        println!("[THREAD] Received 4 or more messages, stop receiving");
                        break;
                    }
                }
                Err(e) => {
                    eprintln!("[THREAD] Failed to receive msg: {e}");
                    break;
                }
            }
        }
        println!("[THREAD] Exiting message listener");
    });

    // Our event handler
    let mut handler = EventHandlerBuilder::with_context(Context { tx: tx.clone() })
        .on_order(Arc::new(|ctx, ob_id, event| {
            match ctx.tx.send(Message::Order(event)) {
                Ok(_) => {
                    println!("[OB{ob_id}] Sent order msg");
                }
                Err(e) => {
                    eprintln!("Failed to send msg: {e}")
                }
            }
        }))
        .on_trade(Arc::new(|ctx, ob_id, event| {
            match ctx.tx.send(Message::Trade(event)) {
                Ok(_) => {
                    println!("[OB{ob_id}] Sent trade msg");
                }
                Err(e) => {
                    eprintln!("Failed to send msg: {e}")
                }
            }
        }))
        .build();

    // Make an orderbook with id of 1
    let mut ob = Orderbook::new().with_id(1).with_event_handler(&mut handler);

    // Place a limit order (make)
    ob.limit(orderbook::ffi::order {
        order_id: 1,
        price: 1000,
        size: 10,
        cum_filled_size: 0,
        side: Side::Bid.into(),
        limit: null_mut(),
        prev: null_mut(),
        next: null_mut(),
    });

    // Place a market order (take)
    ob.execute(2, Side::Ask.into(), 5, 5, true);

    // Wait for awhile
    std::thread::sleep(Duration::from_millis(1000));

    let store = store.lock().unwrap();
    println!(
        "Received {} order events and {} trade events",
        store.order_events.len(),
        store.trade_events.len()
    );
}
