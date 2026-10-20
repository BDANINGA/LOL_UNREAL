# D Exhaust / F Flash visuals

The existing `LOL_SummonerSpellComponent` now supplies separate summoner VFX:

- `/Game/VFX/SummonerSpells/P_FlashDeparture`: diagonal white snap, lemon-yellow
  angular afterimage and 12 inward diamond fragments.
- `/Game/VFX/SummonerSpells/P_FlashArrival`: a larger diagonal white snap, lemon-yellow
  afterimage and 20 outward diamond fragments.
- `/Game/VFX/SummonerSpells/P_ExhaustAura`: two amber binding bands and inward
  shards/motes. A local-space looping effect follows the exhausted champion.

Flash bursts finish within 0.25 seconds and auto-destroy. There is no ring or
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
