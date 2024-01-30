mod app;
use app::app;

const TAILWIND_PATH: &'static str = debug_or_else("tailwind.css", "public/tailwind.css");

fn main() {
    run();
}

#[cfg(not(target_arch = "wasm32"))]
fn run() {
    std::env::var("RUST_LOG").unwrap_or_else(|_| {
        std::env::set_var("RUST_LOG", "info");
        "info".to_string()
    });
    pretty_env_logger::try_init_timed().expect("Failed to initialize logger");
    dioxus_desktop::launch_cfg(
        app,
        dioxus_desktop::Config::new().with_custom_head(format!(
            r#"<link rel="stylesheet" href="{}">"#,
            TAILWIND_PATH
        )),
    );
}

#[allow(unused)]
const fn debug_or_else(a: &'static str, b: &'static str) -> &'static str {
    #[cfg(debug_assertions)]
    return a;
    #[cfg(not(debug_assertions))]
    return b;
}
