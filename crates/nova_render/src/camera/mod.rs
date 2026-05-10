pub mod components;
pub mod plugin;
pub mod systems;

pub mod prelude {
    pub use super::components::SliceCamera;
    pub use super::plugin::NovaCameraPlugin;
}