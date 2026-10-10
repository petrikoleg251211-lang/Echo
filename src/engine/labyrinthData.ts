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
    sector: 'Upper Drainage',
    hint: 'Click (or press E) to cast a Directional Sonar Ping. Press Space for a 360° Radial Pulse. Reach the East Conduit.',
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
    sector: 'Upper Drainage',
    hint: 'Forged iron baffles reflect 94% of wave energy in amber trajectories. Bounce pings off angled plates to see around corners.',
    primaryMat: 'Stone',
    secondaryMat: 'Metal',
    hasEnemy: false,
    isCheckpoint: false,
    layoutVariant: 1,
  },
  {
    name: '03. Flooded Sluice',
    sector: 'Aqueduct Sector',
    hint: 'A blind Listener patrols the far bank. Activate the Harmonic Lever (F) to open the violet Resonance Gate on the east wall.',
    primaryMat: 'Water',
    secondaryMat: 'Stone',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 2,
    lore: {
      title: 'Acoustic Log #02 — The Listeners',
      excerpt: 'They have no eyes. They triangulate the origin of high-amplitude vibrations. Hold Shift to sneak when crossing open stone.',
    },
  },
  {
    name: '04. Iron Foundry',
    sector: 'Industrial Core',
    hint: 'High metallic reflection carries sound across the room. Hold Shift to keep footstep intensity below the Listener threshold.',
    primaryMat: 'Metal',
    secondaryMat: 'Metal',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 3,
  },
  {
    name: '05. Whispering Ossuary',
    sector: 'Catacomb Tier',
    hint: 'Damp moss absorbs 65% of acoustic energy (green traces fade quickly). Checkpoint reached.',
    primaryMat: 'DampMoss',
    secondaryMat: 'Stone',
    hasEnemy: false,
    isCheckpoint: true,
    layoutVariant: 0,
    lore: {
      title: 'Acoustic Log #03 — Moss Dampening',
      excerpt: 'Where subterranean moss coats the crypts, echoes die almost immediately—blinding the surveyor, yet muffling footsteps from predators.',
    },
  },
  {
    name: '06. Collapsed Vault',
    sector: 'Catacomb Tier',
    hint: 'Right-Click (or press G) to throw a Decoy Stone into the northern alcove, luring the Listener away from the Resonance Lever.',
    primaryMat: 'Stone',
    secondaryMat: 'DampMoss',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 2,
  },
  {
    name: '07. Resonance Chamber',
    sector: 'Acoustic Sanctum',
    hint: 'Angled metallic mirrors create triple-bounce reflections that illuminate the entire chamber from a single directional ping.',
    primaryMat: 'Metal',
    secondaryMat: 'Stone',
    hasEnemy: false,
    isCheckpoint: false,
    layoutVariant: 1,
    lore: {
      title: 'Acoustic Log #04 — Harmonic Gates',
      excerpt: 'The purple Resonance Gates only retract when their paired tuning lever vibrates at 440 Hz.',
    },
  },
  {
    name: '08. Warden Watchpost',
    sector: 'Security Ring',
    hint: 'Narrow stone chicanes restrict movement. Scout the patrol cadence with quiet directional pings before advancing.',
    primaryMat: 'Stone',
    secondaryMat: 'Metal',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 3,
  },
  {
    name: '09. Subterranean Canal',
    sector: 'Aqueduct Sector',
    hint: 'Unlock the east canal lock via the Harmonic Lever (F) while evading the patrolling Listener. Checkpoint active.',
    primaryMat: 'Water',
    secondaryMat: 'Metal',
    hasEnemy: true,
    isCheckpoint: true,
    layoutVariant: 2,
    lore: {
      title: 'Acoustic Log #05 — Hydro-Acoustics',
      excerpt: 'Water channels scatter wavefronts at 64% intensity, revealing the bridge pilings in shimmering teal.',
    },
  },
  {
    name: '10. Ritual Atrium',
    sector: 'Deep Vaults',
    hint: 'Concentric stone monoliths form an acoustic shadow in the center of the atrium.',
    primaryMat: 'Stone',
    secondaryMat: 'Stone',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 1,
  },
  {
    name: '11. Rust Corridor',
    sector: 'Industrial Core',
    hint: 'Corroded iron bulkheads amplify sprinting footsteps. Use Sneak stance (Shift) and Decoy Stones (Right-Click).',
    primaryMat: 'Metal',
    secondaryMat: 'DampMoss',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 3,
    lore: {
      title: 'Acoustic Log #06 — Bulkhead Resonance',
      excerpt: 'One careless sprint on iron grating alerts every entity within sixty meters.',
    },
  },
  {
    name: '12. Generator Hall',
    sector: 'Industrial Core',
    hint: 'Activate the turbine bypass lever (F) to open the eastern blast door. Checkpoint active.',
    primaryMat: 'Metal',
    secondaryMat: 'Metal',
    hasEnemy: true,
    isCheckpoint: true,
    layoutVariant: 2,
  },
  {
    name: '13. Silent Archive',
    sector: 'Deep Vaults',
    hint: 'Moss-covered archive stacks deaden echoes. Navigate the aisles carefully.',
    primaryMat: 'DampMoss',
    secondaryMat: 'Stone',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 0,
    lore: {
      title: 'Acoustic Log #07 — The Final Shaft',
      excerpt: 'Two chambers remain before the surface lift. Keep at least one decoy stone for the Warden Sanctum.',
    },
  },
  {
    name: '14. Warden Sanctum',
    sector: 'Abyssal Threshold',
    hint: 'Lure the sanctum guardian away from the central harmonic lever (F) to unseal the Ascent Gate.',
    primaryMat: 'Stone',
    secondaryMat: 'Metal',
    hasEnemy: true,
    isCheckpoint: false,
    layoutVariant: 2,
  },
  {
    name: '15. Ascent Gate',
    sector: 'Surface Conduit',
    hint: 'Emit a final pulse to illuminate the Surface Lift on the eastern platform and complete your escape.',
    primaryMat: 'Metal',
    secondaryMat: 'Stone',
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
      // Angled acoustic mirrors
      walls.push(
        { a: { x: 430, y: 120 }, b: { x: 520, y: 200 }, material: 'Metal' },
        { a: { x: 430, y: 480 }, b: { x: 520, y: 400 }, material: 'Metal' },
        { a: { x: 750, y: 200 }, b: { x: 830, y: 280 }, material: 'Metal' }
      );
    } else if (bp.layoutVariant === 2) {
      addBox(walls, 260, 140, 110, 140, bp.secondaryMat);
      addBox(walls, 480, 260, 120, 170, bp.primaryMat);
      addBox(walls, 700, 130, 95, 160, bp.secondaryMat);
      walls.push({ a: { x: 380, y: 430 }, b: { x: 450, y: 360 }, material: 'Metal' });
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
