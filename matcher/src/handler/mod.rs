mod stdout;
pub use stdout::*;

#[cfg(feature = "rtrb")]
mod rtrb;
#[cfg(feature = "rtrb")]
pub use rtrb::*;

#[cfg(feature = "disruptor")]
mod disruptor;
#[cfg(feature = "disruptor")]
pub use disruptor::*;

#[derive(Debug, Clone)]
pub enum Event {
    Order {
        id: u64,
        event: orderbook::OrderEvent,
    },
    Trade {
        id: u64,
        event: orderbook::TradeEvent,
    },
}

pub trait Handler {
    type Ctx;

    fn on_order(ctx: &mut Self::Ctx, id: u64, event: orderbook::OrderEvent);
    fn on_trade(ctx: &mut Self::Ctx, id: u64, event: orderbook::TradeEvent);
}
