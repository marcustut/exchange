use std::ptr::null_mut;

use orderbook::{EventHandler, Orderbook, Side};

fn main() {
    let handler = EventHandler::with_context(())
        .handle_order_event(Box::new(|_ctx, event_type, event| {
            println!("{:?}: {:?}", event_type, event);
        }))
        .handle_trade_event(Box::new(|_ctx, event| {
            println!("{:?}", event);
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
}
