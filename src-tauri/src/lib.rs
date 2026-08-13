use serde::Serialize;
use std::{fs, path::PathBuf};
use tauri::Manager;

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
struct MissionFile {
    path: String,
    name: String,
    bytes: Vec<u8>,
}

fn mission_dialog() -> rfd::FileDialog {
    rfd::FileDialog::new().add_filter("Mission Files", &["mis", "txt"])
}

#[tauri::command]
fn open_mission() -> Result<Option<MissionFile>, String> {
    let Some(path) = mission_dialog().pick_file() else {
        return Ok(None);
    };
    let bytes =
        fs::read(&path).map_err(|error| format!("Could not read {}: {error}", path.display()))?;
    let name = path
        .file_name()
        .and_then(|name| name.to_str())
        .unwrap_or("mission.mis")
        .to_owned();

    Ok(Some(MissionFile {
        path: path.to_string_lossy().into_owned(),
        name,
        bytes,
    }))
}

#[tauri::command]
fn save_mission(
    contents: String,
    current_path: Option<String>,
    suggested_name: String,
    force_picker: bool,
) -> Result<Option<String>, String> {
    let path = if !force_picker {
        current_path.map(PathBuf::from)
    } else {
        None
    };
    let path = match path {
        Some(path) => path,
        None => {
            let Some(path) = mission_dialog().set_file_name(&suggested_name).save_file() else {
                return Ok(None);
            };
            path
        }
    };

    fs::write(&path, contents)
        .map_err(|error| format!("Could not write {}: {error}", path.display()))?;
    Ok(Some(path.to_string_lossy().into_owned()))
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .setup(|app| {
            if let Some(window) = app.get_webview_window("main") {
                window.set_title("BHD .MIS Editor")?;
            }
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![open_mission, save_mission])
        .run(tauri::generate_context!())
        .expect("error while running the BHD .MIS Editor");
}
