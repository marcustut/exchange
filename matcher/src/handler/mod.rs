mod stdout;
pub use stdout::*;

#[cfg(feature = "rtrb")]
mod rtrb;
#[cfg(feature = "rtrb")]
pub use rtrb::*;

pub trait Handler {
    type Ctx;

    fn on_order(ctx: &mut Self::Ctx, id: u64, event: orderbook::OrderEvent);
    fn on_trade(ctx: &mut Self::Ctx, id: u64, event: orderbook::TradeEvent);
}
