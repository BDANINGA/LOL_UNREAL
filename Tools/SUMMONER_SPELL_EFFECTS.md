# D Exhaust / F Flash visuals

The existing `LOL_SummonerSpellComponent` now supplies separate summoner VFX:

- `/Game/VFX/SummonerSpells/P_FlashDeparture`: diagonal white snap, lemon-yellow
  angular afterimage, gold core and 26 inward diamond fragments.
- `/Game/VFX/SummonerSpells/P_FlashArrival`: a larger diagonal white snap, lemon-yellow
  afterimage, gold core and 34 outward diamond fragments.
- `/Game/VFX/SummonerSpells/P_ExhaustAura`: two amber binding bands and inward
  shards/motes. A local-space looping effect follows the exhausted champion.

Flash particles finish within 0.55 seconds and auto-destroy. The main snap lasts
0.24 seconds and the broad diagonal rift lasts 0.48 seconds. There is no ring or
circular wave: the three Flash-only materials (`M_FlashStar`, `M_FlashSlash`,
`M_FlashChip`) create angular silhouettes. This distinguishes Flash from Ezreal E's
longer gold/cyan ring blink. Exhaust continues using the unchanged HitImpact materials.

Flash's cosmetic multicast runs only after successful server TeleportTo, using the
actual departure and final actor locations. Invalid, blocked or cooldown-rejected
casts do not emit it. Hidden owners and dedicated servers skip rendering.

Exhaust uses a replicated server expiration timestamp instead of an unreliable
start-only event. Clients gaining relevance can show the remaining effect. One
attached component is reused on refresh and cleaned up on expiration, death and
owner teardown. While active, a 0.05-second tick updates fog visibility and expiration;
it is disabled otherwise. This timer is cosmetic: original slow, damage reduction,
range and cooldown calculations remain intact.

Assets are hard-referenced for cooking. `Summoner Spell > VFX` on the summoner
component exposes the three effects and an Enable Spell Effects switch.
Author Flash materials with `Tools/create_flash_materials.py`, then rebuild only
Flash with `Tools/build_flash_effects.py` (`LOL.Summoner.BuildFlash`).
`Tools/build_summoner_effects.py` rebuilds all three particle systems through the
editor-only `LOL.Summoner.BuildEffects` command.

The standalone fixture is `prepare_summoner_effects_test.py`, followed by
`test_summoner_effects.py`. It exercises actual server spell methods, particle
counts, locations, rejection, tracking, refresh, gameplay values and cleanup.
Reports and previews are written to `Saved/DesignBackups`.

Initial revision validated on 2026-10-15: Editor Development and Win64 Shipping builds succeeded.
Full Shipping content cook passed with 0 errors and 0 warnings; all three new
particle systems are present in cooked content.
The original standalone PIE integration tests passed for Flash endpoints and
the Exhaust aura. Both rendered previews were inspected.
Refresh reuses one aura; expiry restores the original slow/damage values. True
damage remains exempt from Exhaust as in the original gameplay implementation.
Multiplayer transport and a launched packaged executable were not tested.

Flash redesign (2026-10-15): Editor build and the existing actual-cast integration
test passed again (14/22 particles at Flash endpoints). Inspected slowed preview
frames `FlashRedesign_0.png` through `FlashRedesign_2.png` in Saved/DesignBackups.
Exhaust and Ezreal E asset hashes are unchanged. The redesigned assets have not
been recooked; the full cook result above applies to the initial revision.

Visibility revision (2026-10-25): larger 380/440 cm snap and 360/420 cm rift,
thicker Flash-only masks, stronger midpoint opacity, 26/34 fragments and an added
gold core. Flash-specific emissive strength is 5 (previously 4). Rebuild only Flash;
shared HitImpact, Exhaust and Ezreal assets are not edited. The normal-speed
`test_flash_visibility.py` uses the summoner fixture and writes samples at roughly
0.07/0.23/0.4 seconds plus endpoint, rejection and cleanup results to
`Saved/Validation/FlashVisibility`. This revision has not been recooked or tested
in a launched packaged multiplayer session.
Editor Development build and the normal-speed test passed: both endpoints still
contained live particles at 0.083, 0.233 and 0.400 seconds (29/37, 28/36, 23/30),
then auto-cleaned. Diagnostic captures can be enabled with `capture_frames=True`;
keep it false for timing assertions because high-resolution capture stalls frames.
