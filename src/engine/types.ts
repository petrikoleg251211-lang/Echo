export type SurfaceMaterial =
  | 'Stone'
  | 'Metal'
  | 'Water'
  | 'DampMoss'
  | 'ResonanceDoor'
  | 'EchoCrystal';

export type BiomeId =
  | 'DrownedGalleries'
  | 'CrystalVaults'
  | 'IronFoundry'
  | 'LivingMycelium';

export interface BiomeSpec {
  id: BiomeId;
  name: string;
  subtitle: string;
  acousticRule: string;
  accentHex: string;
}

export const BIOME_SPECS: Record<BiomeId, BiomeSpec> = {
  DrownedGalleries: {
    id: 'DrownedGalleries',
    name: 'THE DROWNED GALLERIES',
    subtitle: 'Biome I (Rooms 1-8) · Hydro-Acoustic Conduction',
    acousticRule: 'Footsteps muffled (-25% noise), Water waves travel +35% farther',
    accentHex: '#2dd4bf',
  },
  CrystalVaults: {
    id: 'CrystalVaults',
    name: 'THE CRYSTAL VAULTS',
    subtitle: 'Biome II (Rooms 9-16) · Prismatic Refraction & Room 16 Mini-Boss',
    acousticRule: 'EchoCrystal splits 1 ray into 3 chaotic prism rays; shattering glass spikes +24 noise',
    accentHex: '#e879f9',
  },
  IronFoundry: {
    id: 'IronFoundry',
    name: 'THE IRON FOUNDRY',
    subtitle: 'Biome III (Rooms 17-24) · Industrial Steam-Hammer Masking',
    acousticRule: '94% metal reflection; periodic Steam-Hammer Press masks footsteps (-75% noise for 1.35s)',
    accentHex: '#f59e0b',
  },
  LivingMycelium: {
    id: 'LivingMycelium',
    name: 'THE LIVING MYCELIUM',
    subtitle: 'Biome IV (Rooms 25-32) · Bioluminescent Spore Absorption',
    acousticRule: 'Organic walls absorb sound (1.55x faster silence decay); spores softly reveal moving hunters',
    accentHex: '#4ade80',
  },
};

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
    acousticNote: '64% reflectivity · Teal ripple scatter (+35% range)',
  },
  DampMoss: {
    reflectivity: 0.35,
    persistenceSec: 1.6,
    rgb: [74, 222, 128],
    hex: '#4ade80',
    label: 'Damp Moss / Mycelium',
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
  EchoCrystal: {
    reflectivity: 0.96,
    persistenceSec: 5.8,
    rgb: [232, 121, 249],
    hex: '#e879f9',
    label: 'Echo Crystal Prism',
    acousticNote: '96% reflectivity · Splits 1 ray into 3 chaotic prism rays (+18% energy)',
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
  crystalSplitApplied?: boolean;
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
  biome: BiomeId;
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
