#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use std::{cell::UnsafeCell, ffi::CStr, os::raw::c_void};

use libffi::high::{CType, ClosureMut3};

pub mod ffi {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

/// Error that can occur during orderbook operations such as handling limit / market orders
/// or canceling existing orders.
#[derive(Debug, thiserror::Error)]
pub enum OrderbookError {
    #[error("Order is not found")]
    OrderNotFound,

    #[error("Order size <= 0")]
    InvalidOrderSize,
}

impl From<ffi::orderbook_error> for OrderbookError {
    fn from(value: ffi::orderbook_error) -> Self {
        match value {
            ffi::orderbook_error_OBERR_ORDER_NOT_FOUND => Self::OrderNotFound,
            ffi::orderbook_error_OBERR_INVALID_ORDER_SIZE => Self::InvalidOrderSize,
            _ => unreachable!(),
        }
    }
}

/// Reason for rejection if an order is rejected, this data is embedded in [`OrderEvent`].
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum RejectReason {
    NoError,
    NoLiquidity,
}

impl From<ffi::reject_reason> for RejectReason {
    fn from(value: ffi::reject_reason) -> Self {
        match value {
            ffi::reject_reason_REJECT_REASON_NO_ERROR => Self::NoError,
            ffi::reject_reason_REJECT_REASON_NO_LIQUIDITY => Self::NoLiquidity,
            _ => unreachable!(),
        }
    }
}

/// Order side
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Side {
    Bid,
    Ask,
}

#[cfg(feature = "random")]
impl rand::distributions::Distribution<Side> for rand::distributions::Standard {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> Side {
        match rng.gen_range(0..=1) {
            0 => Side::Bid,
            _ => Side::Ask,
        }
    }
}

impl Side {
    pub fn inverse(self) -> Self {
        match self {
            Side::Bid => Side::Ask,
            Side::Ask => Side::Bid,
        }
    }
}

impl From<ffi::side> for Side {
    fn from(value: ffi::side) -> Self {
        match value {
            ffi::side_SIDE_BID => Side::Bid,
            ffi::side_SIDE_ASK => Side::Ask,
            _ => unreachable!(),
        }
    }
}
impl From<Side> for ffi::side {
    fn from(value: Side) -> Self {
        match value {
            Side::Bid => ffi::side_SIDE_BID,
            Side::Ask => ffi::side_SIDE_ASK,
        }
    }
}

/// Orderbook that supports a maker-taker (limit-market) scheme. Note that the actual data
/// structure and its operations are implemented in C. This struct provides a safe wrapper
/// around the underlying C code through FFI.
#[derive(Debug)]
pub struct Orderbook {
    ob: UnsafeCell<ffi::orderbook>,
}

impl Orderbook {
    /// Creates a new orderbook.
    pub fn new() -> Self {
        Self {
            ob: UnsafeCell::new(unsafe { ffi::orderbook_new() }),
        }
    }

    /// Attach an event handler to handle orderbook events such as partial fills, etc.
    pub fn with_event_handler(self, handler: *mut ffi::event_handler) -> Self {
        unsafe { ffi::orderbook_set_event_handler(self.ob.get(), handler) }
        self
    }

    /// Attach an id to the orderbook
    pub fn with_id(mut self, id: u64) -> Self {
        self.ob.get_mut().id = id;
        self
    }

    /// Read the best bid / ask
    pub fn best(&self, side: Side) -> Option<&ffi::limit> {
        let tree = match side {
            Side::Bid => unsafe { *(*self.ob.get()).bid },
            Side::Ask => unsafe { *(*self.ob.get()).ask },
        };
        unsafe { tree.best.as_ref() }
    }

    /// Read the top N bids or asks from the book
    pub fn top_n(&self, side: Side, n: u32) -> Vec<ffi::limit> {
        let mut limits = Vec::with_capacity(n as usize);
        unsafe {
            let i = ffi::orderbook_top_n(
                self.ob.get(),
                side.into(),
                n,
                limits.spare_capacity_mut().as_mut_ptr() as *mut ffi::limit,
            );
            limits.set_len(i as usize);
        };
        limits
    }

    /// Place a limit order. Always a maker order (adding volume to the book).
    pub fn limit(&mut self, order: ffi::order) {
        unsafe { ffi::orderbook_limit(self.ob.get(), order) }
    }

    /// Execute an order. Always a taker order (reducing volume from the book).
    ///
    /// Note that the function returns a `u64` representing the remaining size that has not been filled.
    /// If the execute request has been fully filled then 0 will be returned.
    pub fn execute(
        &mut self,
        order_id: u64,
        side: Side,
        size: u64,
        execute_size: u64,
        is_market: bool,
    ) -> u64 {
        unsafe {
            ffi::orderbook_execute(
                self.ob.get(),
                order_id,
                side.into(),
                size,
                execute_size,
                is_market,
            )
        }
    }

    /// Cancel a limit order.
    pub fn cancel(&mut self, order_id: u64) -> Result<(), OrderbookError> {
        let err = unsafe { ffi::orderbook_cancel(self.ob.get(), order_id) };
        match err {
            ffi::orderbook_error_OBERR_OKAY => Ok(()),
            err => Err(OrderbookError::from(err)),
        }
    }

    /// Amend a limit order.
    pub fn amend_size(&mut self, order_id: u64, size: u64) -> Result<(), OrderbookError> {
        let err = unsafe { ffi::orderbook_amend_size(self.ob.get(), order_id, size) };
        match err {
            ffi::orderbook_error_OBERR_OKAY => Ok(()),
            err => Err(OrderbookError::from(err)),
        }
    }
}

impl std::fmt::Display for Orderbook {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        // Call print in C to get a heap-allocated C-string
        let char_ptr = unsafe { ffi::orderbook_print(self.ob.get()) };

        // Convert the C-string to a Rust string
        let str = unsafe { CStr::from_ptr(char_ptr) }
            .to_str()
            .unwrap()
            .to_owned();

        // Free the C-string
        unsafe { libc::free(char_ptr as *mut c_void) };

        write!(f, "{}", str)
    }
}

impl Drop for Orderbook {
    fn drop(&mut self) {
        unsafe { ffi::orderbook_free(self.ob.get()) }
    }
}

/// Type of an order event, it is embedded in [`OrderEvent`].
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum OrderStatus {
    Created,
    Cancelled,
    Rejected,
    Filled,
    PartiallyFilled,
    PartiallyFilledCancelled,
}

impl From<ffi::order_status> for OrderStatus {
    fn from(value: ffi::order_status) -> Self {
        match value {
            ffi::order_status_ORDER_STATUS_CREATED => Self::Created,
            ffi::order_status_ORDER_STATUS_CANCELLED => Self::Cancelled,
            ffi::order_status_ORDER_STATUS_REJECTED => Self::Rejected,
            ffi::order_status_ORDER_STATUS_FILLED => Self::Filled,
            ffi::order_status_ORDER_STATUS_PARTIALLY_FILLED => Self::PartiallyFilled,
            ffi::order_status_ORDER_STATUS_PARTIALLY_FILLED_CANCELLED => {
                Self::PartiallyFilledCancelled
            }
            _ => unreachable!(),
        }
    }
}

/// An event triggered when the status of an order was updated.
#[derive(Debug, Clone, PartialEq)]
pub struct OrderEvent {
    pub status: OrderStatus,
    pub order_id: u64,
    pub filled_size: u64,
    pub cum_filled_size: u64,
    pub remaining_size: u64,
    pub price: u64,
    pub side: Side,
    pub reject_reason: RejectReason,
}

impl From<ffi::order_event> for OrderEvent {
    fn from(value: ffi::order_event) -> Self {
        Self {
            status: value.status.into(),
            order_id: value.order_id,
            filled_size: value.filled_size,
            cum_filled_size: value.cum_filled_size,
            remaining_size: value.remaining_size,
            price: value.price,
            side: value.side.into(),
            reject_reason: value.reject_reason.into(),
        }
    }
}

// An event triggered when a trade occurs.
#[derive(Debug, Clone, PartialEq)]
pub struct TradeEvent {
    pub size: u64,
    pub price: u64,
    pub side: Side,
    pub buyer_order_id: u64,
    pub seller_order_id: u64,
}

impl From<ffi::trade_event> for TradeEvent {
    fn from(value: ffi::trade_event) -> Self {
        Self {
            size: value.size,
            price: value.price,
            side: value.side.into(),
            buyer_order_id: value.buyer_order_id,
            seller_order_id: value.seller_order_id,
        }
    }
}

/// A safe wrapper to help construct [`ffi::event_handler`] with an added feature of allowing
/// dependency injection through context.
pub struct EventHandlerBuilder<Ctx> {
    order_event_handler: Option<Box<dyn Fn(&mut Ctx, u64, OrderEvent)>>,
    trade_event_handler: Option<Box<dyn Fn(&mut Ctx, u64, TradeEvent)>>,
    ctx: Ctx,
}

impl<Ctx> EventHandlerBuilder<Ctx> {
    pub fn with_context(ctx: Ctx) -> Self {
        Self {
            order_event_handler: None,
            trade_event_handler: None,
            ctx,
        }
    }

    pub fn on_order(mut self, handler: impl Fn(&mut Ctx, u64, OrderEvent) + 'static) -> Self {
        self.order_event_handler = Some(Box::new(handler));
        self
    }

    pub fn on_trade(mut self, handler: impl Fn(&mut Ctx, u64, TradeEvent) + 'static) -> Self {
        self.trade_event_handler = Some(Box::new(handler));
        self
    }

    pub fn build(self) -> ffi::event_handler {
        ffi::event_handler::from(self)
    }
}

impl<Ctx> From<EventHandlerBuilder<Ctx>> for ffi::event_handler {
    fn from(value: EventHandlerBuilder<Ctx>) -> Self {
        let mut event_handler = unsafe { ffi::event_handler_new() };

        event_handler.user_data = unsafe { std::mem::transmute(Box::pin(value.ctx)) };

        if let Some(handler) = value.order_event_handler {
            let closure = Box::leak(Box::new(
                move |ob_id: u64, event: ffi::order_event, user_data: *mut c_void| {
                    let ctx: *mut Ctx = user_data as *mut Ctx;
                    handler(unsafe { ctx.as_mut() }.unwrap(), ob_id, event.into());
                },
            ));
            let callback = ClosureMut3::new(closure);
            let &code = callback.code_ptr();
            let ptr: unsafe extern "C" fn(u64, ffi::order_event, *mut c_void) =
                unsafe { std::mem::transmute(code) };
            std::mem::forget(callback);
            event_handler.handle_order_event = Some(ptr);
        }

        if let Some(handler) = value.trade_event_handler {
            let closure = Box::leak(Box::new(
                move |ob_id: u64, event: ffi::trade_event, user_data: *mut c_void| {
                    let ctx = user_data as *mut Ctx;
                    handler(unsafe { ctx.as_mut() }.unwrap(), ob_id, event.into());
                },
            ));
            let callback = ClosureMut3::new(closure);
            let &code = callback.code_ptr();
            let ptr: unsafe extern "C" fn(u64, ffi::trade_event, *mut c_void) =
                unsafe { std::mem::transmute(code) };
            std::mem::forget(callback);
            event_handler.handle_trade_event = Some(ptr);
        }

        event_handler
    }
}

unsafe impl CType for ffi::order_event {
    fn reify() -> libffi::high::Type<Self> {
        libffi::high::Type::make(libffi::middle::Type::structure([
            libffi::middle::Type::c_uint(), // status
            libffi::middle::Type::u64(),    // order_id
            libffi::middle::Type::u64(),    // filled_size
            libffi::middle::Type::u64(),    // cum_filled_size
            libffi::middle::Type::u64(),    // remaining_size
            libffi::middle::Type::u64(),    // price
            libffi::middle::Type::c_uint(), // side
            libffi::middle::Type::c_uint(), // reject_reason
        ]))
    }

    type RetType = ffi::order_event;
}

unsafe impl CType for ffi::trade_event {
    fn reify() -> libffi::high::Type<Self> {
        libffi::high::Type::make(libffi::middle::Type::structure([
            libffi::middle::Type::u64(),    // size
            libffi::middle::Type::u64(),    // price
            libffi::middle::Type::c_uint(), // side
        ]))
    }

    type RetType = ffi::trade_event;
}

#[cfg(test)]
mod tests {
    use std::ptr;

    use super::*;

    #[test]
    fn test_best() {
        let mut ob = Orderbook::new();
        assert!(ob.best(Side::Bid).is_none());
        ob.limit(ffi::order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
            user_data: ptr::null_mut(),
        });
        assert!(ob
            .best(Side::Bid)
            .is_some_and(|best| best.price == 1000 && best.volume == 10));
        ob.cancel(1).unwrap();
        assert!(ob.best(Side::Bid).is_none());
    }

    #[test]
    fn test_top_n() {
        let mut ob = Orderbook::new();
        assert_eq!(ob.top_n(Side::Bid, 5).len(), 0);
        ob.limit(ffi::order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
            user_data: ptr::null_mut(),
        });
        assert_eq!(ob.top_n(Side::Bid, 5).len(), 1);
        ob.cancel(1).unwrap();
        assert_eq!(ob.top_n(Side::Bid, 5).len(), 0);
    }

    #[test]
    fn test_limit() {
        let mut ob = Orderbook::new();
        let mut size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 0);
        ob.limit(ffi::order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
            user_data: ptr::null_mut(),
        });
        size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 1);
    }

    #[test]
    fn test_market() {
        let mut ob = Orderbook::new();
        let mut size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 0);
        ob.limit(ffi::order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
            user_data: ptr::null_mut(),
        });
        size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 1);
        ob.execute(2, Side::Ask, 10, 10, true);
        size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 0);
    }

    #[test]
    fn test_cancel() {
        let mut ob = Orderbook::new();
        let mut size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 0);
        ob.limit(ffi::order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
            user_data: ptr::null_mut(),
        });
        size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 1);
        ob.cancel(1).unwrap();
        size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 0);
    }

    #[test]
    fn test_amend_size() {
        let mut ob = Orderbook::new();
        ob.limit(ffi::order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
            user_data: ptr::null_mut(),
        });
        let mut order_size = unsafe { (*(*ob.ob.get_mut().bid).best).volume };
        assert_eq!(order_size, 10);
        ob.amend_size(1, 100).unwrap();
        order_size = unsafe { (*(*ob.ob.get_mut().bid).best).volume };
        assert_eq!(order_size, 100);
    }
}
