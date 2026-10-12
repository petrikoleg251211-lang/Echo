// Web Audio API synthesizer mirroring C++ SFML AudioManager procedural soundbuffers
// Includes the 5-track EchoLabyrinth Original Soundtrack (OST) DSP composer & 16-bit WAV exporter
import type { SurfaceMaterial, WaveSignature } from './types';

export type SoundtrackTrackId =
  | 'explore'
  | 'tension'
  | 'pursuit'
  | 'sanctum'
  | 'boss';

export type SoundtrackMode = 'adaptive' | SoundtrackTrackId;

export interface SoundtrackTrackMeta {
  id: SoundtrackTrackId;
  number: string;
  title: string;
  subtitle: string;
  filename: string;
  bpm: number;
  keySignature: string;
  durationSec: number;
  description: string;
  accentColor: string;
}

export const SOUNDTRACK_CATALOG: SoundtrackTrackMeta[] = [
  {
    id: 'explore',
    number: '01',
    title: 'Subterranean Survey',
    subtitle: 'Exploration Dark Ambient & Crystal Arpeggio',
    filename: '01_subterranean_survey.wav',
    bpm: 75,
    keySignature: 'D Dorian',
    durationSec: 16.0,
    description:
      'Глибока басова прогресія каверн (Dm → Bbmaj7 → Gm7 → Asus4), детюнений темний пед та кришталевий FM-арпеджіатор із підземною реверберацією.',
    accentColor: '#38bdf8',
  },
  {
    id: 'tension',
    number: '02',
    title: 'Fractured Echoes',
    subtitle: 'Suspicion & Stalker Tension Layer',
    filename: '02_fractured_echoes.wav',
    bpm: 90,
    keySignature: 'D Locrian / Tritone',
    durationSec: 16.0,
    description:
      'Дисонансні інтервали тритону й малої секунди, смичкове металеве тремоло, низький пульс тривоги та синхронізовані хітинові ехолокаційні кліки.',
    accentColor: '#f59e0b',
  },
  {
    id: 'pursuit',
    number: '03',
    title: 'Blind Pursuit',
    subtitle: 'Active Hunt Industrial Darksynth',
    filename: '03_blind_pursuit.wav',
    bpm: 120,
    keySignature: 'D Phrygian',
    durationSec: 8.0,
    description:
      'Бойовий індустріальний дарк-синт переслідування: пробивний 4/4 суб-кік, ковадло-снейр на 2 і 4, 16-ті пневматичні хай-хети, агресивний FM-бас та горни тривоги.',
    accentColor: '#f43f5e',
  },
  {
    id: 'sanctum',
    number: '04',
    title: 'Resonance Sanctum',
    subtitle: 'Safe Haven & Harmonic Crystal Motif',
    filename: '04_resonance_sanctum.wav',
    bpm: 60,
    keySignature: 'D Major 9',
    durationSec: 16.0,
    description:
      'Теплий гармонічний хоральний пед (Dmaj9 → Bm9 → Gmaj7 → A6) та чисті перебори резонансної арфи у кімнатах збереження та біля акустичних вівтарів.',
    accentColor: '#34d399',
  },
  {
    id: 'boss',
    number: '05',
    title: 'The Choir Sovereign',
    subtitle: '3-Phase Abyssal Boss Symphony',
    filename: '05_the_choir_sovereign.wav',
    bpm: 135,
    keySignature: 'D Harmonic Minor',
    durationSec: 16.0,
    description:
      'Симфонія фінального боса: готичний підземний трубний орган, формантний синтез хору Безодні (голосні Ah/Oh), бойові литаври та ритуальні удари ковадла.',
    accentColor: '#c084fc',
  },
];

const SAMPLE_RATE = 44100;
const TWO_PI = Math.PI * 2;

function hashNoise(seed: number): number {
  let s = seed >>> 0;
  s = (s ^ 61) ^ (s >>> 16);
  s = Math.imul(s, 9) >>> 0;
  s = s ^ (s >>> 4);
  s = Math.imul(s, 0x27d4eb2d) >>> 0;
  s = s ^ (s >>> 15);
  return (s & 0xffff) / 32768.0 - 1.0;
}

function softClip(x: number): number {
  return Math.tanh(x);
}

function renderLoopPcm(
  durationSec: number,
  delayTimeSec: number,
  delayFeedback: number,
  delayMix: number,
  trackFn: (t: number, idx: number) => number
): Float32Array {
  const count = Math.floor(SAMPLE_RATE * durationSec);
  const delaySamples = Math.max(1, Math.floor(SAMPLE_RATE * delayTimeSec));
  const dry = new Float32Array(count);

  for (let i = 0; i < count; i++) {
    dry[i] = trackFn(i / SAMPLE_RATE, i);
  }

  // Two-pass circular feedback delay/reverb network for seamless loop wrap-around
  const delayLine = new Float32Array(delaySamples);
  const wet = new Float32Array(count);
  let cursor = 0;
  let lpState = 0;

  for (let pass = 0; pass < 2; pass++) {
    for (let i = 0; i < count; i++) {
      const delayed = delayLine[cursor];
      lpState = lpState * 0.72 + (dry[i] + delayed * delayFeedback) * 0.28;
      delayLine[cursor] = lpState;
      cursor = (cursor + 1) % delaySamples;
      if (pass === 1) {
        wet[i] = dry[i] * (1.0 - delayMix) + delayed * delayMix;
      }
    }
  }

  // 35ms loop-boundary cosine crossfade to guarantee zero-click looping
  const fadeSamples = Math.min(Math.floor(count / 16), Math.floor(SAMPLE_RATE * 0.035));
  for (let i = 0; i < fadeSamples; i++) {
    const alpha = i / fadeSamples;
    const w = 0.5 - 0.5 * Math.cos(Math.PI * alpha);
    const tailIdx = count - fadeSamples + i;
    const blended = wet[i] * w + wet[tailIdx] * (1.0 - w);
    wet[i] = blended;
    wet[tailIdx] = blended * (1.0 - w) + wet[tailIdx] * w;
  }

  const out = new Float32Array(count);
  for (let i = 0; i < count; i++) {
    out[i] = Math.max(-1, Math.min(1, softClip(wet[i])));
  }
  return out;
}

function generateTrackPcm(id: SoundtrackTrackId): Float32Array {
  switch (id) {
    case 'explore': {
      const bassRoots = [73.42, 58.27, 49.0, 55.0];
      const padFifths = [110.0, 87.31, 73.42, 82.41];
      const padColors = [174.61, 146.83, 116.54, 164.81];
      const arpNotes = [293.66, 349.23, 440.0, 523.25, 440.0, 329.63, 392.0, 293.66];

      return renderLoopPcm(16.0, 0.375, 0.46, 0.34, (t, idx) => {
        const bar = Math.floor(t / 4.0) & 3;
        const barPhase = t % 4.0;
        const barEnv =
          Math.sin(Math.min(1.0, barPhase / 0.45) * 1.570796) *
          (0.85 + 0.15 * Math.sin(TWO_PI * 0.25 * barPhase));

        const root = bassRoots[bar];
        const fifth = padFifths[bar];
        const color = padColors[bar];

        const sub =
          (Math.sin(TWO_PI * root * t) +
            0.35 * Math.sin(TWO_PI * (root * 0.5) * t) +
            0.22 * Math.sin(TWO_PI * (root * 2.0) * t)) *
          0.36 *
          barEnv;

        const pad =
          (Math.sin(TWO_PI * (fifth - 0.32) * t) +
            Math.sin(TWO_PI * (fifth + 0.32) * t) +
            0.7 * Math.sin(TWO_PI * color * t) +
            0.4 * Math.sin(TWO_PI * (color * 1.5) * t + Math.sin(TWO_PI * 0.25 * t))) *
          0.16 *
          barEnv;

        const step = Math.floor(t / 1.0) % arpNotes.length;
        const noteTime = t % 1.0;
        const bellFreq = arpNotes[(step + bar) % arpNotes.length];
        const bellMod = Math.sin(TWO_PI * (bellFreq * 2.0) * noteTime) * Math.exp(-noteTime * 5.5);
        const bell =
          Math.sin(TWO_PI * bellFreq * noteTime + 1.4 * bellMod) *
          Math.exp(-noteTime * 3.2) *
          0.14;

        const air = hashNoise(idx) * 0.015 * (0.5 + 0.5 * Math.sin(TWO_PI * 0.125 * t));
        return sub + pad + bell + air;
      });
    }

    case 'tension': {
      const dissonantA = [146.83, 155.56, 138.59, 164.81];
      const dissonantB = [207.65, 220.0, 196.0, 233.08];

      return renderLoopPcm(16.0, 0.28, 0.52, 0.38, (t, idx) => {
        const bar = Math.floor(t / 4.0) & 3;
        const fA = dissonantA[bar];
        const fB = dissonantB[bar];

        const tremolo = 0.62 + 0.38 * Math.sin(TWO_PI * 6.0 * t);
        const mod = Math.sin(TWO_PI * (fA * 3.0) * t) * 0.45;
        const strings =
          (Math.sin(TWO_PI * fA * t + mod) +
            0.85 * Math.sin(TWO_PI * fB * t) +
            0.4 * Math.sin(TWO_PI * (fB * 1.99) * t)) *
          0.2 *
          tremolo;

        const beatT = t % 0.5;
        const pulseEnv = Math.exp(-beatT * 7.5);
        const dreadPulse = Math.sin(TWO_PI * (65.41 - beatT * 18.0) * beatT) * pulseEnv * 0.34;

        const clickPhase = (t + 0.125) % 0.75;
        const click =
          clickPhase < 0.045
            ? Math.exp(-clickPhase * 85.0) *
              (Math.sin(TWO_PI * 1320.0 * clickPhase) + hashNoise(idx) * 0.4) *
              0.22
            : 0.0;

        return strings + dreadPulse + click;
      });
    }

    case 'pursuit': {
      const bassSeq = [
        73.42, 73.42, 146.83, 73.42,
        87.31, 73.42, 103.83, 98.0,
        73.42, 73.42, 146.83, 73.42,
        110.0, 103.83, 87.31, 65.41,
      ];

      return renderLoopPcm(8.0, 0.1875, 0.35, 0.22, (t, idx) => {
        const beatT = t % 0.5;
        const barT = t % 2.0;
        const stepT = t % 0.125;
        const stepIdx = Math.floor(t / 0.125) & 15;

        const kickPitch = 52.0 + 115.0 * Math.exp(-beatT * 36.0);
        const kick = Math.sin(TWO_PI * kickPitch * beatT) * Math.exp(-beatT * 11.5) * 0.52;

        let snare = 0.0;
        const snarePhase = (barT + 0.5) % 1.0;
        if (snarePhase < 0.24) {
          const sEnv = Math.exp(-snarePhase * 22.0);
          const body =
            Math.sin(TWO_PI * 210.0 * snarePhase) + 0.6 * Math.sin(TWO_PI * 465.0 * snarePhase);
          const noise = hashNoise(idx + 4096) * 0.85;
          snare = (body * 0.45 + noise * 0.55) * sEnv * 0.38;
        }

        const hatAccent = (stepIdx & 3) === 2 ? 1.0 : 0.45;
        const hat = hashNoise(idx + 16384) * Math.exp(-stepT * 55.0) * 0.16 * hatAccent;

        const bassFreq = bassSeq[stepIdx];
        const fmEnv = Math.exp(-stepT * 18.0);
        const mod = Math.sin(TWO_PI * (bassFreq * 2.0) * t) * (1.8 * fmEnv + 0.4);
        const bass = Math.sin(TWO_PI * bassFreq * t + mod) * Math.exp(-stepT * 9.0) * 0.34;

        let horn = 0.0;
        if (barT < 0.45) {
          const hEnv =
            Math.sin(Math.min(1.0, barT / 0.05) * 1.570796) * Math.exp(-barT * 5.2);
          const hFreq = t < 4.0 ? 146.83 : 155.56;
          horn =
            (Math.sin(TWO_PI * hFreq * t) +
              0.7 * Math.sin(TWO_PI * (hFreq * 2.0) * t) +
              0.45 * Math.sin(TWO_PI * (hFreq * 3.0) * t)) *
            hEnv *
            0.22;
        }

        return kick + snare + hat + bass + horn;
      });
    }

    case 'sanctum': {
      const chords = [
        [146.83, 220.0, 277.18, 329.63],
        [123.47, 185.0, 220.0, 277.18],
        [98.0, 196.0, 246.94, 293.66],
        [110.0, 220.0, 277.18, 329.63],
      ];
      const harpPattern = [293.66, 369.99, 440.0, 554.37, 659.25, 554.37, 440.0, 369.99];

      return renderLoopPcm(16.0, 0.42, 0.55, 0.44, (t) => {
        const bar = Math.floor(t / 4.0) & 3;
        const barPhase = t % 4.0;
        const swell = 0.72 + 0.28 * Math.sin(TWO_PI * 0.25 * barPhase);
        const chord = chords[bar];

        const pad =
          (0.45 * Math.sin(TWO_PI * chord[0] * t) +
            0.35 * Math.sin(TWO_PI * chord[1] * t) +
            0.28 * Math.sin(TWO_PI * chord[2] * t) +
            0.22 * Math.sin(TWO_PI * chord[3] * t)) *
          0.24 *
          swell;

        const harpIdx = Math.floor(t / 0.5) & 7;
        const pluckT = t % 0.5;
        const hFreq = harpPattern[harpIdx];
        const harp =
          (Math.sin(TWO_PI * hFreq * pluckT) +
            0.4 * Math.sin(TWO_PI * (hFreq * 2.0) * pluckT) * Math.exp(-pluckT * 10.0) +
            0.15 * Math.sin(TWO_PI * (hFreq * 3.0) * pluckT) * Math.exp(-pluckT * 16.0)) *
          Math.exp(-pluckT * 5.2) *
          0.2;

        return pad + harp;
      });
    }

    case 'boss': {
      const organChords = [
        [73.42, 146.83, 174.61, 220.0],
        [58.27, 116.54, 174.61, 233.08],
        [49.0, 98.0, 146.83, 196.0],
        [55.0, 110.0, 138.59, 233.08],
      ];
      const choirMelody = [293.66, 349.23, 329.63, 293.66, 233.08, 261.63, 220.0, 277.18];

      return renderLoopPcm(16.0, 0.333, 0.48, 0.36, (t, idx) => {
        const bar = Math.floor(t / 4.0) & 3;
        const org = organChords[bar];

        const organVoice = (f: number) =>
          Math.sin(TWO_PI * f * t) +
          0.55 * Math.sin(TWO_PI * (f * 2.0) * t) +
          0.3 * Math.sin(TWO_PI * (f * 3.0) * t) +
          0.2 * Math.sin(TWO_PI * (f * 4.0) * t);

        const organ =
          (0.45 * organVoice(org[0]) +
            0.3 * organVoice(org[1]) +
            0.25 * organVoice(org[2]) +
            0.22 * organVoice(org[3])) *
          0.14;

        const melIdx = Math.floor(t / 2.0) & 7;
        const melNoteT = t % 2.0;
        const vibrato = Math.sin(TWO_PI * 5.4 * t) * 1.8;
        const f0 = choirMelody[melIdx] + vibrato;
        const h1 = Math.sin(TWO_PI * f0 * t);
        const h2 = Math.sin(TWO_PI * (f0 * 2.0) * t) * 0.85;
        const h3 = Math.sin(TWO_PI * (f0 * 3.0) * t) * 0.7;
        const h4 = Math.sin(TWO_PI * (f0 * 4.0) * t) * 0.45;
        const choirEnv =
          Math.sin(Math.min(1.0, melNoteT / 0.22) * 1.570796) * (1.0 - 0.25 * (melNoteT / 2.0));
        const choir = (h1 * 0.35 + h2 * 0.4 + h3 * 0.35 + h4 * 0.25) * choirEnv * 0.24;

        const beatT = t % 0.5;
        const subBeatT = t % 0.25;
        const subStep = Math.floor(t / 0.25) & 7;
        const timpaniFreq = org[0] + 65.0 * Math.exp(-beatT * 24.0);
        const timpani = Math.sin(TWO_PI * timpaniFreq * beatT) * Math.exp(-beatT * 8.5) * 0.36;

        let anvil = 0.0;
        if (subStep === 2 || subStep === 6 || subStep === 7) {
          const aEnv = Math.exp(-subBeatT * 26.0);
          anvil =
            (Math.sin(TWO_PI * 620.0 * subBeatT) +
              0.65 * Math.sin(TWO_PI * 1140.0 * subBeatT) +
              hashNoise(idx) * 0.5) *
            aEnv *
            0.22;
        }

        return organ + choir + timpani + anvil;
      });
    }
  }
}

export function encodeWavBlobFromFloat32(samples: Float32Array, sampleRate = SAMPLE_RATE): Blob {
  const numChannels = 1;
  const bytesPerSample = 2;
  const dataBytes = samples.length * bytesPerSample;
  const buffer = new ArrayBuffer(44 + dataBytes);
  const view = new DataView(buffer);

  const writeString = (offset: number, str: string) => {
    for (let i = 0; i < str.length; i++) {
      view.setUint8(offset + i, str.charCodeAt(i));
    }
  };

  writeString(0, 'RIFF');
  view.setUint32(4, 36 + dataBytes, true);
  writeString(8, 'WAVE');
  writeString(12, 'fmt ');
  view.setUint32(16, 16, true);
  view.setUint16(20, 1, true); // PCM
  view.setUint16(22, numChannels, true);
  view.setUint32(24, sampleRate, true);
  view.setUint32(28, sampleRate * numChannels * bytesPerSample, true);
  view.setUint16(32, numChannels * bytesPerSample, true);
  view.setUint16(34, 16, true);
  writeString(36, 'data');
  view.setUint32(40, dataBytes, true);

  let offset = 44;
  for (let i = 0; i < samples.length; i++) {
    const clamped = Math.max(-1, Math.min(1, samples[i]));
    const pcm16 = Math.round(clamped * 28500);
    view.setInt16(offset, pcm16, true);
    offset += 2;
  }

  return new Blob([buffer], { type: 'audio/wav' });
}

interface ActiveTrackNode {
  source: AudioBufferSourceNode;
  gain: GainNode;
}

class SoundSynthesizer {
  private ctx: AudioContext | null = null;
  private muted = false;
  private musicVolume = 0.72;
  private ostRunning = false;
  private mode: SoundtrackMode = 'adaptive';

  private pcmCache: Partial<Record<SoundtrackTrackId, Float32Array>> = {};
  private bufferCache: Partial<Record<SoundtrackTrackId, AudioBuffer>> = {};
  private activeTracks: Partial<Record<SoundtrackTrackId, ActiveTrackNode>> = {};

  private ensureContext(): AudioContext | null {
    if (this.muted) return null;
    if (!this.ctx) {
      const AudioCtx =
        window.AudioContext ||
        (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
      this.ctx = new AudioCtx();
    }
    if (this.ctx.state === 'suspended') {
      this.ctx.resume();
    }
    return this.ctx;
  }

  public setMuted(m: boolean) {
    this.muted = m;
    if (m) {
      this.syncTrackGains(0, false, false, false);
    } else if (this.ostRunning) {
      this.syncTrackGains(0, false, false, false);
    }
  }

  public isMuted(): boolean {
    return this.muted;
  }

  public setMusicVolume(vol0to1: number) {
    this.musicVolume = Math.max(0, Math.min(1, vol0to1));
  }

  public getMusicVolume(): number {
    return this.musicVolume;
  }

  public getMode(): SoundtrackMode {
    return this.mode;
  }

  public isOstRunning(): boolean {
    return this.ostRunning;
  }

  public getTrackPcm(id: SoundtrackTrackId): Float32Array {
    if (!this.pcmCache[id]) {
      this.pcmCache[id] = generateTrackPcm(id);
    }
    return this.pcmCache[id]!;
  }

  public exportTrackWavBlob(id: SoundtrackTrackId): Blob {
    const pcm = this.getTrackPcm(id);
    return encodeWavBlobFromFloat32(pcm, SAMPLE_RATE);
  }

  private ensureTrackBuffer(ctx: AudioContext, id: SoundtrackTrackId): AudioBuffer {
    if (!this.bufferCache[id]) {
      const pcm = this.getTrackPcm(id);
      const buf = ctx.createBuffer(1, pcm.length, SAMPLE_RATE);
      buf.getChannelData(0).set(pcm);
      this.bufferCache[id] = buf;
    }
    return this.bufferCache[id]!;
  }

  public startSoundtrack(initialMode?: SoundtrackMode) {
    if (initialMode) {
      this.mode = initialMode;
    }
    const ctx = this.ensureContext();
    if (!ctx) return;

    if (this.ostRunning) {
      this.syncTrackGains(0, false, false, false);
      return;
    }

    const trackIds: SoundtrackTrackId[] = ['explore', 'tension', 'pursuit', 'sanctum', 'boss'];
    const now = ctx.currentTime;

    for (const id of trackIds) {
      const buffer = this.ensureTrackBuffer(ctx, id);
      const source = ctx.createBufferSource();
      source.buffer = buffer;
      source.loop = true;

      const gain = ctx.createGain();
      gain.gain.setValueAtTime(0.0001, now);

      source.connect(gain);
      gain.connect(ctx.destination);
      source.start(now);

      this.activeTracks[id] = { source, gain };
    }

    this.ostRunning = true;
    this.syncTrackGains(0, false, false, false);
  }

  public stopSoundtrack() {
    if (!this.ostRunning) return;
    const trackIds: SoundtrackTrackId[] = ['explore', 'tension', 'pursuit', 'sanctum', 'boss'];
    for (const id of trackIds) {
      const node = this.activeTracks[id];
      if (node) {
        try {
          node.source.stop();
          node.source.disconnect();
          node.gain.disconnect();
        } catch {
          // ignore already stopped
        }
      }
      delete this.activeTracks[id];
    }
    this.ostRunning = false;
  }

  public setSoundtrackMode(mode: SoundtrackMode) {
    this.mode = mode;
    if (!this.ostRunning && !this.muted) {
      this.startSoundtrack(mode);
    } else {
      this.syncTrackGains(0, false, false, false);
    }
  }

  public updateAdaptiveMix(
    danger0to1: number,
    isPursuing: boolean,
    isSanctum: boolean,
    isBossChamber = false
  ) {
    if (!this.ostRunning) return;
    this.syncTrackGains(danger0to1, isPursuing, isSanctum, isBossChamber);
  }

  private syncTrackGains(
    danger0to1: number,
    isPursuing: boolean,
    isSanctum: boolean,
    isBossChamber: boolean
  ) {
    if (!this.ctx || !this.ostRunning) return;
    const now = this.ctx.currentTime;
    const master = this.muted ? 0 : this.musicVolume;

    const targets: Record<SoundtrackTrackId, number> = {
      explore: 0,
      tension: 0,
      pursuit: 0,
      sanctum: 0,
      boss: 0,
    };

    if (this.mode !== 'adaptive') {
      targets[this.mode] = 0.65 * master;
    } else {
      const bossWeight = isBossChamber ? 1.0 : 0.0;
      const pursuitWeight = isPursuing ? (isBossChamber ? 0.45 : 1.0) : 0.0;
      const tensionWeight =
        Math.max(0, Math.min(1, (danger0to1 - 0.12) / 0.75)) * (isBossChamber ? 0.35 : 1.0);
      const sanctumWeight =
        !isBossChamber && isSanctum && !isPursuing && danger0to1 < 0.25 ? 0.85 : 0.0;
      const exploreDuck = 1.0 - 0.55 * Math.max(bossWeight, pursuitWeight);

      targets.explore = 0.44 * exploreDuck * master;
      targets.tension = 0.52 * tensionWeight * master;
      targets.pursuit = 0.66 * pursuitWeight * master;
      targets.sanctum = 0.5 * sanctumWeight * master;
      targets.boss = 0.72 * bossWeight * master;
    }

    const trackIds: SoundtrackTrackId[] = ['explore', 'tension', 'pursuit', 'sanctum', 'boss'];
    for (const id of trackIds) {
      const node = this.activeTracks[id];
      if (node) {
        node.gain.gain.setTargetAtTime(targets[id], now, 0.18);
      }
    }
  }

  public playWaveEmission(sig: WaveSignature, enabled = true) {
    if (!enabled) return;
    this.playSignature(sig);
  }

  public playSurfaceReflectionSample(material: SurfaceMaterial, enabled = true) {
    if (!enabled) return;
    const ctx = this.ensureContext();
    if (!ctx) return;
    const now = ctx.currentTime;
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.connect(gain);
    gain.connect(ctx.destination);

    switch (material) {
      case 'Stone':
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(320, now);
        osc.frequency.exponentialRampToValueAtTime(180, now + 0.25);
        gain.gain.setValueAtTime(0.2, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.25);
        osc.start(now);
        osc.stop(now + 0.25);
        break;
      case 'Metal':
        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(840, now);
        osc.frequency.exponentialRampToValueAtTime(520, now + 0.45);
        gain.gain.setValueAtTime(0.22, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.45);
        osc.start(now);
        osc.stop(now + 0.45);
        break;
      case 'DampMoss':
        osc.type = 'sine';
        osc.frequency.setValueAtTime(160, now);
        osc.frequency.exponentialRampToValueAtTime(95, now + 0.14);
        gain.gain.setValueAtTime(0.14, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.14);
        osc.start(now);
        osc.stop(now + 0.14);
        break;
      case 'Water':
        osc.type = 'sine';
        osc.frequency.setValueAtTime(520, now);
        osc.frequency.exponentialRampToValueAtTime(760, now + 0.35);
        gain.gain.setValueAtTime(0.18, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.35);
        osc.start(now);
        osc.stop(now + 0.35);
        break;
      case 'ResonanceDoor':
        osc.type = 'sine';
        osc.frequency.setValueAtTime(1180, now);
        osc.frequency.exponentialRampToValueAtTime(880, now + 0.65);
        gain.gain.setValueAtTime(0.24, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.65);
        osc.start(now);
        osc.stop(now + 0.65);
        break;
    }
  }

  public setMusicMode(mode: 'auto' | SoundtrackTrackId, enabled = true) {
    this.setMuted(!enabled);
    const mapped: SoundtrackMode = mode === 'auto' ? 'adaptive' : mode;
    this.startSoundtrack(mapped);
    this.setSoundtrackMode(mapped);
  }

  public stopMusic() {
    this.stopSoundtrack();
  }

  public setMusicMasterVolume(vol0to1: number) {
    this.setMusicVolume(vol0to1);
    if (this.ostRunning) {
      this.syncTrackGains(0, false, false, false);
    }
  }

  public updateAdaptiveMusic(
    danger0to1: number,
    isPursuing: boolean,
    isSanctum: boolean,
    isBossChamber: boolean,
    audioEnabled: boolean
  ) {
    this.setMuted(!audioEnabled);
    if (!audioEnabled) return;
    this.updateAdaptiveMix(danger0to1, isPursuing, isSanctum, isBossChamber);
  }

  public exportTrackWav(id: SoundtrackTrackId) {
    const meta = SOUNDTRACK_CATALOG.find((t) => t.id === id);
    const blob = this.exportTrackWavBlob(id);
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = meta ? meta.filename : `${id}.wav`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    setTimeout(() => URL.revokeObjectURL(url), 2500);
  }

  public playSignature(sig: WaveSignature) {
    const ctx = this.ensureContext();
    if (!ctx) return;

    const now = ctx.currentTime;
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.connect(gain);
    gain.connect(ctx.destination);

    switch (sig) {
      case 'DirectionalPing':
        osc.type = 'sine';
        osc.frequency.setValueAtTime(580, now);
        osc.frequency.exponentialRampToValueAtTime(340, now + 0.42);
        gain.gain.setValueAtTime(0.22, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.42);
        osc.start(now);
        osc.stop(now + 0.42);
        break;

      case 'RadialPulse':
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(220, now);
        osc.frequency.exponentialRampToValueAtTime(110, now + 0.78);
        gain.gain.setValueAtTime(0.3, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.78);
        osc.start(now);
        osc.stop(now + 0.78);
        break;

      case 'FootstepSneak':
        osc.type = 'sine';
        osc.frequency.setValueAtTime(75, now);
        osc.frequency.exponentialRampToValueAtTime(42, now + 0.1);
        gain.gain.setValueAtTime(0.08, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.1);
        osc.start(now);
        osc.stop(now + 0.1);
        break;

      case 'FootstepWalk':
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(118, now);
        osc.frequency.exponentialRampToValueAtTime(52, now + 0.14);
        gain.gain.setValueAtTime(0.15, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.14);
        osc.start(now);
        osc.stop(now + 0.14);
        break;

      case 'FootstepSprint':
        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(175, now);
        osc.frequency.exponentialRampToValueAtTime(64, now + 0.19);
        gain.gain.setValueAtTime(0.22, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.19);
        osc.start(now);
        osc.stop(now + 0.19);
        break;

      case 'DecoyImpact':
        osc.type = 'square';
        osc.frequency.setValueAtTime(780, now);
        osc.frequency.setValueAtTime(920, now + 0.09);
        gain.gain.setValueAtTime(0.24, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.55);
        osc.start(now);
        osc.stop(now + 0.55);
        break;

      case 'MechanismClunk':
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(140, now);
        osc.frequency.setValueAtTime(440, now + 0.12);
        gain.gain.setValueAtTime(0.28, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.65);
        osc.start(now);
        osc.stop(now + 0.65);
        break;

      case 'EnemyTremor':
        osc.type = 'sine';
        osc.frequency.setValueAtTime(64, now);
        osc.frequency.exponentialRampToValueAtTime(36, now + 0.28);
        gain.gain.setValueAtTime(0.18, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.28);
        osc.start(now);
        osc.stop(now + 0.28);
        break;
    }
  }
}

export const OST_TRACKS = SOUNDTRACK_CATALOG;
export const soundSynth = new SoundSynthesizer();
