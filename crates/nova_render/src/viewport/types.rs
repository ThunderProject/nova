use bevy::prelude::*;

#[repr(u8)]
#[derive(Debug, Copy, Clone, Eq, PartialEq, Hash)]
pub enum ViewportId {
    Axial = 0,
    Sagittal = 1,
    Coronal = 2,
    Volume3D = 3,
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Hash)]
pub enum ViewportGridSlot {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
}

impl ViewportGridSlot {
    #[inline(always)]
    pub const fn as_uvec2(self) -> UVec2 {
        match self {
            Self::TopLeft => UVec2::new(0, 0),
            Self::TopRight => UVec2::new(1, 0),
            Self::BottomLeft => UVec2::new(0, 1),
            Self::BottomRight => UVec2::new(1, 1),
        }
    }
}

impl ViewportId {
    pub const COUNT: usize = 4;

    pub const ALL: [Self; Self::COUNT] = [
        Self::Axial,
        Self::Sagittal,
        Self::Coronal,
        Self::Volume3D,
    ];

    #[inline(always)]
    pub const fn index(self) -> usize {
        self as usize
    }

    #[inline(always)]
    pub const fn camera_order(self) -> isize {
        self as isize
    }

    #[inline(always)]
    pub const fn grid_slot(self) -> ViewportGridSlot {
        match self {
            Self::Axial => ViewportGridSlot::TopLeft,
            Self::Sagittal => ViewportGridSlot::TopRight,
            Self::Coronal => ViewportGridSlot::BottomLeft,
            Self::Volume3D => ViewportGridSlot::BottomRight,
        }
    }

    #[inline(always)]
    pub const fn grid_position(self) -> UVec2 {
        self.grid_slot().as_uvec2()
    }
}