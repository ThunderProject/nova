use bevy::{
    input::mouse::MouseMotion,
    prelude::*,
    window::PrimaryWindow,
};

use super::{
    layout::ActiveViewport,
    messages::ViewportCommand,
};

const PAN_SENSITIVITY: f32 = 1.0;
const DRAG_ZOOM_SENSITIVITY: f32 = 0.01;

pub fn viewport_pan(
    buttons: Res<ButtonInput<MouseButton>>,
    keyboard: Res<ButtonInput<KeyCode>>,
    active: Res<ActiveViewport>,
    mut mouse_motion: MessageReader<MouseMotion>,
    mut writer: MessageWriter<ViewportCommand>,
) {
    let middle_pan = buttons.pressed(MouseButton::Middle);

    let shift_left_pan = buttons.pressed(MouseButton::Left) &&
                        (keyboard.pressed(KeyCode::ShiftLeft) ||
                        keyboard.pressed(KeyCode::ShiftRight));

    if !middle_pan && !shift_left_pan {
        mouse_motion.clear();
        return;
    }

    let delta = mouse_motion
        .read()
        .fold(Vec2::ZERO, |acc, event| acc + event.delta);

    if delta == Vec2::ZERO {
        return;
    }

    writer.write(ViewportCommand::Pan {
        viewport: active.0,
        delta_physical_px: delta * PAN_SENSITIVITY,
    });
}

pub fn viewport_zoom(
    buttons: Res<ButtonInput<MouseButton>>,
    active: Res<ActiveViewport>,
    window: Single<&Window, With<PrimaryWindow>>,
    mut mouse_motion: MessageReader<MouseMotion>,
    mut writer: MessageWriter<ViewportCommand>,
) {
    if !buttons.pressed(MouseButton::Right) {
        mouse_motion.clear();
        return;
    }

    let delta = mouse_motion
        .read()
        .fold(Vec2::ZERO, |acc, event| acc + event.delta);

    if delta == Vec2::ZERO {
        return;
    }

    let Some(cursor_logical) = window.cursor_position() else {
        return;
    };

    let cursor_physical = cursor_logical * window.scale_factor() as f32;
    let factor = (-delta.y * DRAG_ZOOM_SENSITIVITY).exp();

    writer.write(ViewportCommand::Zoom {
        viewport: active.0,
        factor,
        anchor_physical_px: cursor_physical,
    });
}

pub fn viewport_reset(
    keyboard: Res<ButtonInput<KeyCode>>,
    active: Res<ActiveViewport>,
    mut writer: MessageWriter<ViewportCommand>,
) {
    if !keyboard.just_pressed(KeyCode::KeyR) {
        return;
    }

    writer.write(ViewportCommand::ResetView {
        viewport: active.0,
    });
}