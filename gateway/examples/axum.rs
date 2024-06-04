fn main() {
    let rt = tokio::runtime::Builder::new_multi_thread()
        .enable_io()
        .enable_time()
        .build()
        .unwrap();

    rt.block_on(publisher::axum::spawn("127.0.0.1:8080"))
        .unwrap();
}
