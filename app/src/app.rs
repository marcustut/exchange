use dioxus::prelude::*;

pub fn app(cx: Scope) -> Element {
    log::info!("Hello, world!");
    cx.render(rsx! {
        body {
            class: "w-screen h-screen flex justify-center items-center bg-neutral-900 text-neutral-100",
            div {
                "Hello, world!"
            }
        }
    })
}
