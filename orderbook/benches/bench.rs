use std::io::BufRead;

use criterion::{black_box, criterion_group, criterion_main, Criterion};
use orderbook::Orderbook;

const FILENAME: &'static str = "data/l3_orderbook_100k.ndjson";

#[derive(Clone)]
struct Message {
    order_id: u64,
    price: u64,
    size: u64,
    side: String,
    message_type: String,
}

#[inline]
fn process_messages(ob: &mut Orderbook, msgs: Vec<Message>) {
    msgs.into_iter()
        .for_each(|msg| match msg.message_type.as_str() {
            "order_created" => match msg.price {
                0 => {
                    ob.execute(
                        msg.order_id,
                        match msg.side.as_str() {
                            "bid" => orderbook::Side::Bid,
                            "ask" => orderbook::Side::Ask,
                            _ => unreachable!(),
                        },
                        msg.size,
                        msg.size,
                        true,
                    );
                }
                _ => ob.limit(orderbook::ffi::order {
                    order_id: msg.order_id,
                    price: msg.price,
                    size: msg.size,
                    cum_filled_size: 0,
                    side: match msg.side.as_str() {
                        "bid" => orderbook::Side::Bid.into(),
                        "ask" => orderbook::Side::Ask.into(),
                        _ => unreachable!(),
                    },
                    limit: std::ptr::null_mut(),
                    prev: std::ptr::null_mut(),
                    next: std::ptr::null_mut(),
                    user_data: std::ptr::null_mut(),
                }),
            },
            "order_deleted" => {
                let _ = ob.cancel(msg.order_id);
            }
            "order_changed" => {
                let _ = ob.amend_size(msg.order_id, msg.size);
            }
            _ => unreachable!(),
        });
}

fn benchmark(c: &mut Criterion) {
    let mut msgs = vec![];
    let file = std::fs::File::open(FILENAME).unwrap();
    for line in std::io::BufReader::new(file).lines().flatten() {
        let value = serde_json::from_str::<serde_json::Value>(&line).unwrap();
        let msg = Message {
            order_id: value["data"]["id"].as_u64().unwrap(),
            price: (value["data"]["price"].as_f64().unwrap() * 10f64.powi(8)) as u64,
            size: (value["data"]["amount"].as_f64().unwrap() * 10f64.powi(8)) as u64,
            side: match value["data"]["order_type"].as_u64().unwrap() {
                0 => "bid",
                1 => "ask",
                _ => unreachable!(),
            }
            .to_string(),
            message_type: value["event"].as_str().unwrap().to_string(),
        };
        msgs.push(msg);
    }

    let mut ob = Orderbook::new();

    let mut group = c.benchmark_group("group");

    group.bench_function("l3_orderbook_100k", |b| {
        b.iter(|| process_messages(black_box(&mut ob), black_box(msgs.clone())))
    });

    group.finish()
}

criterion_group!(benches, benchmark);
criterion_main!(benches);
