#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use std::{cell::UnsafeCell, ffi::CStr, os::raw::c_void};

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[derive(Debug, thiserror::Error)]
pub enum OrderbookError {
    #[error("Order is not found")]
    OrderNotFound,

    #[error("Order size <= 0")]
    InvalidOrderSize,

    #[error("Unknown")]
    Unknown,
}

impl From<orderbook_error> for OrderbookError {
    fn from(value: orderbook_error) -> Self {
        match value {
            orderbook_error_OBERR_ORDER_NOT_FOUND => Self::OrderNotFound,
            orderbook_error_OBERR_INVALID_ORDER_SIZE => Self::InvalidOrderSize,
            _ => Self::Unknown,
        }
    }
}

/// Order side
#[derive(Debug, Clone, Copy)]
pub enum Side {
    Bid,
    Ask,
}

impl From<Side> for side {
    fn from(value: Side) -> Self {
        match value {
            Side::Bid => side_SIDE_BID,
            Side::Ask => side_SIDE_ASK,
        }
    }
}

#[derive(Debug)]
pub struct Orderbook {
    ob: UnsafeCell<orderbook>,
}

impl Orderbook {
    /// Creates a new orderbook.
    pub fn new() -> Self {
        Self {
            ob: UnsafeCell::new(unsafe { orderbook_new() }),
        }
    }

    /// Read the top N bids or asks from the book
    pub fn top_n(&self, side: Side, n: u32) -> Vec<limit> {
        let mut limits = Vec::with_capacity(n as usize);
        unsafe {
            let i = orderbook_top_n(
                self.ob.get(),
                side.into(),
                n,
                limits.spare_capacity_mut().as_mut_ptr() as *mut limit,
            );
            limits.set_len(i as usize);
        };
        limits
    }

    /// Place a limit order. Always a maker order (adding volume to the book).
    pub fn limit(&mut self, order: order) {
        unsafe { orderbook_limit(self.ob.get(), order) }
    }

    /// Place a market order. Always a taker order (reducing volume from the book).
    ///
    /// Note that the function returns a `u64` representing the remaining size that has not been filled.
    /// If the market request has been fully filled then 0 will be returned.
    pub fn market(&mut self, order_id: u64, side: Side, size: u64) -> u64 {
        unsafe { orderbook_market(self.ob.get(), order_id, side.into(), size) }
    }

    /// Cancel a limit order.
    pub fn cancel(&mut self, order_id: u64) -> Result<(), OrderbookError> {
        let err = unsafe { orderbook_cancel(self.ob.get(), order_id) };
        match err {
            orderbook_error_OBERR_OKAY => Ok(()),
            err => Err(OrderbookError::from(err)),
        }
    }

    /// Amend a limit order.
    pub fn amend_size(&mut self, order_id: u64, size: u64) -> Result<(), OrderbookError> {
        let err = unsafe { orderbook_amend_size(self.ob.get(), order_id, size) };
        match err {
            orderbook_error_OBERR_OKAY => Ok(()),
            err => Err(OrderbookError::from(err)),
        }
    }
}

impl std::fmt::Display for Orderbook {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        // Call print in C to get a heap-allocated C-string
        let char_ptr = unsafe { orderbook_print(self.ob.get()) };

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
        unsafe { orderbook_free(self.ob.get()) }
    }
}

#[cfg(test)]
mod tests {
    use std::ptr;

    use super::*;

    #[test]
    fn test_top_n() {
        let mut ob = Orderbook::new();
        assert_eq!(ob.top_n(Side::Bid, 5).len(), 0);
        ob.limit(order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
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
        ob.limit(order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
        });
        size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 1);
    }

    #[test]
    fn test_market() {
        let mut ob = Orderbook::new();
        let mut size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 0);
        ob.limit(order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
        });
        size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 1);
        ob.market(2, Side::Ask, 10);
        size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 0);
    }

    #[test]
    fn test_cancel() {
        let mut ob = Orderbook::new();
        let mut size = unsafe { (*(ob.ob.get_mut().bid)).size };
        assert_eq!(size, 0);
        ob.limit(order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
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
        ob.limit(order {
            order_id: 1,
            price: 1000,
            size: 10,
            cum_filled_size: 0,
            side: Side::Bid.into(),
            limit: ptr::null_mut(),
            prev: ptr::null_mut(),
            next: ptr::null_mut(),
        });
        let mut order_size = unsafe { (*(*ob.ob.get_mut().bid).best).volume };
        assert_eq!(order_size, 10);
        ob.amend_size(1, 100).unwrap();
        order_size = unsafe { (*(*ob.ob.get_mut().bid).best).volume };
        assert_eq!(order_size, 100);
    }
}
