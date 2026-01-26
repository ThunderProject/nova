use std::sync::Arc;
use authenticated_command::authenticated_command;
use tracing::{debug, info};
use nova_project::project::*;
use nova_di::ioc;


#[authenticated_command]
pub async fn create_new_project(params: ProjectParams) -> Result<(), String> {
    info!("Creating new project: {}", params.project_name);
    debug!("Working directory: {}", params.working_directory);
    debug!("Imported files: {:?}", params.imported_files);

    // The UI has already shown a big yellow warning that the contents of the
    // selected folder will be overwritten or deleted, and the user explicitly
    // confirmed (otherwise we wouldn’t be here).
    // At this point, data loss is the user’s decision, not a bug.
    // It’s called informed consent.
    let result = Project::new_project(params, true).await;
    if let Ok(project) = result {
        let arc = Arc::new(project);
        ioc::singleton::ioc().register(move || Arc::clone(&arc));
        info!("Project successfully created");
        Ok(())
    }
    else {
        Err(format!("Project creation failed: {:?}", result.err()))
    }
}

#[authenticated_command]
pub async fn open_project(file: String) -> Result<(), String> {
    info!("Opening project from file: {}", file);
    Ok(())
}
