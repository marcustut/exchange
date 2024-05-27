use std::ptr::null_mut;

use orderbook::{Orderbook, Side};

fn main() {
    let mut ob = Orderbook::new();

    println!("{}", ob);
    println!("{:?}", ob.top_n(Side::Bid, 5));

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

    println!("{}", ob);
    println!("{:?}", ob.top_n(Side::Bid, 5));

    ob.cancel(1).unwrap();

    println!("{}", ob);
    println!("{:?}", ob.top_n(Side::Bid, 5));
}
