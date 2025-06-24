mod commands;

use std::fs::create_dir_all;
use nova::application::App;
use nova::core::zip::Zip;
use tracing::{info, Level};
use tracing_subscriber::fmt::time::OffsetTime;
use crate::commands::file_system::file_system::*;
use crate::commands::project::project::*;
use time::{UtcOffset};
use time::macros::format_description;

fn setup_logging() {
    let fmt = format_description!("[year]-[month]-[day] [hour]:[minute]:[second].[subsecond digits:6]");
    let offset = UtcOffset::current_local_offset().unwrap_or(UtcOffset::UTC);
    let timer = OffsetTime::new(offset, fmt);

    let exe_path = std::env::current_exe().expect("Failed to get current exe path");
    let log_dir = exe_path.parent().unwrap().join("logs");

    create_dir_all(&log_dir).expect("Failed to create logs directory");

    let log_path = log_dir.join("nova.log");

    let writer = {
        use std::{fs::File, sync::Mutex};
        let log_file = File::create(&log_path).expect("Failed to create the log file");
        Mutex::new(log_file)
    };

    let builder = tracing_subscriber::fmt()
        .with_max_level(Level::DEBUG)
        .with_file(false)
        .with_line_number(false)
        .with_thread_ids(true) // include the thread ID of the current thread
        .with_thread_names(true)
        .with_target(true)
        .with_timer(timer)
        .with_writer(writer)
        .with_ansi(false);
    builder.init();
}
#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    setup_logging();
    let _app = App::initialize();
    
    //#[cfg(debug_assertions)]
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
            write_file,
            open_project,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
