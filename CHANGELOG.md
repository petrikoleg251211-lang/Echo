# EchoLabyrinth — Changelog

All notable architectural and gameplay changes across **EchoLabyrinth** (`C++17 / SFML 2.6.1` native project & web companion studio) are documented in this file.

---

## [5.1.0] — Stage 1: Gameplay Quality, Balance Table, 4 Difficulties, 3-Room Tutorial & Save V6 (2026-10-11)

### Added
- **Stage 1.1 — Game Feel (`Config.hpp`, `Game.cpp`, `AcousticSystem.cpp`, `EnemyAI.cpp`, `BossAI.cpp`):**
  - Hit-stop freeze window (`45 ms` on sonar hit, `60 ms` on enemy shatter).
  - Exponentially decaying camera shake (`exp(-7.5 * t)`) scaled by impact amplitude.
  - Harmonically pulsating wall echo traces (`sin(age * 11.5)`).
  -Distinct per-material footstep timbres (`Stone`, `Concrete`, `Metal`, `Water`, `DampMoss`, `Glass`).
  - Enemy attack telegraphing (`0.45s` melee windup and `0.42s` ranged SonicBolt windup) with converging warning ring, acoustic pulse, and spatial audio cue before damage lands.
- **Stage 1.2 — Centralized `BalanceTable` & `--balance-report` (`Config.hpp`, `GameBalance.cpp`):**
  - Centralized `cfg::balance::BalanceTable` and headless `--balance-report` CLI mode simulating 1,000 bot runs, printing average duration, win rate (`11.4%` on `Surveyor`, within target `8%–15%`), and Top-5 relics by pick/win contribution.
- **Stage 1.3 — 4 Difficulty Modes (`Explorer`, `Surveyor`, `Abyssal`, `Silence`):**
  - `Explorer` (Easy: 8 HP, `-20%` enemy HP, `+25%` silence decay & shards).
  - `Surveyor` (Normal: 6 HP, standard acoustic balance).
  - `Abyssal` (Hard: `+25%` enemy spawns, slower silence decay `x0.72`, `-25%` Echo Shards).
  - `Silence` (Hardcore: locked `1 HP` even with armor relics, no minimap, `+25%` enemies).
- **Stage 1.4 — Interactive 3-Room Tutorial (`Game.cpp`, `GameHudRender.cpp`):**
  - Step-by-step onboarding (`Move + Sneak [Shift]` -> `Sonar Ping/Whistle [LMB/E/SPACE/Q]` -> `Echo Trap [R] / Decoy [RMB/G]`) that vanishes once completed and persists `tutorial_done` in save files.
- **Stage 1.5 — Readability of Fear (`GameHudRender.cpp`, `GameNoise.cpp`):**
  - HUD Noise Cause badge showing the exact action that triggered the latest noise spike (`((o)) CAUSE: [PING] SONAR CONE (+12)`).
  - Off-screen/perimeter directional threat chevrons (`INVESTIGATING SOUND`, `HUNTING YOU!`, `STRIKE INCOMING!`) with distance readout.
- **Save Format V6 (`ECHOLABYRINTH_SAVE_V6`):**
  - Persists `difficulty` and `tutorial_done` with FNV-1a checksum and full backwards compatibility for `V5`, `V4`, `V3`, `V2`, and `V1` saves (verified in Suite 18).

---

## [5.0.1] — Stage 0: Project Hygiene, Modular Decomposition & 5-Track OST (2026-10-11)

### Added
- **5-Layer Original Soundtrack (`SoundtrackComposer.cpp` / `AudioManager`):**
  - Synthesized 5 seamless 16-bit 44.1 kHz PCM musical tracks with circular feedback delay/reverb and zero-click loop boundaries (`01_subterranean_survey.wav`, `02_fractured_echoes.wav`, `03_blind_pursuit.wav`, `04_resonance_sanctum.wav`, `05_the_choir_sovereign.wav`), plus `--export-ost` CLI mode.
- **Modular C++17 Architecture (Rule 6: all source files `<= ~900` lines):**
  - Split `Labyrinth.cpp` (1387 lines) into `LabyrinthGen.cpp`, `LabyrinthRooms.cpp`, `Relics.cpp`, and `Codex.cpp`.
  - Split `Game.cpp` (995 lines) into `Game.cpp`, `GameRun.cpp`, and `GameNoise.cpp`.
  - Split `EnemyAI.cpp` (943 lines) into `EnemyAI.cpp`, `EnemyVariants.cpp`, and `BossAI.cpp`.
  - Split `AudioManager.cpp` (1135 lines) into `AudioManager.cpp` and `SoundtrackComposer.cpp`.
- **Code Formatting & Editor Standards:**
  - Added `.clang-format` (Google/LLVM C++17 style, 4 spaces, 110-column limit) and `.editorconfig`.
- **CI Pipeline Enhancements (`.github/workflows/ci.yml`):**
  - Added `FetchContent` caching (`build/_deps`), build artifact upload (`EchoLabyrinth_SFML.exe` + `assets/`), and a dedicated `ubuntu-asan-ubsan` sanitizer job (`-fsanitize=address,undefined`) running all 18 deterministic `--self-test` suites.

### Changed
- Synchronized deterministic `--self-test` suite count (**18/18 suites**) across `GameSelfTest.cpp`, `cpp/README.md`, `cpp/build_windows.bat`, and `.github/workflows/ci.yml`.
- Renamed web package in `package.json` from `react-example` to `echolabyrinth-web` (`v5.0.0`) and removed unused dependencies (`motion`, `autoprefixer`).

---

## [5.0.0] — "Resonance" (Blocks 1–5 Expansion)

### Added
- **Block 1 — Sound as Risk Resource:**
  - Global Noise Meter (`0..100`) with 4 alert tiers (`Quiet`, `Audible`, `Loud`, `Scream`), cross-room sound propagation through open portals (`propagateNoiseFromRoom`), neighboring Hunter Room Breach (`reopenRoomForNoiseIntrusion`), and `Ghost Clear` bonus (`+50% Echo Shards` + `Silent Step` relic when room peak noise `<= 30`).
- **Block 2 — Material Acoustic Physics:**
  - Extended surface materials (`Stone`, `Concrete`, `Metal` lossless `+2` bounces, `Water` `2.0x` propagation range + `+15` splash noise, `DampMoss` `80%` absorption & stealth shelter, `ResonanceDoor`, and breakable `Glass` partitions with `+40` shatter noise).
  - Harmonic Pipe Columns (`4.0s` resonant lure hum when struck by sonar).
- **Block 3 — Extended Tactical Arsenal:**
  - 3 Sonar Frequency Tuning modes (`[1] Low Sub-Bass`, `[2] Mid Standard`, `[3] High Ultrasonic`).
  - Narrow `15°` Ultrasonic Whistle (`[Q]`, `+5` noise), Seismic Stomp (`[V]`, enemy knockback + glass shatter, `+35` noise), and Delayed Echo Trap / Beacon Mine (`[T]`, `3.0s` timer + 3 lure bursts).
- **Block 4 — Extended Bestiary (`9 EnemyVariant`):**
  - Added `SilentStalker` (zero passive movement tremors, `0.5s` inhale telegraph before lunge), `DeafBrute` (ignores airborne sounds, reacts only to ground vibrations), `Mimic` (disguised as resonator crystal until scanned with High-Freq `[3]` / Whistle), and `EchoSwarm` (repeats & amplifies nearby player sounds by `+12` noise).
- **Block 5 — Dynamic Room Events, 12 Relics, 12 Codex Logs & Save V5 (`ELSV5`):**
  - Added `EchoStorm`, `AcousticBlackout`, and `ResonanceOvercharge` chamber events.
  - Expanded relic pool to 12 relics (`SilentStep`, `PhaseDampener`, `UltrasonicLens`, `AbyssalTuningFork`) and 12 persistent Audio-Lore Codex entries (`[J]`).
  - Upgraded `SaveSystem` to `ECHOLABYRINTH_SAVE_V5` (`ELSV5`) with full backwards compatibility for V4 saves.

---

## [4.0.0] — "Abyssal Choir" (Roguelike Progression & 3-Phase Boss)

### Added
- 32-chamber seed-deterministic 2D grid generator (`Start`, `Combat`, `Treasure`, `Secret`, `Puzzle`, `Sanctuary`, `Shop`, `Elite`, `Boss`, `FinalExit`).
- Echo Shards currency, Quartermaster Shop, 8 core Acoustic Relics, and 3-phase `AbyssalBoss` (*The Choir Sovereign*).
- `SaveSystem` V4 format with FNV-1a checksum verification.

---

## [3.0.0] — "2.5D Acoustic Perspective"

### Added
- Wavefront `.obj` 3D mesh loader (`AssetManager`) with procedural 64x64 textures and 2.5D perspective extrusion for walls, resonators, and hunter archetypes (`Stalker`, `Warden`, `Shrieker`, `Phantom`).
- OpenAL 3D spatial audio with wall occlusion attenuation (`computeTransmissionFactor`).

---

## [2.0.0] — "Harmonic Reflections"

### Added
- Pre-allocated raycasting pool (`kMaxActiveRays = 2048`, `kMaxEchoTraces = 2400`) with multi-bounce specular wall reflections, `AcousticResonator` crystal puzzles, and `DecoyProjectile` lures.

---

## [1.0.0] — "Initial Acoustic Prototype"

### Added
- Core C++17 / SFML 2.6 fixed-timestep (`60 Hz`) loop, player `Sneak / Walk / Sprint` stances, directional sonar ping, radial pulse, and basic `Patrol -> Investigate -> Pursue` AI state machine.
