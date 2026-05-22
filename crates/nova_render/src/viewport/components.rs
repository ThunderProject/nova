use bevy::prelude::*;

use crate::types::WorldPosition;
use super::types::ViewportId;

#[derive(Component, Debug, Copy, Clone)]
pub struct NovaViewport {
    pub id: ViewportId,
}

#[derive(Component, Debug, Copy, Clone)]
pub struct ViewportSlot {
    pub grid: UVec2,
}

#[derive(Component, Debug, Copy, Clone, Default)]
pub struct ViewportRect {
    pub position: UVec2,
    pub size: UVec2,
}

impl ViewportRect {
    #[inline]
    pub fn contains(self, point: UVec2) -> bool {
        if point.x < self.position.x || point.y < self.position.y {
            return false;
        }

        let local = point - self.position;
        local.x < self.size.x && local.y < self.size.y
    }

    #[inline]
    pub fn is_empty(self) -> bool {
        self.size.x == 0 || self.size.y == 0
    }
}

#[derive(Component, Debug, Copy, Clone)]
pub struct ViewportPlane {
    pub id: ViewportId,
}

#[derive(Component, Debug, Copy, Clone)]
pub struct ViewportFocusOutline;

#[derive(Component, Debug, Copy, Clone)]
pub struct SliceViewport2d {
    pub zoom: f32,
    pub pan: WorldPosition,
}

impl Default for SliceViewport2d {
    fn default() -> Self {
        Self {
            zoom: 1.0,
            pan: WorldPosition(Vec2::ZERO),
        }
    }
}