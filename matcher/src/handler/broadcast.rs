#![cfg(feature = "broadcast")]

use crate::IdGenerator;

use super::{Event, Handler};

use tokio::sync::broadcast::Sender;

#[derive(Debug)]
pub struct BroadcastHandler {
    tx: Sender<Event>,
    trade_id_gen: IdGenerator,
}

impl BroadcastHandler {
    pub fn new(tx: Sender<Event>) -> Self {
        Self {
            tx,
            trade_id_gen: IdGenerator::new(),
        }
    }
}

impl Handler for BroadcastHandler {
    type Ctx = Self;

    fn on_order(ctx: &mut Self::Ctx, id: u64, event: orderbook::OrderEvent) {
        match ctx.tx.send(Event::Order {
            ob_id: id,
            event,
            timestamp: chrono::Utc::now(),
        }) {
            Ok(_) => {}
            Err(e) => eprintln!("broadcast channel is full: {}", e),
        }
    }

    fn on_trade(ctx: &mut Self::Ctx, id: u64, event: orderbook::TradeEvent) {
        match ctx.tx.send(Event::Trade {
            ob_id: id,
            trade_id: ctx.trade_id_gen.next_id(),
            event,
            timestamp: chrono::Utc::now(),
        }) {
            Ok(_) => {}
            Err(e) => eprintln!("broadcast channel is full: {}", e),
        }
    }
}
