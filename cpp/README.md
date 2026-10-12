# EchoLabyrinth — v5.4.0 "Resonance" (C++17 / SFML 2.6.1 / OpenAL 3D / GLSL PostFX)

A standalone, zero-placeholder C++17 acoustic roguelike survival horror engine built on **SFML 2.6.1** (`Graphics`, `Window`, `Audio`, `System`) with **OpenAL 3D spatial audio**, **GLSL `#version 120` Post-Processing Shaders (Bloom, Chromatic Aberration, Shockwave UV, CRT Scanlines)**, **6-stem adaptive dark-ambient soundtrack (`SoundtrackComposer`) with real-time Low-Pass & Schroeder Reverb DSP**, **4 procedural biomes**, **13 enemy archetypes + Room 16 Mini-Boss (*The Conductor*) + Room 31 3-Phase Final Boss (*The Choir Sovereign*)**, **4 Colorblind Palettes & Directional Closed Captions (`[F6]`/`[F7]`/`[F8]`)**, and **CPack Portable ZIP/NSIS/TGZ Release Packaging**.

---

## Stage 0–4 Architecture & Modular Source Layout (`24 Modules + Shaders + CPack`)

```text
cpp/
├── CMakeLists.txt              # CMake 3.16+ (auto-fetches SFML 2.6.1, ASan/UBSan, CPack ZIP/NSIS/TGZ)
├── LICENSE                     # MIT License (C++17 Code) + CC0 1.0 Public Domain (3D/Audio/Shader Assets)
├── CHANGELOG.md                # Full version history (v1.0 -> v5.4.0 Stages 0–4)
├── .clang-format               # Google/LLVM C++17 style (4 spaces, 110 column limit)
├── build_windows.bat           # 1-click Windows build + 21-suite self-test + launch script
├── resources/
│   └── echolabyrinth.rc        # Windows VS_VERSION_INFO executable metadata (v5.4.0.0)
├── assets/
│   ├── ASSET_MANIFEST.md       # CC0 1.0 Public Domain asset & audio registry
│   ├── icon/
│   │   └── echolabyrinth_icon.svg # Official vector sonar-reticle application icon
│   ├── models/                 # 7 Wavefront .obj 3D meshes
│   └── shaders/
│       └── acoustic_postfx.frag # GLSL #version 120 Bloom, Chromatic Aberration & Shockwave shader
├── include/
│   ├── Config.hpp              # Centralized balance, 4 biomes, 4 difficulties, colorblind & captions
│   ├── AssetManager.hpp        # Wavefront .obj parser, 64x64 textures & 64x64 window icon generator
│   ├── AcousticSystem.hpp      # Multi-bounce DDA raycaster, EchoCrystal ray splitting & Echo Traps
│   ├── AudioManager.hpp        # OpenAL 3D spatial sound, 6-stem mixer & Low-Pass/Reverb DSP
│   ├── PostProcessor.hpp       # Off-screen FBO & GLSL fragment shader post-processing pipeline
│   ├── Player.hpp              # Movement, frequency tuning, stamina, charged ping, relics
│   ├── EnemyAI.hpp             # 13 enemy archetypes + Conductor Mini-Boss + Choir Sovereign Boss FSM
│   ├── Labyrinth.hpp           # 32-room procedural graph, 4 biomes, room events, pedestals, codex
│   ├── SaveSystem.hpp          # ELSV6 checksummed persistence + V5/V4/V3 backward migration
│   ├── UiRenderer.hpp          # Custom vector bitmap font & HUD primitives
│   └── Game.hpp                # Master game state, hit-stop, closed captions, benchmark & balance CLI
└── src/
    ├── main.cpp                # CLI entry (--self-test, --balance-report, --benchmark, --export-ost, --version)
    ├── AssetManager.cpp        # Mesh parser, procedural 64x64 textures & 64x64 window icon synthesis
    ├── AcousticSystem.cpp      # Ray propagation, EchoCrystal prism splitting & Echo Traps
    ├── AudioManager.cpp        # OpenAL 3D cues, 6-stem crossfader & One-Pole LPF / Schroeder Reverb
    ├── SoundtrackComposer.cpp  # 5-track procedural dark-ambient OST DSP & WAV exporter
    ├── PostProcessor.cpp       # RenderTexture FBO & GLSL shader uniform manager
    ├── Player.cpp              # Locomotion, sonar abilities & collision resolution
    ├── Relics.cpp              # 12 Relic specs, pedestal pool rotation & harmonic synergies
    ├── Codex.cpp               # 12 Audio-Lore Codex transcripts & collection state
    ├── EnemyAI.cpp             # Core FSM navigation, 0.45s attack telegraph, Leech & Banshee logic
    ├── EnemyVariants.cpp       # 13 enemy archetype parameters, noise tiers & stomp response
    ├── BossAI.cpp              # 3-phase Choir Sovereign, 2-phase Conductor & TuningFork beams
    ├── LabyrinthGen.cpp        # 32-room procedural layout, 4 biomes & connectivity guarantees
    ├── LabyrinthRooms.cpp      # Active room state, cross-room door propagation & BFS check
    ├── SaveSystem.cpp          # Checksummed save serialization & V3/V4/V5->V6 migration
    ├── UiRenderer.cpp          # Vector typography & UI primitives
    ├── Game.cpp                # Window lifecycle, runtime icon, hit-stop & spatial sound captions
    ├── GameRun.cpp             # Run start/end, biome audio sync, wave spawning & V6 checkpoints
    ├── GameNoise.cpp           # Global noise meter, shockwave triggers & closed captions
    ├── GameInput.cpp           # Keyboard/Mouse controls & [F6]/[F7]/[F8] accessibility hotkeys
    ├── GameWorldRender.cpp     # 2.5D world rendering routed through PostProcessor GLSL pipeline
    ├── GameHudRender.cpp       # Tactical HUD, closed captions panel, minimap & accessibility menu
    ├── GameBalance.cpp         # 1000-run Monte Carlo bot, 1200-frame benchmark & Suites 18–21
    └── GameSelfTest.cpp        # 21-suite deterministic headless verification runner
```

---

## Core Gameplay Systems

### 1. Multi-Frequency Sonar (`1` / `2` / `3`) & Acoustic Materials
- **Low (`1` — 110 Hz Infra-Scan)**: Penetrates **Glass** and **Water Curtain** walls directly while reflecting off Stone and Metal. Reveals hidden sub-structural pathways.
- **Mid (`2` — 440 Hz Balanced)**: Standard tactical navigation sweep with balanced acoustic attenuation and reflection decay.
- **High (`3` — 1760 Hz Resonance)**: **Shatters brittle Glass walls** on contact, clears **Water Curtains**, and triggers **Resonance Locks** at `>= 0.72` energy.
- **8 Surface Materials**:
  - `Stone` (`0.84` reflectivity) — cold slate acoustic reflection.
  - `Metal` (`0.96` reflectivity) — high-intensity specular bounce + harmonic ringing.
  - `Moss` (`0.38` reflectivity) — dampens footsteps (`0.45x` noise) and absorbs sonar rays.
  - `CrackedWall` (`0.72` reflectivity) — destructible via **Acoustic Stomp (`V`)**.
  - `Glass` (`0.90` reflectivity) — shattered by High-Frequency (`3`) sonar or Stomp (`V`), causing a loud `58.0` noise spike.
  - `Water` (`0.65` reflectivity) — penetrable by Low-Frequency (`1`), cleared by High-Frequency (`3`).
  - `SoundFoam` (`0.12` reflectivity) — anechoic lining that swallows `88%` of wave energy and dampens footsteps (`0.25x`).
  - `EchoCrystal` (`1.18` reflectivity) — **amplifies reflected sonar energy (`> 1.0x`)** and shifts bounce color toward violet/cyan.

### 2. Global Noise Meter (`0..100`) & Cross-Room Sound Propagation
- Every player action contributes to the global **Noise Level (`0..100`)**:
  - **Tier 0 (`0..24` — CALM)**: Silence timer (`3.5s`) decays noise rapidly (`11/s`). Clearing a combat chamber without exceeding `25` awards a **GHOST CLEAR (`+15 Echo Shards`)**.
  - **Tier 1 (`25..49` — SUSPICION)**: Patrol speed increases (`1.15x`) and enemies investigate recent acoustic origins.
  - **Tier 2 (`50..74` — ALERT)**: Extended hearing radius (`1.30x`) and coordinated search patterns.
  - **Tier 3 (`75..100` — HUNT / CROSS-ROOM BREACH)**: All enemies in the chamber lock onto the player (`1.38x` speed). Crossing `75` triggers an **Acoustic Breach** through doors (`0.85` open / `0.35` closed attenuation), pulling reinforcements from adjacent chambers!

### 3. 8 Enemy Archetypes + 3-Phase Final Boss (*The Choir Sovereign*)
1. **Stalker**: Balanced acoustic hunter (`Investigate` -> `Chase`).
2. **Brute**: Heavy armored juggernaut (`120 HP`); requires **Charged Sonar (`Hold SPACE`)** or **Acoustic Stomp (`V`)** to stun.
3. **Phantom**: Phase-shifting wraith that teleports behind the player when struck by high-energy pings.
4. **Hound**: Fast pack predator (`248 px/s`) drawn to continuous footstep trails.
5. **Mimic**: Disguises its acoustic signature as a gold **Key** or **Harmonic Node** until approached within `145 px` or hit by Charged Ping.
6. **Siren**: Emits periodic **Jamming Pulses** (`6.5s` cycle) that scramble the player's sonar HUD and elevate chamber noise.
7. **Warden**: Projects a directional **Frontal Armor Shield** that reflects uncharged sonar pings (`88%` damage reduction from the front; flank or use Charged Ping / Low-Freq penetration).
8. **Scuttler**: Chitinous swarm unit that **feeds on sonar energy**, healing and accelerating (`1.55x`) when struck by standard pings — eliminate silently with **Acoustic Stomp (`V`)** or lure away with **Decoy Stones (`G`)**.
9. **Boss — *The Choir Sovereign*** (`Room 31`):
   - **Phase 1 (`100%..66% HP`)**: Radial 8-bolt Sonic Volleys (`5.5s`) + rotating Resonance Shield.
   - **Phase 2 (`66%..33% HP`)**: Spiral 12-bolt Volleys + Arena Blackout bursts (`6.5s` cycle).
   - **Phase 3 (`< 33% HP`)**: Relentless 16-bolt Choir Novas (`2.8s`) + high-speed pursuit (`1.35x`).

### 4. Procedural 5-Track Adaptive Soundtrack (`SoundtrackComposer`)
- Synthesizes **5 full-length stereo 44.1 kHz 16-bit PCM compositions** at startup and exports standalone `.wav` files into `assets/music/`:
  1. `01_Subterranean_Survey.wav` — D Dorian dark ambient drone, cavern bells & hydrophone droplets.
  2. `02_Fractured_Echoes.wav` — Unsettling metallic bowed tritone textures & insectoid clicks.
  3. `03_Blind_Pursuit.wav` — 128 BPM industrial polyrhythmic percussion & aggressive sub-bass.
  4. `04_The_Choir_Sovereign.wav` — 140 BPM gothic formant choir, pipe organ pedals & brass stabs.
  5. `05_Sanctuary_Of_Silence.wav` — Ethereal Cydonian harmonic pads & warm crystalline harp.

---

## Controls

| Input | Action |
| :--- | :--- |
| **W / A / S / D** or **Arrows** | Move Explorer |
| **Left Shift (Hold)** | Sprint (`1.60x` speed, drains Stamina, louder footsteps) |
| **Left Ctrl / C (Hold)** | Sneak (`0.52x` speed, silent footsteps) |
| **1 / 2 / 3** | Select Sonar Frequency: `1` Low (110 Hz), `2` Mid (440 Hz), `3` High (1760 Hz) |
| **Space (Tap)** or **Left Click** | Directional Sonar Ping (`64°` cone toward mouse cursor) |
| **Space (Hold >= 0.45s -> Release)** | **Charged Penetrating Ping** (`1.45x` range, `2.6x` stun, pierces obstacles) |
| **Q** or **Right Click** | **360° Radial Pulse** (`120 rays`, consumes `35%` Resonator Energy) |
| **G** | Throw **Decoy Stone** toward mouse cursor (lures enemies to impact point) |
| **F** | Deploy **Echo Trap** (`12s` autonomous beacon pulsing every `1.5s`, max 2) |
| **V** | **Acoustic Stomp** (close-range shockwave: knocks back enemies, breaches Cracked Walls, shatters Glass) |
| **E** | Interact with **Relic Pedestal** |
| **Tab** | Toggle **Audio-Lore Codex** & **Active Relic Synergies** overlay |
| **P** or **Esc** | Pause / Settings / Manual Checkpoint Save (`F5` Quick-Save, `F9` Quick-Load) |

---

## Building & Running (Windows & Linux)

### Windows (Visual Studio 2022 / MSVC)
1. Double-click `build_windows.bat` or run from Developer PowerShell:
   ```bat
   cd cpp
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release --parallel 4
   .\build\bin\Release\EchoLabyrinth_SFML.exe --self-test
   .\build\bin\Release\EchoLabyrinth_SFML.exe
   ```

### Linux (Ubuntu / Debian GCC 13+ or Clang 16+)
```bash
sudo apt-get update && sudo apt-get install -y \
  cmake g++ libxrandr-dev libxcursor-dev libudev-dev \
  libopenal-dev libflac-dev libvorbis-dev libgl1-mesa-dev libegl1-mesa-dev libfreetype-dev

cd cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
./build/bin/EchoLabyrinth_SFML --self-test
./build/bin/EchoLabyrinth_SFML
```

### Linux with AddressSanitizer + UndefinedBehaviorSanitizer (`ASan + UBSan`)
```bash
cd cpp
cmake -B build-san -DCMAKE_BUILD_TYPE=RelWithDebInfo -DECHO_ENABLE_SANITIZERS=ON
cmake --build build-san --parallel 4
./build-san/bin/EchoLabyrinth_SFML --self-test
```

### CLI Verification Flags
- `--self-test` : Runs the deterministic **18-suite headless engine verification** (OBJ parser, 64×64 textures, multi-bounce acoustics, 8 materials, Echo Trap, 8 enemy archetypes, 3-phase Choir Sovereign Boss, 32-room procedural connectivity + BFS door reachability, ELSV5 save/load + V3/V4 migration, global noise & cross-room propagation, 12 Relics + synergies, 12 Codex entries, 5-track procedural OST composer & WAV export, and Stage 0 modular integrity).
- `--seed <uint32>` : Launches the game with a fixed procedural generation seed.
