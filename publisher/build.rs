fn main() {
    println!("cargo:rustc-link-lib=z");
    println!("cargo:rustc-link-lib=uv");
    println!("cargo:rustc-link-lib=ssl");
    println!("cargo:rustc-link-lib=crypto");
    println!("cargo:rustc-link-lib=stdc++");
}
