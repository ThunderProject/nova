use std::path::Path;

pub mod file_system {
    use std::path::Path;
    use tokio::fs;

    #[tauri::command]
    pub async fn read_file_to_string(file: String) -> Result<String, String> {
        FileSystem::read(file).await
    }

    #[tauri::command]
    pub async fn create_dir(dir: String) -> bool {
        FileSystem::create_dir(dir).await
    }

    #[tauri::command]
    pub async fn create_dir_recursive(dir: String) -> bool {
        FileSystem::create_dir_recursive(dir).await
    }

    #[tauri::command]
    pub async fn remove_dir(dir: String) -> bool {
        FileSystem::remove_dir(dir).await
    }

    #[tauri::command]
    pub async fn remove_dir_recursive(dir: String) -> bool {
        FileSystem::remove_dir_recursive(dir).await
    }

    #[tauri::command]
    pub async fn remove_file(file: String) -> bool {
        FileSystem::remove_file(file).await
    }

    #[tauri::command]
    pub async fn rename_path(from: String, to: String) -> bool {
        FileSystem::rename(from, to).await
    }

    #[tauri::command]
    pub async fn path_exists(path: String) -> bool {
        FileSystem::exists(path).await
    }

    #[tauri::command]
    pub async fn write_file(path: String, contents: String) -> bool {
        FileSystem::write(path, contents).await
    }

    struct FileSystem;

    impl FileSystem {
        pub async fn read(path: impl AsRef<Path>) -> Result<String, String> {
            match fs::read_to_string(path).await {
                Ok(content) => Ok(content),
                Err(_) => Err("Failed to read file".into()),
            }
        }

        pub async fn create_dir(path: impl AsRef<Path>) -> bool {
            match fs::create_dir(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn create_dir_recursive(path: impl AsRef<Path>) -> bool {
            match fs::create_dir_all(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn remove_dir(path: impl AsRef<Path>) -> bool {
            match fs::remove_dir(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn remove_dir_recursive(path: impl AsRef<Path>) -> bool {
            match fs::remove_dir_all(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn remove_file(path: impl AsRef<Path>) -> bool {
            match fs::remove_file(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn rename(from: impl AsRef<Path>, to: impl AsRef<Path>) -> bool {
            match fs::rename(from, to).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn exists(path: impl AsRef<Path>) -> bool {
            match fs::try_exists(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn write(path: impl AsRef<Path>, contents: impl AsRef<[u8]>) -> bool {
            match fs::write(path, contents).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }
    }
}
