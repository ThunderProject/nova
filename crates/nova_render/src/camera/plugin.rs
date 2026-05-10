use bevy::prelude::*;

use super::systems::update_slice_camera;

pub struct NovaCameraPlugin;

impl Plugin for NovaCameraPlugin {
    fn build(&self, app: &mut App) {
        app.add_systems(Update, update_slice_camera);
    }
}