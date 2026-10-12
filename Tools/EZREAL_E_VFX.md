# Ezreal E blink effects

E spawns independent world-space bursts at the server's actual departure and
arrival positions after successful movement. Skill learning, mana, cooldown,
range, teleport behavior and existing damage logic are unchanged.

- `/Game/Level/ezreal/FX/E/P_EzrealEDeparture`: contracting gold ring, short vertical
  flare, 28 inward gold shards and 22 inward cyan motes (53 particles total).
- `/Game/Level/ezreal/FX/E/P_EzrealEArrival`: expanding gold ring, bright central
  flash/vertical flare, 38 outward shards and 30 cyan motes (71 particles total).

Both are one-shot Cascade systems with a maximum particle lifetime of 0.6 s.
They reuse the existing procedural `M_HitFlash`, `M_HitSpark` and `M_HitRing`
materials without modifying them. They have no collision, lights or damage.

Champion settings under **Ezreal | E VFX**: `bEnableBlinkEffect`,
`BlinkEffectScale` (default 1), `EDepartureEffect` and `EArrivalEffect`.
The cosmetic multicast is unreliable, skips dedicated-server rendering and
locally hidden champions, and auto-destroys the particle components. Visibility
uses the existing actor-hidden state; it does not independently query fog at
each endpoint. Network delivery needs a separate multiplayer playtest.

Authoring: after an Editor build, run `Tools/build_ezreal_e_blink.py` through
`Tools/unreal.ps1`. This rebuilds only these two particle assets, overwriting
manual edits to them. The command is excluded from non-editor builds.

Validation: `prepare_ezreal_e_preview.py` creates an unsaved test scene;
`test_ezreal_e_blink.py` invokes actual E casts and checks unlearned/cooldown
rejection, range, endpoint positions, burst emission, cleanup and effect toggles.
Reports and screenshots are written to `Saved/DesignBackups/EzrealE*`.
End PIE and reload the original map without saving the preview scene.

Validated 2026-10-12: real E casts moved 475 units and emitted exactly two
systems at the recorded endpoints. Unlearned/cooldown rejection, both active
bursts, hidden/disabled suppression and cleanup passed. Three animation stages
were captured and visually reviewed. Editor build and Win64 Shipping build/full
cook passed (0 errors, 0 warnings). Log: `Saved/Logs/EzrealE_Blink_ShippingCook.log`.
Packaged launch and multiplayer sessions were not tested.
