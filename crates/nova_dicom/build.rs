use std::{env, path::PathBuf};

fn main() {
    let manifest_dir = PathBuf::from(
        env::var("CARGO_MANIFEST_DIR").expect("Cargo should set CARGO_MANIFEST_DIR"),
    );

    let cargo_profile = env::var("PROFILE").expect("Cargo should set PROFILE");

    let cmake_profile = match cargo_profile.as_str() {
        "debug" => "Debug",
        "release" => "Release",
        other => panic!("unsupported Cargo profile: {other}"),
    };

    let packages_dir = manifest_dir
        .join("../..")
        .join("packages")
        .canonicalize()
        .expect("failed to find packages directory");

    let build_root = packages_dir
        .join("build")
        .join(cmake_profile)
        .join("cmake");

    let api_dir = build_root.join("api");
    let api_lib = api_dir.join("libnova_api.so");

    if !api_lib.exists() {
        panic!("missing native shared library: {}", api_lib.display());
    }

    cxx_build::bridge("src/bridge/dicom_bridge.rs")
        .include(packages_dir.join("api"))
        .include(&packages_dir)
        .flag_if_supported("-std=c++23")
        .compile("nova_dicom_cxx");

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/bridge/mod.rs");
    println!("cargo:rerun-if-changed=src/bridge/dicom_bridge.rs");

    println!(
        "cargo:rerun-if-changed={}",
        packages_dir.join("api/dicom_api.h").display()
    );

    println!("cargo:rerun-if-changed={}", api_lib.display());

    println!("cargo:rustc-link-search=native={}", api_dir.display());
    println!("cargo:rustc-link-lib=dylib=nova_api");
}