#![cfg(feature = "rtrb")]

use std::{
    sync::{atomic, Arc},
    thread,
};

use super::Handler;

use rtrb::{Consumer, Producer};

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

pub struct Handle {
    handle: thread::JoinHandle<()>,
    shutdown: Arc<atomic::AtomicBool>,
}

impl Handle {
    pub fn join(self) -> Result<(), Box<dyn std::any::Any + Send + 'static>> {
        self.handle.join()
    }

    pub fn shutdown(self) -> Result<(), Box<dyn std::any::Any + Send + 'static>> {
        self.shutdown.store(true, atomic::Ordering::Relaxed);
        self.join()
    }
}

#[derive(Debug)]
#[repr(C)]
pub struct RtrbHandler {
    tx: Producer<Event>,
}

impl RtrbHandler {
    pub fn new(tx: Producer<Event>) -> Self {
        Self { tx }
    }

    pub fn spawn<Ctx: Send + Sync + 'static>(
        mut rx: Consumer<Event>,
        ctx: Ctx,
        handler: impl Fn(&mut Ctx, Event) + Send + Sync + 'static,
    ) -> Handle {
        let shutdown = Arc::new(atomic::AtomicBool::new(false));

        let _shutdown = shutdown.clone();
        let handle = thread::spawn(move || {
            let mut ctx = ctx;
            loop {
                if _shutdown.load(atomic::Ordering::Relaxed) {
                    // println!("Shutting down rtrb consumer");
                    break;
                }

                match rx.pop() {
                    Ok(msg) => handler(&mut ctx, msg),
                    Err(e) => match e {
                        rtrb::PopError::Empty => continue,
                    },
                }
            }
        });
        Handle { handle, shutdown }
    }
}

impl Handler for RtrbHandler {
    type Ctx = Self;

    fn on_order(ctx: &mut Self::Ctx, id: u64, event: orderbook::OrderEvent) {
        match ctx.tx.push(Event::Order { id, event }) {
            Ok(_) => {}
            Err(e) => eprintln!("rtrb is full: {}", e),
        }
    }

    fn on_trade(ctx: &mut Self::Ctx, id: u64, event: orderbook::TradeEvent) {
        match ctx.tx.push(Event::Trade { id, event }) {
            Ok(_) => {}
            Err(e) => eprintln!("rtrb is full: {}", e),
        }
    }
}
