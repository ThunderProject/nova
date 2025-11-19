use std::sync::Arc;
use authenticated_command::authenticated_command;
use nova::ioc;
use nova::project::project::{Project, ProjectParams};
use tracing::{debug, info};

#[authenticated_command]
pub async fn create_new_project(params: ProjectParams) -> Result<(), String> {
    info!("Creating new project: {}", params.project_name);
    debug!("Working directory: {}", params.working_directory);
    debug!("Imported files: {:?}", params.imported_files);

    let result = Project::new_project(params).await;
    if let Ok(project) = result {
        let arc = Arc::new(project);
        ioc::singleton::ioc().register(move || Arc::clone(&arc));
        info!("Project successfully created");
        Ok(())
    }
    else {
        Err(format!("Project creation failed: {:?}", result.err()).into())
    }
}

#[authenticated_command]
pub async fn open_project(file: String) -> Result<(), String> {
    info!("Opening project from file: {}", file);
    Ok(())
}