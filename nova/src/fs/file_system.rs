pub mod file_system {
    use futures::stream::{FuturesUnordered, StreamExt};
    use rayon::iter::{IntoParallelIterator, ParallelIterator};
    use std::path::{Path, PathBuf};

    pub struct FileSystem;
    
    impl FileSystem {

        pub async fn read_async(path: impl AsRef<Path>) -> Result<String, String> {
            match tokio::fs::read_to_string(path).await {
                Ok(content) => Ok(content),
                Err(_) => Err("Failed to read file".into()),
            }
        }

        pub fn read(path: impl AsRef<Path>) -> Result<String, String> {
            match std::fs::read_to_string(path) {
                Ok(content) => Ok(content),
                Err(_) => Err("Failed to read file".into()),
            }
        }

        pub async fn create_dir_async(path: impl AsRef<Path>) -> bool {
            match tokio::fs::create_dir(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub fn create_dir(path: impl AsRef<Path>) -> bool {
            match std::fs::create_dir(path) {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn create_dir_recursive_async(path: impl AsRef<Path>) -> bool {
            match tokio::fs::create_dir_all(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub fn create_dir_recursive(path: impl AsRef<Path>) -> bool {
            match std::fs::create_dir_all(path) {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn remove_dir_async(path: impl AsRef<Path>) -> bool {
            match tokio::fs::remove_dir(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub fn remove_dir(path: impl AsRef<Path>) -> bool {
            match std::fs::remove_dir(path) {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn remove_dir_recursive_async(path: impl AsRef<Path>) -> bool {
            match tokio::fs::remove_dir_all(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub fn remove_dir_recursive(path: impl AsRef<Path>) -> bool {
            match std::fs::remove_dir_all(path) {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn remove_file_async(path: impl AsRef<Path>) -> bool {
            match tokio::fs::remove_file(path).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub fn remove_file(path: impl AsRef<Path>) -> bool {
            match std::fs::remove_file(path) {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn rename_async(from: impl AsRef<Path>, to: impl AsRef<Path>) -> bool {
            match tokio::fs::rename(from, to).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub fn rename(from: impl AsRef<Path>, to: impl AsRef<Path>) -> bool {
            match std::fs::rename(from, to) {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn exists_async(path: impl AsRef<Path>) -> bool {
            tokio::fs::try_exists(path).await.unwrap_or_else(|_| false)
        }

        pub fn exists(path: impl AsRef<Path>) -> bool {
            std::fs::exists(path).unwrap_or_else(|_| false)
        }

        pub async fn write_async(path: impl AsRef<Path>, contents: impl AsRef<[u8]>) -> bool {
            match tokio::fs::write(path, contents).await {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub fn write(path: impl AsRef<Path>, contents: impl AsRef<[u8]>) -> bool {
            match std::fs::write(path, contents) {
                Ok(_) => true,
                Err(_) => false,
            }
        }

        pub async fn is_empty_async(path: impl AsRef<Path>) -> bool {
            let path = path.as_ref();

            if !path.is_dir() {
                return false;
            }

            match tokio::fs::read_dir(path).await {
                Ok(mut entries) => entries.next_entry().await.transpose().is_none(),
                Err(_) => false,
            }
        }

        pub fn is_empty(path: impl AsRef<Path>) -> bool {
            let path = path.as_ref();

            if !path.is_dir() {
                return false;
            }

            match std::fs::read_dir(path) {
                Ok(mut entries) => entries.next().is_none(),
                Err(_) => false,
            }
        }

        pub async fn clear_dir_async<P: AsRef<Path>>(path: P) -> std::io::Result<()> {
            let mut dir = tokio::fs::read_dir(path).await?;

            while let Some(entry) = dir.next_entry().await? {
                let path = entry.path();
                let metadata = tokio::fs::metadata(&path).await?;

                match metadata.is_dir() {
                    true => tokio::fs::remove_dir_all(&path).await?,
                    false => tokio::fs::remove_file(&path).await?,
                }
            }
            Ok(())
        }

        pub fn clear_dir<P: AsRef<Path>>(path: P) -> std::io::Result<()> {
            for entry in std::fs::read_dir(path)? {
                let entry = entry?;
                let path = entry.path();

                match path.is_dir() {
                    true => std::fs::remove_dir_all(&path)?,
                    false => std::fs::remove_file(&path)?,
                }
            }
            Ok(())
        }

        pub fn clear_dir_par<P: AsRef<Path>>(path: P) -> std::io::Result<()> {
            let entries = std::fs::read_dir(path)?
                .collect::<Result<Vec<_>, _>>()?;

            entries
                .into_par_iter()
                .map(|entry| FileSystem::remove_entry_recursive(entry.path()))
                .collect::<Result<(), _>>()
        }

        fn remove_entry_recursive(path: PathBuf) -> std::io::Result<()> {
            match path.is_dir() {
                true => {
                    let entries = match std::fs::read_dir(&path) {
                        Ok(entries) => entries.collect::<Result<Vec<_>, _>>()?,
                        Err(e) => return Err(e),
                    };

                    entries.into_par_iter()
                        .map(|entry| FileSystem::remove_entry_recursive(entry.path()))
                        .collect::<Result<(), _>>()?;

                    std::fs::remove_dir(path)
                }
                _ => std::fs::remove_file(&path),
            }
        }

        pub async fn clear_dir_par_async<P: AsRef<Path>>(path: P) -> std::io::Result<()> {
            let mut dir = tokio::fs::read_dir(path).await?;
            let mut tasks = FuturesUnordered::new();

            while let Some(entry) = dir.next_entry().await? {
                let path = entry.path();
                tasks.push(Self::remove_entry_recursive_async(path));
            }

            while let Some(result) = tasks.next().await {
                result?;
            }
            Ok(())

        }

        async fn remove_entry_recursive_async(path: PathBuf) -> std::io::Result<()> {
            let metadata = match tokio::fs::metadata(&path).await {
                Ok(meta) => meta,
                Err(e) => return Err(e),
            };

            match metadata.is_dir() {
                true => {
                    let mut dir = match tokio::fs::read_dir(&path).await {
                        Ok(d) => d,
                        Err(e) => return Err(e),
                    };

                    let mut tasks = FuturesUnordered::new();
                    while let Some(entry) = dir.next_entry().await? {
                        tasks.push(Self::remove_entry_recursive_async(entry.path()));
                    }

                    while let Some(result) = tasks.next().await {
                        result?;
                    }

                    tokio::fs::remove_dir(&path).await
                },
                _ => tokio::fs::remove_file(&path).await
            }
        }
    }
}

#[cfg(test)]
mod tests {
     use crate::fs::file_system::file_system::FileSystem;
    use tempfile::tempdir;

    #[tokio::test]
    async fn test_create_and_remove_dir_async() {
        let tmp = tempdir().unwrap();
        let dir_path = tmp.path().join("dir");
        
        assert!(FileSystem::create_dir_async(&dir_path).await);
        assert!(dir_path.exists());

        assert!(FileSystem::remove_dir_async(&dir_path).await);
        assert!(!dir_path.exists());
    }

    #[test]
    fn test_create_and_remove_dir_sync() {
        let tmp = tempdir().unwrap();
        let dir_path = tmp.path().join("dir");

        assert!(FileSystem::create_dir(&dir_path));
        assert!(dir_path.exists());

        assert!(FileSystem::remove_dir(&dir_path));
        assert!(!dir_path.exists());
    }
    
    #[tokio::test]
    async fn test_create_and_remove_dir_recursive_async() {
        let tmp = tempdir().unwrap();
        let nested_path = tmp.path().join("a/b/c");

        assert!(FileSystem::create_dir_recursive_async(&nested_path).await);
        assert!(nested_path.exists());

        assert!(FileSystem::remove_dir_recursive_async(&tmp.path().join("a")).await);
        assert!(!tmp.path().join("a").exists());
    }

    #[test]
    fn test_create_and_remove_dir_recursive_sync() {
        let tmp = tempdir().unwrap();
        let nested_path = tmp.path().join("a/b/c");

        assert!(FileSystem::create_dir_recursive(&nested_path));
        assert!(nested_path.exists());

        assert!(FileSystem::remove_dir_recursive(&tmp.path().join("a")));
        assert!(!tmp.path().join("a").exists());
    }

    #[tokio::test]
    async fn test_write_and_read_async() {
        let tmp = tempdir().unwrap();
        let file = tmp.path().join("test.txt");

        assert!(FileSystem::write_async(&file, "hello").await);
        let content = FileSystem::read_async(&file).await.unwrap();
        assert_eq!(content, "hello");
    }

    #[test]
    fn test_write_and_read_sync() {
        let tmp = tempdir().unwrap();
        let file = tmp.path().join("test.txt");

        assert!(FileSystem::write(&file, "hello"));
        let content = FileSystem::read(&file).unwrap();
        assert_eq!(content, "hello");
    }

    #[tokio::test]
    async fn test_rename_async() {
        let tmp = tempdir().unwrap();
        let src = tmp.path().join("a.txt");
        let dst = tmp.path().join("b.txt");

        FileSystem::write_async(&src, "data").await;
        assert!(FileSystem::rename_async(&src, &dst).await);
        assert!(!src.exists());
        assert!(dst.exists());
    }

    #[test]
    fn test_rename_sync() {
        let tmp = tempdir().unwrap();
        let src = tmp.path().join("a.txt");
        let dst = tmp.path().join("b.txt");

        FileSystem::write(&src, "data");
        assert!(FileSystem::rename(&src, &dst));
        assert!(!src.exists());
        assert!(dst.exists());
    }

    #[tokio::test]
    async fn test_exists_async() {
        let tmp = tempdir().unwrap();
        let file = tmp.path().join("exists.txt");

        assert!(!FileSystem::exists_async(&file).await);
        FileSystem::write_async(&file, "ok").await;
        assert!(FileSystem::exists_async(&file).await);
    }

    #[test]
    fn test_exists_sync() {
        let tmp = tempdir().unwrap();
        let file = tmp.path().join("exists.txt");

        assert!(!FileSystem::exists(&file));
        FileSystem::write(&file, "ok");
        assert!(FileSystem::exists(&file));
    }

    #[tokio::test]
    async fn test_is_empty_async() {
        let tmp = tempdir().unwrap();
        let dir = tmp.path().join("empty_dir");
        FileSystem::create_dir(&dir);

        assert!(FileSystem::is_empty_async(&dir).await);

        let file = dir.join("not_empty.txt");
        FileSystem::write_async(&file, "data").await;
        assert!(!FileSystem::is_empty_async(&dir).await);
    }

    #[test]
    fn test_is_empty_sync() {
        let tmp = tempdir().unwrap();
        let dir = tmp.path().join("empty_dir");
        FileSystem::create_dir(&dir);

        assert!(FileSystem::is_empty(&dir));

        let file = dir.join("not_empty.txt");
        FileSystem::write(&file, "data");
        assert!(!FileSystem::is_empty(&dir));
    }

    #[tokio::test]
    async fn test_clear_dir_async() {
        let tmp = tempdir().unwrap();
        let dir = tmp.path();

        for i in 0..5 {
            FileSystem::write_async(dir.join(format!("file{}.txt", i)), "test").await;
        }

        FileSystem::clear_dir_async(dir).await.unwrap();
        assert!(FileSystem::is_empty_async(dir).await);
    }

    #[test]
    fn test_clear_dir_sync() {
        let tmp = tempdir().unwrap();
        let dir = tmp.path();

        for i in 0..5 {
            FileSystem::write(dir.join(format!("file{}.txt", i)), "test");
        }

        FileSystem::clear_dir(dir).unwrap();
        assert!(FileSystem::is_empty(dir));
    }

    #[test]
    fn test_clear_dir_par() {
        let tmp = tempdir().unwrap();
        let dir = tmp.path();

        for i in 0..5 {
            FileSystem::write(dir.join(format!("file{}.txt", i)), "test");
        }

        FileSystem::clear_dir_par(dir).unwrap();
        assert!(FileSystem::is_empty(dir));
    }

    #[tokio::test]
    async fn test_clear_dir_par_async() {
        let tmp = tempdir().unwrap();
        let dir = tmp.path();

        let nested = dir.join("nested");
        FileSystem::create_dir_recursive_async(&nested).await;

        for i in 0..3 {
            let file = nested.join(format!("file{}.txt", i));
            FileSystem::write_async(file, "nested").await;
        }

        FileSystem::clear_dir_par_async(dir).await.unwrap();
        assert!(FileSystem::is_empty_async(dir).await);
    }
}
