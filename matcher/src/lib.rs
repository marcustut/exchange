use orderbook::{ffi, Orderbook, OrderbookError};

pub mod handler;
mod symbol_table;

pub use orderbook::{EventHandlerBuilder, OrderEvent, Side, TradeEvent};
pub use symbol_table::{Symbol, SymbolTable};

#[derive(Debug, Clone, Copy)]
pub struct IdGenerator {
    current: u64,
}

impl IdGenerator {
    pub fn new() -> Self {
        Self { current: 0 }
    }

    pub fn next_id(&mut self) -> u64 {
        self.current += 1;
        self.current
    }
}

#[derive(Debug, Clone)]
pub struct Order {
    pub size: f64,
    pub price: f64,
    pub side: Side,
}

impl Order {
    #[cfg(feature = "random")]
    pub fn random(
        price_range: impl rand::distributions::uniform::SampleRange<f64>,
        size_range: impl rand::distributions::uniform::SampleRange<f64>,
    ) -> Self {
        use rand::Rng;
        let mut rng = rand::thread_rng();

        Self {
            price: rng.gen_range(price_range),
            size: rng.gen_range(size_range),
            side: rng.gen(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct SymbolMetadata {
    pub symbol: String,
    pub price_precision: u8,
    pub size_precision: u8,
}

struct SymbolEntry {
    ob: Orderbook,
    metadata: SymbolMetadata,
}

#[derive(Debug, thiserror::Error)]
pub enum MatcherError {
    #[error("Symbol {0:?} not found")]
    SymbolNotFound(Symbol),
    #[error("Orderbook error: {0}")]
    Orderbook(#[from] OrderbookError),
}

/// Order matching engine that supports multiple symbols, the matching process is always
/// single-threaded. To utilise more CPU cores, it is recommended that the user split up
/// different symbols and spawn multiple matchers on different threads.
///
/// NOTE: The `event_handler` has a lifetime guarantee that as long as the matcher is in
/// scope, the event_handler must also be in scope, otherwise stack-use-after-scope might
/// occur in the underlying C orderbook lib.
pub struct Matcher<'event_handler> {
    books: SymbolTable<SymbolEntry>,
    order_id_gen: IdGenerator,
    event_handler: &'event_handler mut ffi::event_handler,
}

impl<'event_handler> Matcher<'event_handler> {
    pub fn new(event_handler: &'event_handler mut ffi::event_handler) -> Self {
        Self {
            books: SymbolTable::new(),
            order_id_gen: IdGenerator::new(),
            event_handler,
        }
    }

    pub fn add_symbol(&mut self, symbol: Symbol, metadata: SymbolMetadata) {
        let ob = Orderbook::new()
            .with_id(symbol as u64)
            .with_event_handler(self.event_handler);
        self.books.insert(symbol, SymbolEntry { ob, metadata });
    }

    pub fn get_orderbook(&self, symbol: Symbol) -> Option<&Orderbook> {
        match self.books.get(symbol) {
            None => None,
            Some(book) => Some(&book.ob),
        }
    }

    pub fn get_symbol_table(&self) -> Vec<(u64, &SymbolMetadata)> {
        self.books
            .table
            .iter()
            .filter_map(|entry| entry.as_ref().map(|e| (0u64, &e.metadata)))
            .collect::<Vec<_>>()
    }

    pub fn cancel(&mut self, symbol: Symbol, order_id: u64) -> Result<(), MatcherError> {
        let book = match self.books.get_mut(symbol) {
            None => return Err(MatcherError::SymbolNotFound(symbol)),
            Some(book) => book,
        };

        book.ob.cancel(order_id)?;

        Ok(())
    }

    pub fn amend_size(
        &mut self,
        symbol: Symbol,
        order_id: u64,
        size: u64,
    ) -> Result<(), MatcherError> {
        let book = match self.books.get_mut(symbol) {
            None => return Err(MatcherError::SymbolNotFound(symbol)),
            Some(book) => book,
        };

        book.ob.amend_size(order_id, size)?;

        Ok(())
    }

    pub fn order(&mut self, symbol: Symbol, order: Order) -> Result<(), MatcherError> {
        let book = match self.books.get_mut(symbol) {
            None => return Err(MatcherError::SymbolNotFound(symbol)),
            Some(book) => book,
        };

        let order_id = self.order_id_gen.next_id();
        let size = scale_float(order.size, book.metadata.size_precision);

        // market if price is zero
        if order.price == 0.0 {
            book.ob.execute(order_id, order.side, size, size, true);

        // limit otherwise
        } else {
            let price = scale_float(order.price, book.metadata.price_precision);

            // If price is equals to current best on the other side, execute it
            match book.ob.best(order.side.inverse()) {
                Some(best) if best.price == price => {
                    let remaining_size = book.ob.execute(
                        order_id,
                        order.side,
                        size,
                        std::cmp::min(best.volume, size),
                        false,
                    );

                    // let the remaining go through as limit order
                    if remaining_size > 0 {
                        book.ob.limit(orderbook::ffi::order {
                            order_id,
                            price,
                            size: remaining_size,
                            cum_filled_size: size - remaining_size,
                            side: order.side.into(),
                            limit: std::ptr::null_mut(),
                            prev: std::ptr::null_mut(),
                            next: std::ptr::null_mut(),
                            user_data: std::ptr::null_mut(),
                        });
                    }
                }
                _ => {
                    book.ob.limit(orderbook::ffi::order {
                        order_id,
                        price,
                        size,
                        cum_filled_size: 0,
                        side: order.side.into(),
                        limit: std::ptr::null_mut(),
                        prev: std::ptr::null_mut(),
                        next: std::ptr::null_mut(),
                        user_data: std::ptr::null_mut(),
                    });
                }
            }
        }

        Ok(())
    }
}

fn scale_float(num: f64, precision: u8) -> u64 {
    (num * 10_u32.pow(precision as u32) as f64) as u64
}
