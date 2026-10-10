import cmakeRaw from '../../cpp/CMakeLists.txt?raw';
import configHppRaw from '../../cpp/include/Config.hpp?raw';
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
    summary: 'CMake 3.20+ configuration targeting C++17 & SFML 2.6 with automatic FetchContent fallback and Windows DLL copy.',
    content: cmakeRaw,
  },
  {
    path: 'cpp/include/Config.hpp',
    module: 'Core Config & Materials',
    summary: 'Compile-time constants, SurfaceMaterial reflectivity/persistence tables, and WaveSignature definitions.',
    content: configHppRaw,
  },
  {
    path: 'cpp/include/AcousticSystem.hpp',
    module: 'Acoustic Simulation',
    summary: 'Pre-allocated ray-front and wall-echo ring buffers, AcousticEvent struct, and batched sf::VertexArray interface.',
    content: acousticHppRaw,
  },
  {
    path: 'cpp/src/AcousticSystem.cpp',
    module: 'Acoustic Simulation',
    summary: '2D segment-segment intersection, specular normal reflection R = D - 2(D·N)N, material attenuation, and sf::BlendAdd rendering.',
    content: acousticCppRaw,
  },
  {
    path: 'cpp/include/Labyrinth.hpp',
    module: 'World & 15-Room Topology',
    summary: 'RoomDefinition, RoomPortal, InteractiveMechanism, and LoreCollectible structures.',
    content: labyrinthHppRaw,
  },
  {
    path: 'cpp/src/Labyrinth.cpp',
    module: 'World & 15-Room Topology',
    summary: 'Procedural & authored construction of all 15 labyrinth chambers, harmonic switches, and resonance gates.',
    content: labyrinthCppRaw,
  },
  {
    path: 'cpp/include/Player.hpp',
    module: 'Player Controller',
    summary: 'MovementStance (Sneak, Walk, Sprint), circle-segment sliding collision, and sonar cooldown management.',
    content: playerHppRaw,
  },
  {
    path: 'cpp/src/Player.cpp',
    module: 'Player Controller',
    summary: 'Frame-rate-independent movement, stride-based footstep wave emission, directional cone ping, radial pulse, and decoy stone throw.',
    content: playerCppRaw,
  },
  {
    path: 'cpp/include/EnemyAI.hpp',
    module: 'Hostile AI (The Listener)',
    summary: '6-state finite state machine (Idle, Patrol, InvestigateSound, SearchLastKnown, PursuePlayer, Attack, ReturnToPatrol).',
    content: enemyHppRaw,
  },
  {
    path: 'cpp/src/EnemyAI.cpp',
    module: 'Hostile AI (The Listener)',
    summary: 'Perceived loudness calculation with wall occlusion, suspicion accumulation, and enemy footstep tremor emission.',
    content: enemyCppRaw,
  },
  {
    path: 'cpp/include/SaveSystem.hpp',
    module: 'Persistence & Checkpoints',
    summary: 'SaveData struct and deterministic file serialization interface.',
    content: saveHppRaw,
  },
  {
    path: 'cpp/src/SaveSystem.cpp',
    module: 'Persistence & Checkpoints',
    summary: 'Stream-safe checkpoint, switch state, and lore archive serialization.',
    content: saveCppRaw,
  },
  {
    path: 'cpp/src/main.cpp',
    module: 'Application Lifecycle',
    summary: 'Fixed-timestep accumulator loop (60 Hz), procedural OpenAL PCM buffer synthesis, and SFML window/event handling.',
    content: mainCppRaw,
  },
];
