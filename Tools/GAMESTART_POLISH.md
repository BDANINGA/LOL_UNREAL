# GameStart UI polish — 2026-09-14

Target: `/Game/UI/wbp_gamestart`.

Preserves the background, logo, central panel and left/right room actions. Adds a thin gold panel frame, centered nickname field and small captions; corrects button proportions and provides distinct hover/pressed tints. Tidies the IP overlay and fixes the Address spelling.

`text_id` stays a TextBlock directly under CanvasPanel; `text_ip` stays under Overlay_0. PC_GameStart's runtime editable replacements and the original button names remain compatible. No C++ or EventGraph changes.

Validation: blueprint compiled and saved; exported EventGraph is byte-for-byte identical. PIE checked runtime editable inputs, bound host/join and IP-commit delegates, join-panel reveal, empty-name and empty-address guards. Visually reviewed start and join screens. No remote multiplayer connection test performed.

Backup and previews: `LOL_UNREAL/Saved/DesignBackups/GameStartBefore_20260914/`.

Reapply: `./Tools/unreal.ps1 exec -ScriptFile ./Tools/restyle_gamestart.py` (stop PIE first). Run `prepare_gamestart_test.py`, then `test_gamestart_ui.py` for the smoke check. Bridge accepts `list` and optional `-NodeId` to choose a specific editor when more than one is open.
