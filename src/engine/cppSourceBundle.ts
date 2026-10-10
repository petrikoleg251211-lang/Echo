import cmakeRaw from '../../cpp/CMakeLists.txt?raw';
import readmeRaw from '../../cpp/README.md?raw';
import batRaw from '../../cpp/build_windows.bat?raw';
import configHppRaw from '../../cpp/include/Config.hpp?raw';
import gameHppRaw from '../../cpp/include/Game.hpp?raw';
import gameCppRaw from '../../cpp/src/Game.cpp?raw';
import audioHppRaw from '../../cpp/include/AudioManager.hpp?raw';
import audioCppRaw from '../../cpp/src/AudioManager.cpp?raw';
import uiHppRaw from '../../cpp/include/UiRenderer.hpp?raw';
import uiCppRaw from '../../cpp/src/UiRenderer.cpp?raw';
import acousticHppRaw from '../../cpp/include/AcousticSystem.hpp?raw';
import acousticCppRaw from '../../cpp/src/AcousticSystem.cpp?raw';
import labyrinthHppRaw from '../../cpp/include/Labyrinth.hpp?raw';
import labyrinthCppRaw from '../../cpp/src/Labyrinth.cpp?raw';
import playerHppRaw from '../../cpp/include/Player.hpp?raw';
import playerCppRaw from '../../cpp/src/Player.cpp?raw';
import enemyHppRaw from '../../cpp/include/EnemyAI.hpp?raw';
import enemyCppRaw from '../../cpp/src/EnemyAI.cpp?raw';
import saveHppRaw from '../../cpp/include/SaveSystem.hpp?raw';
import saveCppRaw from '../../cpp/src/SaveSystem.cpp?raw';
import mainCppRaw from '../../cpp/src/main.cpp?raw';

export interface CppSourceFile {
  path: string;
  module: string;
  summary: string;
  content: string;
}

export const CPP_SOURCE_FILES: CppSourceFile[] = [
  {
    path: 'cpp/CMakeLists.txt',
    module: 'Build System',
    summary: 'CMake 3.20+ configuration targeting C++17 & SFML 2.5/2.6 with FetchContent fallback and POST_BUILD Windows DLL copy.',
    content: cmakeRaw,
  },
  {
    path: 'cpp/build_windows.bat',
    module: 'Build Script',
    summary: 'One-click Windows 10/11 CMake configuration, Release build, self-test runner, and executable launcher.',
    content: batRaw,
  },
  {
    path: 'cpp/README.md',
    module: 'Documentation',
    summary: 'Ukrainian build and gameplay manual for Windows 10/11 (Visual Studio 2022 + CMake).',
    content: readmeRaw,
  },
  {
    path: 'cpp/include/Config.hpp',
    module: 'Core Config & Materials',
    summary: 'AppState enum, SurfaceMaterial visual/acoustic properties, and pre-allocated pool constants.',
    content: configHppRaw,
  },
  {
    path: 'cpp/include/Game.hpp',
    module: 'Application & Menus',
    summary: 'Complete native SFML Game lifecycle, Main/Pause/Settings/Controls/Victory/GameOver menus, HUD, Minimap, and self-test suite.',
    content: gameHppRaw,
  },
  {
    path: 'cpp/src/Game.cpp',
    module: 'Application & Menus',
    summary: 'Implementation of the 60 Hz fixed-timestep loop, letterbox view scaling, menus, HUD, 15-room minimap, and automated self-tests.',
    content: gameCppRaw,
  },
  {
    path: 'cpp/include/AcousticSystem.hpp',
    module: 'Acoustic Simulation',
    summary: 'Ray-front, WallEchoTrace, and ExpandingWaveRing pools, spatial illumination queries, and VertexArray batching.',
    content: acousticHppRaw,
  },
  {
    path: 'cpp/src/AcousticSystem.cpp',
    module: 'Acoustic Simulation',
    summary: '2D segment intersection, specular reflection, 3D wall extrusion quads, material surface details, and additive wave rendering.',
    content: acousticCppRaw,
  },
  {
    path: 'cpp/include/AudioManager.hpp',
    module: 'Procedural Audio',
    summary: 'Polyphonic 16-voice sf::Sound pool, procedural 44.1 kHz PCM sf::SoundBuffer synthesizer, and looping ambient drone.',
    content: audioHppRaw,
  },
  {
    path: 'cpp/src/AudioManager.cpp',
    module: 'Procedural Audio',
    summary: 'Synthesizes footsteps, directional sonar pings, radial gongs, decoy clatters, mechanical switches, enemy alerts, and cavern drone.',
    content: audioCppRaw,
  },
  {
    path: 'cpp/include/UiRenderer.hpp',
    module: 'UI & Vector Font',
    summary: 'Resilient font loader with built-in 14-segment geometric vector glyph fallback so UI never crashes if .ttf files are absent.',
    content: uiHppRaw,
  },
  {
    path: 'cpp/src/UiRenderer.cpp',
    module: 'UI & Vector Font',
    summary: 'System font discovery (Windows/Linux) + crisp sf::VertexArray vector stroke text, panels, and progress bars.',
    content: uiCppRaw,
  },
  {
    path: 'cpp/include/Labyrinth.hpp',
    module: 'World & 15-Room Topology',
    summary: 'RoomDefinition, RoomPortal, InteractiveMechanism, LoreCollectible, and full state restoration interface.',
    content: labyrinthHppRaw,
  },
  {
    path: 'cpp/src/Labyrinth.cpp',
    module: 'World & 15-Room Topology',
    summary: '6 distinct architectural chamber archetypes across 15 interconnected rooms with sealed Resonance Gate bulkheads.',
    content: labyrinthCppRaw,
  },
  {
    path: 'cpp/include/Player.hpp',
    module: 'Player Controller',
    summary: 'MovementStance (Sneak, Walk, Sprint), DecoyProjectile trajectory, and surveyor rendering.',
    content: playerHppRaw,
  },
  {
    path: 'cpp/src/Player.cpp',
    module: 'Player Controller',
    summary: 'Circle-segment sliding collision, stride-based footstep emission, directional sonar cone, radial pulse, and wall-clamped decoy throw.',
    content: playerCppRaw,
  },
  {
    path: 'cpp/include/EnemyAI.hpp',
    module: 'Hostile AI (The Listener)',
    summary: '6-state sound-hunting FSM with obstacle waypoint steering, sliding collision, and echo-based visibility.',
    content: enemyHppRaw,
  },
  {
    path: 'cpp/src/EnemyAI.cpp',
    module: 'Hostile AI (The Listener)',
    summary: 'Perceived loudness calculation with wall occlusion, suspicion decay, waypoint steering, and animated sensor tendrils.',
    content: enemyCppRaw,
  },
  {
    path: 'cpp/include/SaveSystem.hpp',
    module: 'Persistence & Checkpoints',
    summary: 'Executable-relative path resolution and full checkpoint/switch/lore/visited state serialization.',
    content: saveHppRaw,
  },
  {
    path: 'cpp/src/SaveSystem.cpp',
    module: 'Persistence & Checkpoints',
    summary: 'Corruption-resilient save parser with strict bounds clamping for rooms, decoys, volume, switches, and lore.',
    content: saveCppRaw,
  },
  {
    path: 'cpp/src/main.cpp',
    module: 'Entry Point',
    summary: 'Resolves executable directory via std::filesystem, supports --self-test CLI flag, and launches echo::Game.',
    content: mainCppRaw,
  },
];
