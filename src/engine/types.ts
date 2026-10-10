export type SurfaceMaterial = 'Stone' | 'Metal' | 'Water' | 'DampMoss' | 'ResonanceDoor';

export type WaveSignature =
  | 'FootstepSneak'
  | 'FootstepWalk'
  | 'FootstepSprint'
  | 'DirectionalPing'
  | 'RadialPulse'
  | 'DecoyImpact'
  | 'MechanismClunk'
  | 'EnemyTremor';

export type MovementStance = 'Sneak' | 'Walk' | 'Sprint';

export type EnemyState =
  | 'Idle'
  | 'Patrol'
  | 'InvestigateSound'
  | 'SearchLastKnown'
  | 'PursuePlayer'
  | 'Attack'
  | 'ReturnToPatrol';

export type GameState = 'TITLE_MENU' | 'PLAYING' | 'PAUSED' | 'VICTORY' | 'GAME_OVER';

export interface Vec2 {
  x: number;
  y: number;
}

export interface MaterialSpec {
  reflectivity: number;
  persistenceSec: number;
  rgb: [number, number, number];
  hex: string;
  label: string;
  acousticNote: string;
}

export const MATERIAL_SPECS: Record<SurfaceMaterial, MaterialSpec> = {
  Stone: {
    reflectivity: 0.82,
    persistenceSec: 3.8,
    rgb: [56, 189, 248],
    hex: '#38bdf8',
    label: 'Stone Masonry',
    acousticNote: '82% reflectivity · Cold cyan echo trace',
  },
  Metal: {
    reflectivity: 0.94,
    persistenceSec: 5.2,
    rgb: [245, 158, 11],
    hex: '#f59e0b',
    label: 'Forged Iron',
    acousticNote: '94% reflectivity · High-persistence amber ring',
  },
  Water: {
    reflectivity: 0.64,
    persistenceSec: 2.7,
    rgb: [45, 212, 191],
    hex: '#2dd4bf',
    label: 'Subterranean Water',
    acousticNote: '64% reflectivity · Teal ripple scatter',
  },
  DampMoss: {
    reflectivity: 0.35,
    persistenceSec: 1.6,
    rgb: [74, 222, 128],
    hex: '#4ade80',
    label: 'Damp Moss',
    acousticNote: '35% reflectivity · High acoustic absorption',
  },
  ResonanceDoor: {
    reflectivity: 0.88,
    persistenceSec: 4.5,
    rgb: [167, 139, 250],
    hex: '#a78bfa',
    label: 'Resonance Gate',
    acousticNote: '88% reflectivity · Unlocks via harmonic lever',
  },
};

export interface WallSegment {
  a: Vec2;
  b: Vec2;
  material: SurfaceMaterial;
  doorId?: number;
  isOpen?: boolean;
}

export interface AcousticRay {
  pos: Vec2;
  prevPos: Vec2;
  dir: Vec2;
  speed: number;
  intensity: number;
  maxDistance: number;
  traveled: number;
  reflections: number;
  maxReflections: number;
  signature: WaveSignature;
  rgb: [number, number, number];
  active: boolean;
}

export interface WallEchoTrace {
  hitPoint: Vec2;
  tangent: Vec2;
  halfLength: number;
  intensity: number;
  age: number;
  lifetime: number;
  material: SurfaceMaterial;
  active: boolean;
}

export interface AcousticEvent {
  origin: Vec2;
  loudness: number;
  radius: number;
  signature: WaveSignature;
  emittedByPlayer: boolean;
}

export interface RoomPortal {
  position: Vec2;
  radius: number;
  targetRoomId: number;
  targetSpawnPos: Vec2;
  directionLabel: string;
  requiredSwitchId?: number;
}

export interface InteractiveMechanism {
  id: number;
  position: Vec2;
  activated: boolean;
  label: string;
  unlocksDoorId: number;
}

export interface LoreCollectible {
  id: number;
  position: Vec2;
  collected: boolean;
  title: string;
  excerpt: string;
}

export interface RoomDefinition {
  id: number;
  name: string;
  sectorTitle: string;
  acousticHint: string;
  playerDefaultSpawn: Vec2;
  walls: WallSegment[];
  portals: RoomPortal[];
  mechanisms: InteractiveMechanism[];
  collectibles: LoreCollectible[];
  enemyPatrolNodes: Vec2[];
  isCheckpoint: boolean;
  isExitRoom: boolean;
}

export interface AcousticConfig {
  waveSpeedMultiplier: number;
  maxReflections: number;
  rayDensityMultiplier: number;
  echoPersistenceMultiplier: number;
  ambientVisibility: number; // 0 = pure darkness, up to 0.25 faint debug outline
  audioEnabled: boolean;
}
