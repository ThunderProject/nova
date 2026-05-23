use std::path::{Path, PathBuf};

use rayon::iter::{IntoParallelRefIterator, ParallelIterator};
use tracing::debug;
use walkdir::WalkDir;

use crate::{DicomReader, file_loader::error::DicomFileError};

const FILE_SUFFIXES_TO_SKIP: &[&str] = &[
    "~",
    ".tmp",
    ".bak",
    ".json",
    ".txt",
    ".zip",
];

#[derive(Debug)]
pub struct DicomFile {
    pub path: PathBuf,

    pub sop_instance_uid: Option<String>,
    pub study_instance_uid: Option<String>,
    pub series_instance_uid: Option<String>,
    pub patient_id: Option<String>,
    pub patient_name: Option<String>,
    pub modality: Option<String>,
}

pub fn process_file(path: impl AsRef<Path>) -> Result<DicomFile, DicomFileError> {
    let path = path.as_ref();
    let filename = path
        .file_name()
        .and_then(|name| name.to_str())
        .unwrap_or("<unknown>");

    debug!(file = filename, "processing DICOM file");

    let mut reader = DicomReader::new();

    reader
        .load(path)
        .map_err(|err| DicomFileError::ReadFailed {
            path: path.to_owned(),
            message: err.to_string(),
        })?;

    let metadata = reader
        .metadata()
        .map_err(|err| DicomFileError::ReadFailed {
            path: path.to_owned(),
            message: err.to_string(),
        })?;

    let dicom_file = DicomFile {
        path: path.to_owned(),

        // TODO: add SOPInstanceUID to Metadata soon.
        sop_instance_uid: None,

        study_instance_uid: non_empty(metadata.study.instance_uid),
        series_instance_uid: non_empty(metadata.series.instance_uid),
        patient_id: non_empty(metadata.patient.id),
        patient_name: non_empty(metadata.patient.name),
        modality: non_empty(metadata.series.modality),
    };

    Ok(dicom_file)
}


pub fn process_directory(root: impl AsRef<Path>,) -> Result<Vec<DicomFile>, DicomFileError> {
    let files = collect_files(root)?;

    files
        .par_iter()
        .map(process_file)
        .collect()
}

pub fn collect_files(base_path: impl AsRef<Path>) -> Result<Vec<PathBuf>, DicomFileError> {
    let base_path = base_path.as_ref();

    if !base_path.is_dir() {
        return Err(DicomFileError::NotADirectory {
            path: base_path.to_owned(),
        });
    }

    let mut files = Vec::new();
    for entry in WalkDir::new(base_path).follow_links(false) {
        let entry = entry.map_err(|err| DicomFileError::DirectoryTraversal {
            message: err.to_string(),
            path: base_path.to_owned()
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

    filename.starts_with('.') || FILE_SUFFIXES_TO_SKIP
        .iter()
        .any(|suffix| filename.ends_with(suffix))
}

fn non_empty(value: String) -> Option<String> {
    let value = value.trim().to_owned();

    match value.is_empty() {
        true => None,
        _ => Some(value)
    }
}