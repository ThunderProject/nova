use std::{env, fs, path::PathBuf};

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let header_path = manifest_dir.join("src/dicom/cxx/api");

    cxx_build::bridge("src/dicom/bridge/dicom_bridge.rs")
        .include(&header_path)
        .flag_if_supported("/std:c++latest")
        .compile("nova_dicom");

    //let profile = env::var("PROFILE").expect("Cargo should always set PROFILE");
    let profile = "release";
    let lib_dir = manifest_dir.join(format!("src/dicom/cxx/build/x86_64/{}", profile));
    
    println!("cargo:rustc-link-search=native={}", lib_dir.display());
    println!("cargo:rustc-link-core=dylib=dicom_api");
    println!("cargo:rustc-link-lib=dylib=dicom_api");
    
    if cfg!(target_os = "windows") {
        let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
        let target_dir = out_dir.ancestors().nth(3).unwrap();
        
        let dll_src = lib_dir.join("dicom_api.dll");
        let dll_dst = target_dir.join("dicom_api.dll");

        if dll_src.exists() {
            fs::copy(&dll_src, &dll_dst).expect(format!("Failed to copy dicom_api.dll into target dir. dir: {}", dll_src.to_str().unwrap()).as_str());
            println!("cargo:info=Copied dicom_api.dll to {}", dll_dst.display());
        } else {
            println!("cargo:warning=dicom_api.dll not found at {}", dll_src.display());
        }
    }
}
