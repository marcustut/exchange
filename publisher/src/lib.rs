use std::sync::Arc;

use ende::TryEncodeWithCtx;
use matcher::{handler::Event, IdGenerator, Symbol};
use tokio::io::AsyncWriteExt;

pub mod uws;

#[derive(Debug)]
pub struct TCPPublisher {}

impl TCPPublisher {
    pub fn new() -> Self {
        Self {}
    }

    pub fn spawn(&mut self, mut rx: rtrb::Consumer<Event>) {
        std::thread::spawn(move || {
            let rt = tokio::runtime::Runtime::new().unwrap();

            rt.block_on(async {
                let addr = "127.0.0.1:8080";
                let listener = tokio::net::TcpListener::bind(addr).await.unwrap();
                println!("started listening on {}", addr);

                let clients = Arc::new(tokio::sync::Mutex::new(vec![]));

                let _clients = clients.clone();
                let accept_connections = async {
                    loop {
                        match listener.accept().await {
                            Ok(client) => {
                                println!("accepted a client from {}", client.1);
                                _clients.lock().await.push(client);
                            }
                            Err(e) => eprintln!("failed accept: {}", e),
                        }
                    }
                };

                let publish_messages = tokio::spawn(async move {
                    let encoder = ende::Encoder::new();
                    let mut trade_id_gen = IdGenerator::new();
                    loop {
                        match rx.pop() {
                            Ok(event) => {
                                for (stream, addr) in clients.lock().await.iter_mut() {
                                    let bytes = match &event {
                                        Event::Order {
                                            id,
                                            event,
                                            timestamp,
                                        } => continue,
                                        Event::Trade {
                                            id,
                                            event,
                                            timestamp,
                                        } => encoder.try_encode(
                                            &event,
                                            (
                                                trade_id_gen.next_id(),
                                                Symbol::from_repr(*id).unwrap(),
                                                *timestamp,
                                            ),
                                        ),
                                    }
                                    .unwrap();
                                    match stream.write(&bytes).await {
                                        Ok(_) => {}
                                        Err(e) => {
                                            eprintln!("failed to write to {}: {}", addr, e);
                                        }
                                    }
                                }
                            }
                            Err(rtrb::PopError::Empty) => {}
                        }
                    }
                });

                tokio::select! {
                    _ = accept_connections => {}
                    _ = publish_messages => {}
                }
            });
        });
    }
}
