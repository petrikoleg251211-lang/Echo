import cmakeRaw from '../../cpp/CMakeLists.txt?raw';
import readmeRaw from '../../cpp/README.md?raw';
import batRaw from '../../cpp/build_windows.bat?raw';
import manifestRaw from '../../cpp/assets/ASSET_MANIFEST.md?raw';
import surveyorObjRaw from '../../cpp/assets/models/surveyor_rig.obj?raw';
import stalkerObjRaw from '../../cpp/assets/models/listener_stalker.obj?raw';
import wardenObjRaw from '../../cpp/assets/models/listener_warden.obj?raw';
import crystalObjRaw from '../../cpp/assets/models/resonator_crystal.obj?raw';
import leverObjRaw from '../../cpp/assets/models/harmonic_lever.obj?raw';
import terminalObjRaw from '../../cpp/assets/models/surveyor_terminal.obj?raw';
import turbineObjRaw from '../../cpp/assets/models/vent_turbine.obj?raw';
import configHppRaw from '../../cpp/include/Config.hpp?raw';
import assetHppRaw from '../../cpp/include/AssetManager.hpp?raw';
import assetCppRaw from '../../cpp/src/AssetManager.cpp?raw';
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
    summary: 'CMake 3.20+ configuration targeting C++17 & SFML 2.5/2.6 with POST_BUILD copy of 3D assets/ and Windows runtime DLLs.',
    content: cmakeRaw,
  },
  {
    path: 'cpp/build_windows.bat',
    module: 'Build Script',
    summary: 'One-click Windows 10/11 CMake configuration, Release build, 9-suite self-test runner, and executable launcher.',
    content: batRaw,
  },
  {
    path: 'cpp/README.md',
    module: 'Documentation',
    summary: 'Ukrainian build, architecture, 2.5D/3D asset pipeline, and gameplay manual for Windows 10/11 (Visual Studio 2022 + CMake).',
    content: readmeRaw,
  },
  {
    path: 'cpp/assets/ASSET_MANIFEST.md',
    module: 'Asset Manifest (CC0)',
    summary: 'Complete registry of 3D Wavefront OBJ models, procedural 64x64 RGBA material textures, OpenAL 3D sound effects, and CC0 licensing.',
    content: manifestRaw,
  },
  {
    path: 'cpp/include/Config.hpp',
    module: 'Core Config & Materials',
    summary: 'AppState enum, 6 SurfaceMaterials with reflectivity & transmission coefficients, WaveSignature types, and pool constants.',
    content: configHppRaw,
  },
  {
    path: 'cpp/include/AssetManager.hpp',
    module: '2.5D/3D Asset Manager',
    summary: 'Wavefront .obj 3D mesh loader, procedural 64x64 RGBA material texture generator, and 2.5D oblique normal-lit renderer.',
    content: assetHppRaw,
  },
  {
    path: 'cpp/src/AssetManager.cpp',
    module: '2.5D/3D Asset Manager',
    summary: 'Implements OBJ parsing with procedural fallback meshes, 6 material textures, and depth-sorted 2.5D triangle projection.',
    content: assetCppRaw,
  },
  {
    path: 'cpp/include/Game.hpp',
    module: 'Application & Menus',
    summary: 'Complete native SFML Game lifecycle, Main/Pause/Settings/Controls/Victory/GameOver menus, HUD, Minimap, and 9-suite self-test.',
    content: gameHppRaw,
  },
  {
    path: 'cpp/src/Game.cpp',
    module: 'Application & Menus',
    summary: '60 Hz fixed-timestep loop, 2.5D prop rendering, 4-channel audio mixer settings, 15-room minimap, and 9-suite automated self-tests.',
    content: gameCppRaw,
  },
  {
    path: 'cpp/include/AcousticSystem.hpp',
    module: 'Acoustic Simulation',
    summary: 'Ray-front, WallEchoTrace, ExpandingWaveRing, and AcousticResonator pools, multi-wall transmission, and VertexArray batching.',
    content: acousticHppRaw,
  },
  {
    path: 'cpp/src/AcousticSystem.cpp',
    module: 'Acoustic Simulation',
    summary: '2D ray-segment intersection, specular reflection, 2.5D oblique wall extrusion quads, textured bevels, and additive wave rendering.',
    content: acousticCppRaw,
  },
  {
    path: 'cpp/include/AudioManager.hpp',
    module: '3D Spatial Audio & Music',
    summary: 'OpenAL 3D spatial positioning, 20-voice polyphonic pool, 29 procedural SoundEffectIds, and 4-layer adaptive soundtrack.',
    content: audioHppRaw,
  },
  {
    path: 'cpp/src/AudioManager.cpp',
    module: '3D Spatial Audio & Music',
    summary: 'Synthesizes material-specific footsteps, sonar pings, enemy 3D calls, environmental ambience, and 4 crossfading music layers.',
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
    summary: 'RoomDefinition, DecoProp, EnemyVariant (Stalker/Warden), AcousticResonator, AcousticHazardZone, and BFS traversability verifier.',
    content: labyrinthHppRaw,
  },
  {
    path: 'cpp/src/Labyrinth.cpp',
    module: 'World & 15-Room Topology',
    summary: '15 distinct interconnected chambers with architectural props, brittle shale hazards, resonators, and 2D BFS grid verifier.',
    content: labyrinthCppRaw,
  },
  {
    path: 'cpp/include/Player.hpp',
    module: 'Player Controller',
    summary: 'MovementStance (Sneak, Walk, Sprint), material floor detector, walk-cycle animation phase, and 2.5D Surveyor rig rendering.',
    content: playerHppRaw,
  },
  {
    path: 'cpp/src/Player.cpp',
    module: 'Player Controller',
    summary: 'Circle-segment sliding collision, stride-based footstep emission, directional sonar cone, radial pulse, and 2.5D Surveyor rig.',
    content: playerCppRaw,
  },
  {
    path: 'cpp/include/EnemyAI.hpp',
    module: 'Hostile AI (The Listener)',
    summary: '6-state sound-hunting FSM supporting Stalker and Warden variants with smooth animation state blending and 2.5D mesh rendering.',
    content: enemyHppRaw,
  },
  {
    path: 'cpp/src/EnemyAI.cpp',
    module: 'Hostile AI (The Listener)',
    summary: 'Multi-wall material occlusion perception, suspicion decay, waypoint steering, and articulated 2.5D Stalker/Warden carapace.',
    content: enemyCppRaw,
  },
  {
    path: 'cpp/include/SaveSystem.hpp',
    module: 'Persistence & Checkpoints',
    summary: 'V3 checkpoint & audio mixer state serialization with FNV-1a checksum verification.',
    content: saveHppRaw,
  },
  {
    path: 'cpp/src/SaveSystem.cpp',
    module: 'Persistence & Checkpoints',
    summary: 'Atomic .tmp write + rename, FNV-1a checksum validation, and full session restoration.',
    content: saveCppRaw,
  },
  {
    path: 'cpp/src/main.cpp',
    module: 'Entry Point',
    summary: 'Resolves executable directory via std::filesystem, supports --self-test CLI flag, and launches echo::Game.',
    content: mainCppRaw,
  },
  {
    path: 'cpp/assets/models/surveyor_rig.obj',
    module: '3D Models (OBJ)',
    summary: '3D Wavefront OBJ mesh for the Surveyor player suit, acoustic visor, and sonar emitter.',
    content: surveyorObjRaw,
  },
  {
    path: 'cpp/assets/models/listener_stalker.obj',
    module: '3D Models (OBJ)',
    summary: '3D Wavefront OBJ mesh for The Listener (agile Stalker carapace variant).',
    content: stalkerObjRaw,
  },
  {
    path: 'cpp/assets/models/listener_warden.obj',
    module: '3D Models (OBJ)',
    summary: '3D Wavefront OBJ mesh for The Listener (armored Heavy Warden juggernaut variant).',
    content: wardenObjRaw,
  },
  {
    path: 'cpp/assets/models/resonator_crystal.obj',
    module: '3D Models (OBJ)',
    summary: '3D Wavefront OBJ mesh for the octahedron Harmonic Resonator Crystal.',
    content: crystalObjRaw,
  },
  {
    path: 'cpp/assets/models/harmonic_lever.obj',
    module: '3D Models (OBJ)',
    summary: '3D Wavefront OBJ mesh for the Harmonic Conduit Lever pedestal.',
    content: leverObjRaw,
  },
  {
    path: 'cpp/assets/models/surveyor_terminal.obj',
    module: '3D Models (OBJ)',
    summary: '3D Wavefront OBJ mesh for the Surveyor Log Terminal console.',
    content: terminalObjRaw,
  },
  {
    path: 'cpp/assets/models/vent_turbine.obj',
    module: '3D Models (OBJ)',
    summary: '3D Wavefront OBJ mesh for the Subterranean Ventilation Turbine rotor.',
    content: turbineObjRaw,
  },
];
