#include "Game.hpp"
#include <SFML/System/Clock.hpp>
#include <algorithm>
#include <cmath>

namespace echo {

namespace {
float distSq2D(sf::Vector2f a, sf::Vector2f b) noexcept {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return dx * dx + dy * dy;
}
} // namespace

Game::Game(const std::string& exeDir)
    : m_exeDir(exeDir)
    , m_saveFilePath(SaveSystem::resolveSaveFilePath(exeDir, "echolabyrinth_save.dat"))
    , m_worldView(sf::FloatRect(0.0f, 0.0f, cfg::kWorldWidth, cfg::kWorldHeight)) {

    m_particles.resize(64);
    for (std::size_t i = 0; i < m_particles.size(); ++i) {
        m_particles[i].pos = {
            static_cast<float>((i * 197) % static_cast<int>(cfg::kWorldWidth)),
            static_cast<float>((i * 131) % static_cast<int>(cfg::kWorldHeight))
        };
        m_particles[i].vel = {
            static_cast<float>(static_cast<int>(i % 5) - 2) * 5.0f,
            static_cast<float>(static_cast<int>(i % 3) - 1) * 4.0f
        };
        m_particles[i].size = (i % 2 == 0) ? 2.0f : 1.5f;
    }

    SaveData existing{};
    if (SaveSystem::loadFromFile(m_saveFilePath, existing)) {
        m_metaTotalRuns = existing.totalRuns;
        m_metaTotalVictories = existing.totalVictories;
        m_metaBossesDefeated = existing.bossesDefeated;
        m_metaTotalGhostClears = existing.metaTotalGhostClears;
        m_discoveredCodexIds = existing.discoveredCodexIds;
        m_difficulty = static_cast<DifficultyMode>(std::clamp(existing.difficulty, 0, 3));
        m_tutorialDone = existing.tutorialDone;
        m_tutorialRoomStage = m_tutorialDone ? 3 : 0;
    }
}

void Game::triggerCameraShake(float amplitude, float durationSec) noexcept {
    if (amplitude >= m_cameraShakeAmplitude || m_cameraShakeTimer <= 0.02f) {
        m_cameraShakeAmplitude = amplitude;
        m_cameraShakeElapsed = 0.0f;
    }
    m_cameraShakeTimer = std::max(m_cameraShakeTimer, durationSec);
}

void Game::updateTutorialProgression() {
    if (m_tutorialDone) {
        m_tutorialRoomStage = 3;
        return;
    }

    if (m_tutorialRoomStage == 0) {
        if (m_tutStepMoved && m_tutStepSneaked) {
            m_tutorialRoomStage = 1;
        }
    } else if (m_tutorialRoomStage == 1) {
        if (m_tutStepPinged) {
            m_tutorialRoomStage = 2;
        }
    } else if (m_tutorialRoomStage == 2) {
        if (m_tutStepTrappedOrDecoy) {
            m_tutorialRoomStage = 3;
            m_tutorialDone = true;
            m_statusBannerText = "ACOUSTIC ONBOARDING COMPLETE — SURVIVE THE ABYSS!";
            m_statusBannerTimer = 3.2f;
            saveCheckpoint();
        }
    }
}

int Game::run() {
    m_window = std::make_unique<sf::RenderWindow>(
        sf::VideoMode(cfg::kDefaultWindowWidth, cfg::kDefaultWindowHeight),
        "EchoLabyrinth 5.0 'Resonance' — Acoustic Roguelike Horror",
        sf::Style::Default
    );
    m_window->setFramerateLimit(120);
    updateLetterboxView(cfg::kDefaultWindowWidth, cfg::kDefaultWindowHeight);

    m_assets.initialize(m_exeDir, true);
    m_ui.initialize(m_exeDir);
    m_audio = std::make_unique<AudioManager>();
    m_audio->initialize(m_exeDir);
    m_audio->startAmbientDrone();

    enterRoom(0, m_labyrinth.currentRoom().playerDefaultSpawn, false);
    m_state = AppState::MainMenu;
    rebuildMenuButtons();

    sf::Clock clock;
    float accumulator = 0.0f;

    while (m_window && m_window->isOpen()) {
        handleEvents();

        const float frameDt = std::min(clock.restart().asSeconds(), 0.1f);
        accumulator += frameDt;

        while (accumulator >= cfg::kFixedTimeStep) {
            updateFixed(cfg::kFixedTimeStep);
            accumulator -= cfg::kFixedTimeStep;
        }

        render();
    }

    if (m_audio) {
        m_audio->stopAmbientDrone();
    }
    return 0;
}

void Game::updateLetterboxView(unsigned int winW, unsigned int winH) {
    if (winW == 0 || winH == 0) return;
    const float windowRatio = static_cast<float>(winW) / static_cast<float>(winH);
    const float viewRatio = cfg::kWorldWidth / cfg::kWorldHeight;

    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float posX = 0.0f;
    float posY = 0.0f;

    if (windowRatio > viewRatio) {
        sizeX = viewRatio / windowRatio;
        posX = (1.0f - sizeX) * 0.5f;
    } else {
        sizeY = windowRatio / viewRatio;
        posY = (1.0f - sizeY) * 0.5f;
    }

    m_worldView.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
}

void Game::toggleFullscreen() {
    if (!m_window) return;
    m_isFullscreen = !m_isFullscreen;
    if (m_isFullscreen) {
        m_window->create(
            sf::VideoMode::getDesktopMode(),
            "EchoLabyrinth 5.0 'Resonance' — Acoustic Roguelike Horror",
            sf::Style::Fullscreen
        );
    } else {
        m_window->create(
            sf::VideoMode(cfg::kDefaultWindowWidth, cfg::kDefaultWindowHeight),
            "EchoLabyrinth 5.0 'Resonance' — Acoustic Roguelike Horror",
            sf::Style::Default
        );
    }
    m_window->setFramerateLimit(120);
    const sf::Vector2u sz = m_window->getSize();
    updateLetterboxView(sz.x, sz.y);
}

void Game::updateFixed(float dt) {
    for (auto& pt : m_particles) {
        pt.pos += pt.vel * dt;
        if (pt.pos.x < 0.0f) pt.pos.x += cfg::kWorldWidth;
        if (pt.pos.x > cfg::kWorldWidth) pt.pos.x -= cfg::kWorldWidth;
        if (pt.pos.y < 0.0f) pt.pos.y += cfg::kWorldHeight;
        if (pt.pos.y > cfg::kWorldHeight) pt.pos.y -= cfg::kWorldHeight;
    }

    if (m_state == AppState::Playing) {
        updatePlaying(dt);
    } else {
        updateMenuBackground(dt);
    }
}

void Game::updateMenuBackground(float dt) {
    m_menuPulseTimer += dt;
    if (m_menuPulseTimer >= 2.4f) {
        m_menuPulseTimer = 0.0f;
        const sf::Vector2f origin{
            340.0f + std::cos(m_runTimeSec + 1.2f) * 260.0f,
            400.0f + std::sin(m_runTimeSec * 0.7f) * 140.0f
        };
        m_acoustics.emitPulse(origin, 360.0f, 580.0f, 0.85f, 84, WaveSignature::RadialPulse, 0.0f, 6.2831853f, false);
    }
    m_acoustics.update(dt, m_labyrinth.currentRoomMut().walls);
    m_acoustics.clearRecentEvents();
}

void Game::updatePlaying(float dt) {
    m_runTimeSec += dt;
    m_statusBannerTimer = std::max(0.0f, m_statusBannerTimer - dt);
    m_loreDisplayTimer = std::max(0.0f, m_loreDisplayTimer - dt);
    if (m_cameraShakeTimer > 0.0f) {
        m_cameraShakeTimer = std::max(0.0f, m_cameraShakeTimer - dt);
        m_cameraShakeElapsed += dt;
    } else {
        m_cameraShakeAmplitude = 0.0f;
        m_cameraShakeElapsed = 0.0f;
    }
    m_shaleCrackCooldown = std::max(0.0f, m_shaleCrackCooldown - dt);

    // Stage 1.1: Hit-stop freeze window (30-60 ms) on sonar hit against an enemy
    if (m_hitStopTimer > 0.0f) {
        m_hitStopTimer = std::max(0.0f, m_hitStopTimer - dt);
        return;
    }

    sf::Vector2f moveInput{0.0f, 0.0f};
    MovementStance stance = MovementStance::Walk;
    if (m_window && m_window->isOpen()) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) moveInput.y -= 1.0f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) moveInput.y += 1.0f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) moveInput.x -= 1.0f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) moveInput.x += 1.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
            stance = MovementStance::Sneak;
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
            stance = MovementStance::Sprint;
        }
    }

    auto& room = m_labyrinth.currentRoomMut();
    const BiomeSpec bSpec = getBiomeSpec(room.biome);
    m_labyrinth.updateAnimations(dt);

    // Stage 2: Iron Foundry periodic steam-hammer press (masks player footsteps by -75% for 1.35s every 5.5s)
    if (room.biome == BiomeId::IronFoundry) {
        m_foundryHammerMaskTimer = std::max(0.0f, m_foundryHammerMaskTimer - dt);
        m_foundryHammerTimer += dt;
        if (m_foundryHammerTimer >= 5.5f) {
            m_foundryHammerTimer = 0.0f;
            m_foundryHammerMaskTimer = 1.35f;
            m_acoustics.emitPulse({640.0f, 400.0f}, 390.0f, 520.0f, 0.80f, 64, WaveSignature::MechanismClunk, 0.0f, 6.2831853f, false);
            if (m_audio) m_audio->playEffect(SoundEffectId::DistantMetalClang, 0.78f, 0.72f);
        }
    } else {
        m_foundryHammerTimer = 0.0f;
        m_foundryHammerMaskTimer = 0.0f;
    }

    // Block 5.1: Dynamic RoomEvent — EchoStorm periodic ambient sonar sweep every 3.5s
    if (room.roomEvent == RoomEvent::EchoStorm && !room.cleared) {
        m_echoStormTimer += dt;
        if (m_echoStormTimer >= 3.5f) {
            m_echoStormTimer = 0.0f;
            m_acoustics.emitPulse({640.0f, 400.0f}, 440.0f, 700.0f, 0.95f, 96, WaveSignature::ResonatorHum, 0.0f, 6.2831853f, false);
            registerNoiseAction(NoiseAction::EchoStormPulse, 1.0f, {640.0f, 400.0f});
            if (m_audio) m_audio->playWaveSignature(WaveSignature::ResonatorHum);
        }
    }

    WaveSignature emittedSig{};
    if (m_player.update(dt, moveInput, stance, m_mouseWorldPos, room.walls, m_acoustics, emittedSig)) {
        if (m_audio) m_audio->updateListener(m_player.position(), m_player.facingAngleRad());

        if (emittedSig == WaveSignature::FootstepSneak ||
            emittedSig == WaveSignature::FootstepWalk ||
            emittedSig == WaveSignature::FootstepSprint) {
            SurfaceMaterial floorMat = m_player.detectFloorMaterial(room.walls, room.floorMaterial);

            bool steppedInPuddle = (floorMat == SurfaceMaterial::Water);
            if (!steppedInPuddle) {
                for (const auto& prop : room.props) {
                    if (prop.type == PropType::WaterPuddle &&
                        distSq2D(m_player.position(), prop.position) <= (prop.scale * 1.25f) * (prop.scale * 1.25f)) {
                        steppedInPuddle = true;
                        floorMat = SurfaceMaterial::Water;
                        break;
                    }
                }
            }

            if (m_audio) {
                m_audio->playMaterialFootstep(
                    floorMat,
                    emittedSig == WaveSignature::FootstepSneak,
                    emittedSig == WaveSignature::FootstepSprint,
                    m_player.position()
                );
            }

            if (steppedInPuddle && emittedSig != WaveSignature::FootstepSneak && !m_player.hasRelic(RelicId::MossTreadBoots)) {
                m_acoustics.emitWaterRipples(m_player.position(), 240.0f * bSpec.soundRangeMult);
                registerNoiseAction(NoiseAction::WaterSplash, 1.0f, m_player.position());
            } else if (room.biome == BiomeId::DrownedGalleries && emittedSig == WaveSignature::FootstepSneak) {
                // Stage 2: Drowned Galleries — soft ripples visualize muffled footsteps on water
                m_acoustics.emitWaterRipples(m_player.position(), 130.0f);
            }
        } else {
            if (m_audio) m_audio->playWaveSignature(emittedSig);
            if (emittedSig == WaveSignature::DecoyImpact) {
                registerNoiseAction(NoiseAction::DecoyImpact, 1.0f, m_mouseWorldPos);
            } else if (emittedSig == WaveSignature::EchoTrapPulse) {
                const sf::Vector2f trapPos = m_player.activeEchoTrap().position;
                for (auto& enemy : m_enemies) {
                    if (enemy.isActive()) {
                        enemy.applyNoiseTierStimulus(trapPos, NoiseTier::Loud, true);
                    }
                }
                m_statusBannerText = "ECHO BEACON MINE DETONATED — HUNTERS LURED TO TRAP!";
                m_statusBannerTimer = 2.4f;
            }
        }

        if (!m_player.hasRelic(RelicId::MossTreadBoots) &&
            (emittedSig == WaveSignature::FootstepWalk || emittedSig == WaveSignature::FootstepSprint) &&
            m_shaleCrackCooldown <= 0.0f) {
            for (const auto& hz : room.hazards) {
                if (distSq2D(m_player.position(), hz.position) <= hz.radius * hz.radius) {
                    m_shaleCrackCooldown = 1.15f;
                    m_acoustics.emitPulse(
                        m_player.position(),
                        400.0f,
                        520.0f,
                        1.15f,
                        88,
                        WaveSignature::ShaleCrack,
                        0.0f,
                        6.2831853f,
                        true
                    );
                    registerNoiseAction(NoiseAction::RadialPulse, 1.0f, m_player.position());
                    if (m_audio) m_audio->playWaveSignature(WaveSignature::ShaleCrack);
                    m_statusBannerText = "WARNING: BRITTLE SHALE FRACTURED — HIGH-VOLUME ECHO EMITTED!";
                    m_statusBannerTimer = 2.6f;
                    break;
                }
            }
        }
    }

    if (m_player.movedLastFrame()) {
        m_tutStepMoved = true;
        if (m_player.activeStance() == MovementStance::Sneak) {
            m_tutStepSneaked = true;
        }
        // Stage 2: Sprinting for >= 0.22s shakes off any attached Acoustic Leech
        if (m_player.activeStance() == MovementStance::Sprint) {
            m_leechSprintDetachTimer += dt;
            if (m_leechSprintDetachTimer >= 0.22f) {
                for (auto& enemy : m_enemies) {
                    if (enemy.detachLeech(m_player.position())) {
                        m_statusBannerText = "ACOUSTIC LEECH SHAKEN OFF BY SPRINT!";
                        m_statusBannerTimer = 2.2f;
                    }
                }
            }
        } else {
            m_leechSprintDetachTimer = 0.0f;
        }
    } else {
        m_leechSprintDetachTimer = 0.0f;
    }
    updateTutorialProgression();

    updateNoiseSystem(dt);

    if (m_audio) m_audio->updateListener(m_player.position(), m_player.facingAngleRad());

    for (auto& emitter : room.ambientEmitters) {
        emitter.timer += dt;
        if (emitter.timer >= emitter.intervalSec) {
            emitter.timer = 0.0f;
            m_acoustics.emitPulse(
                emitter.position,
                265.0f,
                (emitter.isSteamVent ? 260.0f : 210.0f) * bSpec.soundRangeMult,
                emitter.isSteamVent ? 0.55f : 0.42f,
                32,
                WaveSignature::AmbientDrip,
                0.0f,
                6.2831853f,
                false
            );
            const float occ = AcousticSystem::computeTransmissionFactor(emitter.position, m_player.position(), room.walls);
            if (m_audio) {
                m_audio->playSpatial(
                    emitter.isSteamVent ? SoundEffectId::SteamHiss : SoundEffectId::WaterDrip,
                    emitter.position,
                    occ,
                    0.65f,
                    1.0f
                );
            }
        }
    }

    const float roomChargeScale = (room.roomEvent == RoomEvent::ResonanceOvercharge) ? 2.0f : 1.0f;
    const float acousticDt = (room.roomEvent == RoomEvent::AcousticBlackout) ? (dt * 1.45f) : dt;
    const std::vector<int> newlyUnlockedBySonar = m_acoustics.update(
        acousticDt,
        room.walls,
        &room.resonators,
        m_player.resonatorChargeMultiplier() * roomChargeScale
    );

    sf::Vector2f glassPos = m_player.position();
    const int shatteredCount = m_acoustics.consumeShatteredGlassCount(&glassPos);
    if (shatteredCount > 0) {
        registerNoiseAction(NoiseAction::GlassShatter, 1.0f, glassPos);
        m_acoustics.emitPulse(glassPos, 430.0f, 480.0f, 1.20f, 72, WaveSignature::GlassShatterWave, 0.0f, 6.2831853f, true);
        if (m_audio) m_audio->playWaveSignature(WaveSignature::GlassShatterWave);
        triggerCameraShake(cfg::feel::kShakeAmplitudeAlert, 0.25f);
        m_statusBannerText = "BRITTLE GLASS SHATTERED BY RESONANCE (+40 NOISE)!";
        m_statusBannerTimer = 3.0f;
    }

    const DifficultyProfile diff = getDifficultyProfile(m_difficulty);

    for (int unlockedId : newlyUnlockedBySonar) {
        m_labyrinth.activateSwitch(unlockedId);
        if (m_audio) {
            m_audio->playWaveSignature(WaveSignature::ResonatorHum);
            m_audio->playSpatial(SoundEffectId::GateSlideOpen, {640.0f, 400.0f}, 0.9f, 1.0f, 0.95f);
        }

        if (unlockedId >= 900) {
            m_acoustics.emitPulse(m_player.position(), 420.0f, 520.0f, 1.2f, 96, WaveSignature::ResonatorHum, 0.0f, 6.2831853f, false);
            m_statusBannerText = "SECRET ACOUSTIC FRACTURE UNSEALED BY SONAR!";
            m_statusBannerTimer = 3.8f;
        } else if (unlockedId < 700) {
            if (room.type == RoomType::Puzzle && room.shardReward > 0) {
                const int puzzleShards = static_cast<int>(std::round(
                    static_cast<float>(room.shardReward) * m_player.shardGainMultiplier() * diff.shardRewardMult
                ));
                m_player.addEchoShards(puzzleShards);
                room.shardReward = 0;
                m_statusBannerText = "HARMONIC PUZZLE SOLVED (+" + std::to_string(puzzleShards) + " ECHO SHARDS)!";
            } else {
                m_statusBannerText = "HARMONIC RESONATOR CHARGED — BARRIER UNSEALED";
            }
            m_statusBannerTimer = 3.6f;
        }
    }

    for (const auto& emitter : room.ambientEmitters) {
        if (emitter.isSteamVent && emitter.timer < 0.75f) {
            if (distSq2D(m_player.position(), emitter.position) <= 95.0f * 95.0f) {
                m_acoustics.clearRecentEvents();
                break;
            }
        }
    }

    float maxSuspicion = 0.0f;
    bool anyPursuing = false;
    bool anyActiveBefore = false;
    bool allInactiveNow = true;

    const float roomDmgScale = (room.roomEvent == RoomEvent::ResonanceOvercharge) ? 1.25f : 1.0f;

    for (auto& enemy : m_enemies) {
        if (!enemy.isActive()) continue;
        anyActiveBefore = true;

        if (enemy.variant() == EnemyVariant::AbyssalBoss &&
            enemy.bossAcousticPhase() != BossArenaAcousticPhase::HarmonicChoir) {
            m_bossDissonanceTimer += dt;
            if (m_bossDissonanceTimer >= 4.2f) {
                m_bossDissonanceTimer = 0.0f;
                m_acoustics.emitPulse(enemy.position(), 470.0f, 680.0f, 1.25f, 96, WaveSignature::EnemyTremor, 0.0f, 6.2831853f, false);
                if (m_player.movedLastFrame() && m_player.activeStance() != MovementStance::Sneak) {
                    registerNoiseAction(NoiseAction::BossDissonanceShock, 1.0f, m_player.position());
                    m_statusBannerText = "CHOIR SOVEREIGN DISSONANCE WAVE — SNEAK [SHIFT] OR STOP TO AVOID +15 NOISE!";
                    m_statusBannerTimer = 2.6f;
                }
            }
        }

        bool reflectedHit = false;
        bool highFreqHit = false;
        const float impactEnergy = m_acoustics.consumeRayImpactAt(
            enemy.position(),
            enemy.hitRadius(),
            reflectedHit,
            &highFreqHit
        );
        if (impactEnergy > 0.0f) {
            const float dmg = impactEnergy * cfg::balance::kDefaultBalance.sonarBaseDamageScale *
                              m_player.sonarDamageMultiplier(reflectedHit, highFreqHit) * roomDmgScale;
            if (m_player.hasRelic(RelicId::EchoSiphonCoil)) {
                m_player.restoreStamina(5.5f);
            }
            registerNoiseAction(NoiseAction::CombatHit, 1.0f, enemy.position());

            const bool defeated = enemy.applyAcousticDamage(
                dmg,
                reflectedHit,
                m_player.position(),
                room.walls,
                highFreqHit
            );

            // Stage 1.1: Trigger Hit-Stop (30-60 ms) and Exponential Screen Shake on sonar impact
            m_hitStopTimer = std::max(
                m_hitStopTimer,
                defeated ? cfg::feel::kHitStopEnemyKillSec : cfg::feel::kHitStopSonarHitSec
            );
            triggerCameraShake(
                defeated ? cfg::feel::kShakeAmplitudeKill : cfg::feel::kShakeAmplitudeSonarHit,
                defeated ? 0.28f : 0.16f
            );
            if (m_audio && !defeated) {
                m_audio->playSpatial(SoundEffectId::ResonatorHum, enemy.position(), 0.9f, 0.65f, 1.42f);
            }

            if (defeated) {
                ++m_enemiesDisruptedThisRun;
                if (enemy.variant() == EnemyVariant::AbyssalBoss || enemy.variant() == EnemyVariant::TheConductor) {
                    ++m_metaBossesDefeated;
                }
                if (m_audio) m_audio->playSpatial(SoundEffectId::ResonatorHum, enemy.position(), 1.0f, 0.95f, 1.25f);
                m_acoustics.emitPulse(enemy.position(), 410.0f, 420.0f, 1.10f, 72, WaveSignature::ResonatorHum, 0.0f, 6.2831853f, false);
                continue;
            }
        }

        const EnemyState prevState = enemy.state();
        if (enemy.update(dt, m_player.position(), room.walls, m_acoustics)) {
            const float occ = AcousticSystem::computeTransmissionFactor(enemy.position(), m_player.position(), room.walls);
            if (m_audio) {
                if (enemy.state() == EnemyState::PursuePlayer) {
                    m_audio->playSpatial(SoundEffectId::EnemyHuntRoar, enemy.position(), occ, 1.0f, 0.95f);
                } else {
                    m_audio->playSpatial(SoundEffectId::EnemyAlert, enemy.position(), occ, 0.95f, 1.0f);
                }
            }
            triggerCameraShake(cfg::feel::kShakeAmplitudeAlert, 0.30f);
        } else if (prevState != EnemyState::SearchLastKnown && enemy.state() == EnemyState::SearchLastKnown) {
            const float occ = AcousticSystem::computeTransmissionFactor(enemy.position(), m_player.position(), room.walls);
            if (m_audio) m_audio->playSpatial(SoundEffectId::EnemySearchClick, enemy.position(), occ, 0.85f, 1.0f);
        }

        // Stage 1.1: Play audible warning cue when enemy begins attack telegraph windup (0.3-0.6s)
        if (enemy.consumeTelegraphAlert()) {
            const float occ = AcousticSystem::computeTransmissionFactor(enemy.position(), m_player.position(), room.walls);
            if (m_audio) {
                m_audio->playSpatial(SoundEffectId::EnemyAlert, enemy.position(), occ, 1.0f, 1.32f);
            }
        }

        if (enemy.consumeSwarmAmplification()) {
            registerNoiseAction(NoiseAction::EchoSwarmAmplify, 1.0f, enemy.position());
            for (auto& other : m_enemies) {
                if (other.isActive() && &other != &enemy) {
                    other.applyNoiseTierStimulus(m_player.position(), NoiseTier::Loud, true);
                }
            }
            m_statusBannerText = "ECHO SWARM AMPLIFIED YOUR SOUND (+12 NOISE) — HUNTERS ALERTED!";
            m_statusBannerTimer = 2.6f;
        }

        // Stage 2: Banshee / Phase-2 Conductor piercing wail (+22 noise & alerts chamber)
        if (enemy.consumeBansheeWail()) {
            registerNoiseAction(NoiseAction::BansheeWail, 1.0f, enemy.position());
            if (m_audio) {
                m_audio->playSpatial(SoundEffectId::EnemyHuntRoar, enemy.position(), 1.0f, 1.0f, 1.52f);
            }
            triggerCameraShake(cfg::feel::kShakeAmplitudeAlert, 0.28f);
            for (auto& other : m_enemies) {
                if (other.isActive() && &other != &enemy) {
                    other.applyNoiseTierStimulus(m_player.position(), NoiseTier::Loud, false);
                }
            }
            m_statusBannerText = (enemy.variant() == EnemyVariant::TheConductor)
                ? "THE CONDUCTOR UNLEASHED CRESCENDO WAIL (+22 NOISE)!"
                : "BANSHEE PIERCING WAIL (+22 NOISE) — CHAMBER HUNTERS ALERTED!";
            m_statusBannerTimer = 2.6f;
        }

        // Stage 2: Acoustic Leech parasitic drain (+8 noise & -16 Stamina per pulse until detached)
        if (enemy.consumeLeechDrainPulse()) {
            m_player.setStamina(m_player.stamina() - 16.0f);
            registerNoiseAction(NoiseAction::LeechDrain, 1.0f, m_player.position());
            if (m_audio) {
                m_audio->playSpatial(SoundEffectId::ElectricalBuzz, m_player.position(), 1.0f, 0.85f, 1.25f);
            }
            m_statusBannerText = "ACOUSTIC LEECH DRAINING STAMINA (+8 NOISE) — SPRINT [CTRL] OR STOMP [C] TO DETACH!";
            m_statusBannerTimer = 2.4f;
        }

        const bool meleeHit = enemy.hasCaughtPlayer();
        const bool boltHit = enemy.consumeBoltHitOnPlayer(m_player.position(), cfg::kPlayerRadius);
        if (meleeHit || boltHit) {
            if (meleeHit) {
                enemy.clearContactState();
            }
            const int dmgTaken = meleeHit ? cfg::balance::kDefaultBalance.enemyMeleeDamage : cfg::balance::kDefaultBalance.enemyBoltDamage;
            if (m_player.applyDamage(dmgTaken, m_acoustics)) {
                if (m_audio) m_audio->playEffect(SoundEffectId::PlayerCaught, 0.95f, 1.0f);
                triggerCameraShake(cfg::feel::kShakeAmplitudeDamage, 0.40f);
                m_statusBannerText = "HULL INTEGRITY DAMAGED (-1 HP)!";
                m_statusBannerTimer = 2.2f;

                if (m_player.isDead()) {
                    saveCheckpoint();
                    m_state = AppState::GameOver;
                    rebuildMenuButtons();
                    m_acoustics.clearRecentEvents();
                    return;
                }
            }
        }

        if (enemy.isActive()) {
            allInactiveNow = false;
            maxSuspicion = std::max(maxSuspicion, enemy.suspicion());
            if (enemy.state() == EnemyState::PursuePlayer) {
                anyPursuing = true;
            }
        }
    }

    if (anyActiveBefore && allInactiveNow && !room.cleared) {
        if (!room.noiseIntrusionActive && (m_currentWaveIndex + 1 < room.totalWaves)) {
            ++m_currentWaveIndex;
            spawnWaveForCurrentRoom(m_currentWaveIndex);
            if (m_audio) m_audio->playEffect(SoundEffectId::EnemyAlert, 0.9f, 0.88f);
            m_statusBannerText = "WAVE 2 REINFORCEMENTS DETECTED — MAINTAIN SONAR LOCK!";
            m_statusBannerTimer = 3.2f;
        } else {
            const bool wasIntrusionEncounter = room.noiseIntrusionActive;
            bool isGhostClear = false;
            const int baseReward = static_cast<int>(std::round(
                static_cast<float>(room.shardReward) * m_player.shardGainMultiplier() * diff.shardRewardMult
            ));
            const int shardsEarned = Labyrinth::computeClearShardReward(baseReward, m_roomPeakNoise, isGhostClear, room.roomEvent);
            room.shardReward = 0;
            if (!wasIntrusionEncounter) {
                room.ghostCleared = isGhostClear;
            }
            m_labyrinth.markRoomCleared(room.id);
            m_player.addEchoShards(shardsEarned);

            if (room.type == RoomType::Boss) {
                m_player.heal(2);
                m_statusBannerText = "THE CHOIR SOVEREIGN SHATTERED! ASCENT CONDUIT UNSEALED (+" + std::to_string(shardsEarned) + " SHARDS)";
            } else if (wasIntrusionEncounter) {
                m_statusBannerText = "INTRUDER DISRUPTED — CHAMBER DOORS RE-UNSEALED!";
            } else if (isGhostClear) {
                ++m_ghostClearsThisRun;
                ++m_metaTotalGhostClears;
                std::string bonusMsg = "GHOST CLEAR (PEAK NOISE <= 30)! +" + std::to_string(shardsEarned) + " SHARDS";
                if (!m_player.hasRelic(RelicId::SilentStep)) {
                    m_player.addRelic(RelicId::SilentStep, &m_acoustics);
                    bonusMsg += " & SILENT STEP RELIC UNLOCKED!";
                }
                m_statusBannerText = bonusMsg;
            } else {
                m_statusBannerText = "CHAMBER CLEARED — DOORS UNSEALED (+" + std::to_string(shardsEarned) + " ECHO SHARDS)";
            }
            m_statusBannerTimer = 4.0f;
            if (m_audio) {
                m_audio->playEffect(SoundEffectId::VictoryChord, 0.85f, 1.08f);
                m_audio->playSpatial(SoundEffectId::GateSlideOpen, {640.0f, 400.0f}, 1.0f, 1.0f, 1.0f);
            }
            saveCheckpoint();
        }
    }

    const float danger0to1 = std::clamp(std::max(maxSuspicion, m_noiseLevel * 0.85f) / 100.0f, 0.0f, 1.0f);
    if (m_audio) {
        bool isBossActive = (room.type == RoomType::Boss && !room.cleared);
        int activeBossPhase = 1;
        for (const auto& enemy : m_enemies) {
            if (enemy.isActive() && enemy.variant() == EnemyVariant::AbyssalBoss) {
                isBossActive = true;
                activeBossPhase = std::max(activeBossPhase, enemy.bossPhase());
            }
        }
        m_audio->updateAdaptiveAudio(
            dt,
            danger0to1,
            anyPursuing,
            room.isCheckpoint,
            m_player.stamina(),
            isBossActive,
            activeBossPhase
        );
    }

    m_acoustics.clearRecentEvents();

    for (const auto& portal : room.portals) {
        if (distSq2D(m_player.position(), portal.position) <= portal.radius * portal.radius) {
            if (portal.requiredSwitchId != -1 && !m_labyrinth.isSwitchActivated(portal.requiredSwitchId)) {
                if (m_statusBannerTimer <= 0.1f) {
                    if (portal.isSecretDoor) {
                        m_statusBannerText = "HOLLOW MASONRY DETECTED — PING [LMB] TO FRACTURE SECRET WALL";
                    } else if (room.combatLocked) {
                        m_statusBannerText = "DOORWAY SEALED DURING COMBAT — DISRUPT ALL HUNTERS";
                    } else {
                        m_statusBannerText = "LOCKED: " + portal.label + " (ACTIVATE MECHANISM OR DEFEAT BOSS)";
                    }
                    m_statusBannerTimer = 1.8f;
                }
            } else {
                if (portal.isVictoryExit) {
                    ++m_metaTotalVictories;
                    saveCheckpoint();
                    if (m_audio) m_audio->playEffect(SoundEffectId::VictoryChord, 1.0f, 1.0f);
                    m_state = AppState::Victory;
                    rebuildMenuButtons();
                } else {
                    if (m_audio) m_audio->playEffect(SoundEffectId::SwitchClunk, 0.65f, 1.15f);
                    enterRoom(portal.targetRoomId, portal.targetSpawnPos, true);
                }
                return;
            }
        }
    }
}

} // namespace echo
