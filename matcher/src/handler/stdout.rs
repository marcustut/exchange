use crate::Symbol;

use super::Handler;

pub struct StdOutHandler {
    id_as_symbol: bool,
    counter: u64,
}

impl StdOutHandler {
    pub fn new() -> Self {
        Self {
            id_as_symbol: false,
            counter: 0,
        }
    }

    pub fn with_id_as_symbol(mut self, id_as_symbol: bool) -> Self {
        self.id_as_symbol = id_as_symbol;
        self
    }
}

impl Handler for StdOutHandler {
    type Ctx = Self;

    fn on_order(ctx: &mut Self::Ctx, id: u64, event: orderbook::OrderEvent) {
        ctx.counter += 1;
        let id = match ctx.id_as_symbol {
            true => match Symbol::from_repr(id) {
                Some(symbol) => symbol.to_string(),
                None => id.to_string(),
            },
            false => id.to_string(),
        };
        println!("[{:?}] {} {:?}", id, ctx.counter, event)
    }

    fn on_trade(ctx: &mut Self::Ctx, id: u64, event: orderbook::TradeEvent) {
        ctx.counter += 1;
        let id = match ctx.id_as_symbol {
            true => match Symbol::from_repr(id) {
                Some(symbol) => symbol.to_string(),
                None => id.to_string(),
            },
            false => id.to_string(),
        };
        println!("[{:?}] {} {:?}", id, ctx.counter, event)
    }
}
