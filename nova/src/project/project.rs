use std::fs::File;
use std::io;
use std::path::{Path, PathBuf};
use arc_swap::ArcSwap;
use ripunzip::{NullProgressReporter, UnzipEngine, UnzipOptions};
use serde::Deserialize;
use thiserror::Error;
use tracing::{error, info};
use crate::compression::zip::Zip;
use crate::fs::file_system::file_system::FileSystem;

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
}

impl Project {

    pub fn new_project(project_params: ProjectParams) -> Result<Self, ProjectError> {
        // The UI has already shown a big yellow warning that the contents of the
        // selected folder will be overwritten or deleted, and the user explicitly
        // confirmed (otherwise we wouldn’t be here).
        // At this point, data loss is the user’s decision, not a bug.
        // It’s called informed consent.
        FileSystem::clear_dir_par(&project_params.working_directory)?;
        
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

                match ext {
                    "zip" => {
                        let result = Zip::unzip(file_name, "");
                        match result {
                            Ok(zipfile) => {
                                info!("successfully unzipped file!");
                            }
                            Err(e) => {
                                error!("failed to unzipped file: {}", e);
                            }
                        }
                        return;
                    }
                    _ => return,
                }
            }
        }
    }
}