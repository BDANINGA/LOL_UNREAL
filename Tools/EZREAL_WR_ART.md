# W / R unified arcane art

W uses the new `/Game/Level/ezreal/FX/Missiles/SM_EzrealEssenceFlux` mesh:
a hollow gold halo, a tilted inner orbit, soft cyan rims and swept-back filaments.
The two materials, `M_EzrealWCore` and `M_EzrealWEnergy`, share the gold, ivory
and cyan palette of Q/basic attacks and animate light flowing around the rings.
The mesh faces +X; W's visual rotation is zero and its visual scale is 0.85.
Its existing particle trail is retained. It has 2,720 triangles and no shadow casting.

R retains its existing broad travelling crescent and particle-tail component.
`/Game/Level/ezreal/FX/R/M_EzrealRWave` now has an ivory leading edge, a slimmer
gold body, braided cyan inner ribbons and a second fine gold arc. Its colors
match Q/W/basic attack materials, and lower body emission preserves the internal
detail. Width, fade timing and projectile lifetime remain the same.

Changes are visual only. Range, speed, collision, damage and penetration logic
are unchanged. Q/basic attack assets and E blink assets are not rebuilt.

Authoring:
- Run `generate_ezreal_w_mesh.py` in Python to create the original OBJ source.
- Run `import_ezreal_w_mesh.py` in Unreal Python to import only W and its materials.
- Run `create_ezreal_r_material.py` in Unreal Python to update R's shader graph.

Validation uses fresh unsaved scenes via `prepare_ezreal_q_preview.py` /
`test_ezreal_w_trail.py` and `prepare_ezreal_r_preview.py` /
`test_ezreal_r_visual.py`. Reports/screenshots are in `Saved/DesignBackups`.

Validated on 2026-10-13:
- Editor Development and Win64 Shipping builds succeeded.
- W projectile mesh/orientation, flight particles, detached tail and cleanup passed.
- R movement, unchanged collision radius, detached tail, cleanup and legacy toggle passed.
- Both runtime screenshots were visually inspected; preview scenes were discarded.
- Shipping content cook succeeded with 0 errors and 0 warnings. The new W mesh,
  both W materials and the updated R material are present in cooked content.
- Multiplayer and a launched packaged executable were not tested in this revision.
