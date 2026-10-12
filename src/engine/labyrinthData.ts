import { RoomDefinition, SurfaceMaterial, WallSegment } from './types';

function addBox(
  walls: WallSegment[],
  x: number,
  y: number,
  w: number,
  h: number,
  material: SurfaceMaterial
): void {
  walls.push(
    { a: { x, y }, b: { x: x + w, y }, material },
    { a: { x: x + w, y }, b: { x: x + w, y: y + h }, material },
    { a: { x: x + w, y: y + h }, b: { x, y: y + h }, material },
    { a: { x, y: y + h }, b: { x, y }, material }
  );
}

function addRoomPerimeter(
  walls: WallSegment[],
  material: SurfaceMaterial,
  hasWestDoor: boolean,
  hasEastDoor: boolean
): void {
  const left = 60;
  const top = 50;
  const right = 940;
  const bottom = 550;

  // Top & Bottom walls
  walls.push({ a: { x: left, y: top }, b: { x: right, y: top }, material });
  walls.push({ a: { x: right, y: bottom }, b: { x: left, y: bottom }, material });

  // West wall (with optional portal alcove at y = 250..350)
  if (hasWestDoor) {
    walls.push({ a: { x: left, y: top }, b: { x: left, y: 245 }, material });
    walls.push({ a: { x: left, y: 355 }, b: { x: left, y: bottom }, material });
  } else {
    walls.push({ a: { x: left, y: bottom }, b: { x: left, y: top }, material });
  }

  // East wall (with optional portal alcove at y = 250..350)
  if (hasEastDoor) {
    walls.push({ a: { x: right, y: top }, b: { x: right, y: 245 }, material });
    walls.push({ a: { x: right, y: 355 }, b: { x: right, y: bottom }, material });
  } else {
    walls.push({ a: { x: right, y: top }, b: { x: right, y: bottom }, material });
  }
}

interface Blueprint {
  name: string;
  sector: string;
  hint: string;
  primaryMat: SurfaceMaterial;
  secondaryMat: SurfaceMaterial;
  hasEnemy: boolean;
  isCheckpoint: boolean;
  layoutVariant: number;
  lore?: { title: string; excerpt: string };
}

const BLUEPRINTS: Blueprint[] = [
  {
    name: '01. Entry Cistern',
    sector: 'Biome I · The Drowned Galleries',
    hint: 'Click (or press E) to cast a Directional Sonar Ping. Press Space for a 360° Radial Pulse. Water waves carry +35% farther.',
    primaryMat: 'Stone',
    secondaryMat: 'Water',
    hasEnemy: false,
    isCheckpoint: true,
    layoutVariant: 0,
    lore: {
      title: 'Acoustic Log #01 — Calibration',
      excerpt: 'Conventional lamps fail in the heavy vapor below. Only tuned acoustic impulses reveal the geometry of the cistern walls.',
    },
  },
  {
    name: '02. Echoing Gallery',
    sector: 'Biome I · The Drowned Galleries',
    hint: 'Subterranean water channels conduct sonar waves 35% farther while muffling footstep splashes (-25% noise).',
    primaryMat: 'Water',
    secondaryMat: 'Stone',
    hasEnemy: false,
    isCheckpoint: false,
    layoutVariant: 1,
  },
  {
    name: '03. Flooded Sluice',
    sector: 'Biome I · The Drowned Galleries',
    hint: 'A blind Listener patrols the far bank. Activate the Harmonic Lever (F) to open the violet Resonance Gate on the east wall.',
    primaryMat: 'Water',
    secondaryMat: 'Stone',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 2,
    lore: {
      title: 'Acoustic Log #02 — The Listeners & Leeches',
      excerpt: 'In the flooded galleries, Acoustic Leeches latch onto careless surveyors, draining stamina until shaken loose by a Sprint or Radial Pulse.',
    },
  },
  {
    name: '04. Prismatic Vestibule',
    sector: 'Biome II · The Crystal Vaults',
    hint: 'Magenta EchoCrystal pillars split 1 sonar ray into 3 chaotic prism rays (+18% energy) to illuminate blind corners.',
    primaryMat: 'Stone',
    secondaryMat: 'EchoCrystal',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 1,
  },
  {
    name: '05. Crystal Choir Vault',
    sector: 'Biome II · The Crystal Vaults',
    hint: 'EchoCrystal monoliths refract sonar pulses across the entire vault. Watch out for Banshee wails! Checkpoint reached.',
    primaryMat: 'EchoCrystal',
    secondaryMat: 'Stone',
    hasEnemy: false,
    isCheckpoint: true,
    layoutVariant: 0,
    lore: {
      title: 'Acoustic Log #03 — Prismatic Refraction',
      excerpt: 'EchoCrystal facets split any incident acoustic beam into three harmonic prism rays, lighting up the dark—and alerting Banshees.',
    },
  },
  {
    name: '06. Refraction Sanctum',
    sector: 'Biome II · The Crystal Vaults',
    hint: 'Right-Click (or press G) to throw a Decoy Stone into the northern alcove, luring the hunter away from the Resonance Lever.',
    primaryMat: 'Stone',
    secondaryMat: 'EchoCrystal',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 2,
  },
  {
    name: '07. Conductor’s Amphitheater (Mini-Boss)',
    sector: 'Biome II · The Crystal Vaults (Chamber 16)',
    hint: 'Elite Chamber: Angled EchoCrystal mirrors create multi-bounce reflections needed to break Tuning Fork & Conductor shields.',
    primaryMat: 'EchoCrystal',
    secondaryMat: 'Metal',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 1,
    lore: {
      title: 'Acoustic Log #04 — The Conductor',
      excerpt: 'In the sixteenth chamber stands The Conductor, sweeping a four-beam rotating acoustic cross across the crystal amphitheater.',
    },
  },
  {
    name: '08. Steam-Hammer Press Hall',
    sector: 'Biome III · The Iron Foundry',
    hint: 'Forged iron reflects 94% of wave energy. In the native C++ Foundry, periodic Steam-Hammer clangs mask footsteps by -75%.',
    primaryMat: 'Metal',
    secondaryMat: 'Metal',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 3,
  },
  {
    name: '09. Slag Sluiceway',
    sector: 'Biome III · The Iron Foundry',
    hint: 'Unlock the east foundry lock via the Harmonic Lever (F) while evading the patrolling Warden. Checkpoint active.',
    primaryMat: 'Metal',
    secondaryMat: 'Water',
    hasEnemy: true,
    isCheckpoint: true,
    layoutVariant: 2,
    lore: {
      title: 'Acoustic Log #05 — Foundry Pistons',
      excerpt: 'Every 5.5 seconds the colossal steam-hammers strike the anvil floor, masking a surveyor’s sprint from nearby Wardens.',
    },
  },
  {
    name: '10. Tuning Fork Garrison',
    sector: 'Biome III · The Iron Foundry',
    hint: 'Stationary Tuning Fork Constructs block direct pings—bounce sonar off angled iron walls to deal 2.25x reflected damage.',
    primaryMat: 'Metal',
    secondaryMat: 'Stone',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 1,
  },
  {
    name: '11. Rust Bulkhead',
    sector: 'Biome III · The Iron Foundry',
    hint: 'Corroded iron bulkheads amplify sprinting footsteps. Use Sneak stance (Shift) and Decoy Stones (Right-Click).',
    primaryMat: 'Metal',
    secondaryMat: 'DampMoss',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 3,
    lore: {
      title: 'Acoustic Log #06 — Bulkhead Resonance',
      excerpt: 'One careless sprint on unmasked iron grating alerts every entity within sixty meters.',
    },
  },
  {
    name: '12. Spore Threshold',
    sector: 'Biome IV · The Living Mycelium',
    hint: 'Organic mycelium walls absorb sound (1.55x faster silence decay). Activate the lever (F) to open the eastern gate.',
    primaryMat: 'DampMoss',
    secondaryMat: 'Stone',
    hasEnemy: true,
    isCheckpoint: true,
    layoutVariant: 2,
  },
  {
    name: '13. Bioluminescent Grove',
    sector: 'Biome IV · The Living Mycelium',
    hint: 'Mycelial mats swallow echoes rapidly, but glowing spores softly reveal moving predators in the dark.',
    primaryMat: 'DampMoss',
    secondaryMat: 'EchoCrystal',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 0,
    lore: {
      title: 'Acoustic Log #07 — The Living Mycelium',
      excerpt: 'The fungal carpet drinks sound like dry earth drinks rain, yet its bioluminescent spores flare whenever a predator stalks across them.',
    },
  },
  {
    name: '14. Sovereign Antechamber',
    sector: 'Biome IV · The Living Mycelium',
    hint: 'Lure the sanctum guardian away from the central harmonic lever (F) to unseal the Ascent Gate.',
    primaryMat: 'DampMoss',
    secondaryMat: 'EchoCrystal',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 2,
  },
  {
    name: '15. Ascent Gate',
    sector: 'Biome IV · Surface Conduit',
    hint: 'Emit a final pulse to illuminate the Surface Lift on the eastern platform and complete your escape.',
    primaryMat: 'EchoCrystal',
    secondaryMat: 'Metal',
    hasEnemy: false,
    isCheckpoint: true,
    layoutVariant: 1,
    lore: {
      title: 'Acoustic Log #08 — Emergence',
      excerpt: 'Sunlight filters down the ventilation shaft. The labyrinth falls silent behind you.',
    },
  },
];

export function createLabyrinthRooms(): RoomDefinition[] {
  return BLUEPRINTS.map((bp, i) => {
    const walls: WallSegment[] = [];
    const hasWest = i > 0;
    const hasEast = true; // Room 14 east portal is the Escape Lift
    const biome =
      i < 3
        ? 'DrownedGalleries'
        : i < 7
        ? 'CrystalVaults'
        : i < 11
        ? 'IronFoundry'
        : 'LivingMycelium';

    addRoomPerimeter(walls, bp.primaryMat, hasWest, hasEast);

    if (bp.layoutVariant === 0) {
      addBox(walls, 250, 130, 95, 140, bp.secondaryMat);
      addBox(walls, 250, 340, 95, 130, bp.primaryMat);
      addBox(walls, 520, 210, 110, 180, bp.secondaryMat);
      addBox(walls, 730, 120, 80, 130, bp.primaryMat);
      addBox(walls, 730, 350, 80, 130, bp.secondaryMat);
    } else if (bp.layoutVariant === 1) {
      addBox(walls, 290, 180, 90, 240, bp.secondaryMat);
      addBox(walls, 580, 110, 100, 150, bp.primaryMat);
      addBox(walls, 580, 340, 100, 150, bp.primaryMat);
      // Angled acoustic mirrors (EchoCrystal in CrystalVaults, Metal elsewhere)
      const mirrorMat: SurfaceMaterial = biome === 'CrystalVaults' ? 'EchoCrystal' : 'Metal';
      walls.push(
        { a: { x: 430, y: 120 }, b: { x: 520, y: 200 }, material: mirrorMat },
        { a: { x: 430, y: 480 }, b: { x: 520, y: 400 }, material: mirrorMat },
        { a: { x: 750, y: 200 }, b: { x: 830, y: 280 }, material: mirrorMat }
      );
    } else if (bp.layoutVariant === 2) {
      addBox(walls, 260, 140, 110, 140, bp.secondaryMat);
      addBox(walls, 480, 260, 120, 170, bp.primaryMat);
      addBox(walls, 700, 130, 95, 160, bp.secondaryMat);
      walls.push({
        a: { x: 380, y: 430 },
        b: { x: 450, y: 360 },
        material: biome === 'CrystalVaults' ? 'EchoCrystal' : 'Metal',
      });
    } else {
      addBox(walls, 230, 100, 85, 220, bp.secondaryMat);
      addBox(walls, 430, 270, 90, 220, bp.primaryMat);
      addBox(walls, 630, 100, 90, 220, bp.secondaryMat);
      addBox(walls, 790, 310, 70, 170, bp.primaryMat);
    }

    const mechanisms = [];
    let requiredSwitchId: number | undefined;

    // Rooms with layoutVariant === 2 have a locked Resonance Gate & Harmonic Lever
    if (bp.layoutVariant === 2) {
      requiredSwitchId = 100 + i;
      mechanisms.push({
        id: requiredSwitchId,
        position: { x: 540, y: 155 },
        activated: false,
        label: 'Harmonic Conduit Lever',
        unlocksDoorId: requiredSwitchId,
      });
      walls.push({
        a: { x: 895, y: 235 },
        b: { x: 895, y: 365 },
        material: 'ResonanceDoor',
        doorId: requiredSwitchId,
        isOpen: false,
      });
    }

    const portals = [];
    if (hasWest) {
      portals.push({
        position: { x: 78, y: 300 },
        radius: 26,
        targetRoomId: i - 1,
        targetSpawnPos: { x: 855, y: 300 },
        directionLabel: `West to Room ${String(i).padStart(2, '0')}`,
      });
    }

    portals.push({
      position: { x: 922, y: 300 },
      radius: 26,
      targetRoomId: i < 14 ? i + 1 : 14,
      targetSpawnPos: { x: 135, y: 300 },
      directionLabel: i < 14 ? `East to Room ${String(i + 2).padStart(2, '0')}` : 'Surface Ascent Lift',
      requiredSwitchId,
    });

    const collectibles = bp.lore
      ? [
          {
            id: 200 + i,
            position: { x: 410, y: 475 },
            collected: false,
            title: bp.lore.title,
            excerpt: bp.lore.excerpt,
          },
        ]
      : [];

    const enemyPatrolNodes = bp.hasEnemy
      ? [
          { x: 640, y: 185 },
          { x: 835, y: 185 },
          { x: 835, y: 465 },
          { x: 425, y: 475 },
        ]
      : [];

    return {
      id: i,
      name: bp.name,
      sectorTitle: bp.sector,
      biome,
      acousticHint: bp.hint,
      playerDefaultSpawn: { x: 135, y: 300 },
      walls,
      portals,
      mechanisms,
      collectibles,
      enemyPatrolNodes,
      isCheckpoint: bp.isCheckpoint,
      isExitRoom: i === 14,
    };
  });
}

