#include "Game.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace echo {

namespace {
float distSqNoise(sf::Vector2f a, sf::Vector2f b) noexcept {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return dx * dx + dy * dy;
}

sf::Vector2f closestPointOnSegNoise(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) noexcept {
    const sf::Vector2f ab = b - a;
    const float lenSq = ab.x * ab.x + ab.y * ab.y;
    if (lenSq < 1e-6f) return a;
    const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lenSq, 0.0f, 1.0f);
    return a + ab * t;
}
} // namespace

void Game::registerNoiseAction(NoiseAction action, float dt, sf::Vector2f stimulusOrigin) {
    const bool isFootstep =
        (action == NoiseAction::StepSneak || action == NoiseAction::StepWalk || action == NoiseAction::StepSprint);
    const DifficultyProfile diff = getDifficultyProfile(m_difficulty);
    float mult = m_player.noiseGainMultiplier(isFootstep) * diff.noiseGainMult;

    const auto& room = m_labyrinth.currentRoom();
    const BiomeSpec bSpec = getBiomeSpec(room.biome);

    // Stage 2: Biome footstep acoustic modifier (DrownedGalleries muffles steps x0.75, LivingMycelium x0.80)
    if (isFootstep) {
        mult *= bSpec.stepNoiseMult;
        // Stage 2: Iron Foundry periodic steam-hammer press masks player footsteps by -75% while active
        if (room.biome == BiomeId::IronFoundry && isFoundryHammerMaskingActive()) {
            mult *= 0.25f;
        }
    }

    // Block 2.1: Standing inside a DampMoss shelter dampens footstep noise in half
    if (isFootstep && AcousticSystem::isInDampMossShelter(m_player.position(), room.walls, room.floorMaterial)) {
        mult *= 0.50f;
    }
    // Block 5.1: ResonanceOvercharge chamber increases noise gain by +20%
    if (room.roomEvent == RoomEvent::ResonanceOvercharge) {
        mult *= 1.20f;
    }

    const float prevNoise = m_noiseLevel;
    m_noiseLevel = applyNoiseAction(m_noiseLevel, action, dt, mult);
    m_roomPeakNoise = std::max(m_roomPeakNoise, m_noiseLevel);

    // Stage 1.5: Track last meaningful noise cause for HUD readability
    if (action != NoiseAction::SilenceDecay && m_noiseLevel > prevNoise + 0.05f) {
        m_lastNoiseAction = action;
        if (isFootstep) {
            m_lastNoiseDelta = (action == NoiseAction::StepSprint) ? (cfg::noise::kSprintNoisePerSec * mult)
                             : (action == NoiseAction::StepWalk)   ? (cfg::noise::kWalkNoisePerSec * mult)
                                                                   : (cfg::noise::kSneakNoisePerSec * mult);
        } else {
            m_lastNoiseDelta = m_noiseLevel - prevNoise;
        }
        m_lastNoiseOrigin = stimulusOrigin;
        m_lastNoiseSourceTimer = 3.2f;
    }

    // Stage 3: Trigger GLSL radial shockwave distortion & closed-caption subtitles on major acoustic pulses
    if (action == NoiseAction::RadialPulse ||
        action == NoiseAction::StompImpact ||
        action == NoiseAction::GlassShatter ||
        action == NoiseAction::BossDissonanceShock ||
        action == NoiseAction::BansheeWail) {
        m_postFxShockwaveTimer = 0.55f;
        m_postFxShockwaveOrigin = stimulusOrigin;
    }

    if (action == NoiseAction::RadialPulse) {
        pushSoundCaption("RADIAL SONAR PULSE (+30 NOISE)", stimulusOrigin, sf::Color(56, 189, 248));
    } else if (action == NoiseAction::StompImpact) {
        pushSoundCaption("SEISMIC STOMP SHOCKWAVE (+35 NOISE)", stimulusOrigin, sf::Color(251, 146, 60));
    } else if (action == NoiseAction::GlassShatter) {
        pushSoundCaption("BRITTLE GLASS SHATTERED (+40 NOISE)", stimulusOrigin, sf::Color(244, 63, 94));
    } else if (action == NoiseAction::DecoyImpact) {
        pushSoundCaption("DECOY STONE IMPACT (+15 NOISE)", stimulusOrigin, sf::Color(167, 139, 250));
    } else if (action == NoiseAction::WaterSplash) {
        pushSoundCaption("HYDRO-ACOUSTIC WATER SPLASH (+15 NOISE)", stimulusOrigin, sf::Color(56, 189, 248));
    } else if (action == NoiseAction::BansheeWail) {
        pushSoundCaption("PIERCING BANSHEE WAIL (+22 NOISE)", stimulusOrigin, sf::Color(244, 63, 94));
    } else if (action == NoiseAction::LeechDrain) {
        pushSoundCaption("ACOUSTIC LEECH PARASITIC DRAIN (+8 NOISE)", stimulusOrigin, sf::Color(250, 204, 21));
    } else if (action == NoiseAction::BossDissonanceShock) {
        pushSoundCaption("CHOIR SOVEREIGN DISSONANCE SHOCKWAVE (+15 NOISE)", stimulusOrigin, sf::Color(244, 63, 94));
    }

    if (action == NoiseAction::DirectionalPing ||
        action == NoiseAction::RadialPulse ||
        action == NoiseAction::DecoyImpact ||
        action == NoiseAction::CombatHit ||
        action == NoiseAction::StepSprint ||
        action == NoiseAction::WhistlePing ||
        action == NoiseAction::StompImpact ||
        action == NoiseAction::GlassShatter ||
        action == NoiseAction::WaterSplash ||
        action == NoiseAction::EchoSwarmAmplify ||
        action == NoiseAction::EchoStormPulse ||
        action == NoiseAction::BossDissonanceShock ||
        action == NoiseAction::BansheeWail ||
        action == NoiseAction::LeechDrain) {
        m_noiseSilenceGraceTimer = 0.35f;
    }

    const bool isGroundVib =
        (action == NoiseAction::StepSprint ||
         action == NoiseAction::RadialPulse ||
         action == NoiseAction::StompImpact ||
         action == NoiseAction::GlassShatter ||
         action == NoiseAction::WaterSplash ||
         action == NoiseAction::BossDissonanceShock);

    const NoiseTier tier = classifyNoiseTier(m_noiseLevel);
    if (tier != NoiseTier::Quiet) {
        for (auto& enemy : m_enemies) {
            if (enemy.isActive()) {
                const sf::Vector2f target = (tier == NoiseTier::Scream) ? m_player.position() : stimulusOrigin;
                enemy.applyNoiseTierStimulus(target, tier, isGroundVib);
            }
        }
    }
}

void Game::executePlayerStomp() {
    if (!m_player.triggerStomp(m_acoustics)) return;

    if (m_audio) m_audio->playWaveSignature(WaveSignature::StompWave);
    triggerCameraShake(cfg::feel::kShakeAmplitudeDamage, 0.28f);
    registerNoiseAction(NoiseAction::StompImpact, 1.0f, m_player.position());

    auto& room = m_labyrinth.currentRoomMut();
    const sf::Vector2f pPos = m_player.position();

    // Block 3.2: Stomp pushes nearby enemies back and staggers them (also detaches any Leech!)
    int pushedEnemies = 0;
    for (auto& enemy : m_enemies) {
        if (enemy.isActive()) {
            enemy.detachLeech(pPos);
            if (enemy.applyStompKnockback(pPos, cfg::kStompRadius, cfg::kStompKnockback, room.walls)) {
                ++pushedEnemies;
            }
        }
    }

    // Block 3.2 & 2.3: Stomp shatters any Brittle Glass walls within Stomp radius
    bool shatteredAnyGlass = false;
    for (auto& w : room.walls) {
        if (!w.isOpen && w.material == SurfaceMaterial::Glass) {
            const sf::Vector2f c = closestPointOnSegNoise(pPos, w.a, w.b);
            if (distSqNoise(pPos, c) <= cfg::kStompRadius * cfg::kStompRadius) {
                w.isOpen = true;
                shatteredAnyGlass = true;
                if (w.doorId != -1) {
                    m_labyrinth.activateSwitch(w.doorId);
                }
            }
        }
    }

    if (shatteredAnyGlass) {
        registerNoiseAction(NoiseAction::GlassShatter, 1.0f, pPos);
        if (m_audio) m_audio->playWaveSignature(WaveSignature::GlassShatterWave);
        m_statusBannerText = "STOMP SHATTERED BRITTLE GLASS BARRIER (+40 NOISE)!";
        m_statusBannerTimer = 3.0f;
    } else if (pushedEnemies > 0) {
        m_statusBannerText = "SEISMIC STOMP REPELLED " + std::to_string(pushedEnemies) + " HUNTER(S) (+35 NOISE)";
        m_statusBannerTimer = 2.4f;
    }
}

void Game::updateNoiseSystem(float dt) {
    m_noiseSilenceGraceTimer = std::max(0.0f, m_noiseSilenceGraceTimer - dt);
    m_noiseIntrusionCooldown = std::max(0.0f, m_noiseIntrusionCooldown - dt);
    m_lastNoiseSourceTimer = std::max(0.0f, m_lastNoiseSourceTimer - dt);

    if (m_player.movedLastFrame()) {
        NoiseAction stepAct = NoiseAction::StepWalk;
        if (m_player.activeStance() == MovementStance::Sneak) {
            stepAct = NoiseAction::StepSneak;
        } else if (m_player.activeStance() == MovementStance::Sprint) {
            stepAct = NoiseAction::StepSprint;
        }
        registerNoiseAction(stepAct, dt, m_player.position());
    }

    // Silence decay (-10/sec base, scaled by difficulty, Biome, DampMoss shelter, and PhaseDampener relic)
    if ((!m_player.movedLastFrame() || m_player.activeStance() == MovementStance::Sneak) &&
        m_noiseSilenceGraceTimer <= 0.0f) {
        const auto& curRoom = m_labyrinth.currentRoom();
        const bool inMossShelter = AcousticSystem::isInDampMossShelter(
            m_player.position(),
            curRoom.walls,
            curRoom.floorMaterial
        );
        const DifficultyProfile diff = getDifficultyProfile(m_difficulty);
        const BiomeSpec bSpec = getBiomeSpec(curRoom.biome);
        const float decayMult = (inMossShelter ? 2.0f : 1.0f) *
                                bSpec.silenceDecayMult *
                                m_player.silenceDecayMultiplier() *
                                diff.silenceDecayMult;
        m_noiseLevel = applyNoiseAction(m_noiseLevel, NoiseAction::SilenceDecay, dt, decayMult);
    }

    // Cross-room sound propagation through open doors (50% attenuation per hop, up to 2 rooms)
    const int curRoomId = m_labyrinth.currentRoomId();
    auto& room = m_labyrinth.currentRoomMut();
    const auto leaks = m_labyrinth.propagateNoiseFromRoom(curRoomId, m_noiseLevel, 2);

    // Record cross-room acoustic alert state on neighboring rooms so enemies inside react
    for (const auto& leak : leaks) {
        if (leak.leakedNoise > cfg::noise::kThresholdAudible) {
            auto& nbRoom = m_labyrinth.roomAtMut(static_cast<std::size_t>(leak.roomId));
            if (leak.leakedNoise >= nbRoom.pendingNoiseAlertLevel) {
                nbRoom.pendingNoiseAlertLevel = leak.leakedNoise;
                nbRoom.pendingNoiseAlertDir = leak.entryDirInTarget;
            }
        }
    }

    // Block 1.3: Neighboring Hunter Room Breach when loud/scream noise leaks through open doorways
    if (room.cleared && !room.noiseIntrusionConsumed &&
        room.type != RoomType::Start && room.type != RoomType::Sanctuary &&
        room.type != RoomType::FinalExit && m_noiseIntrusionCooldown <= 0.0f &&
        m_noiseLevel >= cfg::noise::kThresholdLoud) {
        for (const auto& leak : leaks) {
            if (leak.gridDistance == 1 && leak.leakedNoise > cfg::noise::kThresholdAudible) {
                const auto& nbRoom = m_labyrinth.roomAt(static_cast<std::size_t>(leak.roomId));
                if ((!nbRoom.cleared && !nbRoom.enemySpawns.empty()) ||
                    (!nbRoom.enemySpawns.empty() && m_noiseLevel >= cfg::noise::kThresholdScream)) {
                    const CardinalDir doorwayInCurrent = oppositeDir(leak.entryDirInTarget);
                    room.pendingNoiseAlertDir = doorwayInCurrent;
                    if (m_labyrinth.reopenRoomForNoiseIntrusion(curRoomId)) {
                        m_noiseIntrusionCooldown = 14.0f;
                        const sf::Vector2f entryPos = Labyrinth::spawnPositionForEntry(doorwayInCurrent);
                        const DifficultyProfile diff = getDifficultyProfile(m_difficulty);

                        EnemyAI intruder;
                        intruder.configurePatrol(
                            {entryPos, m_player.position(), {640.0f, 400.0f}},
                            (m_noiseLevel >= cfg::noise::kThresholdScream) ? EnemyVariant::Phantom : EnemyVariant::Stalker
                        );
                        intruder.applyDifficultyScaling(diff.enemyHpMult, diff.enemySpeedMult);
                        intruder.applyNoiseTierStimulus(m_player.position(), classifyNoiseTier(m_noiseLevel), true);
                        m_enemies.push_back(std::move(intruder));

                        if (m_audio) m_audio->playEffect(SoundEffectId::EnemyHuntRoar, 0.95f, 0.92f);
                        triggerCameraShake(cfg::feel::kShakeAmplitudeAlert, 0.35f);
                        m_statusBannerText = "NOISE BREACH! NEIGHBORING HUNTER ATTRACTED THROUGH OPEN DOORWAY!";
                        m_statusBannerTimer = 3.8f;
                    }
                    break;
                }
            }
        }
    }
}

} // namespace echo
