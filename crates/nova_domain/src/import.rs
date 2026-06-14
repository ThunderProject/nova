use std::path::Path;
use anyhow::{Context, Result};
use crate::{examination::Examination, ids::{ExaminationId, PatientId, StudyId}, image::{ImageDimensions3D, ImageVolumeData, PixelFormat}, model::NovaModel, patient::Patient, study::Study};

pub fn load_model(path: impl AsRef<Path>) -> Result<NovaModel> {
    let path = path.as_ref();

    match path.is_dir() {
        true => load_dir(path),
        _ => load_file(path)
    }
}


fn load_dir(path: &Path) -> Result<NovaModel> {
    let dicoms = nova_dicom::file_loader::dicom_file_loader::process_directory(path)
        .with_context(|| format!("failed to load input directory: {}", path.display()))?;
    dicoms_to_model(dicoms)
}

fn load_file(path: &Path) -> Result<NovaModel> {
    let dicom = nova_dicom::file_loader::dicom_file_loader::process_file(path)
        .with_context(|| format!("failed to load input file: {}", path.display()))?;

    dicoms_to_model(vec![dicom])
}

fn dicoms_to_model(dicoms: Vec<nova_dicom::dicom::Dicom>) -> Result<NovaModel> {
    let mut model = NovaModel::default();
    let mut examinations = Vec::with_capacity(dicoms.len());

    let mut patient_name = String::from("Unknown patient");
    let mut study_name = String::from("Unnamed study");

    for (index, dicom) in dicoms.into_iter().enumerate() {
        if index == 0 {
            patient_name = non_empty_or(dicom.metadata.patient.name.clone(), "Unknown patient");
            study_name = non_empty_or(dicom.metadata.study.description.clone(), "Unnamed study");
        }

        let image_volume = model.image_volumes.push(ImageVolumeData {
            dimensions: ImageDimensions3D {
                width: dicom.pixel_data_info.dims.width,
                height: dicom.pixel_data_info.dims.height,
                depth: dicom.pixel_data_info.dims.frames.max(1),
            },
            spacing: [1.0, 1.0, 1.0],
            origin: [0.0, 0.0, 0.0],
            direction: [
                [1.0, 0.0, 0.0],
                [0.0, 1.0, 0.0],
                [0.0, 0.0, 1.0],
            ],
            pixel_format: map_pixel_format(dicom.pixel_data_info.format),
            samples_per_voxel: dicom.pixel_data_info.samples_per_pixel,
            pixel_bytes: dicom.pixel_data,
        });

        examinations.push(Examination {
            id: ExaminationId(index as u32),
            name: non_empty_or(dicom.metadata.series.description, "Unnamed examination"),
            image_volume,
        });
    }

    model.patients.push(Patient {
        id: PatientId(0),
        display_name: patient_name,
        studies: vec![Study {
            id: StudyId(0),
            name: study_name,
            examinations,
        }],
    });

    Ok(model)
}

fn map_pixel_format(format: nova_dicom::dicom::PixelSampleFormat) -> PixelFormat {
    match format {
        nova_dicom::dicom::PixelSampleFormat::U8 => PixelFormat::U8,
        nova_dicom::dicom::PixelSampleFormat::U16 => PixelFormat::U16,
        nova_dicom::dicom::PixelSampleFormat::S16 => PixelFormat::I16,
        nova_dicom::dicom::PixelSampleFormat::U32 => PixelFormat::U32,
        nova_dicom::dicom::PixelSampleFormat::S32 => PixelFormat::I32,
    }
}

fn non_empty_or(value: String, fallback: &str) -> String {
    let value = value.trim();

    if value.is_empty() {
        fallback.to_owned()
    } else {
        value.to_owned()
    }
}