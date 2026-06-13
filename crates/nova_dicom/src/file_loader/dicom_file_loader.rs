use std::path::{Path, PathBuf};

use rayon::iter::{IntoParallelRefIterator, ParallelIterator};
use tracing::debug;
use walkdir::WalkDir;

use crate::{
    bridge::dicom_bridge::ffi,
    dicom::Dicom,
    file_loader::error::DicomFileError,
};

const FILE_SUFFIXES_TO_SKIP: &[&str] = &["~", ".tmp", ".bak", ".json", ".txt", ".zip"];

pub fn process_file(path: impl AsRef<Path>) -> Result<Dicom, DicomFileError> {
    let path = path.as_ref();

    let filename = path
        .file_name()
        .and_then(|name| name.to_str())
        .unwrap_or("<unknown>");

    debug!(file = filename, "processing DICOM file");

    Dicom::load(path).map_err(|err| DicomFileError::ReadFailed {
        path: path.to_owned(),
        message: err.to_string(),
    })
}

pub fn process_directory(root: impl AsRef<Path>) -> Result<Vec<Dicom>, DicomFileError> {
    let files = collect_files(root)?;

    files
        .par_iter()
        .map_init(
            ffi::dicom_api_create,
            |ffi_bridge, path| {
                process_file_with_bridge(ffi_bridge, path)
            },
        )
        .collect()
}

fn process_file_with_bridge(
    ffi_bridge: &mut cxx::UniquePtr<ffi::dicom_api>,
    path: &Path,
) -> Result<Dicom, DicomFileError> {
    let filename = path
        .file_name()
        .and_then(|name| name.to_str())
        .unwrap_or("<unknown>");

    debug!(file = filename, "processing DICOM file");

    Dicom::load_with_bridge(ffi_bridge, path).map_err(|err| DicomFileError::ReadFailed {
        path: path.to_owned(),
        message: err.to_string(),
    })
}

pub fn collect_files(base_path: impl AsRef<Path>) -> Result<Vec<PathBuf>, DicomFileError> {
    let base_path = base_path.as_ref();

    if !base_path.is_dir() {
        return Err(DicomFileError::NotADirectory {
            path: base_path.to_owned(),
        });
    }

    let mut files = Vec::new();

    for entry in WalkDir::new(base_path)
        .follow_links(false)
        .into_iter()
        .filter_entry(|entry| {
            let name = entry.file_name().to_string_lossy();
            !name.starts_with('.')
        })
    {
        let entry = entry.map_err(|err| DicomFileError::DirectoryTraversal {
            message: err.to_string(),
            path: base_path.to_owned(),
        })?;

        if !entry.file_type().is_file() {
            continue;
        }

        let path = entry.into_path();

        if skip_file(&path) {
            continue;
        }

        files.push(path);
    }

    files.sort_unstable();
    Ok(files)
}

fn skip_file(path: &Path) -> bool {
    let Some(filename) = path.file_name().and_then(|name| name.to_str()) else {
        return true;
    };

    if filename.starts_with('.') {
        return true;
    }

    FILE_SUFFIXES_TO_SKIP
        .iter()
        .any(|suffix| filename.ends_with(suffix))
}