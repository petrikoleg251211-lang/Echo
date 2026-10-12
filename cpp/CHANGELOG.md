# EchoLabyrinth — Changelog

All notable architectural and gameplay changes across **EchoLabyrinth** (`C++17 / SFML 2.6.1` native project & web companion studio) are documented in this file.

---

## [5.4.0] — Stage 4: Release Packaging, CPack Portable Archives, Window Icon & Stress Benchmark (2026-10-12)

### Added
- **CPack Cross-Platform Release Packaging (`CMakeLists.txt` & `.github/workflows/release.yml`):**
  - Integrated `include(CPack)` generating standalone portable `ZIP` / `NSIS` packages on Windows (bundling `EchoLabyrinth_SFML.exe`, `openal32.dll`, SFML runtime DLLs, `assets/`, `README.md`, `CHANGELOG.md`, and `LICENSE`) and `TGZ` / `ZIP` packages on Linux.
  - Added multi-platform `.github/workflows/release.yml` workflow executing `--self-test` (21 suites), `--benchmark 1200`, `--balance-report 1000`, and `--export-ost` before publishing portable archives.
- **Application Window Icon & Windows Resource Metadata (`AssetManager`, `resources/echolabyrinth.rc`, `assets/icon/echolabyrinth_icon.svg`):**
  - Procedural 64×64 RGBA sonar-reticle icon (`AssetManager::generateWindowIconImage()`) applied at runtime via `m_window->setIcon(64, 64, ...)` with concentric emerald/cyan/violet sonar rings, directional scan cone, and diamond crystal core.
  - Added scalable vector icon (`assets/icon/echolabyrinth_icon.svg`), Windows `VS_VERSION_INFO` resource script (`resources/echolabyrinth.rc`), and dual `MIT + CC0 1.0 Public Domain` `LICENSE` file.
- **CLI Stress Benchmark & Release Flags (`--benchmark`, `--version`, `--export-ost`):**
  - Added `Game::runBenchmark()` (`--benchmark [frames]`) stress-testing 120-ray radial pulses, `EchoCrystal` secondary ray splitting, and 4 concurrent Stage 2 enemy AI variants (`Conductor`, `Banshee`, `TuningForkConstruct`, `Leech`) in Room 16, reporting `avgFrameMs`, `p99FrameMs`, `estimatedFps`, `peakActiveRays`, and `crystalSplitsTriggered`.
- **Verification Suite 21 (`runStage4SelfTest`):** Deterministic headless verification of 64×64 window icon synthesis, 180-frame high-load benchmark (`avgFrameMs < 16.67 ms`), and release asset integrity.

---

## [5.3.0] — Stage 3: Visuals, GLSL Shaders, 6-Stem Adaptive Audio DSP & Accessibility (2026-10-12)

### Added
- **GLSL Post-Processing Shader Pipeline (`PostProcessor.hpp/cpp` & `assets/shaders/acoustic_postfx.frag`):**
  - Off-screen `sf::RenderTexture` capture + custom GLSL `#version 120` fragment shader (`sf::Shader`) with embedded fallback for headless CI.
  - **9-Tap Gaussian Bloom Glow** on bright sonar wavefronts and `EchoCrystal` prism reflections, dynamically scaled by active ray count.
  - **Radial Chromatic Aberration** (`RGB` channel separation) that ramps up in `Loud`/`Scream` noise tiers (`> 60`) and spikes during `Hit-Stop` impacts.
  - **Acoustic Shockwave UV Refraction Ripple** triggered on `Radial Pulse`, `Stomp`, `Glass Shatter`, `Banshee Wail`, and `Boss Dissonance` shockwaves.
  - **Subtle CRT Scanline & Vignette** overlay with toggleable PostFX (`[F8]`) and Photosensitivity Flash Reduction mode.
- **6-Stem Adaptive Audio Crossfade & Real-Time Low-Pass / Schroeder Reverb DSP (`AudioManager` & `SoundtrackComposer`):**
  - `AdaptiveStemWeights` & `AudioManager::computeAdaptiveStemWeights` managing 6 dynamic stems (`subDrone`, `sonarArp`, `tensionStrings`, `pursuitPercussion`, `sanctumPad`, `bossChoir`) across Exploration, Suspicion, Active Pursuit, Sanctuary, and 3-phase Boss combat.
  - Biome-aware real-time DSP parameters (`lowPassCutoffHz` muffled at `2950 Hz` in `DrownedGalleries` vs crisp `14800 Hz` in `CrystalVaults`; `reverbWetMix` cathedral `0.58` in `CrystalVaults`).
  - Procedural `AudioManager::applyLowPassFilter` (one-pole RC filter) and `AudioManager::applyReverbTail` (Schroeder comb feedback delay network) on 16-bit PCM buffers.
- **Accessibility Suite (`Config.hpp`, `GameHudRender.cpp`, `SaveSystem.cpp`):**
  - **4 Colorblind Modes (`[F6]`):** `Normal`, `Deuteranopia` (Red-Green safe Cobalt/Amber), `Protanopia` (Cyan/Gold), and `Tritanopia` (Blue-Yellow safe Crimson/Teal) transforming both wall material echoes and HUD/caption colors.
  - **Directional Visual Sound Subtitles / Closed Captions (`[F7]`):** Real-time HUD closed captions (`[N 20m] HUNTER PURSUIT ROAR!`, `[E 14m] HIGH-PRESSURE STEAM VENT HISS`) with 8-way compass bearings and distance in meters for D/deaf and hard-of-hearing players.
  - **Camera Shake Intensity Slider (`0%–100%`) & Photosensitivity Flash Reduction:** Persisted in `ECHOLABYRINTH_SAVE_V6` (`accessibility` record) with FNV-1a checksum validation.
- **Verification Suite 20 (`runStage3SelfTest`):** Deterministic headless verification of GLSL shader uniforms, 6-stem adaptive audio weights, PCM low-pass/reverb DSP, 4 colorblind palettes, directional closed captions, and Save V6 round-trip.

---

## [5.2.0] — Stage 2: Content & Biomes Expansion (2026-10-11)

### Added
- **4 Distinct Subterranean Biomes (8 chambers each across the 32-room run):**
  - **Biome I — The Drowned Galleries (Rooms 1–8):** Water muffles footsteps (`-25%` noise) while hydro-acoustics carry water-reflected waves `+35%` farther.
  - **Biome II — The Crystal Vaults (Rooms 9–16):** Introduces `SurfaceMaterial::EchoCrystal` prism walls (`96%` reflectivity) that split any incident sonar ray into **3 chaotic prism rays** (`+18%` energy), plus fragile glass hazards.
  - **Biome III — The Iron Foundry (Rooms 17–24):** Industrial metal chambers (`94%` reflectivity) with a rhythmic **Steam-Hammer Press** every `5.5s` that masks player footsteps by `-75%` for `1.35s`.
  - **Biome IV — The Living Mycelium (Rooms 25–32):** Sound-absorbing organic walls (`1.55x` faster silence decay) and bioluminescent spores that softly illuminate moving predators.
- **4 New Enemy Archetypes & Room 16 Mini-Boss (`13 EnemyVariant` total):**
  - **Leech (`EnemyVariant::Leech`):** Fast acoustic parasite that latches onto the player without instant death, draining stamina and pulsing `+8` noise every `0.8s` until shaken off via `Sprint [LShift]` (`0.7s`) or `Radial Pulse [Space]`.
  - **Banshee (`EnemyVariant::Banshee`):** Unleashes a piercing wail (`+22` noise + room-wide shockwave) when alerted.
  - **Tuning Fork Construct (`EnemyVariant::TuningForkConstruct`):** Stationary turret immune to direct pings (`0.0x`), vulnerable to reflected wall bounces (`2.25x`), firing twin parallel acoustic beams.
  - **The Conductor (`EnemyVariant::TheConductor`):** Guaranteed Chamber 16 Mini-Boss (`155 HP`, 2 phases) that sweeps a 4-beam rotating acoustic cross and summons resonators in Phase 2 (`<= 50% HP`).
- **Verification Suite 19 (`runStage2SelfTest`):** Automated verification of all 4 biomes, `EchoCrystal` 1->3 ray splitting, biome noise modifiers, and all 4 new enemy archetypes.

---

## [5.1.0] — Stage 1: Gameplay Quality, Balance & Onboarding (2026-10-11)

### Added
- **Game Feel & Hit-Stop (`30–60 ms`):** Frame-freeze impact window on sonar hits/kills, exponential-decay directional camera shake, and `0.45s` enemy attack telegraph windup with visual/audio cues.
- **Data-Driven `BalanceTable` & `--balance-report` CLI (`GameBalance.cpp`):** Centralized balance table and headless 1000-run bot simulator targeting `8%–15%` win rate on `Surveyor` difficulty with Top-5 relic rankings.
- **4 Difficulty Modes:** `Explorer` (Story), `Surveyor` (Standard), `Abyssal` (`+25%` enemies, slower silence decay, fewer shards), and `Silence` (1-HP hardcore, no minimap).
- **Interactive 3-Room Tutorial & Save V6 (`ELSV6`):** Step-by-step onboarding in Rooms 1–3 (`tutorialDone` persisted in `ECHOLABYRINTH_SAVE_V6` with full V5 migration) and HUD wave-form noise meter showing the exact cause of the last noise spike + off-screen threat arrows.

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
