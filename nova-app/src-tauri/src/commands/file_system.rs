use authenticated_command::authenticated_command;
use nova_fs::file_system::FileSystem;

#[authenticated_command]
pub async fn read_file_to_string(file: String) -> Result<String, String> {
    FileSystem::read_async(file).await
}

#[authenticated_command]
pub fn join(parts: Vec<String>) -> Result<String, String> {
    Ok(FileSystem::join(parts))
}

#[tauri::command]
pub async fn create_dir(dir: String) -> Result<bool, String> {
    Ok(FileSystem::create_dir_async(dir).await)
}

#[authenticated_command]
pub async fn create_dir_recursive(dir: String) -> Result<bool, String> {
    Ok(FileSystem::create_dir_recursive_async(dir).await)
}

#[authenticated_command]
pub async fn remove_dir(dir: String) -> Result<bool, String> {
    Ok(FileSystem::remove_dir_async(dir).await)
}

#[authenticated_command]
pub async fn remove_dir_recursive(dir: String) -> Result<bool, String> {
    Ok(FileSystem::remove_dir_recursive_async(dir).await)
}

#[authenticated_command]
pub async fn remove_file(file: String) -> Result<bool, String> {
    Ok(FileSystem::remove_file_async(file).await)
}

#[authenticated_command]
pub async fn rename_path(from: String, to: String) -> Result<bool, String> {
    Ok(FileSystem::rename_async(from, to).await)
}

#[authenticated_command]
pub async fn path_exists(path: String) -> Result<bool, String> {
    Ok(FileSystem::exists_async(path).await)
}

#[authenticated_command]
pub async fn write_file(path: String, contents: String) -> Result<bool, String> {
    Ok(FileSystem::write_async(path, contents).await)
}

#[authenticated_command]
pub async fn is_empty(path: String) -> Result<bool, String> {
    Ok(FileSystem::is_empty_async(path).await)
}
