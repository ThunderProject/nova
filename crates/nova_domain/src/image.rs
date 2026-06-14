use bytes::Bytes;
use crate::ids::ImageVolumeId;


#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PixelFormat {
    U8,
    U16,
    I16,
    U32,
    I32,
}

impl PixelFormat {
    pub const fn bytes_per_sample(self) -> usize {
        match self {
            Self::U8 => 1,
            Self::U16 | Self::I16 => 2,
            Self::U32 | Self::I32 => 4,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ImageDimensions3D {
    pub width: u32,
    pub height: u32,
    pub depth: u32,
}

impl ImageDimensions3D {
    pub fn voxel_count(self) -> Option<usize> {
        let width = self.width as usize;
        let height = self.height as usize;
        let depth = self.depth as usize;

        width.checked_mul(height)?.checked_mul(depth)
    }
}

#[derive(Debug, Default)]
pub struct ImageVolumes {
    dimensions: Vec<ImageDimensions3D>,
    spacing: Vec<[f32; 3]>,
    origin: Vec<[f32; 3]>,
    direction: Vec<[[f32; 3]; 3]>,

    pixel_format: Vec<PixelFormat>,
    samples_per_voxel: Vec<u16>,
    pixel_bytes: Vec<Bytes>,
}

#[derive(Debug, Clone)]
pub struct ImageVolumeData {
    pub dimensions: ImageDimensions3D,
    pub spacing: [f32; 3],
    pub origin: [f32; 3],
    pub direction: [[f32; 3]; 3],

    pub pixel_format: PixelFormat,
    pub samples_per_voxel: u16,
    pub pixel_bytes: Bytes,
}

#[derive(Debug, Clone, Copy)]
pub struct ImageVolume<'a> {
    pub id: ImageVolumeId,

    pub dimensions: ImageDimensions3D,
    pub spacing: [f32; 3],
    pub origin: [f32; 3],
    pub direction: [[f32; 3]; 3],

    pub pixel_format: PixelFormat,
    pub samples_per_voxel: u16,
    pub pixel_bytes: &'a Bytes,
}

impl ImageVolumes {
    pub fn push(&mut self, volume: ImageVolumeData) -> ImageVolumeId {
        debug_assert!(volume.dimensions.width > 0);
        debug_assert!(volume.dimensions.height > 0);
        debug_assert!(volume.dimensions.depth > 0);
        debug_assert!(volume.samples_per_voxel > 0);

        let index = self.dimensions.len();
        let id = ImageVolumeId(index.try_into().expect("too many image volumes"));

        self.dimensions.push(volume.dimensions);
        self.spacing.push(volume.spacing);
        self.origin.push(volume.origin);
        self.direction.push(volume.direction);
        self.pixel_format.push(volume.pixel_format);
        self.samples_per_voxel.push(volume.samples_per_voxel);
        self.pixel_bytes.push(volume.pixel_bytes);

        id
    }

    pub fn get(&self, id: ImageVolumeId) -> Option<ImageVolume<'_>> {
        let index = id.0 as usize;

        Some(ImageVolume {
            id,
            dimensions: *self.dimensions.get(index)?,
            spacing: *self.spacing.get(index)?,
            origin: *self.origin.get(index)?,
            direction: *self.direction.get(index)?,
            pixel_format: *self.pixel_format.get(index)?,
            samples_per_voxel: *self.samples_per_voxel.get(index)?,
            pixel_bytes: self.pixel_bytes.get(index)?,
        })
    }

    pub fn middle(&self) -> Option<ImageVolume<'_>> {
        if self.is_empty() {
            return None;
        }

        self.get(ImageVolumeId((self.len() / 2) as u32))
    }

    pub fn len(&self) -> usize {
        self.dimensions.len()
    }

    pub fn is_empty(&self) -> bool {
        self.dimensions.is_empty()
    }
}

impl ImageVolume<'_> {
    pub fn bytes_per_voxel(&self) -> usize {
        self.pixel_format.bytes_per_sample() * self.samples_per_voxel as usize
    }

    pub fn slice_byte_len(&self) -> Option<usize> {
        let width = self.dimensions.width as usize;
        let height = self.dimensions.height as usize;

        width.checked_mul(height)?.checked_mul(self.bytes_per_voxel())
    }

    pub fn middle_slice_bytes(&self) -> Option<&[u8]> {
        let z = self.dimensions.depth / 2;
        self.slice_bytes(z)
    }

    pub fn slice_bytes(&self, z: u32) -> Option<&[u8]> {
        if z >= self.dimensions.depth {
            return None;
        }

        let slice_len = self.slice_byte_len()?;
        let start = (z as usize).checked_mul(slice_len)?;
        let end = start.checked_add(slice_len)?;

        self.pixel_bytes.get(start..end)
    }
}