use serde::Deserialize;
use thiserror;

#[derive(Debug, Clone, Deserialize)]
pub struct Metadata {
    pub patient: Patient,
    pub study: Study,
    pub series: Series,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Patient {
    pub name: String,
    pub id: String,
    pub birth_date: String,
    pub birth_time: String,
    pub sex: String,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Study {
    pub instance_uid: String,
    pub id: String,
    pub date: String,
    pub time: String,
    pub accession_number: String,
    pub description: String,
    pub referring_physician_name: String,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Series {
    pub instance_uid: String,
    pub date: String,
    pub time: String,
    pub description: String,
    pub number: String,
    pub body_part_examined: String,
    pub performing_physician_name: String,
    pub smallest_pixel_value: String,
    pub largest_pixel_value: String,
    pub modality: String,
}

#[derive(Debug, Clone, Deserialize)]
pub struct PixelDataInfo {
    pub width: u32,
    pub height: u32,
    pub frames: u32,
    pub samples_per_pixel: u16,
    pub bits_allocated: u16,
    pub bits_stored: u16,
    pub high_bit: u16,
    pub planar_configuration: u16,
    pub photometric_interpretation: String,
    pub sample_format: u8,
}

#[derive(Debug, Clone)]
pub struct DicomFile {
    pub metadata: Metadata,
    pub pixel_data_info: PixelDataInfo,
    pub pixel_data: bytes::Bytes,
}

use crate::bridge::binary_reader::BinaryReadError;

#[derive(Debug, thiserror::Error)]
pub enum DicomError {
    #[error("C++ DICOM error: {0}")]
    Cxx(#[from] cxx::Exception),

    #[error("DICOM wire parse error: {0}")]
    Wire(#[from] BinaryReadError),

    #[error("DICOM reader was null")]
    NullReader,

    #[error("DICOM metadata buffer was null")]
    NullMetadataBuffer,

    #[error("DICOM pixel data info buffer was null")]
    NullPixelDataInfoBuffer,

    #[error("DICOM pixel buffer was null")]
    NullPixelBuffer,

    #[error("DICOM file buffer was null")]
    NullFileBuffer,
}


