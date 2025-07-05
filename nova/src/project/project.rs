use std::io;
use std::path::{Path};
use anyhow::anyhow;
use arc_swap::ArcSwap;
use serde::Deserialize;
use tempfile::tempdir;
use thiserror::Error;
use tracing::{debug, error, info};
use crate::compression::zip::{UnzipAppError, Zip};
use crate::fs::file_system::FileSystem;

const ALLOWED_FILE_EXTENSIONS: [&str; 3] = ["zip", "dcm", "dicom"];

#[derive(Deserialize)]
pub struct ProjectParams {
    pub project_name: String,
    pub working_directory: String,
    pub imported_files: Vec<String>,
}

pub struct Project {
    pub project_name: ArcSwap<String>,
    pub working_directory: ArcSwap<String>,
    pub imported_files: ArcSwap<Vec<String>>,
}

#[derive(Error, Debug)]
pub enum ProjectError {
    #[error("I/O error: {0}")]
    Io(#[from] io::Error),
    
    #[error("Unzip failed: {0}")]
    Anyhow(#[from] anyhow::Error),

    #[error("Unzip error: {0}")]
    UnzipAppError(#[from] UnzipAppError),
}

impl Project {

    pub async fn new_project(project_params: ProjectParams) -> Result<Self, ProjectError> {
        // The UI has already shown a big yellow warning that the contents of the
        // selected folder will be overwritten or deleted, and the user explicitly
        // confirmed (otherwise we wouldn’t be here).
        // At this point, data loss is the user’s decision, not a bug.
        // It’s called informed consent.
        FileSystem::clear_dir_par(&project_params.working_directory)?;

        if project_params.imported_files.is_empty() {
            return Ok(
                Self {
                    project_name: ArcSwap::from_pointee(project_params.project_name),
                    working_directory: ArcSwap::from_pointee(project_params.working_directory),
                    imported_files: ArcSwap::from_pointee(project_params.imported_files)
                }
            );
        }

        let project_files_dir = format!("{}\\{}", project_params.working_directory, "projectFiles");
        if !FileSystem::create_dir_recursive_async(&project_files_dir).await {
            return Err(ProjectError::Anyhow(anyhow!("")))
        }

        Self::load_imported_files(&project_params.imported_files, &project_files_dir).await?;

        Ok(
            Self {
                project_name: ArcSwap::from_pointee(project_params.project_name),
                working_directory: ArcSwap::from_pointee(project_params.working_directory),
                imported_files: ArcSwap::from_pointee(project_params.imported_files)
            }
        )
    }
    
    pub fn open(file_name: &str) {
        let path = Path::new(file_name);

        if let Some(extension) = path.extension() {
            if let Some(ext) = extension.to_str() {
                if !ALLOWED_FILE_EXTENSIONS.contains(&ext) {
                    return;
                }

                if ext == "zip" {
                    let result = Zip::unzip(file_name, "");
                    match result {
                        Ok(_zipfile) => {
                            info!("successfully unzipped file!");
                        }
                        Err(e) => {
                            error!("failed to unzipped file: {}", e);
                        }
                    };
                }
            }
        }
    }

    async fn load_imported_files(files: &[String], project_files_dir: &str) -> anyhow::Result<()> {
        let project_files_dir = Path::new(project_files_dir);

        for file in files {
            let path = Path::new(file);

            if ! Self::check_file_extension(path, "zip") {
                debug!("File is not a zip file. continue: {:?}", path);
                continue;
            }

            info!("unzipping...");

            let file_name = path.file_name().ok_or_else(|| anyhow::anyhow!("Missing file name in path: {:?}", file))?;

            let dst_file_path = project_files_dir.join(file_name);

            debug!("Copying {:?} to {:?}", file, dst_file_path);

            let sized_copied = tokio::fs::copy(file, &dst_file_path).await?;

            debug!("Successfully copied {} bytes to {:?}", sized_copied, dst_file_path);

            let temp_dir = tempdir()?;

            if let Some(temp_path) = temp_dir.path().to_str() {
                match Zip::unzip(file, temp_path) {
                    Ok(()) => debug!("successfully unzipped file to {:?}", temp_path),
                    _ => { error!("Failed to unzip file to {:?}", temp_path) }
                }
                // load dicom file(s)
            }
            else {
                error!("Temp dir path {:?} is not valid UTF-8", temp_dir.path());
            }
        }

        Ok(())
    }

    fn check_file_extension(file_name: &Path, ext: &str) -> bool {
        match file_name.extension() {
            Some(extension) => extension == ext,
            None => false,
        }
    }
}