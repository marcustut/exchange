use std::{net::SocketAddr, ops::ControlFlow};

use axum::{
    extract::{
        connect_info::ConnectInfo,
        ws::{Message, WebSocket, WebSocketUpgrade},
        State,
    },
    response::IntoResponse,
    routing::get,
    Router,
};
use axum_extra::TypedHeader;
use ende::TryEncodeWithCtx;
use futures::{sink::SinkExt, stream::StreamExt};
use tokio::{
    net::{TcpListener, ToSocketAddrs},
    sync::broadcast::{error::RecvError, Receiver},
};

use matcher::{handler::Event, Symbol};

struct AppState {
    rx: Receiver<Event>,
}

impl Clone for AppState {
    fn clone(&self) -> Self {
        Self {
            rx: self.rx.resubscribe(),
        }
    }
}

pub async fn spawn<A>(addr: A, rx: Receiver<Event>) -> Result<(), std::io::Error>
where
    A: ToSocketAddrs,
{
    let app = Router::new()
        .route("/ws", get(ws_handler))
        .with_state(AppState { rx });

    // run it with hyper
    let listener = TcpListener::bind(addr).await?;
    println!("listening on {}", listener.local_addr()?);
    axum::serve(
        listener,
        app.into_make_service_with_connect_info::<SocketAddr>(),
    )
    .await
}

/// The handler for the HTTP request (this gets called when the HTTP GET lands at the start
/// of websocket negotiation). After this completes, the actual switching from HTTP to
/// websocket protocol will occur.
/// This is the last point where we can extract TCP/IP metadata such as IP address of the client
/// as well as things from HTTP headers such as user-agent of the browser etc.
async fn ws_handler(
    ws: WebSocketUpgrade,
    user_agent: Option<TypedHeader<headers::UserAgent>>,
    ConnectInfo(addr): ConnectInfo<SocketAddr>,
    State(AppState { rx }): State<AppState>,
) -> impl IntoResponse {
    let user_agent = if let Some(TypedHeader(user_agent)) = user_agent {
        user_agent.to_string()
    } else {
        String::from("Unknown browser")
    };
    println!("`{user_agent}` at {addr} connected.");
    // finalize the upgrade process by returning upgrade callback.
    // we can customize the callback by sending additional info such as address.
    ws.on_upgrade(move |socket| handle_socket(socket, addr, rx))
}

/// Actual websocket statemachine (one will be spawned per connection)
async fn handle_socket(mut socket: WebSocket, who: SocketAddr, mut rx: Receiver<Event>) {
    let encoder = ende::Encoder::new();

    // send a ping (unsupported by some browsers) just to kick things off and get a response
    if socket.send(Message::Ping(vec![1, 2, 3])).await.is_ok() {
        println!("Pinged {who}...");
    } else {
        println!("Could not send ping {who}!");
        // no Error here since the only thing we can do is to close the connection.
        // If we can not send messages, there is no way to salvage the statemachine anyway.
        return;
    }

    // receive single message from a client (we can either receive or send with socket).
    // this will likely be the Pong for our Ping or a hello message from client.
    // waiting for message from a client will block this task, but will not block other client's
    // connections.
    if let Some(msg) = socket.recv().await {
        if let Ok(msg) = msg {
            if process_message(msg, who).is_break() {
                return;
            }
        } else {
            println!("client {who} abruptly disconnected");
            return;
        }
    }

    // Send a welcome message to the client
    if socket
        .send(Message::Text(format!("Welcome to exchange")))
        .await
        .is_err()
    {
        println!("client {who} abruptly disconnected");
        return;
    }

    // By splitting socket we can send and receive at the same time. In this example we will send
    // unsolicited messages to client based on some sort of server's internal event (i.e .timer).
    let (mut sender, mut receiver) = socket.split();

    // Spawn a task to push events to client
    let mut send_task = tokio::spawn(async move {
        loop {
            match rx.recv().await {
                Ok(Event::Trade {
                    ob_id,
                    trade_id,
                    event,
                    timestamp,
                }) => {
                    let bytes = encoder
                        .try_encode(
                            &event,
                            (
                                trade_id,
                                Symbol::from_repr(ob_id).expect("should not fail"),
                                timestamp,
                            ),
                        )
                        .expect("should not fail");

                    // In case of any websocket error, we exit.
                    if sender.send(Message::Binary(bytes)).await.is_err() {
                        return ();
                    }
                }
                Ok(Event::Order {
                    ob_id,
                    event,
                    timestamp,
                }) =>
                // In case of any websocket error, we exit.
                {
                    // if sender
                    //     .send(Message::Text(format!("Order event {id} at {timestamp}")))
                    //     .await
                    //     .is_err()
                    // {
                    //     return ();
                    // }
                }
                Err(RecvError::Closed) => return (),
                Err(RecvError::Lagged(_)) => continue,
            }
        }
    });

    // This second task will receive messages from client and print them on server console
    let mut recv_task = tokio::spawn(async move {
        while let Some(Ok(msg)) = receiver.next().await {
            // TODO: should add to subscription
            if process_message(msg, who).is_break() {
                break;
            }
        }
        ()
    });

    // If any one of the tasks exit, abort the other.
    tokio::select! {
        rv_a = (&mut send_task) => {
            match rv_a {
                Ok(_) => println!("connection to {who} was closed"),
                Err(a) => println!("Error sending messages {a:?}")
            }
            recv_task.abort();
        },
        rv_b = (&mut recv_task) => {
            match rv_b {
                Ok(_) => println!("connection to {who} was closed"),
                Err(b) => println!("Error receiving messages {b:?}")
            }
            send_task.abort();
        }
    }

    // returning from the handler closes the websocket connection
    println!("Websocket context {who} destroyed");
}

/// helper to print contents of messages to stdout. Has special treatment for Close.
fn process_message(msg: Message, who: SocketAddr) -> ControlFlow<(), ()> {
    match msg {
        Message::Text(t) => {
            println!(">>> {who} sent str: {t:?}");
        }
        Message::Binary(d) => {
            println!(">>> {} sent {} bytes: {:?}", who, d.len(), d);
        }
        Message::Close(c) => {
            if let Some(cf) = c {
                println!(
                    ">>> {} sent close with code {} and reason `{}`",
                    who, cf.code, cf.reason
                );
            } else {
                println!(">>> {who} somehow sent close message without CloseFrame");
            }
            return ControlFlow::Break(());
        }

        Message::Pong(v) => {
            println!(">>> {who} sent pong with {v:?}");
        }
        // You should never need to manually handle Message::Ping, as axum's websocket library
        // will do so for you automagically by replying with Pong and copying the v according to
        // spec. But if you need the contents of the pings you can see them here.
        Message::Ping(v) => {
            println!(">>> {who} sent ping with {v:?}");
        }
    }
    ControlFlow::Continue(())
}
