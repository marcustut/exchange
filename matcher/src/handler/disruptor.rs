#![cfg(feature = "disruptor")]

use super::{Event, Handler};

use disruptor::{Producer, Sequence, SingleConsumerBarrier, SingleProducer};

pub struct DisruptorHandler {
    tx: SingleProducer<Event, SingleConsumerBarrier>,
}

impl DisruptorHandler {
    pub fn new(tx: SingleProducer<Event, SingleConsumerBarrier>) -> Self {
        Self { tx }
    }

    pub fn handle<Ctx>(
        mut ctx: Ctx,
        handler: impl Fn(&mut Ctx, &Event, Sequence, bool),
    ) -> impl FnMut(&Event, Sequence, bool) {
        move |event: &Event, seq: Sequence, end_of_batch: bool| {
            let ctx = &mut ctx;
            handler(ctx, event, seq, end_of_batch)
        }
    }
}

impl Handler for DisruptorHandler {
    type Ctx = Self;

    fn on_order(ctx: &mut Self::Ctx, id: u64, event: orderbook::OrderEvent) {
        ctx.tx.publish(|e| {
            *e = Event::Order {
                id,
                event,
                timestamp: chrono::Utc::now(),
            };
        })
    }

    fn on_trade(ctx: &mut Self::Ctx, id: u64, event: orderbook::TradeEvent) {
        ctx.tx.publish(|e| {
            *e = Event::Trade {
                id,
                event,
                timestamp: chrono::Utc::now(),
            };
        })
    }
}
