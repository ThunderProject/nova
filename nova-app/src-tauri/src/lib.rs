mod FileManager;
use tokio::runtime::Runtime;

#[tauri::command]
fn greet(name: &str) -> String {
    println!("Hello from tauri, {}!", name);
    format!("Hello from tauri, {}!", name)
}

#[tauri::command]
async fn read_file_to_string(name: String) -> Result<String, String> {
    FileManager::FileManager::read(name).await
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    #[cfg(debug_assertions)]
    // let devtools = tauri_plugin_devtools::init();
    let fs = tauri_plugin_fs::init();
    let mut builder = tauri::Builder::default().plugin(fs);

    #[cfg(debug_assertions)]
    {
        // builder = builder
        //     .plugin(devtools)
        //     .plugin(fs);
    }

    builder
        .invoke_handler(tauri::generate_handler![greet, read_file_to_string])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
