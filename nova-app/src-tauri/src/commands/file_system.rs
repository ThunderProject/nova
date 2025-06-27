pub mod file_system {
    use nova::fs::file_system::file_system::FileSystem;

    #[tauri::command]
    pub async fn read_file_to_string(file: String) -> Result<String, String> {
        FileSystem::read_async(file).await
    }

    #[tauri::command]
    pub async fn create_dir(dir: String) -> bool {
        FileSystem::create_dir_async(dir).await
    }

    #[tauri::command]
    pub async fn create_dir_recursive(dir: String) -> bool {
        FileSystem::create_dir_recursive_async(dir).await
    }

    #[tauri::command]
    pub async fn remove_dir(dir: String) -> bool {
        FileSystem::remove_dir_async(dir).await
    }

    #[tauri::command]
    pub async fn remove_dir_recursive(dir: String) -> bool {
        FileSystem::remove_dir_recursive_async(dir).await
    }

    #[tauri::command]
    pub async fn remove_file(file: String) -> bool {
        FileSystem::remove_file_async(file).await
    }

    #[tauri::command]
    pub async fn rename_path(from: String, to: String) -> bool {
        FileSystem::rename_async(from, to).await
    }

    #[tauri::command]
    pub async fn path_exists(path: String) -> bool {
        FileSystem::exists_async(path).await
    }

    #[tauri::command]
    pub async fn write_file(path: String, contents: String) -> bool {
        FileSystem::write_async(path, contents).await
    }

    #[tauri::command]
    pub async fn is_empty(path: String) -> bool {
        FileSystem::is_empty_async(path).await
    }
}
