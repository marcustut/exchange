use std::{
    borrow::Borrow,
    env,
    ffi::OsStr,
    path::{Path, PathBuf},
    process::Command,
};

fn run_command_or_fail<P, S>(dir: &str, cmd: P, args: &[S])
where
    P: AsRef<Path>,
    S: Borrow<str> + AsRef<OsStr>,
{
    let cmd = cmd.as_ref();
    let cmd = if cmd.components().count() > 1 && cmd.is_relative() {
        // If `cmd` is a relative path (and not a bare command that should be
        // looked up in PATH), absolutize it relative to `dir`, as otherwise the
        // behavior of std::process::Command is undefined.
        // https://github.com/rust-lang/rust/issues/37868
        PathBuf::from(dir)
            .join(cmd)
            .canonicalize()
            .expect("canonicalization failed")
    } else {
        PathBuf::from(cmd)
    };
    eprintln!(
        "Running command: \"{} {}\" in dir: {}",
        cmd.display(),
        args.join(" "),
        dir
    );
    let ret = Command::new(cmd).current_dir(dir).args(args).status();
    match ret.map(|status| (status.success(), status.code())) {
        Ok((true, _)) => (),
        Ok((false, Some(c))) => panic!("Command failed with error code {}", c),
        Ok((false, None)) => panic!("Command got killed"),
        Err(e) => panic!("Command failed with error: {}", e),
    }
}

fn main() {
    let out_dir = env::var("OUT_DIR").expect("OUT_DIR missing");

    println!("Building orderbook");

    // Setup meson build dir
    run_command_or_fail(".", "meson", &["setup", "--wipe", "build"]);
    // Run ninja build
    run_command_or_fail(".", "ninja", &["-C", "build"]);
    // Copy built lib to OUT_DIR
    run_command_or_fail(".", "cp", &["build/liborderbook.a", &out_dir]);
    run_command_or_fail(".", "cp", &["-r", "build/liborderbook.a.p", &out_dir]);
    run_command_or_fail(".", "cp", &["build/liborderbook.so", &out_dir]);
    run_command_or_fail(".", "cp", &["-r", "build/liborderbook.so.p", &out_dir]);

    // Tell cargo to look for built libraries in the specified directory
    println!("cargo:rustc-link-search={}", out_dir);
    // Tell cargo to tell rustc to link our library. Cargo will
    // automatically know it must look for a `liborderbook.a` file.
    println!("cargo:rustc-link-lib=orderbook");
    println!("cargo:root={}", out_dir);

    // Configure bindgen
    let bindings = bindgen::Builder::default()
        .header("include/orderbook.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .formatter(bindgen::Formatter::Rustfmt)
        .generate()
        .expect("Failed to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(out_dir);
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
