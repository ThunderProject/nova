use std::path::Path;

use bytes::{Buf, Bytes};
use cxx::{UniquePtr, let_cxx_string};
use crate::{bridge::{binary_reader::{BinaryReadError, BinaryReader, Result}, dicom_bridge::{CxxByteBuffer, ffi}}, types::dicom_types::DicomError};

#[derive(Debug, Clone,Copy,PartialEq,Eq)]
#[repr(u8)]
pub enum Modality {
    Unknown = 0,
    UltraSoundBoneDensitometry,
    BiomagneticImaging,
    XrayBoneDensitometry,
    ComputedRadiography,
    ComputedTomography,
    Diaphanography,
    DigitalRadiography,
    IntraOralRadiography,
    Mammography,
    MagneticResonance,
    Plan,
    PositronEmissionTomography,
    RTImage,
    RTDose,
    RTStruct,
    RTPlan,
    RTRecord,
    Seg,
    Ultrasound,
}

impl TryFrom<u8> for Modality {
    type Error = BinaryReadError;

    fn try_from(value: u8) -> Result<Self> {
        Ok(match value {
            0 => Self::Unknown,
            1 => Self::UltraSoundBoneDensitometry,
            2 => Self::BiomagneticImaging,
            3 => Self::XrayBoneDensitometry,
            4 => Self::ComputedRadiography,
            5 => Self::ComputedTomography,
            6 => Self::Diaphanography,
            7 => Self::DigitalRadiography,
            8 => Self::IntraOralRadiography,
            9 => Self::Mammography,
            10 => Self::MagneticResonance,
            11 => Self::Plan,
            12 => Self::PositronEmissionTomography,
            13 => Self::RTImage,
            14 => Self::RTDose,
            15 => Self::RTStruct,
            16 => Self::RTPlan,
            17 => Self::RTRecord,
            18 => Self::Seg,
            19 => Self::Ultrasound,
            _ => return Err(BinaryReadError::InvalidModality(value)),
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum PhotometricInterpretation {
    Monochrome1 = 0,
    Monochrome2,
    PaletteColor,
    Rgb,
    Hsv,
    Argb,
    Cmyk,
    YbrFull,
    YbrFull422,
    YbrPartial422,
    YbrPartial420,
    YbrIct,
    YbrRct,
}

impl TryFrom<u8> for PhotometricInterpretation {
    type Error = BinaryReadError;

    fn try_from(value: u8) -> Result<Self> {
        Ok(match value {
            0 => Self::Monochrome1,
            1 => Self::Monochrome2,
            2 => Self::PaletteColor,
            3 => Self::Rgb,
            4 => Self::Hsv,
            5 => Self::Argb,
            6 => Self::Cmyk,
            7 => Self::YbrFull,
            8 => Self::YbrFull422,
            9 => Self::YbrPartial422,
            10 => Self::YbrPartial420,
            11 => Self::YbrIct,
            12 => Self::YbrRct,
            _ => return Err(BinaryReadError::InvalidPhotometricInterpretation(value)),
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum PixelSampleFormat {
    U8 = 0,
    U16,
    S16,
    U32,
    S32,
}

impl TryFrom<u8> for PixelSampleFormat {
    type Error = BinaryReadError;

    fn try_from(value: u8) -> Result<Self> {
        Ok(match value {
            0 => Self::U8,
            1 => Self::U16,
            2 => Self::S16,
            3 => Self::U32,
            4 => Self::S32,
            _ => return Err(BinaryReadError::InvalidPixelSampleFormat(value)),
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Patient {
    pub name: String,
    pub id: String,
    pub birth_date: String,
    pub birth_time: String,
    pub sex: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Study {
    pub instance_uid: String,
    pub id: String,
    pub date: String,
    pub time: String,
    pub accession_number: String,
    pub description: String,
    pub referring_physician_name: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
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
    pub modality: Modality,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Metadata {
    pub patient: Patient,
    pub study: Study,
    pub series: Series,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ImageDimensions {
    pub width: u32,
    pub height: u32,
    pub frames: u32,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PixelDataInfo {
    pub dims: ImageDimensions,
    pub samples_per_pixel: u16,
    pub bits_allocated: u16,
    pub bits_stored: u16,
    pub high_bit: u16,
    pub planar_configuration: u16,
    pub photometric: PhotometricInterpretation,
    pub format: PixelSampleFormat,
}

impl PixelDataInfo {
    #[inline]
    pub fn pixel_count(&self) -> usize {
        self.dims.width as usize * self.dims.height as usize * self.dims.frames as usize
    }

    #[inline]
    pub fn bytes_per_sample(&self) -> usize {
        (self.bits_allocated as usize).div_ceil(8)
    }

    #[inline]
    pub fn expected_pixel_buffer_len(&self) -> usize {
        self.pixel_count() * self.samples_per_pixel as usize * self.bytes_per_sample()
    }
}

pub struct Dicom {
    pub metadata: Metadata,
    pub pixel_data_info: PixelDataInfo,
    pub pixel_data: Bytes,
}

impl Dicom {
    pub fn load(path: impl AsRef<Path>) -> std::result::Result<Self, DicomError> {
        let mut ffi_bridge = ffi::dicom_api_create();

        if ffi_bridge.is_null() {
            return Err(DicomError::NullReader);
        }

        Self::load_with_bridge(&mut ffi_bridge, path)
    }

    pub fn load_with_bridge(
        ffi_bridge: &mut UniquePtr<ffi::dicom_api>,
        path: impl AsRef<Path>,
    ) -> std::result::Result<Self, DicomError> {
        if ffi_bridge.is_null() {
            return Err(DicomError::NullReader);
        }

        let path = path.as_ref().as_os_str().to_string_lossy();
        let_cxx_string!(cxx_path = path.as_ref());

        ffi_bridge.pin_mut().load(&cxx_path)?;

        let reader = ffi_bridge.as_ref().ok_or(DicomError::NullReader)?;

        let buffer = reader.read_file()?;
        let bytes = Self::to_bytes(buffer)?;

        let mut br = BinaryReader::new(bytes);

        let metadata = Self::read_metadata(&mut br)?;
        let pixel_data_info = Self::read_pixel_data_info(&mut br)?;
        let pixel_data = Self::read_pixel_data(&mut br)?;

        br.finish()?;

        debug_assert_eq!(
            pixel_data.len(),
            pixel_data_info.expected_pixel_buffer_len(),
            "pixel byte length does not match pixel_data_info"
        );

        Ok(Self {
            metadata,
            pixel_data_info,
            pixel_data,
        })
    }

    #[inline]
    fn to_bytes(buffer: UniquePtr<cxx::CxxVector<u8>>) -> std::result::Result<Bytes, DicomError> {
        if buffer.is_null() {
            return Err(DicomError::NullFileBuffer);
        }

        Ok(Bytes::from_owner(CxxByteBuffer { buffer }))
    }

    fn read_metadata<B>(reader: &mut BinaryReader<B>) -> Result<Metadata> where B: Buf, {
        let patient = Patient {
            name: reader.read_string()?,
            id: reader.read_string()?,
            birth_date: reader.read_string()?,
            birth_time: reader.read_string()?,
            sex: reader.read_string()?,
        };

        let study = Study {
            instance_uid: reader.read_string()?,
            id: reader.read_string()?,
            date: reader.read_string()?,
            time: reader.read_string()?,
            accession_number: reader.read_string()?,
            description: reader.read_string()?,
            referring_physician_name: reader.read_string()?,
        };

        let series = Series {
            instance_uid: reader.read_string()?,
            date: reader.read_string()?,
            time: reader.read_string()?,
            description: reader.read_string()?,
            number: reader.read_string()?,
            body_part_examined: reader.read_string()?,
            performing_physician_name: reader.read_string()?,
            smallest_pixel_value: reader.read_string()?,
            largest_pixel_value: reader.read_string()?,
            modality: Modality::try_from(reader.read_u8()?)?,
        };

        Ok(Metadata {
            patient,
            study,
            series,
        })
    }

    fn read_pixel_data_info<B>(reader: &mut BinaryReader<B>) -> Result<PixelDataInfo> where B: Buf, {
        Ok(PixelDataInfo {
            dims: ImageDimensions {
                width: reader.read_u32()?,
                height: reader.read_u32()?,
                frames: reader.read_u32()?,
            },

            samples_per_pixel: reader.read_u16()?,
            bits_allocated: reader.read_u16()?,
            bits_stored: reader.read_u16()?,
            high_bit: reader.read_u16()?,
            planar_configuration: reader.read_u16()?,

            photometric: PhotometricInterpretation::try_from(reader.read_u8()?)?,
            format: PixelSampleFormat::try_from(reader.read_u8()?)?,
        })
    }

    fn read_pixel_data<B>(reader: &mut BinaryReader<B>) -> Result<Bytes> where B: Buf, {
        let pixel_len = reader.read_u64()? as usize;
        let pixel_data = reader.read_bytes(pixel_len)?;

        Ok(pixel_data)
    }
}