use crate::Symbol;

pub trait Handler {
    type Ctx;

    fn on_order(ctx: &mut Self::Ctx, id: u64, event: orderbook::OrderEvent);
    fn on_trade(ctx: &mut Self::Ctx, id: u64, event: orderbook::TradeEvent);
}

pub struct StdOutHandler {
    counter: u64,
}

impl StdOutHandler {
    pub fn new() -> Self {
        Self { counter: 0 }
    }
}

impl Handler for StdOutHandler {
    type Ctx = Self;

    fn on_order(ctx: &mut Self::Ctx, id: u64, event: orderbook::OrderEvent) {
        ctx.counter += 1;
        println!("[{:?}] {} {:?}", Symbol::from_repr(id), ctx.counter, event)
    }

    fn on_trade(ctx: &mut Self::Ctx, id: u64, event: orderbook::TradeEvent) {
        ctx.counter += 1;
        println!("[{:?}] {} {:?}", Symbol::from_repr(id), ctx.counter, event)
    }
}
