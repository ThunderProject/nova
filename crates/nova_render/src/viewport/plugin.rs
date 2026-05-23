use bevy::prelude::*;

use super::{
    input::{viewport_pan, viewport_reset, viewport_zoom},
    layout::{ActiveViewport, ViewportLayout},
    messages::ViewportCommand,
    systems::{
        apply_viewport_layout, focus_viewport_from_click, handle_viewport_commands,
        refresh_viewport_layout_on_resize, spawn_viewport_focus_outline, sync_focus_outline
    },
};

pub struct NovaViewportPlugin;

impl Plugin for NovaViewportPlugin {
    fn build(&self, app: &mut App) {
        app.init_resource::<ActiveViewport>()
            .init_resource::<ViewportLayout>()
            .add_message::<ViewportCommand>()
            .add_systems(Startup, spawn_viewport_focus_outline)
            .add_systems(PostStartup, apply_viewport_layout)
            .add_systems(
                Update,
                (
                    refresh_viewport_layout_on_resize,
                    focus_viewport_from_click,
                    viewport_pan,
                    viewport_zoom,
                    viewport_reset,
                    handle_viewport_commands,
                    sync_focus_outline,
                )
                    .chain(),
            );
    }
}
