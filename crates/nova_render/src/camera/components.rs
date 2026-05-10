use bevy::prelude::*;
use crate::types::WorldPosition;

#[derive(Component, Debug, Copy, Clone)]
pub struct SliceCamera {
    pub target: Vec3,
    pub distance: f32,
    pub pan: WorldPosition,
    pub zoom: f32,
}

impl SliceCamera {
    pub fn new(target: Vec3, distance: f32) -> Self {
        Self {
            target: target,
            distance: distance,
            pan: WorldPosition::default(),
            zoom: 1.0,
        }
    }

    pub fn reset(&mut self) {
        self.pan = WorldPosition::default();
        self.zoom = 1.0;
    }
}