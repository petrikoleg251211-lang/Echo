import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  Copy,
  Check,
  Download,
  Volume2,
  VolumeX,
  RotateCcw,
  Play,
  Pause,
  Compass,
  Sliders,
  FileCode,
  ShieldAlert,
  Music,
} from 'lucide-react';
import {
  AcousticConfig,
  AcousticEvent,
  AcousticRay,
  EnemyState,
  GameState,
  MATERIAL_SPECS,
  MovementStance,
  RoomDefinition,
  SurfaceMaterial,
  Vec2,
  WallEchoTrace,
  WallSegment,
  WaveSignature,
} from './engine/types';
import { createLabyrinthRooms } from './engine/labyrinthData';
import { OST_TRACKS, type SoundtrackTrackId, soundSynth } from './engine/soundSynthesizer';
import { CPP_SOURCE_FILES } from './engine/cppSourceBundle';
import { downloadCppProjectZip } from './engine/zipBuilder';

type ActiveTab = 'play' | 'lab' | 'cpp' | 'audit';

const CANVAS_W = 1000;
const CANVAS_H = 600;
const MAX_RAYS = 1800;
const MAX_ECHOES = 1400;

function cross2D(u: Vec2, v: Vec2): number {
  return u.x * v.y - u.y * v.x;
}

function dot2D(u: Vec2, v: Vec2): number {
  return u.x * v.x + u.y * v.y;
}

function dist2D(a: Vec2, b: Vec2): number {
  return Math.hypot(b.x - a.x, b.y - a.y);
}

function normalize2D(v: Vec2): Vec2 {
  const len = Math.hypot(v.x, v.y);
  if (len < 1e-6) return { x: 0, y: 0 };
  return { x: v.x / len, y: v.y / len };
}

function closestPointOnSegment(p: Vec2, a: Vec2, b: Vec2): Vec2 {
  const abx = b.x - a.x;
  const aby = b.y - a.y;
  const lenSq = abx * abx + aby * aby;
  if (lenSq < 1e-6) return a;
  const t = Math.max(0, Math.min(1, ((p.x - a.x) * abx + (p.y - a.y) * aby) / lenSq));
  return { x: a.x + abx * t, y: a.y + aby * t };
}

function intersectSegments(
  p: Vec2,
  p2: Vec2,
  q: Vec2,
  q2: Vec2
): { t: number; point: Vec2 } | null {
  const r = { x: p2.x - p.x, y: p2.y - p.y };
  const s = { x: q2.x - q.x, y: q2.y - q.y };
  const rxs = cross2D(r, s);
  if (Math.abs(rxs) < 1e-6) return null;
  const qp = { x: q.x - p.x, y: q.y - p.y };
  const t = cross2D(qp, s) / rxs;
  const u = cross2D(qp, r) / rxs;
  if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
    return {
      t,
      point: { x: p.x + r.x * t, y: p.y + r.y * t },
    };
  }
  return null;
}

function signatureRgb(sig: WaveSignature): [number, number, number] {
  switch (sig) {
    case 'FootstepSneak':
      return [148, 163, 184];
    case 'FootstepWalk':
      return [56, 189, 248];
    case 'FootstepSprint':
      return [251, 191, 36];
    case 'DirectionalPing':
      return [56, 189, 248];
    case 'RadialPulse':
      return [125, 211, 252];
    case 'DecoyImpact':
      return [245, 158, 11];
    case 'MechanismClunk':
      return [167, 139, 250];
    case 'EnemyTremor':
      return [239, 68, 68];
  }
}

export default function App() {
  const [activeTab, setActiveTab] = useState<ActiveTab>('play');
  const [gameState, setGameState] = useState<GameState>('PLAYING');
  const [rooms, setRooms] = useState<RoomDefinition[]>(() => createLabyrinthRooms());
  const [currentRoomId, setCurrentRoomId] = useState<number>(0);
  const [visitedRooms, setVisitedRooms] = useState<number[]>([0]);
  const [activatedSwitches, setActivatedSwitches] = useState<number[]>([]);
  const [collectedLore, setCollectedLore] = useState<number[]>([]);
  const [decoyCount, setDecoyCount] = useState<number>(4);
  const [statusToast, setStatusToast] = useState<string>('Room 01 initialized. Emit a sound pulse (Click or Space) to reveal surrounding walls.');
  const [selectedCppPath, setSelectedCppPath] = useState<string>(CPP_SOURCE_FILES[3].path);
  const [copiedPath, setCopiedPath] = useState<string | null>(null);

  const [acousticConfig, setAcousticConfig] = useState<AcousticConfig>({
    waveSpeedMultiplier: 1.0,
    maxReflections: 3,
    rayDensityMultiplier: 1.0,
    echoPersistenceMultiplier: 1.0,
    ambientVisibility: 0.04,
    audioEnabled: true,
  });

  const [ostMode, setOstMode] = useState<'auto' | SoundtrackTrackId>('auto');
  const [ostPlaying, setOstPlaying] = useState<boolean>(false);
  const [ostVolume, setOstVolume] = useState<number>(0.78);

  // Live HUD state synced at ~15Hz for crisp UI without React render overhead
  const [hudMetrics, setHudMetrics] = useState({
    activeRays: 0,
    activeEchoes: 0,
    stamina: 100,
    stance: 'Walk' as MovementStance,
    enemyState: 'Idle' as EnemyState,
    enemySuspicion: 0,
    pingCooldown: 0,
    pulseCooldown: 0,
    playerX: 135,
    playerY: 300,
  });

  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const keysPressedRef = useRef<Set<string>>(new Set());
  const mouseWorldRef = useRef<Vec2>({ x: 500, y: 300 });

  // Mutable simulation state inside ref for deterministic 60Hz game loop
  const simRef = useRef({
    playerPos: { x: 135, y: 300 } as Vec2,
    facingAngle: 0,
    stamina: 100,
    stepAccum: 0,
    cooldownPing: 0,
    cooldownPulse: 0,
    cooldownDecoy: 0,
    rays: Array.from({ length: MAX_RAYS }, (): AcousticRay => ({
      pos: { x: 0, y: 0 },
      prevPos: { x: 0, y: 0 },
      dir: { x: 1, y: 0 },
      speed: 360,
      intensity: 0,
      maxDistance: 400,
      traveled: 0,
      reflections: 0,
      maxReflections: 3,
      signature: 'RadialPulse',
      rgb: [56, 189, 248],
      active: false,
    })),
    rayCursor: 0,
    echoes: Array.from({ length: MAX_ECHOES }, (): WallEchoTrace => ({
      hitPoint: { x: 0, y: 0 },
      tangent: { x: 1, y: 0 },
      halfLength: 12,
      intensity: 0,
      age: 0,
      lifetime: 3.5,
      material: 'Stone',
      active: false,
    })),
    echoCursor: 0,
    events: [] as AcousticEvent[],
    enemy: {
      active: false,
      pos: { x: 700, y: 200 } as Vec2,
      targetPos: { x: 700, y: 200 } as Vec2,
      waypoints: [] as Vec2[],
      waypointIdx: 0,
      state: 'Idle' as EnemyState,
      stateTimer: 0,
      suspicion: 0,
      tremorTimer: 0,
    },
    particles: Array.from({ length: 48 }, (_, idx) => ({
      x: (idx * 73) % CANVAS_W,
      y: (idx * 41) % CANVAS_H,
      vx: ((idx % 5) - 2) * 3.5,
      vy: ((idx % 3) - 1) * 3.0,
      size: (idx % 2) + 1,
    })),
  });

  const currentRoom = useMemo(
    () => rooms[currentRoomId] || rooms[0],
    [rooms, currentRoomId]
  );

  const emitPulse = useCallback(
    (
      origin: Vec2,
      baseSpeed: number,
      maxDistance: number,
      initialIntensity: number,
      baseRayCount: number,
      signature: WaveSignature,
      centerAngle = 0,
      spreadAngle = Math.PI * 2,
      emittedByPlayer = true
    ) => {
      const sim = simRef.current;
      const rayCount = Math.max(8, Math.round(baseRayCount * acousticConfig.rayDensityMultiplier));
      const speed = baseSpeed * acousticConfig.waveSpeedMultiplier;
      const fullCircle = spreadAngle >= Math.PI * 2 - 0.01;
      const startAngle = centerAngle - spreadAngle * 0.5;
      const step = fullCircle
        ? spreadAngle / rayCount
        : rayCount > 1
        ? spreadAngle / (rayCount - 1)
        : 0;
      const rgb = signatureRgb(signature);

      for (let i = 0; i < rayCount; i++) {
        const angle = startAngle + i * step;
        const ray = sim.rays[sim.rayCursor];
        sim.rayCursor = (sim.rayCursor + 1) % MAX_RAYS;

        ray.pos = { x: origin.x, y: origin.y };
        ray.prevPos = { x: origin.x, y: origin.y };
        ray.dir = { x: Math.cos(angle), y: Math.sin(angle) };
        ray.speed = speed;
        ray.intensity = initialIntensity;
        ray.maxDistance = maxDistance;
        ray.traveled = 0;
        ray.reflections = 0;
        ray.maxReflections = acousticConfig.maxReflections;
        ray.signature = signature;
        ray.rgb = [...rgb];
        ray.crystalSplitApplied = false;
        ray.active = true;
      }

      sim.events.push({
        origin: { x: origin.x, y: origin.y },
        loudness: initialIntensity,
        radius: maxDistance,
        signature,
        emittedByPlayer,
      });

      soundSynth.playWaveEmission(signature, acousticConfig.audioEnabled);
    },
    [acousticConfig]
  );

  const enterRoom = useCallback(
    (roomId: number, spawnPos?: Vec2) => {
      const target = rooms[roomId];
      if (!target) return;
      const sim = simRef.current;
      const spawn = spawnPos || target.playerDefaultSpawn;
      sim.playerPos = { x: spawn.x, y: spawn.y };
      sim.stamina = 100;
      sim.stepAccum = 0;

      for (const r of sim.rays) r.active = false;
      for (const e of sim.echoes) e.active = false;
      sim.events.length = 0;

      if (target.enemyPatrolNodes.length > 0) {
        sim.enemy.active = true;
        sim.enemy.waypoints = target.enemyPatrolNodes;
        sim.enemy.waypointIdx = 0;
        sim.enemy.pos = { ...target.enemyPatrolNodes[0] };
        sim.enemy.targetPos = { ...target.enemyPatrolNodes[0] };
        sim.enemy.state = 'Patrol';
        sim.enemy.suspicion = 0;
        sim.enemy.stateTimer = 0;
      } else {
        sim.enemy.active = false;
        sim.enemy.state = 'Idle';
        sim.enemy.suspicion = 0;
      }

      setCurrentRoomId(roomId);
      setVisitedRooms((prev) => (prev.includes(roomId) ? prev : [...prev, roomId]));
      setStatusToast(`${target.name} · ${target.acousticHint}`);

      // Initial entry pulse so player sees the doorway
      setTimeout(() => {
        emitPulse(spawn, 340, 380, 0.85, 84, 'RadialPulse', 0, Math.PI * 2, false);
      }, 30);
    },
    [rooms, emitPulse]
  );

  // Trigger initial pulse on mount
  useEffect(() => {
    emitPulse({ x: 135, y: 300 }, 340, 420, 0.9, 96, 'RadialPulse', 0, Math.PI * 2, false);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const handleSaveCheckpoint = useCallback(() => {
    const payload = {
      roomId: currentRoomId,
      visitedRooms,
      activatedSwitches,
      collectedLore,
      decoyCount,
    };
    localStorage.setItem('echolabyrinth_save_v1', JSON.stringify(payload));
    setStatusToast(`Checkpoint saved at ${currentRoom.name}.`);
  }, [currentRoomId, visitedRooms, activatedSwitches, collectedLore, decoyCount, currentRoom.name]);

  const handleLoadCheckpoint = useCallback(() => {
    const raw = localStorage.getItem('echolabyrinth_save_v1');
    if (!raw) {
      setStatusToast('No saved checkpoint found in storage.');
      return;
    }
    try {
      const parsed = JSON.parse(raw);
      if (typeof parsed.roomId === 'number') {
        setVisitedRooms(parsed.visitedRooms || [parsed.roomId]);
        setActivatedSwitches(parsed.activatedSwitches || []);
        setCollectedLore(parsed.collectedLore || []);
        setDecoyCount(typeof parsed.decoyCount === 'number' ? parsed.decoyCount : 3);
        setGameState('PLAYING');
        enterRoom(parsed.roomId);
        setStatusToast(`Loaded checkpoint at Room ${String(parsed.roomId + 1).padStart(2, '0')}.`);
      }
    } catch {
      setStatusToast('Checkpoint file corrupted.');
    }
  }, [enterRoom]);

  const triggerDirectionalPing = useCallback(() => {
    const sim = simRef.current;
    if (sim.cooldownPing > 0) return;
    sim.cooldownPing = 0.55;
    const angle = Math.atan2(
      mouseWorldRef.current.y - sim.playerPos.y,
      mouseWorldRef.current.x - sim.playerPos.x
    );
    sim.facingAngle = angle;
    emitPulse(
      sim.playerPos,
      430,
      540,
      0.92,
      56,
      'DirectionalPing',
      angle,
      Math.PI / 2.8,
      true
    );
  }, [emitPulse]);

  const triggerRadialPulse = useCallback(() => {
    const sim = simRef.current;
    if (sim.cooldownPulse > 0) return;
    sim.cooldownPulse = 2.2;
    emitPulse(
      sim.playerPos,
      400,
      680,
      1.2,
      136,
      'RadialPulse',
      0,
      Math.PI * 2,
      true
    );
  }, [emitPulse]);

  const triggerThrowDecoy = useCallback(
    (targetPt?: Vec2) => {
      const sim = simRef.current;
      if (decoyCount <= 0 || sim.cooldownDecoy > 0) {
        if (decoyCount <= 0) setStatusToast('No Decoy Stones remaining.');
        return;
      }
      sim.cooldownDecoy = 2.5;
      setDecoyCount((c) => Math.max(0, c - 1));
      const dest = targetPt || mouseWorldRef.current;
      emitPulse(dest, 390, 560, 1.35, 110, 'DecoyImpact', 0, Math.PI * 2, true);
      setStatusToast('Decoy Stone thrown — high-amplitude impact emitted at target.');
    },
    [decoyCount, emitPulse]
  );

  const triggerInteract = useCallback(() => {
    const sim = simRef.current;
    // Check mechanisms in current room
    let interacted = false;
    setRooms((prevRooms) =>
      prevRooms.map((r) => {
        if (r.id !== currentRoomId) return r;
        const nextMechs = r.mechanisms.map((m) => {
          if (!m.activated && dist2D(sim.playerPos, m.position) <= 56) {
            interacted = true;
            setActivatedSwitches((sw) => (sw.includes(m.id) ? sw : [...sw, m.id]));
            emitPulse(m.position, 410, 620, 1.25, 100, 'MechanismClunk', 0, Math.PI * 2, true);
            setStatusToast(`${m.label} engaged — Resonance Gate unsealed.`);
            return { ...m, activated: true };
          }
          return m;
        });
        const nextWalls = r.walls.map((w) => {
          if (w.doorId && nextMechs.some((m) => m.unlocksDoorId === w.doorId && m.activated)) {
            return { ...w, isOpen: true };
          }
          return w;
        });
        const nextLore = r.collectibles.map((c) => {
          if (!c.collected && dist2D(sim.playerPos, c.position) <= 52) {
            interacted = true;
            setCollectedLore((cl) => (cl.includes(c.id) ? cl : [...cl, c.id]));
            setDecoyCount((d) => d + 1);
            emitPulse(c.position, 320, 360, 0.7, 64, 'DirectionalPing', 0, Math.PI * 2, false);
            setStatusToast(`Collected ${c.title} (+1 Decoy Stone): "${c.excerpt}"`);
            return { ...c, collected: true };
          }
          return c;
        });
        return {
          ...r,
          mechanisms: nextMechs,
          walls: nextWalls,
          collectibles: nextLore,
        };
      })
    );
    if (!interacted) {
      setStatusToast('No harmonic lever or acoustic log within reach.');
    }
  }, [currentRoomId, emitPulse]);

  // Keyboard listeners
  useEffect(() => {
    const onKeyDown = (e: KeyboardEvent) => {
      const k = e.key.toLowerCase();
      keysPressedRef.current.add(k);

      if (activeTab !== 'play') return;

      if (e.key === ' ') {
        e.preventDefault();
        if (gameState === 'PLAYING') triggerRadialPulse();
      } else if (k === 'e') {
        if (gameState === 'PLAYING') triggerDirectionalPing();
      } else if (k === 'g') {
        if (gameState === 'PLAYING') triggerThrowDecoy();
      } else if (k === 'f') {
        if (gameState === 'PLAYING') triggerInteract();
      } else if (e.key === 'Escape') {
        setGameState((s) => (s === 'PLAYING' ? 'PAUSED' : s === 'PAUSED' ? 'PLAYING' : s));
      }
    };

    const onKeyUp = (e: KeyboardEvent) => {
      keysPressedRef.current.delete(e.key.toLowerCase());
    };

    window.addEventListener('keydown', onKeyDown);
    window.addEventListener('keyup', onKeyUp);
    return () => {
      window.removeEventListener('keydown', onKeyDown);
      window.removeEventListener('keyup', onKeyUp);
    };
  }, [activeTab, gameState, triggerRadialPulse, triggerDirectionalPing, triggerThrowDecoy, triggerInteract]);

  // Main 60Hz simulation & canvas rendering loop
  useEffect(() => {
    let animId = 0;
    let lastTime = performance.now();
    let hudSyncAccum = 0;

    const hasLineOfSight = (a: Vec2, b: Vec2, walls: WallSegment[]): boolean => {
      for (const w of walls) {
        if (w.isOpen) continue;
        if (intersectSegments(a, b, w.a, w.b)) return false;
      }
      return true;
    };

    const stepSimulation = (dt: number) => {
      const sim = simRef.current;
      const room = rooms[currentRoomId] || rooms[0];
      const walls = room.walls;

      sim.cooldownPing = Math.max(0, sim.cooldownPing - dt);
      sim.cooldownPulse = Math.max(0, sim.cooldownPulse - dt);
      sim.cooldownDecoy = Math.max(0, sim.cooldownDecoy - dt);

      // 1. Player input & movement
      const keys = keysPressedRef.current;
      let mx = 0;
      let my = 0;
      if (keys.has('w') || keys.has('arrowup')) my -= 1;
      if (keys.has('s') || keys.has('arrowdown')) my += 1;
      if (keys.has('a') || keys.has('arrowleft')) mx -= 1;
      if (keys.has('d') || keys.has('arrowright')) mx += 1;

      let stance: MovementStance = 'Walk';
      if (keys.has('shift')) stance = 'Sneak';
      else if (keys.has('control') || keys.has('c')) stance = 'Sprint';

      if (mx !== 0 || my !== 0) {
        const norm = normalize2D({ x: mx, y: my });
        sim.facingAngle = Math.atan2(norm.y, norm.x);

        if (stance === 'Sprint' && sim.stamina <= 5) {
          stance = 'Walk';
        }

        const speed = stance === 'Sneak' ? 84 : stance === 'Sprint' ? 230 : 148;
        sim.playerPos.x += norm.x * speed * dt;
        sim.playerPos.y += norm.y * speed * dt;

        // Circle-segment sliding collision resolution
        for (let iter = 0; iter < 3; iter++) {
          for (const w of walls) {
            if (w.isOpen) continue;
            const closest = closestPointOnSegment(sim.playerPos, w.a, w.b);
            const dx = sim.playerPos.x - closest.x;
            const dy = sim.playerPos.y - closest.y;
            const distSq = dx * dx + dy * dy;
            const minR = 13;
            if (distSq < minR * minR && distSq > 1e-6) {
              const d = Math.sqrt(distSq);
              sim.playerPos.x += (dx / d) * (minR - d);
              sim.playerPos.y += (dy / d) * (minR - d);
            }
          }
        }

        if (stance === 'Sprint') {
          sim.stamina = Math.max(0, sim.stamina - 24 * dt);
        } else {
          sim.stamina = Math.min(100, sim.stamina + 14 * dt);
        }

        sim.stepAccum += speed * dt;
        const stride = stance === 'Sneak' ? 54 : stance === 'Sprint' ? 62 : 56;
        if (sim.stepAccum >= stride) {
          sim.stepAccum = 0;
          if (stance === 'Sneak') {
            emitPulse(sim.playerPos, 220, 115, 0.25, 22, 'FootstepSneak', 0, Math.PI * 2, true);
          } else if (stance === 'Walk') {
            emitPulse(sim.playerPos, 290, 240, 0.55, 36, 'FootstepWalk', 0, Math.PI * 2, true);
          } else {
            emitPulse(sim.playerPos, 360, 430, 0.98, 60, 'FootstepSprint', 0, Math.PI * 2, true);
          }
        }
      } else {
        sim.stamina = Math.min(100, sim.stamina + 22 * dt);
      }

      // Auto-interact hint or collect lore when walking directly over it
      for (const lore of room.collectibles) {
        if (!lore.collected && dist2D(sim.playerPos, lore.position) <= 24) {
          triggerInteract();
          break;
        }
      }

      // Check portals
      for (const portal of room.portals) {
        const doorLocked =
          portal.requiredSwitchId !== undefined &&
          !activatedSwitches.includes(portal.requiredSwitchId);
        if (doorLocked) continue;

        if (dist2D(sim.playerPos, portal.position) <= portal.radius) {
          if (room.isExitRoom && portal.position.x > 500) {
            setGameState('VICTORY');
            setStatusToast('Ascent Complete — You escaped the Echo Labyrinth.');
            return;
          }
          enterRoom(portal.targetRoomId, portal.targetSpawnPos);
          return;
        }
      }

      // 2. Update Enemy AI (The Listener)
      const enemy = sim.enemy;
      if (enemy.active) {
        const dPlayer = dist2D(enemy.pos, sim.playerPos);
        if (dPlayer <= 22) {
          enemy.state = 'Attack';
          setGameState('GAME_OVER');
          setStatusToast('The Listener triangulated your position. Restart room or load checkpoint.');
          return;
        }

        for (const ev of sim.events) {
          if (!ev.emittedByPlayer) continue;
          const d = dist2D(enemy.pos, ev.origin);
          if (d > ev.radius * 1.15) continue;
          const occluded = !hasLineOfSight(enemy.pos, ev.origin, walls);
          const occFactor = occluded ? 0.52 : 1.0;
          const perceived = ev.loudness * occFactor * (1 - d / (ev.radius * 1.25));
          if (perceived > 0.13) {
            enemy.targetPos = { ...ev.origin };
            enemy.suspicion = Math.min(100, enemy.suspicion + perceived * 54);
            if (perceived > 0.62 || enemy.suspicion >= 72) {
              enemy.state = 'PursuePlayer';
              enemy.stateTimer = 4.8;
            } else if (enemy.state !== 'PursuePlayer') {
              enemy.state = 'InvestigateSound';
              enemy.stateTimer = 4.2;
            }
          }
        }

        enemy.suspicion = Math.max(0, enemy.suspicion - 6.5 * dt);

        const moveEnemyToward = (target: Vec2, spd: number) => {
          const delta = { x: target.x - enemy.pos.x, y: target.y - enemy.pos.y };
          const dist = Math.hypot(delta.x, delta.y);
          if (dist < 2) return;
          const dir = { x: delta.x / dist, y: delta.y / dist };
          enemy.pos.x += dir.x * spd * dt;
          enemy.pos.y += dir.y * spd * dt;

          // Keep enemy out of walls
          for (const w of walls) {
            if (w.isOpen) continue;
            const c = closestPointOnSegment(enemy.pos, w.a, w.b);
            const dx = enemy.pos.x - c.x;
            const dy = enemy.pos.y - c.y;
            const dSq = dx * dx + dy * dy;
            if (dSq < 14 * 14 && dSq > 1e-6) {
              const d = Math.sqrt(dSq);
              enemy.pos.x += (dx / d) * (14 - d);
              enemy.pos.y += (dy / d) * (14 - d);
            }
          }
        };

        switch (enemy.state) {
          case 'Patrol': {
            if (enemy.waypoints.length > 0) {
              const wp = enemy.waypoints[enemy.waypointIdx];
              moveEnemyToward(wp, 74);
              if (dist2D(enemy.pos, wp) < 16) {
                enemy.waypointIdx = (enemy.waypointIdx + 1) % enemy.waypoints.length;
              }
            }
            break;
          }
          case 'InvestigateSound': {
            moveEnemyToward(enemy.targetPos, 124);
            enemy.stateTimer -= dt;
            if (dist2D(enemy.pos, enemy.targetPos) < 18 || enemy.stateTimer <= 0) {
              enemy.state = 'SearchLastKnown';
              enemy.stateTimer = 3.2;
            }
            break;
          }
          case 'SearchLastKnown': {
            enemy.stateTimer -= dt;
            const orbit = {
              x: enemy.targetPos.x + Math.cos(enemy.stateTimer * 2.6) * 38,
              y: enemy.targetPos.y + Math.sin(enemy.stateTimer * 2.6) * 38,
            };
            moveEnemyToward(orbit, 90);
            if (enemy.stateTimer <= 0) {
              enemy.state = 'ReturnToPatrol';
            }
            break;
          }
          case 'PursuePlayer': {
            moveEnemyToward(enemy.targetPos, 172);
            enemy.stateTimer -= dt;
            if (dist2D(enemy.pos, enemy.targetPos) < 18 && dPlayer > 42) {
              enemy.state = 'SearchLastKnown';
              enemy.stateTimer = 3.4;
            } else if (enemy.stateTimer <= 0) {
              enemy.state = 'SearchLastKnown';
              enemy.stateTimer = 3.0;
            }
            break;
          }
          case 'ReturnToPatrol': {
            if (enemy.waypoints.length > 0) {
              const home = enemy.waypoints[enemy.waypointIdx];
              moveEnemyToward(home, 84);
              if (dist2D(enemy.pos, home) < 18) {
                enemy.state = 'Patrol';
              }
            }
            break;
          }
          default:
            break;
        }

        enemy.tremorTimer += dt;
        const interval = enemy.state === 'PursuePlayer' ? 0.44 : 0.88;
        if (enemy.tremorTimer >= interval) {
          enemy.tremorTimer = 0;
          emitPulse(enemy.pos, 250, 185, 0.55, 26, 'EnemyTremor', 0, Math.PI * 2, false);
        }
      }

      sim.events.length = 0;

      // 3. Update Acoustic Rays & Wall Echo Traces
      for (const ray of sim.rays) {
        if (!ray.active) continue;
        const stepDist = ray.speed * dt;
        ray.prevPos.x = ray.pos.x;
        ray.prevPos.y = ray.pos.y;
        const nextPos = {
          x: ray.pos.x + ray.dir.x * stepDist,
          y: ray.pos.y + ray.dir.y * stepDist,
        };

        let bestT = 2.0;
        let bestHit: Vec2 | null = null;
        let hitWall: WallSegment | null = null;

        for (const w of walls) {
          if (w.isOpen) continue;
          const hit = intersectSegments(ray.pos, nextPos, w.a, w.b);
          if (hit && hit.t < bestT) {
            bestT = hit.t;
            bestHit = hit.point;
            hitWall = w;
          }
        }

        if (hitWall && bestHit) {
          const wallVec = { x: hitWall.b.x - hitWall.a.x, y: hitWall.b.y - hitWall.a.y };
          const tangent = normalize2D(wallVec);
          let normal = { x: -tangent.y, y: tangent.x };
          if (dot2D(ray.dir, normal) > 0) {
            normal = { x: -normal.x, y: -normal.y };
          }

          const matSpec = MATERIAL_SPECS[hitWall.material];
          const trace = sim.echoes[sim.echoCursor];
          sim.echoCursor = (sim.echoCursor + 1) % MAX_ECHOES;
          trace.hitPoint = { x: bestHit.x, y: bestHit.y };
          trace.tangent = tangent;
          trace.halfLength = 9 + 8 * Math.min(1, ray.intensity);
          trace.intensity = Math.min(1, Math.max(0.12, ray.intensity));
          trace.age = 0;
          trace.lifetime = matSpec.persistenceSec * acousticConfig.echoPersistenceMultiplier;
          trace.material = hitWall.material;
          trace.active = true;

          ray.reflections += 1;
          ray.intensity *= matSpec.reflectivity;
          ray.rgb = [...matSpec.rgb];

          if (ray.reflections > ray.maxReflections || ray.intensity < 0.07) {
            ray.active = false;
            continue;
          }

          const dn = dot2D(ray.dir, normal);
          ray.dir = normalize2D({
            x: ray.dir.x - 2 * dn * normal.x,
            y: ray.dir.y - 2 * dn * normal.y,
          });
          const rem = stepDist * (1 - bestT);
          ray.pos = {
            x: bestHit.x + normal.x * 0.6 + ray.dir.x * rem,
            y: bestHit.y + normal.y * 0.6 + ray.dir.y * rem,
          };

          // Stage 2: EchoCrystal splits 1 incident ray into 3 chaotic prism rays (+18% energy)
          if (hitWall.material === 'EchoCrystal' && !ray.crystalSplitApplied) {
            ray.crystalSplitApplied = true;
            ray.intensity = Math.min(1.0, ray.intensity * 1.18);
            const baseAng = Math.atan2(ray.dir.y, ray.dir.x);
            const offsets = [-0.45, 0.45];
            for (const off of offsets) {
              const splitRay = sim.rays[sim.rayCursor];
              sim.rayCursor = (sim.rayCursor + 1) % MAX_RAYS;
              const splitAng = baseAng + off;
              splitRay.pos = { x: ray.pos.x, y: ray.pos.y };
              splitRay.prevPos = { x: bestHit.x, y: bestHit.y };
              splitRay.dir = { x: Math.cos(splitAng), y: Math.sin(splitAng) };
              splitRay.speed = ray.speed * 1.06;
              splitRay.intensity = ray.intensity * 0.86;
              splitRay.maxDistance = ray.maxDistance;
              splitRay.traveled = ray.traveled;
              splitRay.reflections = ray.reflections;
              splitRay.maxReflections = ray.maxReflections;
              splitRay.signature = ray.signature;
              splitRay.rgb = off < 0 ? [232, 121, 249] : [103, 232, 249];
              splitRay.crystalSplitApplied = true;
              splitRay.active = true;
            }
          }
        } else {
          ray.pos = nextPos;
        }

        ray.traveled += stepDist;
        if (ray.traveled >= ray.maxDistance || ray.intensity < 0.05) {
          ray.active = false;
        }
      }

      for (const echo of sim.echoes) {
        if (!echo.active) continue;
        echo.age += dt;
        if (echo.age >= echo.lifetime) {
          echo.active = false;
        }
      }

      // 4. Update atmospheric dust particles
      for (const p of sim.particles) {
        p.x = (p.x + p.vx * dt + CANVAS_W) % CANVAS_W;
        p.y = (p.y + p.vy * dt + CANVAS_H) % CANVAS_H;
      }
    };

    const renderCanvas = () => {
      const canvas = canvasRef.current;
      if (!canvas) return;
      const ctx = canvas.getContext('2d');
      if (!ctx) return;

      const sim = simRef.current;
      const room = rooms[currentRoomId] || rooms[0];

      // 1. Deep darkness fill
      ctx.globalCompositeOperation = 'source-over';
      ctx.fillStyle = '#06080c';
      ctx.fillRect(0, 0, CANVAS_W, CANVAS_H);

      // 2. Subtle ambient architectural outline (configurable in Acoustic Lab, default near-zero)
      if (acousticConfig.ambientVisibility > 0.005) {
        ctx.strokeStyle = `rgba(148, 163, 184, ${acousticConfig.ambientVisibility})`;
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        for (const w of room.walls) {
          if (w.isOpen) continue;
          ctx.moveTo(w.a.x, w.a.y);
          ctx.lineTo(w.b.x, w.b.y);
        }
        ctx.stroke();
      }

      // 3. Atmospheric suspended dust motes
      ctx.fillStyle = 'rgba(148, 163, 184, 0.12)';
      for (const p of sim.particles) {
        ctx.fillRect(p.x, p.y, p.size, p.size);
      }

      // 4. Additive blending for Wall Phosphor Echoes & Acoustic Wavefronts
      ctx.globalCompositeOperation = 'lighter';

      ctx.lineWidth = 3.2;
      for (const echo of sim.echoes) {
        if (!echo.active) continue;
        const lifeRatio = 1 - echo.age / echo.lifetime;
        const alpha = Math.max(0, Math.min(1, lifeRatio * lifeRatio * echo.intensity * 0.85));
        const [r, g, b] = MATERIAL_SPECS[echo.material].rgb;
        ctx.strokeStyle = `rgba(${r}, ${g}, ${b}, ${alpha.toFixed(3)})`;
        ctx.beginPath();
        ctx.moveTo(
          echo.hitPoint.x - echo.tangent.x * echo.halfLength,
          echo.hitPoint.y - echo.tangent.y * echo.halfLength
        );
        ctx.lineTo(
          echo.hitPoint.x + echo.tangent.x * echo.halfLength,
          echo.hitPoint.y + echo.tangent.y * echo.halfLength
        );
        ctx.stroke();
      }

      ctx.lineWidth = 2.0;
      for (const ray of sim.rays) {
        if (!ray.active) continue;
        const fade = 1 - ray.traveled / Math.max(1, ray.maxDistance);
        const alpha = Math.max(0, Math.min(1, fade * ray.intensity * 0.9));
        const [r, g, b] = ray.rgb;
        ctx.strokeStyle = `rgba(${r}, ${g}, ${b}, ${alpha.toFixed(3)})`;
        ctx.beginPath();
        ctx.moveTo(ray.pos.x - ray.dir.x * 13, ray.pos.y - ray.dir.y * 13);
        ctx.lineTo(ray.pos.x, ray.pos.y);
        ctx.stroke();
      }

      // 5. Render Portals, Harmonic Levers, and Collectibles
      ctx.globalCompositeOperation = 'source-over';
      for (const portal of room.portals) {
        const locked =
          portal.requiredSwitchId !== undefined &&
          !activatedSwitches.includes(portal.requiredSwitchId);
        ctx.strokeStyle = locked ? 'rgba(167, 139, 250, 0.35)' : 'rgba(56, 189, 248, 0.55)';
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.arc(portal.position.x, portal.position.y, portal.radius, 0, Math.PI * 2);
        ctx.stroke();

        ctx.fillStyle = locked ? 'rgba(167, 139, 250, 0.7)' : 'rgba(56, 189, 248, 0.85)';
        ctx.font = '500 11px "JetBrains Mono", monospace';
        ctx.textAlign = 'center';
        ctx.fillText(
          locked ? 'LOCKED GATE' : portal.directionLabel,
          portal.position.x,
          portal.position.y - portal.radius - 8
        );
      }

      for (const mech of room.mechanisms) {
        const dist = dist2D(sim.playerPos, mech.position);
        ctx.strokeStyle = mech.activated ? '#4ade80' : '#a78bfa';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(mech.position.x, mech.position.y, 11, 0, Math.PI * 2);
        ctx.stroke();

        if (dist <= 95 || mech.activated) {
          ctx.fillStyle = mech.activated ? '#4ade80' : '#c4b5fd';
          ctx.font = '500 11px "JetBrains Mono", monospace';
          ctx.textAlign = 'center';
          ctx.fillText(
            mech.activated ? 'LEVER ENGAGED' : '[F] HARMONIC LEVER',
            mech.position.x,
            mech.position.y - 16
          );
        }
      }

      for (const lore of room.collectibles) {
        if (lore.collected) continue;
        ctx.strokeStyle = 'rgba(245, 158, 11, 0.75)';
        ctx.lineWidth = 1.5;
        ctx.strokeRect(lore.position.x - 7, lore.position.y - 7, 14, 14);
        if (dist2D(sim.playerPos, lore.position) <= 110) {
          ctx.fillStyle = '#fbbf24';
          ctx.font = '500 11px "JetBrains Mono", monospace';
          ctx.textAlign = 'center';
          ctx.fillText('[F] ACOUSTIC LOG', lore.position.x, lore.position.y - 14);
        }
      }

      // 6. Render Hostile Entity (only visible when near or emitting crimson tremor)
      if (sim.enemy.active) {
        const enemyAlpha =
          sim.enemy.state === 'PursuePlayer'
            ? 0.9
            : sim.enemy.state === 'InvestigateSound'
            ? 0.65
            : 0.32;
        ctx.strokeStyle = `rgba(239, 68, 68, ${enemyAlpha})`;
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(sim.enemy.pos.x, sim.enemy.pos.y, 13, 0, Math.PI * 2);
        ctx.stroke();

        if (sim.enemy.state !== 'Patrol') {
          ctx.fillStyle = 'rgba(248, 113, 113, 0.85)';
          ctx.font = '500 10px "JetBrains Mono", monospace';
          ctx.textAlign = 'center';
          ctx.fillText(sim.enemy.state.toUpperCase(), sim.enemy.pos.x, sim.enemy.pos.y - 18);
        }
      }

      // 7. Render Player Avatar & Directional Acoustic Cone Indicator
      ctx.save();
      ctx.translate(sim.playerPos.x, sim.playerPos.y);
      ctx.rotate(sim.facingAngle);

      ctx.strokeStyle = 'rgba(56, 189, 248, 0.25)';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(0, 0);
      ctx.lineTo(28, -12);
      ctx.moveTo(0, 0);
      ctx.lineTo(28, 12);
      ctx.stroke();

      ctx.fillStyle = '#0f172a';
      ctx.strokeStyle = '#38bdf8';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(0, 0, 11, 0, Math.PI * 2);
      ctx.fill();
      ctx.stroke();
      ctx.restore();
    };

    const tick = (now: number) => {
      const dt = Math.min(0.05, (now - lastTime) / 1000);
      lastTime = now;

      if (activeTab === 'play' && gameState === 'PLAYING') {
        stepSimulation(dt);
      }
      if (activeTab === 'play') {
        renderCanvas();
      }

      hudSyncAccum += dt;
      if (hudSyncAccum >= 0.08) {
        hudSyncAccum = 0;
        const sim = simRef.current;
        let activeRays = 0;
        for (const r of sim.rays) if (r.active) activeRays++;
        let activeEchoes = 0;
        for (const e of sim.echoes) if (e.active) activeEchoes++;

        const keys = keysPressedRef.current;
        const stance: MovementStance = keys.has('shift')
          ? 'Sneak'
          : keys.has('control') || keys.has('c')
          ? 'Sprint'
          : 'Walk';

        const danger0to1 = Math.min(1, sim.enemy.suspicion / 100);
        const isPursuing = sim.enemy.active && sim.enemy.state === 'PursuePlayer';
        const isCheckpoint = currentRoomId === 4 || currentRoomId === 9;
        const isBoss = currentRoomId >= 13;
        if (ostPlaying && ostMode === 'auto') {
          soundSynth.updateAdaptiveMusic(
            danger0to1,
            isPursuing,
            isCheckpoint,
            isBoss,
            acousticConfig.audioEnabled
          );
        }

        setHudMetrics({
          activeRays,
          activeEchoes,
          stamina: Math.round(sim.stamina),
          stance,
          enemyState: sim.enemy.active ? sim.enemy.state : 'Idle',
          enemySuspicion: Math.round(sim.enemy.suspicion),
          pingCooldown: Number(sim.cooldownPing.toFixed(1)),
          pulseCooldown: Number(sim.cooldownPulse.toFixed(1)),
          playerX: Math.round(sim.playerPos.x),
          playerY: Math.round(sim.playerPos.y),
        });
      }

      animId = requestAnimationFrame(tick);
    };

    animId = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(animId);
  }, [activeTab, gameState, rooms, currentRoomId, activatedSwitches, enterRoom, emitPulse, triggerInteract, acousticConfig.ambientVisibility, acousticConfig.audioEnabled, ostPlaying, ostMode]);

  const handleSelectOstMode = (mode: 'auto' | SoundtrackTrackId) => {
    setOstMode(mode);
    setOstPlaying(true);
    soundSynth.setMusicMode(mode, true);
    if (mode === 'auto') {
      setStatusToast('OST switched to Auto-Adaptive Stem Mixing (Exploration / Tension / Pursuit / Sanctum / Boss).');
    } else {
      const meta = OST_TRACKS.find((t) => t.id === mode);
      if (meta) {
        setStatusToast(`Playing OST Track: ${meta.title} (${meta.bpm} BPM · ${meta.keySignature})`);
      }
    }
  };

  const handleToggleOstPlayback = () => {
    if (ostPlaying) {
      soundSynth.stopMusic();
      setOstPlaying(false);
      setStatusToast('Soundtrack playback paused.');
    } else {
      setOstPlaying(true);
      soundSynth.setMusicMode(ostMode, true);
      setStatusToast('EchoLabyrinth Adaptive Original Soundtrack engaged.');
    }
  };

  const handleCanvasMouseMove = (e: React.MouseEvent<HTMLCanvasElement>) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const scaleX = CANVAS_W / rect.width;
    const scaleY = CANVAS_H / rect.height;
    mouseWorldRef.current = {
      x: (e.clientX - rect.left) * scaleX,
      y: (e.clientY - rect.top) * scaleY,
    };
  };

  const handleCanvasMouseDown = (e: React.MouseEvent<HTMLCanvasElement>) => {
    if (gameState !== 'PLAYING') return;
    handleCanvasMouseMove(e);
    if (e.button === 0) {
      triggerDirectionalPing();
    } else if (e.button === 2) {
      e.preventDefault();
      triggerThrowDecoy(mouseWorldRef.current);
    }
  };

  const selectedCppFile = useMemo(
    () => CPP_SOURCE_FILES.find((f) => f.path === selectedCppPath) || CPP_SOURCE_FILES[0],
    [selectedCppPath]
  );

  const handleCopyCode = (path: string, content: string) => {
    navigator.clipboard.writeText(content);
    setCopiedPath(path);
    setTimeout(() => setCopiedPath(null), 1800);
  };

  const handleDownloadCppBundle = () => {
    downloadCppProjectZip();
    setStatusToast('Downloaded EchoLabyrinth_SFML_Cpp17_Project.zip (with CMakeLists.txt & build_windows.bat).');
  };

  return (
    <div className="min-h-screen bg-[#06080c] text-slate-200 flex flex-col">
      {/* Top Bar Contract: 3 zones separated by gap-8 */}
      <header className="flex items-center justify-between gap-8 px-6 py-3.5 border-b border-slate-800/80 bg-[#090d14]">
        <a
          href="#top"
          onClick={(e) => {
            e.preventDefault();
            setActiveTab('play');
          }}
          className="font-display text-lg font-semibold tracking-wider text-slate-100 whitespace-nowrap shrink-0"
        >
          EchoLabyrinth
        </a>

        <nav className="flex items-center gap-6 text-sm font-medium text-slate-400">
          <button
            type="button"
            onClick={() => setActiveTab('play')}
            className={`py-1 transition-colors whitespace-nowrap shrink-0 border-b-2 ${
              activeTab === 'play'
                ? 'text-sky-400 border-sky-400'
                : 'border-transparent hover:text-slate-200'
            }`}
          >
            Play Labyrinth
          </button>
          <button
            type="button"
            onClick={() => setActiveTab('lab')}
            className={`py-1 transition-colors whitespace-nowrap shrink-0 border-b-2 ${
              activeTab === 'lab'
                ? 'text-sky-400 border-sky-400'
                : 'border-transparent hover:text-slate-200'
            }`}
          >
            Acoustic Lab
          </button>
          <button
            type="button"
            onClick={() => setActiveTab('cpp')}
            className={`py-1 transition-colors whitespace-nowrap shrink-0 border-b-2 ${
              activeTab === 'cpp'
                ? 'text-sky-400 border-sky-400'
                : 'border-transparent hover:text-slate-200'
            }`}
          >
            C++17 SFML Source
          </button>
          <button
            type="button"
            onClick={() => setActiveTab('audit')}
            className={`py-1 transition-colors whitespace-nowrap shrink-0 border-b-2 ${
              activeTab === 'audit'
                ? 'text-sky-400 border-sky-400'
                : 'border-transparent hover:text-slate-200'
            }`}
          >
            Project Audit
          </button>
        </nav>

        <div className="flex items-center shrink-0">
          <button
            type="button"
            onClick={handleSaveCheckpoint}
            className="px-4 py-2 text-xs font-semibold text-slate-950 bg-sky-400 rounded-md hover:bg-sky-300 transition-colors whitespace-nowrap shrink-0"
          >
            Save Checkpoint
          </button>
        </div>
      </header>

      {/* Main Content Area */}
      <main className="flex-1 max-w-[1440px] w-full mx-auto px-6 py-5">
        {activeTab === 'play' && (
          <div className="grid grid-cols-1 lg:grid-cols-12 gap-6 items-start">
            {/* Left / Center Column: Active Acoustic Simulation Canvas */}
            <div className="lg:col-span-9 flex flex-col gap-3">
              {/* Unobtrusive Top HUD Bar */}
              <div className="flex flex-wrap items-center justify-between gap-4 px-4 py-2.5 bg-[#0e131b] border border-slate-800/80 rounded-lg text-xs">
                <div className="flex items-center gap-2 text-slate-300 font-medium">
                  <span className="text-sky-400 font-semibold">{currentRoom.name}</span>
                  <span aria-hidden="true" className="text-slate-600">·</span>
                  <span className="text-slate-400">{currentRoom.sectorTitle}</span>
                  <span aria-hidden="true" className="text-slate-600">·</span>
                  <span className="font-mono tabular-nums text-slate-400">
                    POS ({hudMetrics.playerX}, {hudMetrics.playerY})
                  </span>
                </div>

                <div className="flex items-center gap-4 font-mono tabular-nums text-slate-300">
                  <span>
                    Stance: <strong className="text-sky-300">{hudMetrics.stance}</strong>
                  </span>
                  <span aria-hidden="true" className="text-slate-600">·</span>
                  <span>
                    Stamina: <strong>{hudMetrics.stamina}%</strong>
                  </span>
                  <span aria-hidden="true" className="text-slate-600">·</span>
                  <span>
                    Decoys: <strong className="text-amber-400">{decoyCount}</strong>
                  </span>
                  <span aria-hidden="true" className="text-slate-600">·</span>
                  <span>
                    Listener:{' '}
                    <strong
                      className={
                        hudMetrics.enemyState === 'PursuePlayer' || hudMetrics.enemyState === 'Attack'
                          ? 'text-red-400'
                          : hudMetrics.enemyState === 'InvestigateSound' ||
                            hudMetrics.enemyState === 'SearchLastKnown'
                          ? 'text-amber-400'
                          : 'text-slate-400'
                      }
                    >
                      {hudMetrics.enemyState} ({hudMetrics.enemySuspicion}%)
                    </strong>
                  </span>
                </div>
              </div>

              {/* Interactive 2D SFML Viewport Simulation */}
              <div className="relative rounded-lg overflow-hidden border border-slate-800 bg-[#06080c] shadow-2xl">
                <canvas
                  ref={canvasRef}
                  width={CANVAS_W}
                  height={CANVAS_H}
                  onMouseMove={handleCanvasMouseMove}
                  onMouseDown={handleCanvasMouseDown}
                  onContextMenu={(e) => e.preventDefault()}
                  className="w-full h-auto block cursor-crosshair select-none"
                />

                {/* Overlay Modals for Pause / Game Over / Victory */}
                {gameState !== 'PLAYING' && (
                  <div className="absolute inset-0 bg-black/80 backdrop-blur-xs flex items-center justify-center p-6">
                    <div className="max-w-md w-full bg-[#0e131b] border border-slate-800 rounded-lg p-6 text-center space-y-4">
                      <h2 className="font-display text-2xl font-semibold text-slate-100">
                        {gameState === 'PAUSED'
                          ? 'Simulation Paused'
                          : gameState === 'VICTORY'
                          ? 'Surface Ascent Reached'
                          : 'Acoustic Triangulation Lethal'}
                      </h2>
                      <p className="text-sm text-slate-400 leading-relaxed">
                        {gameState === 'PAUSED'
                          ? 'Sound propagation and entity AI are suspended. Resume exploration or load a saved checkpoint.'
                          : gameState === 'VICTORY'
                          ? `You navigated all 15 chambers of the subterranean labyrinth and recovered ${collectedLore.length} of 8 surveyor logs.`
                          : 'The blind Listener tracked the amplitude of your vibrations. Use Sneak stance (Shift) or throw a Decoy Stone (Right-Click) to misdirect pursuit.'}
                      </p>
                      <div className="flex items-center justify-center gap-3 pt-2">
                        {gameState === 'PAUSED' ? (
                          <button
                            type="button"
                            onClick={() => setGameState('PLAYING')}
                            className="px-4 py-2 text-xs font-semibold bg-sky-400 text-slate-950 rounded-md hover:bg-sky-300 transition-colors whitespace-nowrap shrink-0"
                          >
                            Resume Game
                          </button>
                        ) : (
                          <button
                            type="button"
                            onClick={() => {
                              setGameState('PLAYING');
                              enterRoom(gameState === 'VICTORY' ? 0 : currentRoomId);
                            }}
                            className="px-4 py-2 text-xs font-semibold bg-sky-400 text-slate-950 rounded-md hover:bg-sky-300 transition-colors whitespace-nowrap shrink-0"
                          >
                            {gameState === 'VICTORY' ? 'Play Again' : 'Retry Chamber'}
                          </button>
                        )}
                        <button
                          type="button"
                          onClick={handleLoadCheckpoint}
                          className="px-4 py-2 text-xs font-medium bg-slate-800 text-slate-200 rounded-md hover:bg-slate-700 transition-colors whitespace-nowrap shrink-0"
                        >
                          Load Checkpoint
                        </button>
                      </div>
                    </div>
                  </div>
                )}
              </div>

              {/* Action Bar & Status Feedback */}
              <div className="flex flex-wrap items-center justify-between gap-3 px-4 py-3 bg-[#0e131b] border border-slate-800/80 rounded-lg">
                <p className="text-xs text-slate-300 truncate max-w-xl">{statusToast}</p>

                <div className="flex items-center gap-2">
                  <button
                    type="button"
                    onClick={triggerDirectionalPing}
                    className="px-3 py-1.5 text-xs font-medium bg-slate-800/90 hover:bg-slate-700 text-sky-300 rounded-md transition-colors whitespace-nowrap shrink-0 font-mono tabular-nums"
                  >
                    Ping Cone [Click / E] {hudMetrics.pingCooldown > 0 ? `(${hudMetrics.pingCooldown}s)` : ''}
                  </button>
                  <button
                    type="button"
                    onClick={triggerRadialPulse}
                    className="px-3 py-1.5 text-xs font-medium bg-slate-800/90 hover:bg-slate-700 text-sky-200 rounded-md transition-colors whitespace-nowrap shrink-0 font-mono tabular-nums"
                  >
                    Radial Pulse [Space] {hudMetrics.pulseCooldown > 0 ? `(${hudMetrics.pulseCooldown}s)` : ''}
                  </button>
                  <button
                    type="button"
                    onClick={() => triggerThrowDecoy()}
                    className="px-3 py-1.5 text-xs font-medium bg-slate-800/90 hover:bg-slate-700 text-amber-300 rounded-md transition-colors whitespace-nowrap shrink-0 font-mono tabular-nums"
                  >
                    Throw Decoy [R-Click / G] ({decoyCount})
                  </button>
                  <button
                    type="button"
                    onClick={triggerInteract}
                    className="px-3 py-1.5 text-xs font-medium bg-slate-800/90 hover:bg-slate-700 text-violet-300 rounded-md transition-colors whitespace-nowrap shrink-0"
                  >
                    Interact [F]
                  </button>
                  <button
                    type="button"
                    onClick={() =>
                      setAcousticConfig((c) => ({ ...c, audioEnabled: !c.audioEnabled }))
                    }
                    className="p-1.5 text-slate-400 hover:text-slate-200 bg-slate-800/90 rounded-md transition-colors"
                    title={acousticConfig.audioEnabled ? 'Mute Synth Audio' : 'Enable Synth Audio'}
                  >
                    {acousticConfig.audioEnabled ? <Volume2 className="w-4 h-4" /> : <VolumeX className="w-4 h-4" />}
                  </button>
                  <button
                    type="button"
                    onClick={() =>
                      setGameState((s) => (s === 'PLAYING' ? 'PAUSED' : 'PLAYING'))
                    }
                    className="p-1.5 text-slate-400 hover:text-slate-200 bg-slate-800/90 rounded-md transition-colors"
                    title="Pause / Resume"
                  >
                    {gameState === 'PAUSED' ? <Play className="w-4 h-4" /> : <Pause className="w-4 h-4" />}
                  </button>
                </div>
              </div>
            </div>

            {/* Right Sidebar: 15-Room Labyrinth Map, Material Legend & Telemetry */}
            <aside className="lg:col-span-3 flex flex-col gap-5 bg-[#0e131b] border border-slate-800/80 rounded-lg p-4">
              <div>
                <div className="flex items-center justify-between mb-2.5">
                  <h3 className="text-sm font-semibold text-slate-100">
                    15-Room Labyrinth Topology
                  </h3>
                  <span className="text-xs font-mono tabular-nums text-slate-400">
                    {visitedRooms.length}/15 Visited
                  </span>
                </div>
                <div className="grid grid-cols-5 gap-1.5">
                  {rooms.map((r) => {
                    const isCurrent = r.id === currentRoomId;
                    const isVisited = visitedRooms.includes(r.id);
                    return (
                      <button
                        key={r.id}
                        type="button"
                        onClick={() => {
                          setGameState('PLAYING');
                          enterRoom(r.id);
                        }}
                        title={r.name}
                        className={`py-2 rounded text-xs font-mono tabular-nums transition-colors whitespace-nowrap shrink-0 border ${
                          isCurrent
                            ? 'bg-sky-400/20 border-sky-400 text-sky-300 font-semibold'
                            : isVisited
                            ? 'bg-slate-800/70 border-slate-700 text-slate-300 hover:border-slate-500'
                            : 'bg-slate-900/50 border-slate-800/80 text-slate-500 hover:text-slate-300'
                        }`}
                      >
                        {String(r.id + 1).padStart(2, '0')}
                      </button>
                    );
                  })}
                </div>
                <p className="mt-2 text-xs text-slate-400 leading-relaxed">
                  Click any chamber number above to jump directly for testing, or traverse east/west portals in real time.
                </p>
              </div>

              <hr className="border-slate-800/80" />

              {/* Material Acoustic Response Legend */}
              <div>
                <h3 className="text-sm font-semibold text-slate-100 mb-2.5">
                  Surface Acoustic Signatures
                </h3>
                <div className="space-y-2.5">
                  {(Object.keys(MATERIAL_SPECS) as SurfaceMaterial[]).map((mat) => {
                    const spec = MATERIAL_SPECS[mat];
                    return (
                      <div
                        key={mat}
                        className="flex items-center justify-between text-xs gap-2"
                      >
                        <div className="flex items-center gap-2 min-w-0">
                          <span
                            className="w-3 h-1.5 rounded-xs shrink-0"
                            style={{ backgroundColor: spec.hex }}
                          />
                          <span className="text-slate-200 font-medium truncate">
                            {spec.label}
                          </span>
                        </div>
                        <span className="text-slate-400 font-mono tabular-nums shrink-0">
                          {Math.round(spec.reflectivity * 100)}% refl · {spec.persistenceSec}s
                        </span>
                      </div>
                    );
                  })}
                </div>
              </div>

              <hr className="border-slate-800/80" />

              {/* Controls & Acoustic Telemetry */}
              <div className="space-y-2 text-xs">
                <h3 className="text-sm font-semibold text-slate-100">
                  Controls & Engine Pool
                </h3>
                <div className="text-slate-400 space-y-1 leading-relaxed">
                  <p><strong className="text-slate-200">WASD / Arrows:</strong> Move surveyor</p>
                  <p><strong className="text-slate-200">Shift / Ctrl:</strong> Sneak (quiet) / Sprint (loud)</p>
                  <p><strong className="text-slate-200">Left Click / E:</strong> Directional Sonar Cone</p>
                  <p><strong className="text-slate-200">Space:</strong> 360° Radial Resonance Pulse</p>
                  <p><strong className="text-slate-200">Right Click / G:</strong> Throw Decoy Stone</p>
                  <p><strong className="text-slate-200">F:</strong> Pull Harmonic Lever / Collect Log</p>
                </div>
                <div className="pt-2 flex items-center justify-between text-slate-400 font-mono tabular-nums border-t border-slate-800/60">
                  <span>Active Rays: {hudMetrics.activeRays}/{MAX_RAYS}</span>
                  <span>Echoes: {hudMetrics.activeEchoes}/{MAX_ECHOES}</span>
                </div>
                <div className="pt-2 flex items-center gap-2">
                  <button
                    type="button"
                    onClick={handleLoadCheckpoint}
                    className="flex-1 py-1.5 px-3 text-xs font-medium bg-slate-800 hover:bg-slate-700 text-slate-200 rounded transition-colors whitespace-nowrap shrink-0"
                  >
                    Load Checkpoint
                  </button>
                  <button
                    type="button"
                    onClick={() => enterRoom(currentRoomId)}
                    className="py-1.5 px-3 text-xs font-medium bg-slate-800 hover:bg-slate-700 text-slate-300 rounded transition-colors whitespace-nowrap shrink-0"
                    title="Reset Current Chamber"
                  >
                    <RotateCcw className="w-3.5 h-3.5" />
                  </button>
                </div>
                <button
                  type="button"
                  onClick={handleDownloadCppBundle}
                  className="w-full mt-2 py-2 px-3 text-xs font-semibold bg-sky-400/15 hover:bg-sky-400/25 text-sky-300 border border-sky-400/40 rounded transition-colors flex items-center justify-center gap-1.5 whitespace-nowrap shrink-0"
                >
                  <Download className="w-3.5 h-3.5" />
                  <span>Download C++17 SFML (.ZIP)</span>
                </button>
              </div>

              <hr className="border-slate-800/80" />

              {/* Compact Adaptive OST Player in Play Sidebar */}
              <div className="space-y-2.5 text-xs">
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-1.5 text-slate-100 font-semibold">
                    <Music className="w-3.5 h-3.5 text-sky-400" />
                    <span>Adaptive OST Engine</span>
                  </div>
                  <button
                    type="button"
                    onClick={handleToggleOstPlayback}
                    className={`px-2.5 py-1 rounded text-[11px] font-semibold transition-colors ${
                      ostPlaying
                        ? 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/40'
                        : 'bg-sky-400 text-slate-950 hover:bg-sky-300'
                    }`}
                  >
                    {ostPlaying ? 'OST Active' : 'Start OST'}
                  </button>
                </div>
                <div className="grid grid-cols-2 gap-1.5">
                  <button
                    type="button"
                    onClick={() => handleSelectOstMode('auto')}
                    className={`px-2 py-1.5 rounded text-[11px] font-mono truncate border transition-colors ${
                      ostPlaying && ostMode === 'auto'
                        ? 'bg-sky-400/20 border-sky-400 text-sky-300 font-semibold'
                        : 'bg-slate-900/70 border-slate-800 text-slate-400 hover:text-slate-200'
                    }`}
                  >
                    Auto-Adaptive
                  </button>
                  {OST_TRACKS.map((tr, idx) => (
                    <button
                      key={tr.id}
                      type="button"
                      onClick={() => handleSelectOstMode(tr.id)}
                      className={`px-2 py-1.5 rounded text-[11px] font-mono truncate border transition-colors ${
                        ostPlaying && ostMode === tr.id
                          ? 'bg-sky-400/20 border-sky-400 text-sky-300 font-semibold'
                          : 'bg-slate-900/70 border-slate-800 text-slate-400 hover:text-slate-200'
                      }`}
                      title={tr.title}
                    >
                      0{idx + 1}. {tr.title.split(' ')[0]}
                    </button>
                  ))}
                </div>
              </div>
            </aside>
          </div>
        )}

        {/* TAB 2: ACOUSTIC LAB & PARAMETER TUNER */}
        {activeTab === 'lab' && (
          <div className="grid grid-cols-1 lg:grid-cols-12 gap-8">
            <div className="lg:col-span-7 bg-[#0e131b] border border-slate-800/80 rounded-lg p-6 space-y-6">
              <div>
                <h2 className="font-display text-xl font-semibold text-slate-100">
                  Acoustic Propagation Parameters
                </h2>
                <p className="text-sm text-slate-400 mt-1">
                  Adjust real-time wavefront speed, maximum specular reflection depth, ray density, and wall echo decay. Changes apply immediately to the active labyrinth simulation.
                </p>
              </div>

              <div className="space-y-5">
                <div>
                  <div className="flex justify-between text-xs font-mono tabular-nums mb-1.5">
                    <label htmlFor="wave-speed" className="text-slate-300 font-sans font-medium">
                      Wavefront Propagation Speed Multiplier
                    </label>
                    <span className="text-sky-400">{acousticConfig.waveSpeedMultiplier.toFixed(2)}x</span>
                  </div>
                  <input
                    id="wave-speed"
                    type="range"
                    min={0.5}
                    max={2.0}
                    step={0.05}
                    value={acousticConfig.waveSpeedMultiplier}
                    onChange={(e) =>
                      setAcousticConfig((c) => ({
                        ...c,
                        waveSpeedMultiplier: parseFloat(e.target.value),
                      }))
                    }
                    className="w-full accent-sky-400"
                  />
                </div>

                <div>
                  <div className="flex justify-between text-xs font-mono tabular-nums mb-1.5">
                    <label htmlFor="max-bounces" className="text-slate-300 font-sans font-medium">
                      Maximum Specular Wall Reflections (Bounded Depth)
                    </label>
                    <span className="text-sky-400">{acousticConfig.maxReflections} bounces</span>
                  </div>
                  <input
                    id="max-bounces"
                    type="range"
                    min={0}
                    max={6}
                    step={1}
                    value={acousticConfig.maxReflections}
                    onChange={(e) =>
                      setAcousticConfig((c) => ({
                        ...c,
                        maxReflections: parseInt(e.target.value, 10),
                      }))
                    }
                    className="w-full accent-sky-400"
                  />
                </div>

                <div>
                  <div className="flex justify-between text-xs font-mono tabular-nums mb-1.5">
                    <label htmlFor="ray-density" className="text-slate-300 font-sans font-medium">
                      Ray-Sampling Density Multiplier
                    </label>
                    <span className="text-sky-400">{acousticConfig.rayDensityMultiplier.toFixed(2)}x</span>
                  </div>
                  <input
                    id="ray-density"
                    type="range"
                    min={0.5}
                    max={2.0}
                    step={0.1}
                    value={acousticConfig.rayDensityMultiplier}
                    onChange={(e) =>
                      setAcousticConfig((c) => ({
                        ...c,
                        rayDensityMultiplier: parseFloat(e.target.value),
                      }))
                    }
                    className="w-full accent-sky-400"
                  />
                </div>

                <div>
                  <div className="flex justify-between text-xs font-mono tabular-nums mb-1.5">
                    <label htmlFor="echo-persistence" className="text-slate-300 font-sans font-medium">
                      Wall Phosphor Echo Persistence Multiplier
                    </label>
                    <span className="text-sky-400">{acousticConfig.echoPersistenceMultiplier.toFixed(2)}x</span>
                  </div>
                  <input
                    id="echo-persistence"
                    type="range"
                    min={0.4}
                    max={2.5}
                    step={0.1}
                    value={acousticConfig.echoPersistenceMultiplier}
                    onChange={(e) =>
                      setAcousticConfig((c) => ({
                        ...c,
                        echoPersistenceMultiplier: parseFloat(e.target.value),
                      }))
                    }
                    className="w-full accent-sky-400"
                  />
                </div>

                <div>
                  <div className="flex justify-between text-xs font-mono tabular-nums mb-1.5">
                    <label htmlFor="ambient-outline" className="text-slate-300 font-sans font-medium">
                      Ambient Darkness Override (0.00 = Pure Pitch Black)
                    </label>
                    <span className="text-sky-400">{Math.round(acousticConfig.ambientVisibility * 100)}%</span>
                  </div>
                  <input
                    id="ambient-outline"
                    type="range"
                    min={0}
                    max={0.25}
                    step={0.01}
                    value={acousticConfig.ambientVisibility}
                    onChange={(e) =>
                      setAcousticConfig((c) => ({
                        ...c,
                        ambientVisibility: parseFloat(e.target.value),
                      }))
                    }
                    className="w-full accent-sky-400"
                  />
                </div>
              </div>

              <div className="pt-2 flex items-center justify-between border-t border-slate-800">
                <button
                  type="button"
                  onClick={() =>
                    setAcousticConfig({
                      waveSpeedMultiplier: 1.0,
                      maxReflections: 3,
                      rayDensityMultiplier: 1.0,
                      echoPersistenceMultiplier: 1.0,
                      ambientVisibility: 0.04,
                      audioEnabled: true,
                    })
                  }
                  className="px-4 py-2 text-xs font-medium bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-md transition-colors whitespace-nowrap shrink-0"
                >
                  Reset Default Parameters
                </button>
                <button
                  type="button"
                  onClick={() => setActiveTab('play')}
                  className="px-4 py-2 text-xs font-semibold bg-sky-400 hover:bg-sky-300 text-slate-950 rounded-md transition-colors whitespace-nowrap shrink-0"
                >
                  Return to Labyrinth
                </button>
              </div>
            </div>

            <div className="lg:col-span-5 bg-[#0e131b] border border-slate-800/80 rounded-lg p-6 space-y-5">
              <h2 className="font-display text-xl font-semibold text-slate-100">
                Acoustic Material Test Bench
              </h2>
              <p className="text-sm text-slate-400">
                Audition the synthesized acoustic reflection signature and inspect the attenuation coefficients used in <code className="text-sky-300">Config.hpp</code>.
              </p>

              <div className="space-y-3">
                {(Object.keys(MATERIAL_SPECS) as SurfaceMaterial[]).map((mat) => {
                  const spec = MATERIAL_SPECS[mat];
                  return (
                    <div
                      key={mat}
                      className="p-3.5 rounded-lg bg-[#090d14] border border-slate-800/90 flex items-center justify-between gap-4"
                    >
                      <div className="space-y-1">
                        <div className="flex items-center gap-2">
                          <span
                            className="w-3 h-3 rounded-xs shrink-0"
                            style={{ backgroundColor: spec.hex }}
                          />
                          <span className="text-sm font-semibold text-slate-100">
                            {spec.label}
                          </span>
                        </div>
                        <p className="text-xs text-slate-400">{spec.acousticNote}</p>
                      </div>
                      <button
                        type="button"
                        onClick={() => soundSynth.playSurfaceReflectionSample(mat, true)}
                        className="px-3 py-1.5 text-xs font-medium bg-slate-800 hover:bg-slate-700 text-slate-200 rounded transition-colors whitespace-nowrap shrink-0"
                      >
                        Audition Tone
                      </button>
                    </div>
                  );
                })}
              </div>
            </div>

            {/* Full-Width EchoLabyrinth Original Soundtrack (OST) Studio & WAV Exporter */}
            <div className="lg:col-span-12 bg-[#0e131b] border border-slate-800/80 rounded-lg p-6 space-y-5">
              <div className="flex flex-wrap items-center justify-between gap-4">
                <div>
                  <div className="flex items-center gap-2">
                    <Music className="w-5 h-5 text-sky-400" />
                    <h2 className="font-display text-xl font-semibold text-slate-100">
                      EchoLabyrinth Original Soundtrack (OST) — 5-Layer Procedural DSP Composer
                    </h2>
                  </div>
                  <p className="text-sm text-slate-400 mt-1">
                    Synthesized in real time in both C++17 (<code className="text-sky-300">AudioManager.cpp</code> with <code className="text-sky-300">--export-ost</code>) and Web Audio using circular Schroeder reverb, FM metallic chimes, dark gothic organ, and vowel-formant abyssal choir synthesis.
                  </p>
                </div>

                <div className="flex items-center gap-3">
                  <div className="flex items-center gap-2 text-xs font-mono text-slate-300 bg-[#090d14] px-3 py-1.5 rounded border border-slate-800">
                    <span>OST Gain</span>
                    <input
                      type="range"
                      min={0}
                      max={1}
                      step={0.02}
                      value={ostVolume}
                      onChange={(e) => {
                        const v = parseFloat(e.target.value);
                        setOstVolume(v);
                        soundSynth.setMusicMasterVolume(v);
                      }}
                      className="w-24 accent-sky-400"
                    />
                    <span>{Math.round(ostVolume * 100)}%</span>
                  </div>

                  <button
                    type="button"
                    onClick={() => handleSelectOstMode('auto')}
                    className={`px-3.5 py-2 rounded-md text-xs font-semibold transition-colors border ${
                      ostPlaying && ostMode === 'auto'
                        ? 'bg-sky-400/20 border-sky-400 text-sky-300'
                        : 'bg-slate-800 border-slate-700 text-slate-200 hover:bg-slate-700'
                    }`}
                  >
                    Auto-Adaptive Mix
                  </button>

                  <button
                    type="button"
                    onClick={handleToggleOstPlayback}
                    className={`px-4 py-2 rounded-md text-xs font-semibold transition-colors ${
                      ostPlaying
                        ? 'bg-rose-500/20 text-rose-300 border border-rose-500/40 hover:bg-rose-500/30'
                        : 'bg-sky-400 text-slate-950 hover:bg-sky-300'
                    }`}
                  >
                    {ostPlaying ? 'Stop OST Playback' : 'Play Soundtrack'}
                  </button>
                </div>
              </div>

              <div className="grid grid-cols-1 md:grid-cols-5 gap-4">
                {OST_TRACKS.map((track) => {
                  const isSelected = ostPlaying && ostMode === track.id;
                  return (
                    <div
                      key={track.id}
                      className={`p-4 rounded-lg border flex flex-col justify-between gap-4 transition-colors ${
                        isSelected
                          ? 'bg-sky-950/30 border-sky-400/70'
                          : 'bg-[#090d14] border-slate-800/90'
                      }`}
                    >
                      <div className="space-y-2">
                        <div className="flex items-center justify-between gap-2 text-[11px] font-mono text-sky-400">
                          <span>{track.bpm} BPM</span>
                          <span>{track.keySignature}</span>
                        </div>
                        <h3 className="text-sm font-semibold text-slate-100 leading-snug">
                          {track.title}
                        </h3>
                        <p className="text-[11px] font-mono text-slate-500 truncate">
                          {track.filename}
                        </p>
                        <p className="text-xs text-slate-400 leading-relaxed">
                          {track.description}
                        </p>
                      </div>

                      <div className="pt-2 flex flex-col gap-2 border-t border-slate-800/80">
                        <button
                          type="button"
                          onClick={() => handleSelectOstMode(track.id)}
                          className={`w-full py-1.5 px-3 rounded text-xs font-semibold transition-colors ${
                            isSelected
                              ? 'bg-sky-400 text-slate-950'
                              : 'bg-slate-800 hover:bg-slate-700 text-slate-200'
                          }`}
                        >
                          {isSelected ? 'Playing Loop...' : 'Solo Audition'}
                        </button>
                        <button
                          type="button"
                          onClick={() => {
                            soundSynth.exportTrackWav(track.id);
                            setStatusToast(`Exported ${track.filename} (44.1kHz 16-bit PCM WAV).`);
                          }}
                          className="w-full py-1.5 px-3 rounded text-xs font-medium bg-slate-900 hover:bg-slate-800 text-slate-300 border border-slate-700/80 transition-colors flex items-center justify-center gap-1.5"
                        >
                          <Download className="w-3.5 h-3.5" />
                          <span>Export .WAV</span>
                        </button>
                      </div>
                    </div>
                  );
                })}
              </div>
            </div>
          </div>
        )}

        {/* TAB 3: NATIVE C++17 & SFML 2.6 CODEBASE INSPECTOR */}
        {activeTab === 'cpp' && (
          <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
            <div className="lg:col-span-4 bg-[#0e131b] border border-slate-800/80 rounded-lg p-4 flex flex-col justify-between gap-4">
              <div className="space-y-3">
                <div className="flex items-center justify-between">
                  <h2 className="text-sm font-semibold text-slate-100">
                    C++17 & SFML 2.6 Project Files
                  </h2>
                  <span className="text-xs font-mono tabular-nums text-slate-400">
                    {CPP_SOURCE_FILES.length} files
                  </span>
                </div>
                <p className="text-xs text-slate-400 leading-relaxed">
                  All files below live in the workspace <code className="text-sky-300">/cpp</code> directory and follow strict RAII, pre-allocated ring pools, and <code className="text-sky-300">sf::VertexArray</code> batching.
                </p>

                <div className="space-y-1 pt-1">
                  {CPP_SOURCE_FILES.map((file) => {
                    const active = file.path === selectedCppFile.path;
                    return (
                      <button
                        key={file.path}
                        type="button"
                        onClick={() => setSelectedCppPath(file.path)}
                        className={`w-full text-left px-3 py-2 rounded-md text-xs transition-colors flex items-center justify-between gap-2 ${
                          active
                            ? 'bg-sky-400/15 text-sky-300 border border-sky-400/40 font-medium'
                            : 'text-slate-300 hover:bg-slate-800/60 border border-transparent'
                        }`}
                      >
                        <span className="font-mono truncate">{file.path}</span>
                        <span className="text-[11px] text-slate-400 shrink-0">{file.module}</span>
                      </button>
                    );
                  })}
                </div>
              </div>

              <button
                type="button"
                onClick={handleDownloadCppBundle}
                className="w-full py-2.5 px-4 text-xs font-semibold bg-sky-400 hover:bg-sky-300 text-slate-950 rounded-md transition-colors flex items-center justify-center gap-2 whitespace-nowrap shrink-0"
              >
                <Download className="w-4 h-4" />
                <span>Download C++17 SFML Project (.ZIP)</span>
              </button>
            </div>

            <div className="lg:col-span-8 bg-[#0e131b] border border-slate-800/80 rounded-lg overflow-hidden flex flex-col">
              <div className="px-5 py-3.5 border-b border-slate-800 bg-[#090d14] flex items-center justify-between gap-4">
                <div>
                  <h3 className="text-sm font-mono font-semibold text-sky-300">
                    {selectedCppFile.path}
                  </h3>
                  <p className="text-xs text-slate-400 mt-0.5">{selectedCppFile.summary}</p>
                </div>
                <button
                  type="button"
                  onClick={() => handleCopyCode(selectedCppFile.path, selectedCppFile.content)}
                  className="px-3 py-1.5 text-xs font-medium bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-md transition-colors flex items-center gap-1.5 whitespace-nowrap shrink-0"
                >
                  {copiedPath === selectedCppFile.path ? (
                    <>
                      <Check className="w-3.5 h-3.5 text-emerald-400" />
                      <span>Copied</span>
                    </>
                  ) : (
                    <>
                      <Copy className="w-3.5 h-3.5" />
                      <span>Copy File</span>
                    </>
                  )}
                </button>
              </div>
              <pre className="p-5 text-xs font-mono text-slate-200 overflow-x-auto leading-relaxed max-h-[600px]">
                <code>{selectedCppFile.content}</code>
              </pre>
            </div>
          </div>
        )}

        {/* TAB 4: PHASE A–D PROJECT AUDIT & BUILD INSTRUCTIONS */}
        {activeTab === 'audit' && (
          <div className="space-y-6">
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
              <section className="bg-[#0e131b] border border-slate-800/80 rounded-lg p-6 space-y-3">
                <h2 className="font-display text-lg font-semibold text-slate-100">
                  01. EchoLabyrinth 5.0 &ldquo;Resonance&rdquo; — Phase A, B &amp; C Architecture
                </h2>
                <p className="text-sm text-slate-300 leading-relaxed">
                  Sound is sight, weapon, and risk. The native C++17 &amp; SFML 2.6 engine in <code className="text-sky-300">/cpp/</code> implements all 5 Resonance blocks:
                </p>
                <ul className="text-xs text-slate-400 space-y-1.5 leading-relaxed list-disc list-inside">
                  <li><strong className="text-slate-200">Block 1 (Noise Meter &amp; Ghost Clear):</strong> 0–100 continuous Noise Meter with Quiet/Audible/Loud/Scream tiers, 50% cross-room attenuation through open doors, enemy intrusion, and Ghost Clear (<code className="text-sky-300">&le;30</code> peak noise &rarr; 1.5&times; Shards &amp; Silent Step relic).</li>
                  <li><strong className="text-slate-200">Block 2 (Material Physics):</strong> DampMoss (-80% reflection &amp; stealth shelter), Polished Metal (+2 bonus reflections), Brittle Glass (shatters at +40 noise), Water (2&times; range, +15 splash), and 4s Resonator Pipe Columns.</li>
                  <li><strong className="text-slate-200">Block 3 &amp; 4 (Arsenal &amp; 9 Enemies):</strong> Narrow Whistle (+5 noise), Seismic Stomp (+35 noise &amp; knockback), 3s Echo Trap Mines, Low/Mid/High Frequency Tuning, plus SilentStalker, DeafBrute, Mimic, and EchoSwarm.</li>
                  <li><strong className="text-slate-200">Block 5 (Room Events, 12 Relics, 12 Codex Logs &amp; Save V5):</strong> EchoStorm, AcousticBlackout (+50% Shards), ResonanceOvercharge, 3-phase Choir Sovereign arena mechanics, 12 persistent Audio-Lore Codex entries, and <code className="text-sky-300">ECHOLABYRINTH_SAVE_V5</code> (with V4 compatibility) verified by 17/17 self-test suites.</li>
                </ul>
              </section>

              <section className="bg-[#0e131b] border border-slate-800/80 rounded-lg p-6 space-y-3">
                <h2 className="font-display text-lg font-semibold text-slate-100">
                  02. Acoustic Simulation Engineering Trade-Offs
                </h2>
                <ul className="text-sm text-slate-300 space-y-2 leading-relaxed list-disc list-inside">
                  <li>
                    <strong className="text-sky-300">Ray-Sampled Wavefronts vs. Full 2D FDTD Grid:</strong> Finite-Difference Time-Domain wave solvers require costly per-pixel updates every frame. Instead, <code className="text-sky-300">AcousticSystem</code> uses a pre-allocated ring buffer of 2,048 rays with analytical 2D segment-segment intersection and specular normal reflection <code className="text-sky-300">R = D - 2(D·N)N</code>.
                  </li>
                  <li>
                    <strong className="text-sky-300">Wall Phosphor Echo Persistence:</strong> When a ray strikes a wall segment, it spawns a <code className="text-sky-300">WallEchoTrace</code> aligned to the wall tangent. Traces decay quadratically according to material persistence and active frequency (Low/Mid/High).
                  </li>
                  <li>
                    <strong className="text-sky-300">Zero Heap Allocations in Main Loop:</strong> Both rays and wall traces use fixed-capacity ring cursors rendered in batched <code className="text-sky-300">sf::VertexArray(sf::Lines)</code> calls with <code className="text-sky-300">sf::BlendAdd</code>.
                  </li>
                </ul>
              </section>
            </div>

            <section className="bg-[#0e131b] border border-slate-800/80 rounded-lg p-6 space-y-4">
              <h2 className="font-display text-lg font-semibold text-slate-100">
                03. Windows Compilation & Visual Studio / CMake Build Instructions
              </h2>
              <p className="text-sm text-slate-400">
                The <code className="text-sky-300">/cpp/CMakeLists.txt</code> configuration detects an installed SFML 2.6+ SDK automatically, or fetches SFML 2.6.1 via CMake <code className="text-sky-300">FetchContent</code> and copies runtime DLLs (<code className="text-sky-300">openal32.dll</code>, <code className="text-sky-300">sfml-*.dll</code>) into the executable directory on Windows.
              </p>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="bg-[#090d14] border border-slate-800 rounded-md p-4">
                  <h3 className="text-xs font-semibold text-sky-300 mb-2">
                    Option A — Visual Studio 2022 (MSVC x64)
                  </h3>
                  <pre className="text-xs font-mono text-slate-300 overflow-x-auto leading-relaxed">
{`cd cpp
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\\build\\Release\\EchoLabyrinth_SFML.exe`}
                  </pre>
                </div>
                <div className="bg-[#090d14] border border-slate-800 rounded-md p-4">
                  <h3 className="text-xs font-semibold text-sky-300 mb-2">
                    Option B — MinGW-w64 / Ninja
                  </h3>
                  <pre className="text-xs font-mono text-slate-300 overflow-x-auto leading-relaxed">
{`cd cpp
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/EchoLabyrinth_SFML.exe`}
                  </pre>
                </div>
              </div>
            </section>
          </div>
        )}
      </main>
    </div>
  );
}
