# Ezreal Q and basic attack meshes

Original meshes under `/Game/Level/ezreal/FX/Missiles`:

- `SM_EzrealMysticShot`: rounded, tapered cyan/ivory energy core, three curved
  gold/cyan filaments and crossed soft wakes; 512 triangles. Uses the existing
  Q radius normalization and gold/cyan particle trail.
- `SM_EzrealBasicBolt`: smaller golden core, two curved filaments and short wakes;
  384 triangles, 91 cm total length. The Ezreal-only attack mesh transform is now
  unit scale and zero rotation to match the mesh's +X forward axis.

Four unlit procedural materials use UV gradients, edge falloff and flowing light.
The central cores use moderate emissive intensity to preserve their silhouettes.
All mesh sections disable shadow casting, including the opaque energy cores.
Existing source meshes and their materials are retained. Projectile collision,
speed, homing, damage and pooling are unchanged; other champions keep their
existing transforms. Q's new mesh faces +X with a zero rotation offset.

Sources: `Tools/Art/Ezreal/*.obj` and `ArcaneMissiles.mtl`. Run
`generate_ezreal_missile_meshes.py` in Python to regenerate the sources, then run
`import_ezreal_missile_meshes.py` through the Unreal bridge. Reimport overwrites
the two new meshes and updates their four materials; it does not modify old assets.

`prepare_ezreal_missile_preview.py` creates an unsaved scene, and
`test_ezreal_missile_meshes.py` exercises actual Q multicast and pooled basic-attack
launch paths. Results and screenshot: `Saved/DesignBackups/EzrealMissiles*`.
End PIE and reload the original map without saving the test scene.

Validated on 2026-10-12 in PIE: both actual launch paths select the new meshes,
forward orientation and basic-attack scale are correct, Q particles are present,
both projectiles clean up, and a second basic attack reuses the pool with the
same mesh. The final screenshot was reviewed after removing the basic bolt's
shadow. Editor and Win64 Shipping builds and full cook passed (0 errors,
0 warnings). Log: `Saved/Logs/EzrealMissiles_ShippingCook.log`.
Packaged launch and multiplayer sessions were not tested.
