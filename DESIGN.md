# Cat Guard — Game Design Document

Working title. A top-down maze game for the Orbitris handheld (400×240, 1-bit).

**Core fantasy:** you are the night guard of a house full of cats. Armed only with a
torch and your legs, you catch cats one by one and box them before they soil the
rugs and the bed.

---

## 1. High concept

One-line: **guard the house from peeing cats — catch them all and put them in the box.**

Two goals are fused into a single loop:

- **Hunt** — chase down cats, pick them up, carry them to the box.
- **Guard** — patrol the lit areas so no cat finishes marking a rug or the bed.

The game is won by boxing every cat. The game is lost by letting the house get
messed up past a limit. The torch (existing light system) is what makes guarding
interesting: you can only *see* a limited area, so you constantly triage which
location to watch and which cat to chase.

---

## 2. The core loop (one minute of play)

1. You sweep the torch across the corridors. A cat catches the light and bolts for
   the dark.
2. You corner it, grab it (it slows you down), and haul it to the box.
3. While you are busy, another cat slinks onto a rug and starts peeing. You drop
   the first cat, sprint back, and scare the second off before it finishes.
4. Repeat until every cat is boxed — the fewer cats left, the warier and faster
   the survivors get.

---

## 3. Win and lose

### Win

All cats are inside the box. (A cat counts as boxed the moment it is deposited.)

### Lose

The **mess meter** reaches 100%. Every time a cat finishes peeing on a target,
the meter fills:

- peeing on a rug adds `+MESS_RUG`
- peeing on the bed adds `+MESS_BED` (larger — the bed is sacred)

Each target shows its own fill as it is marked (a dithered darkening of the
white rectangle). The meter is shared across all targets.

> Tuning note: the stricter alternative "any single target fully marked = lose"
> is a one-line change if the shared meter feels too forgiving.

### Feedback

- A cat peeing has a ~2 s animation while the target fills. It can be interrupted
  by grabbing it or scaring it with light/approach.
- A cat *needing* to pee telegraphs by walking deliberately toward a target —
  that walk is the moment the player must read and react.

---

## 4. The world

Reuse the existing 15×15 maze (720×720 px world, 48 px corridors, 3×16 px blocks per cell).

| Object | Look | Collision | Purpose |
|---|---|---|---|
| Walls | black (torch-revealed) | solid | maze structure |
| Rug | white rectangle, black outline | **none** (passable) | pee target |
| Bed | white rectangle, black outline (larger) | **none** (passable) | high-value pee target |
| Box | distinct outline marker | **none** (interact) | deposit point |
| Sleeping spot *(later)* | small white marker | none | secondary "park a cat" spot |

Targets are passable so cats and the player walk over them — this is what lets
"guard the rug" and "chase onto the rug" coexist. The box is where the player
stands next to and presses interact.

**Layout:** start with 3–4 rugs + 1 bed spread across the maze, and 1 box near a
corner (far from the bed so carrying a cat is a real commitment).

---

## 5. The player

- Free movement through the maze with the existing circle-vs-wall slide
  collision (`move_with_collision`).
- Carries a torch: the existing radial light + dithered falloff + wall shadow
  cast (`raycast_walls`, `draw_light`).
- One cat at a time, and **carrying slows the player** (weight). No running while
  carrying.

### Controls

| Input | Action |
|---|---|
| D-pad / arrows | move |
| A | interact: grab cat (adjacent) / drop cat / deposit at box (adjacent) |
| B | run (faster, only when not carrying) |

Desktop: arrows, `Z` = A, `X` = B. Device: same as existing `ESP_KEY_*`.

### Pick up / carry / drop

- **Grab:** press A while adjacent to a cat → the cat is picked up.
- **Carry:** the cat is attached to the player. Player speed drops to a fraction
  (tuning: ~65%). The carried cat does not act.
- **Drop:** press A anywhere → the cat is put down at the player's position and
  resumes from a `flee` state.
- **Deposit:** press A while adjacent to the box → the cat is boxed (removed from
  play).

> Weight and one-cat-at-a-time are mandatory. A "panic radius" (grabbing one cat
> makes nearby cats flee) is deferred as optional spice — see §9.

---

## 6. The cats

Start with **4 cats** (tunable up to 6). Visual: white circle with a black
outline; visible only when lit (see §8), with "eyes" (2 pixels) hinted in the
dark.

A cat is a circle like the player, uses the same wall collision, and walks over
targets and the box (no collision). Cats collide with the player and each other.

**Speed:** cats are slower than the walking player but faster than the carrying
player. This is the single most important tuning knob — it defines whether
chases feel winnable.

---

## 7. Enemy AI — state machine + BFS

Deterministic, readable states (no pure random movement). Pathfinding is BFS on
the 15×15 cell grid (225 nodes, trivially cheap on the C3); cats follow the
waypoint chain with the slide collision and re-plan when a state changes. BFS
paths are string-pulled with wall clearance, so cats move diagonally where the
maze allows instead of only along cell axes.

| State | Trigger | Behaviour | Exits |
|---|---|---|---|
| `wander` | default | walk a BFS path to a random floor cell; re-roll on arrival / every few seconds | bladder timer → `seek_target`; lit → `flee`; grabbed → `carried` |
| `seek_target` | bladder timer expires | BFS to the closest **unlit, not-full** target; skip the last target that made the cat flee | reaches target → `peeing`; lit → `flee`; grabbed → `carried` |
| `peeing` | reached target | stationary ~2 s, target fills; interruptible | done → `wander` (timer reset); lit/grabbed → `flee` / `carried` |
| `flee` | lit by torch, or player within N tiles while carrying | BFS to the nearest "hide" cell (dark / far from player); speeds up | escape timer → `wander` |
| `carried` | player grabbed it | follows player; does not act | dropped → `flee`; boxed → removed |
| `wily` | only the last surviving cat | all of the above, plus avoids the torch cone and moves faster | — |

**Visibility / "lit" check:** a cat is lit when its center is inside the light
polygon (i.e. within torch radius and not occluded). Reuse `raycast_walls` for
line-of-sight; "lit" is the natural stealth trigger and means the player chooses
what a cat is afraid of by pointing the torch.

**Hiding cells:** precompute (or find on the fly) floor cells that are outside the
current lit region and reasonably far from the player.

---

## 8. Rendering — darkness overlay

Adopt the subtractive light model so white objects are automatically lit-only:

1. `fill_screen_buffer(LCD_WHITE)` — the world as if fully lit.
2. Draw the scene in true colors: rugs/bed (white + outline), cats (white +
   outline), obstacles and player (black), shadows (black).
3. Draw **darkness last**: for each row, compute the dithered lit-mask (the same
   raycast polygon + AEL span + Bayer dither already implemented), then write its
   **complement** (black) over unlit pixels, leaving lit pixels untouched.

Walls need no explicit drawing (darkness keeps them black). The lit edge keeps
its existing dither fade. Cost is the same byte-batched work as the current
light pass.

**"Eyes in the dark":** after the darkness pass, draw 2 white pixels at any cat
whose centre is unlit, so cats are hinted but not fully visible in the dark.

---

## 9. Difficulty & pacing

The game must get *harder* as cats are boxed (fewer cats should not mean easier).

1. **Panic scalar** (mandatory). A global 0→1 value rises each time a cat is
   boxed. It scales: cat speed, bladder frequency, flee distance, and the `wily`
   behaviour of the last cat.
2. **Wily last cat** (mandatory). When one cat remains, it avoids the torch cone
   (prefers dark waypoints), re-plans faster, and needs to pee more often — the
   "boss" of the run.
3. **Mess feedback** (recommended). The bladder interval shortens as the mess
   meter rises — the dirtier the house, the more frantic the remaining cats. This
   makes guarding pressure visible and self-escalating.
4. **Target diversification** (recommended). Late game, cats prefer the
   *least-guarded* (furthest from the player) target, so one cat can split your
   attention across the map.

Deferred as later spice:
- Carried-cat **struggle timer** (held too long → escapes).
- **Panic radius** on grab (nearby cats immediately flee).
- **Memory**: a cat that escaped a grab is temporarily warier.

---

## 10. Scoring (deferred, maps to existing persistence)

When the run ends (win or lose), compute a rank from: mess left, cats boxed,
time taken. Reuse `HighscoreTable` / `Stats` persistence. Not required for the
first playable build.

---

## 11. Technical integration

`HouseScreen` is the real game screen, registered in `init_game()` following the
existing screen state machine. It uses the shared `world` (maze/collision) and
`light` (raycast/polygon/darkness) modules.

Reuse directly:

- `raycast_walls` / `build_light_polygon` — LOS and "is lit" tests.
- `deepest_contact_walls` / `move_with_collision` — player and cat movement.
- `is_wall_block` — BFS grid definition.
- the light pipeline (`fill_polygon`, `draw_darkness_polygon`) — darkness
  overlay.
- `lcd_*` / `draw_*` primitives, camera, input, persistence.

New pieces:

- BFS pathfinder over the 15×15 cell grid (`pathfind.{h,cpp}`; implemented in M2).
- Cat struct + state machine (`cat.{h,cpp}`; M2-M3 states: `Wander`, `SeekTarget`, `Peeing`, `Flee`).
- Mess meter + target state (`target.h`; implemented in M3).
- Pick-up/carry/deposit logic.

---

## 12. Milestones (build order)

1. **M1 — Scaffold (implemented).** `HouseScreen` renders the maze, darkness overlay, player
   movement, 4 static cats (white + outline), rugs + bed + box as passable
   white-outline rects.
2. **M2 — Cat movement (implemented).** BFS pathfinding + wander + slide collision.
3. **M3 — Guard loop (implemented).** Bladder timer → seek → peeing → mess meter → lose
   condition; light scares cats (`flee`).
4. **M4 — Hunt loop.** Grab / carry (weight) / drop / box deposit → win condition.
5. **M5 — Difficulty.** Panic scalar + wily last cat + target diversification;
   tune speeds/timers.
6. **M6 — Polish.** Eyes in the dark, win/lose screens, scoring + persistence,
   (stretch) sleeping spots, sound/feedback.

---

## 13. Open questions & tuning knobs

- Mess meter: shared vs. per-target lose; exact `MESS_RUG` / `MESS_BED` values.
- Cat speed vs. walking player vs. carrying player.
- Bladder interval range and pee duration.
- "Lit" = inside light polygon (radial) vs. a narrower cone.
- 4 cats vs. 5 (revisit after M3/M4 playtests).

These are all intended to be single values or flags at the top of the new screen,
tuned during M5.
