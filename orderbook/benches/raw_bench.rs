use std::{io::BufRead, time::Instant};

use orderbook::Orderbook;

const FILENAME: &'static str = "data/l3_orderbook_100k.ndjson";
const SAMPLE_SIZE: usize = 100;

#[derive(Clone)]
struct Message {
    order_id: u64,
    price: u64,
    size: u64,
    side: String,
    message_type: String,
}

#[derive(Default)]
struct BenchmarkResult {
    market_elapsed_ns: u64,
    market_count: u64,
    limit_elapsed_ns: u64,
    limit_count: u64,
    cancel_elapsed_ns: u64,
    cancel_count: u64,
    amend_size_elapsed_ns: u64,
    amend_size_count: u64,
}

fn benchmark(msgs: &Vec<Message>) -> BenchmarkResult {
    let mut ob = Orderbook::new();

    let mut result = BenchmarkResult::default();

    for msg in msgs {
        let start = Instant::now();

        match msg.message_type.as_str() {
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
        }

        let elapsed = start.elapsed().as_nanos();

        match msg.message_type.as_str() {
            "order_created" => match msg.price {
                0 => {
                    result.market_count += 1;
                    result.market_elapsed_ns += elapsed as u64;
                }
                _ => {
                    result.limit_count += 1;
                    result.limit_elapsed_ns += elapsed as u64;
                }
            },
            "order_deleted" => {
                result.cancel_count += 1;
                result.cancel_elapsed_ns += elapsed as u64;
            }
            "order_changed" => {
                result.amend_size_count += 1;
                result.amend_size_elapsed_ns += elapsed as u64;
            }
            _ => unreachable!(),
        }
    }

    result
}

fn main() {
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

    let mut result = BenchmarkResult::default();
    let (
        mut market_elapsed_ns,
        mut limit_elapsed_ns,
        mut cancel_elapsed_ns,
        mut amend_size_elapsed_ns,
    ) = (0, 0, 0, 0);

    for i in 0..SAMPLE_SIZE {
        result = benchmark(&msgs);
        market_elapsed_ns += result.market_elapsed_ns;
        limit_elapsed_ns += result.limit_elapsed_ns;
        cancel_elapsed_ns += result.cancel_elapsed_ns;
        amend_size_elapsed_ns += result.amend_size_elapsed_ns;
        if i != 0 {
            market_elapsed_ns /= 2;
            limit_elapsed_ns /= 2;
            cancel_elapsed_ns /= 2;
            amend_size_elapsed_ns /= 2;
        }
    }

    let total_elapsed_ns =
        market_elapsed_ns + limit_elapsed_ns + cancel_elapsed_ns + amend_size_elapsed_ns;

    println!("Over {} samples,", SAMPLE_SIZE);
    println!(
        "Took {:.2}ms to process {} messages",
        total_elapsed_ns as f64 * 10f64.powi(-6),
        msgs.len(),
    );
    println!(
        "Took {}ns to process 1 message",
        total_elapsed_ns / msgs.len() as u64,
    );
    println!("-------------------------------");
    println!(
        "orderbook_market: {}ns/op over {} calls",
        market_elapsed_ns / result.market_count,
        result.market_count,
    );
    println!(
        "orderbook_limit: {}ns/op over {} calls",
        limit_elapsed_ns / result.limit_count,
        result.limit_count,
    );
    println!(
        "orderbook_cancel: {}ns/op over {} calls",
        cancel_elapsed_ns / result.cancel_count,
        result.cancel_count,
    );
    println!(
        "orderbook_amend_size: {}ns/op over {} calls",
        amend_size_elapsed_ns / result.amend_size_count,
        result.amend_size_count,
    );
}
