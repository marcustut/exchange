use std::{
    ptr::null_mut,
    sync::{Arc, Mutex},
};

use orderbook::{order_event, trade_event, EventHandler, OrderEventType, Orderbook, Side};

#[derive(Debug)]
struct EventStore {
    order_events: Vec<(OrderEventType, order_event)>,
    trade_events: Vec<trade_event>,
}

fn main() {
    let store = Arc::new(Mutex::new(EventStore {
        order_events: vec![],
        trade_events: vec![],
    }));

    let handler = EventHandler::with_context(store.clone())
        .handle_order_event(Box::new(|ctx, event_type, event| {
            let mut store = ctx.lock().unwrap();
            store.order_events.push((event_type, event));
        }))
        .handle_trade_event(Box::new(|ctx, event| {
            let mut store = ctx.lock().unwrap();
            store.trade_events.push(event);
        }));
    let mut ob = Orderbook::new().with_handler(handler);

    ob.limit(orderbook::order {
        order_id: 1,
        price: 1000,
        size: 10,
        cum_filled_size: 0,
        side: Side::Bid.into(),
        limit: null_mut(),
        prev: null_mut(),
        next: null_mut(),
    });

    ob.market(2, Side::Ask.into(), 5);

    let store = store.lock().unwrap();
    println!("Order Events: {:?}", store.order_events);
    println!();
    println!("Trade Events: {:?}", store.trade_events);
}
