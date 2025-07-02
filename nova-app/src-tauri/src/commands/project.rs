pub mod project {
    use std::sync::Arc;
    use nova::ioc;
    use nova::project::project::{Project, ProjectParams};
    use tracing::{debug, info};

    #[tauri::command]
    pub async fn create_new_project(params: ProjectParams) -> Result<(), String> {
        info!("Creating new project: {}", params.project_name);
        debug!("Working directory: {}", params.working_directory);
        debug!("Imported files: {:?}", params.imported_files);

        if let Ok(project) = Project::new_project(params) {
            let arc = Arc::new(project);
            ioc::singleton::ioc().register(move || Arc::clone(&arc));
            info!("Project successfully created");
            Ok(())
        }
        else { 
            Err("Project creation failed".into())
        }
    }

    #[tauri::command]
    pub async fn open_project(file: String) -> Result<(), String> {
        info!("Opening project from file: {}", file);
        Ok(())
    }
}