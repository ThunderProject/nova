use bevy::prelude::*;

use super::types::ViewportId;

#[derive(Message, Debug, Copy, Clone)]
pub enum ViewportCommand {
    Focus {
        viewport: ViewportId,
    },
    ResetView {
        viewport: ViewportId,
    },
    Pan {
        viewport: ViewportId,
        delta_physical_px: Vec2,
    },
    Zoom {
        viewport: ViewportId,
        factor: f32,
        anchor_physical_px: Vec2,
    }
}