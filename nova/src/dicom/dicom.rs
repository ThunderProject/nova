use dicom_object::open_file;
use serde::Serialize;

#[derive(Serialize)]
struct Dicom {
    pub rows: u16,
    pub columns: u16,
    pub bits_allocated: u16,
    pub intercept: f64,
    pub slope: f64,
    pub spacing: [f32; 2],
    pub slice_thickness: Option<f32>,
    pub position: [f32; 3],
    pub pixel_data: Vec<u8>,
}

impl Dicom {
    pub fn open<P: AsRef<std::path::Path>>(path: P) -> Result<Self, anyhow::Error> {
        let dicom_object = open_file(path)?;

        let rows = dicom_object.element_by_name("Rows")?.to_int::<u16>()?;
        let columns = dicom_object.element_by_name("Columns")?.to_int::<u16>()?;
        let bits_allocated = dicom_object.element_by_name("BitsAllocated")?.to_int::<u16>()?;

        let intercept = dicom_object.element_by_name("RescaleIntercept").ok()
            .and_then(|e| e.to_float64().ok()).unwrap_or(0.0);
        let slope = dicom_object.element_by_name("RescaleSlope").ok()
            .and_then(|e| e.to_float64().ok()).unwrap_or(1.0);

        let spacing: Vec<f32> = dicom_object.element_by_name("PixelSpacing")?.to_multi_float32()?;

        let slice_thickness = dicom_object.element_by_name("SliceThickness").ok()
            .and_then(|e| e.to_float32().ok());

        let position: Vec<f32> = dicom_object.element_by_name("ImagePositionPatient")?.to_multi_float32()?;
        
        let pixel_data = dicom_object.element_by_name("PixelData")?.to_bytes()?;

        Ok(Self {
            rows,
            columns,
            bits_allocated,
            intercept,
            slope,
            spacing: [spacing[0], spacing[1]],
            slice_thickness,
            position: [position[0], position[1], position[2]],
            pixel_data: Vec::from(pixel_data),
        })
    }
}