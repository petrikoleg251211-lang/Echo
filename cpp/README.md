# EchoLabyrinth — v5.0.1 "Resonance" (C++17 / SFML 2.6.1)

A standalone, zero-placeholder C++17 acoustic roguelike survival horror engine built on **SFML 2.6.1** (`Graphics`, `Window`, `Audio`, `System`) with **OpenAL 3D spatial audio**, **5-track procedural dark-ambient soundtrack (`SoundtrackComposer`)**, **procedural 64×64 normal-mapped surface textures**, **8 distinct enemy archetypes + 3-phase Final Boss (*The Choir Sovereign*)**, **global Noise Level meter (0..100)**, **8 acoustic surface materials**, **32-room procedural labyrinth with dynamic room events**, **12 passive Relics with hidden harmonic synergies**, and **12 Audio-Lore Codex recordings**.

---

## Stage 0 Architecture & Modular Source Layout (`≤ ~900 lines per file`)

```text
cpp/
├── CMakeLists.txt              # CMake 3.16+ (auto-fetches SFML 2.6.1, optional ASan/UBSan)
├── CHANGELOG.md                # Full version history (v1.0 -> v5.0.1)
├── .clang-format               # Google/LLVM C++17 style (4 spaces, 110 column limit)
├── build_windows.bat           # 1-click Windows build + 18-suite self-test + launch script
├── assets/
│   ├── models/                 # Procedural Wavefront .obj meshes (auto-populated)
│   └── textures/               # Procedural 64x64 RGBA + Normal maps
├── include/
│   ├── Config.hpp              # Centralized gameplay, acoustic, relic & codex constants
│   ├── AssetManager.hpp        # Wavefront .obj parser & procedural 64x64 texture generator
│   ├── AcousticSystem.hpp      # Multi-bounce DDA raycaster, materials & Echo Trap
│   ├── AudioManager.hpp        # OpenAL 3D spatial sound & 5-track procedural OST engine
│   ├── Player.hpp              # Movement, frequency tuning, stamina, charged ping, relics
│   ├── EnemyAI.hpp             # 8 enemy archetypes + 3-phase Choir Sovereign Boss FSM
│   ├── Labyrinth.hpp           # 32-room procedural graph, room events, pedestals, codex
│   ├── SaveSystem.hpp          # ELSV5 checksummed persistence + V4/V3 backward migration
│   ├── UiRenderer.hpp          # Custom vector bitmap font & HUD primitives
│   └── Game.hpp                # Master game state, overlays, noise & run lifecycle
└── src/
    ├── main.cpp                # CLI entry point (--self-test, --seed)
    ├── AssetManager.cpp        # Mesh & texture synthesis
    ├── AcousticSystem.cpp      # Ray propagation, material reflections & Echo Traps
    ├── AudioManager.cpp        # OpenAL 3D spatial cues & dynamic crossfade mixer
    ├── SoundtrackComposer.cpp  # 5-track procedural dark-ambient OST DSP & WAV exporter
    ├── Player.cpp              # Locomotion, sonar abilities & collision resolution
    ├── Relics.cpp              # 12 Relic specs, pedestal pool rotation & harmonic synergies
    ├── Codex.cpp               # 12 Audio-Lore Codex transcripts & collection state
    ├── EnemyAI.cpp             # Core FSM navigation, acoustic perception & 2.5D rendering
    ├── EnemyVariants.cpp       # 8 enemy archetype parameters, noise tiers & stomp response
    ├── BossAI.cpp              # 3-phase Choir Sovereign Boss FSM & SonicBolt volleys
    ├── LabyrinthGen.cpp        # 32-room procedural layout, themes & connectivity guarantees
    ├── LabyrinthRooms.cpp      # Active room state, cross-room door propagation & BFS check
    ├── SaveSystem.cpp          # Checksummed save serialization & V3/V4->V5 migration
    ├── UiRenderer.cpp          # Vector typography & UI primitives
    ├── Game.cpp                # Window lifecycle & core gameplay tick
    ├── GameRun.cpp             # Run start/end, room transitions, wave spawning & checkpoints
    ├── GameNoise.cpp           # Global noise meter, silence decay, glass shatter & breaches
    ├── GameInput.cpp           # Keyboard/Mouse controls & tactical overlays
    ├── GameWorldRender.cpp     # 2.5D world rendering, lighting & sonar wavefronts
    ├── GameHudRender.cpp       # Tactical HUD, minimap, boss bar & Codex/Relic modals
    └── GameSelfTest.cpp        # 18-suite deterministic headless verification
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
