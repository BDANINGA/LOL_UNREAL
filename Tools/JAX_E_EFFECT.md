# Jax E counter-strike

`UJaxCounterStrikeVisualComponent` draws three local cosmetic planes while E is
active: a thin ground range ring and two rotating gold ribbons above it. E's
authoritative completion replaces them with a 0.4 second expanding ground wave.
The range ring and damage sweep share `GetCounterStrikeRadius()` (currently 300).
The ring's outer UV radius .94 maps to that world-space radius, and the wave
remains at the strike location if Jax moves afterward.

`bCounterStrikeActive` is replicated with a RepNotify for clients gaining
relevance during the cast; a reliable multicast triggers the short release.
Inactive replication does not erase an already-started release wave. Dedicated
servers allocate no visual layers. Owner-hidden state hides the effect, death
clears it and cancels the pending strike, and inactive effects do not tick.

Character hits now require the body capsule in E's sweep. Previously enemy
`AttackRangeSphere` (400 cm on the test Ezreal) and widget overlaps could cause
E to damage enemies outside the visible range. A body intersecting the ring
still counts, even if its center is just beyond the ring. Damage formula,
cooldown, two-second duration and stun duration are unchanged.

Materials: `/Game/VFX/Jax/M_JaxERange`, `M_JaxESpin`, `M_JaxERelease`.
Author with `Tools/create_jax_e_materials.py` in the full Unreal Editor.
Shader `Age` controls angular motion; `Fade` handles appearance/disappearance.

Run `Tools/prepare_jax_e_test.py` from a clean editor, then execute
`runpy.run_path(r'C:\Nexon\lol_unreal\Tools\test_jax_e_visual.py')` in PIE.
The unsaved fixture checks casts/recasts, moving/hidden/dead casters, 300 cm ring
scale, time-varying rotation, release damage, inside and outside victims, delayed
death cancellation, fixed impact location, cleanup and cooldown rejection.
Results/screenshots: `LOL_UNREAL/Saved/Validation/JaxE`.

Final validation passed: Editor and Game Win64 Development builds and all PIE
checks. The 220 cm target took 79.7 damage; the 440 cm target and the target near
the dead caster stayed at 600 HP. Temporary maps were discarded after testing.

Network replication is implemented but separate networked clients have not been
exercised. Existing packaged output requires recooking/repackaging for the new
assets and gameplay module.
