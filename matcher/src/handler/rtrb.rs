#![cfg(feature = "rtrb")]

use std::{
    sync::{atomic, Arc},
    thread,
};

use super::{Event, Handler};

use rtrb::{Consumer, Producer};

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
        core_id: Option<usize>,
        handler: impl Fn(&mut Ctx, Event) + Send + Sync + 'static,
    ) -> Handle {
        let shutdown = Arc::new(atomic::AtomicBool::new(false));

        let _shutdown = shutdown.clone();
        let handle = thread::spawn(move || {
            if let Some(id) = core_id {
                core_affinity::set_for_current(core_affinity::CoreId { id });
            }

            let mut ctx = ctx;
            loop {
                // NOTE: This is stupid, it incurs additional overhead in a busy-spin thread (there
                // is better way to do this where maybe we only check shutdown every few iterations)
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
        match ctx.tx.push(Event::Order {
            id,
            event,
            timestamp: chrono::Utc::now(),
        }) {
            Ok(_) => {}
            Err(e) => eprintln!("rtrb is full: {}", e),
        }
    }

    fn on_trade(ctx: &mut Self::Ctx, id: u64, event: orderbook::TradeEvent) {
        match ctx.tx.push(Event::Trade {
            id,
            event,
            timestamp: chrono::Utc::now(),
        }) {
            Ok(_) => {}
            Err(e) => eprintln!("rtrb is full: {}", e),
        }
    }
}
