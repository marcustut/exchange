use uwebsockets_rs::{
    app::App,
    listen_socket::ListenSocket,
    us_socket_context_options::UsSocketContextOptions,
    websocket::Opcode,
    websocket_behavior::{CompressOptions, WebSocketBehavior},
};

pub fn spawn(port: i32, config: UsSocketContextOptions) {
    let websocket_behavior = WebSocketBehavior {
        compression: CompressOptions::SharedCompressor.into(),
        max_payload_length: 100 * 1024 * 1024,
        idle_timeout: 16,
        max_backpressure: 100 * 1024 * 1024,
        close_on_backpressure_limit: false,
        reset_idle_timeout_on_send: false,
        send_pings_automatically: true,
        max_lifetime: 16,
        upgrade: None,
        open: Some(Box::new(|ws| {
            println!("WS is opened");

            let data = ws.get_user_data::<String>();
            println!("{data:#?}");
        })),
        message: Some(Box::new(|ws, message, opcode| {
            let user_data = ws.get_user_data::<()>();
            println!("User data: {user_data:#?}");
            println!("{message:#?}");
            if opcode == Opcode::Text {
                let message = std::str::from_utf8(message).unwrap();
                println!("Message: {message}");
            }

            ws.send_with_options(message, opcode, true, true);
        })),
        ping: Some(Box::new(|_, message| {
            println!("Got PING, message: {message:#?}");
        })),
        pong: Some(Box::new(|_, message| {
            println!("Got PONG,  message: {message:#?}");
        })),
        close: Some(Box::new(|_, code, message| {
            println!("WS closed, code: {code}, message: {message:#?}")
        })),
        drain: Some(Box::new(|_| {
            println!("DRAIN");
        })),
        subscription: Some(Box::new(|_, topic, current_subs, prev_subs| {
            println!("SUBSCRIPTION: topic: {topic}, current_subs: {current_subs}, prev_subs: {prev_subs}");
        })),
    };

    App::new(config)
        .ws("/ws", websocket_behavior)
        .listen(port, None::<fn(ListenSocket)>)
        .run();
}
