use bevy::prelude::*;
use bevy::window::{PresentMode, WindowPlugin};
use bevy_framepace::{FramepacePlugin, FramepaceSettings, Limiter};

use crate::camera::plugin::NovaCameraPlugin;
use crate::scene::DummyScenePlugin;
use crate::viewport::prelude::NovaViewportPlugin;

pub fn run() {
    App::new()
        .insert_resource(FramepaceSettings {
            limiter: Limiter::from_framerate(141.0),
        })
        .add_plugins((
            DefaultPlugins.set(WindowPlugin {
                primary_window: Some(Window {
                    title: "Nova Render".into(),
                    resolution: (1280, 900).into(),
                    present_mode: PresentMode::AutoVsync,
                    ..default()
                }),
                ..default()
            }),
            FramepacePlugin,
        ))
        .add_plugins((
            NovaViewportPlugin,
            NovaCameraPlugin,
            DummyScenePlugin,
        ))
        .run();
}