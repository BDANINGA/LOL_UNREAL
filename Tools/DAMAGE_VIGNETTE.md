# Local damage vignette

Accepted positive damage to a player-controlled champion triggers an owning-client
RPC from `ULOL_StatComponent::ApplyDamage`. The controller validates the victim
against its current pawn and reuses a single `ULOL_DamageVignetteWidget`.
No flash is generated from health replication, healing, initialization, rejected
friendly damage or damage to other actors. Dedicated servers create no widget.

The native widget constructs one full-screen image using
`/Game/UI/Effects/M_DamageVignette`. Its material has a fully transparent center
and smooth crimson edges. No texture, camera post-process setting or external
plugin is required. The constructor holds the material reference for cooking.
This is a native widget, not an additional Widget Blueprint asset.

- Rise: 0.045 seconds; total duration: 0.60 seconds, using real time.
- Peak opacity: clamp(0.30 + damage/maxHP * 1.4, 0.30, 0.65).
- Repeated hits refresh from the current opacity without stacking widgets.
- Hidden at completion, cleared when possession changes, removed on controller teardown.
- Non-focusable and hit-test invisible; added to the local player's screen at Z=50.
- `LOL_PlayerController` > UI > Damage Feedback > Enable Damage Vignette toggles new pulses.
- Native widget constants control pulse timing/strength; `create_damage_vignette.py`
  authors the UI material's border width and color.

Validation (2026-10-13): Editor Development and Win64 Shipping builds passed.
Shipping content cooking completed with 0 errors and 0 warnings; the vignette
material is included in cooked content.
`prepare_damage_vignette_test.py` and `test_damage_vignette.py` exercise real
TakeDamage calls in a temporary PIE scene. Local damage, rejected friendly/zero/
negative damage, other victims, smooth fade, repeated hits, disable switch,
wrong-pawn RPC rejection, possession changes, lethal damage and dead victims passed.
The actual rendered UI was inspected in `Saved/DesignBackups/DamageVignette_Preview.png`.
The test scene was discarded and GameStartMap restored. Network transport and a
launched packaged executable were not exercised by this standalone PIE test.
