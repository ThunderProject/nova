mod commands;
use crate::commands::file_system::file_system::*;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    #[cfg(debug_assertions)]
    // let devtools = tauri_plugin_devtools::init();
    let fs = tauri_plugin_fs::init();
    let mut builder = tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(fs);

    #[cfg(debug_assertions)]
    {
        // builder = builder
        //     .plugin(devtools)
        //     .plugin(fs);
    }

    builder
        .invoke_handler(tauri::generate_handler![
            read_file_to_string,
            create_dir,
            create_dir_recursive,
            remove_dir,
            remove_dir_recursive,
            remove_file,
            rename_path,
            path_exists,
            write_file
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
