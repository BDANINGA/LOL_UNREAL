"""Import the curated minimap and skill level PNGs without replacing existing assets."""
import json
from pathlib import Path

import unreal as u

project = Path(u.Paths.project_dir()).resolve()
source = project.parent / "SourceArt" / "UI"
entries = json.loads((source / "import_manifest.json").read_text(encoding="utf-8"))
asset_tools = u.AssetToolsHelpers.get_asset_tools()
results = []

for entry in entries:
    filename = source / entry["source"]
    asset_path = entry["asset_path"]
    if not filename.is_file():
        raise FileNotFoundError(filename)
    if u.EditorAssetLibrary.does_asset_exist(asset_path):
        results.append({"asset": asset_path, "status": "existing_skipped"})
        continue

    task = u.AssetImportTask()
    task.set_editor_property("filename", str(filename))
    task.set_editor_property("destination_path", asset_path.rsplit("/", 1)[0])
    task.set_editor_property("destination_name", asset_path.rsplit("/", 1)[1])
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", False)
    asset_tools.import_asset_tasks([task])

    texture = u.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(texture, u.Texture2D):
        raise RuntimeError(f"Texture import failed: {asset_path}")
    texture.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", u.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)
    texture.set_editor_property("address_x", u.TextureAddress.TA_CLAMP)
    texture.set_editor_property("address_y", u.TextureAddress.TA_CLAMP)
    if not u.EditorAssetLibrary.save_loaded_asset(texture):
        raise RuntimeError(f"Texture save failed: {asset_path}")
    results.append({"asset": asset_path, "source": str(filename), "status": "imported"})

report = project / "Saved" / "AssetSearch" / "ui-icon-import-results.json"
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps(results, indent=2), encoding="utf-8")
u.log(f"UI_ICON_IMPORT_COMPLETE: {len(results)} textures; report: {report}")
