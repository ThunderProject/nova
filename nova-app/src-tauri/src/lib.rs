mod commands;
mod auth_state;

use std::fs::create_dir_all;
use ::nova::application::App;
use tracing::{Event, Level, Subscriber};
use crate::commands::log::*;
use time::{OffsetDateTime, UtcOffset};
use time::macros::format_description;
use tracing_subscriber::{fmt};
use tracing_subscriber::fmt::{FormatEvent, FormatFields};
use tracing_subscriber::registry::LookupSpan;
use crate::auth_state::auth_state::AuthState;
use crate::commands::auth::login;
use crate::commands::file_system::*;
use crate::commands::project::*;

struct LogFormatter;
impl<S, N> FormatEvent<S, N> for LogFormatter
where
    S: Subscriber + for<'a> LookupSpan<'a>,
    N: for<'a> FormatFields<'a> + 'static,
{
    fn format_event(
        &self,
        ctx: &fmt::FmtContext<'_, S, N>,
        mut writer: fmt::format::Writer<'_>,
        event: &Event<'_>,
    ) -> std::fmt::Result {

        let meta = event.metadata();
        let now = OffsetDateTime::now_local().unwrap_or_else(|_| OffsetDateTime::now_utc().to_offset(UtcOffset::UTC));

        let format = format_description!("[year]-[month]-[day] [hour]:[minute]:[second].[subsecond digits:6]");
        let formatted_time = now.format(&format).unwrap_or_else(|_| "unknown-time".to_string());

        write!(writer, "{} {:<5} ", formatted_time, meta.level())?;

        let thread = std::thread::current();
        let thread_id = thread.id();

        match thread.name() {
            Some(name) => write!(writer, "{name} {thread_id:?}")?,
            None => write!(writer, "{thread_id:?}")?
        }

        write!(writer, "{}: ", meta.target())?;

        ctx.format_fields(writer.by_ref(), event)?;

        writeln!(writer)
    }
}

fn setup_logging() {
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
        .with_thread_ids(true)
        .with_thread_names(true)
        .with_target(true)
        .with_writer(writer)
        .with_ansi(false)
        .event_format(LogFormatter);

    builder.init();
}
#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    setup_logging();
    let _app = App::initialize();
    
    //#[cfg(debug_assertions)]
    // let devtools = tauri_plugin_devtools::init();
    let fs = tauri_plugin_fs::init();

    let builder = tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(fs);

    #[cfg(debug_assertions)]
    {
        // builder = builder
        //     .plugin(devtools)
        //     .plugin(fs);
    }

    builder
        .manage(AuthState::default())
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
            create_new_project,
            is_empty,
            join,
            log,
            login
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
