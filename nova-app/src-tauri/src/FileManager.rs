use std::path::Path;
pub struct FileManager;

impl FileManager {
    pub async fn read(path: impl AsRef<Path>) -> Result<String, String> {
        match tokio::fs::read_to_string(&path).await {
            Ok(content) => Ok(content),
            Err(_) => Err("Failed to read file".into()),
        }
    }
}