import { SurfaceMaterial, WaveSignature } from './types';

class SoundSynthesizer {
  private ctx: AudioContext | null = null;

  private ensureContext(): AudioContext | null {
    if (typeof window === 'undefined') return null;
    if (!this.ctx) {
      const AudioCtx = window.AudioContext || (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
      if (AudioCtx) {
        this.ctx = new AudioCtx();
      }
    }
    if (this.ctx && this.ctx.state === 'suspended') {
      this.ctx.resume().catch(() => {});
    }
    return this.ctx;
  }

  public playWaveEmission(signature: WaveSignature, enabled: boolean): void {
    if (!enabled) return;
    const ctx = this.ensureContext();
    if (!ctx) return;

    const now = ctx.currentTime;
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();

    osc.connect(gain);
    gain.connect(ctx.destination);

    switch (signature) {
      case 'DirectionalPing': {
        osc.type = 'sine';
        osc.frequency.setValueAtTime(528, now);
        osc.frequency.exponentialRampToValueAtTime(396, now + 0.36);
        gain.gain.setValueAtTime(0.14, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.38);
        osc.start(now);
        osc.stop(now + 0.4);
        break;
      }
      case 'RadialPulse': {
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(264, now);
        osc.frequency.exponentialRampToValueAtTime(132, now + 0.65);
        gain.gain.setValueAtTime(0.2, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.68);
        osc.start(now);
        osc.stop(now + 0.7);
        break;
      }
      case 'FootstepSneak': {
        osc.type = 'sine';
        osc.frequency.setValueAtTime(95, now);
        osc.frequency.exponentialRampToValueAtTime(48, now + 0.08);
        gain.gain.setValueAtTime(0.035, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.09);
        osc.start(now);
        osc.stop(now + 0.1);
        break;
      }
      case 'FootstepWalk': {
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(135, now);
        osc.frequency.exponentialRampToValueAtTime(62, now + 0.12);
        gain.gain.setValueAtTime(0.075, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.13);
        osc.start(now);
        osc.stop(now + 0.14);
        break;
      }
      case 'FootstepSprint': {
        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(180, now);
        osc.frequency.exponentialRampToValueAtTime(70, now + 0.16);
        gain.gain.setValueAtTime(0.12, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.17);
        osc.start(now);
        osc.stop(now + 0.18);
        break;
      }
      case 'DecoyImpact': {
        osc.type = 'sine';
        osc.frequency.setValueAtTime(740, now);
        osc.frequency.exponentialRampToValueAtTime(290, now + 0.45);
        gain.gain.setValueAtTime(0.18, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.48);
        osc.start(now);
        osc.stop(now + 0.5);
        break;
      }
      case 'MechanismClunk': {
        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(220, now);
        osc.frequency.setValueAtTime(330, now + 0.12);
        osc.frequency.exponentialRampToValueAtTime(440, now + 0.55);
        gain.gain.setValueAtTime(0.16, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.58);
        osc.start(now);
        osc.stop(now + 0.6);
        break;
      }
      case 'EnemyTremor': {
        osc.type = 'sine';
        osc.frequency.setValueAtTime(68, now);
        osc.frequency.exponentialRampToValueAtTime(38, now + 0.28);
        gain.gain.setValueAtTime(0.09, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.3);
        osc.start(now);
        osc.stop(now + 0.32);
        break;
      }
    }
  }

  public playSurfaceReflectionSample(material: SurfaceMaterial, enabled: boolean): void {
    if (!enabled) return;
    const ctx = this.ensureContext();
    if (!ctx) return;

    const now = ctx.currentTime;
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.connect(gain);
    gain.connect(ctx.destination);

    const freqMap: Record<SurfaceMaterial, number> = {
      Stone: 440,
      Metal: 880,
      Water: 330,
      DampMoss: 180,
      ResonanceDoor: 660,
    };

    osc.type = material === 'Metal' ? 'triangle' : 'sine';
    osc.frequency.setValueAtTime(freqMap[material], now);
    gain.gain.setValueAtTime(0.08, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.25);
    osc.start(now);
    osc.stop(now + 0.26);
  }
}

export const soundSynth = new SoundSynthesizer();
