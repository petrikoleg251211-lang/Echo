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

sf::Vector2f closestPointOnSegGame(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) noexcept {
    const sf::Vector2f ab = b - a;
    const float lenSq = ab.x * ab.x + ab.y * ab.y;
    if (lenSq < 1e-6f) return a;
    const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lenSq, 0.0f, 1.0f);
    return a + ab * t;
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
    m_audio->initialize();
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

void Game::spawnWaveForCurrentRoom(int waveIndex) {
    m_enemies.clear();
    const auto& room = m_labyrinth.currentRoom();
    if (room.cleared) return;

    for (const auto& sp : room.enemySpawns) {
        if (sp.waveIndex == waveIndex) {
            EnemyAI enemy;
            enemy.configurePatrol(sp.waypoints, sp.variant);
            m_enemies.push_back(std::move(enemy));
        }
    }
}

void Game::enterRoom(int roomId, sf::Vector2f spawnPos, bool preservePlayerBuild) {
    const int prevRoomId = m_labyrinth.currentRoomId();
    const bool isRoomTransition = preservePlayerBuild && (roomId != prevRoomId);

    m_labyrinth.setCurrentRoom(roomId);
    m_acoustics.clear();

    if (preservePlayerBuild) {
        m_player.setPosition(spawnPos);
        // Attenuate carried-over noise Meter by 50% across the doorway
        if (isRoomTransition) {
            m_noiseLevel *= cfg::noise::kCrossRoomAttenuation;
        }
    } else {
        m_player.reset(spawnPos);
        m_noiseLevel = 0.0f;
    }

    auto& room = m_labyrinth.currentRoomMut();
    m_currentWaveIndex = 0;
    // Initialize m_roomPeakNoise to 0.0f on room entry so noise from the previous room
    // never disqualifies the player from earning Ghost Clear in the newly entered room!
    m_roomPeakNoise = 0.0f;
    m_noiseIntrusionCooldown = 4.0f;

    if (!room.cleared && room.noiseIntrusionActive) {
        m_enemies.clear();
        const sf::Vector2f entryPos = Labyrinth::spawnPositionForEntry(room.pendingNoiseAlertDir);
        EnemyAI intruder;
        intruder.configurePatrol(
            {entryPos, spawnPos, {640.0f, 400.0f}},
            EnemyVariant::Stalker
        );
        intruder.applyNoiseTierStimulus(spawnPos, NoiseTier::Loud, true);
        m_enemies.push_back(std::move(intruder));
        m_labyrinth.setCombatLock(roomId, true);
        m_statusBannerText = "INTRUDER STILL PROWLING CHAMBER — DISRUPT HUNTER TO UNSEAL!";
        m_statusBannerTimer = 3.2f;
    } else if (!room.cleared && !room.enemySpawns.empty()) {
        spawnWaveForCurrentRoom(0);
        // Apply any cross-room sound stimulus that leaked into this room before entry
        if (room.pendingNoiseAlertLevel > cfg::noise::kThresholdAudible) {
            const NoiseTier leakedTier = classifyNoiseTier(room.pendingNoiseAlertLevel);
            const sf::Vector2f doorwayOrigin = Labyrinth::spawnPositionForEntry(room.pendingNoiseAlertDir);
            for (auto& enemy : m_enemies) {
                if (enemy.isActive()) {
                    enemy.applyNoiseTierStimulus(doorwayOrigin, leakedTier, true);
                }
            }
            room.pendingNoiseAlertLevel = 0.0f;
        }
        m_labyrinth.setCombatLock(roomId, true);
        m_statusBannerText = std::string(Labyrinth::roomTypeName(room.type)) + " — KEEP NOISE <= 30 FOR GHOST CLEAR!";
        m_statusBannerTimer = 3.2f;
    } else {
        m_enemies.clear();
        m_labyrinth.setCombatLock(roomId, false);
        m_statusBannerText = room.name + "  //  " + room.sectorTitle;
        m_statusBannerTimer = 3.0f;
    }

    m_acoustics.emitPulse(spawnPos, 380.0f, 360.0f, 0.72f, 56, WaveSignature::FootstepWalk, 0.0f, 6.2831853f, false);

    if (room.isCheckpoint) {
        saveCheckpoint();
    }
}

void Game::startNewGame(std::uint32_t customSeed) {
    const std::uint32_t nextSeed = (customSeed != 0)
        ? customSeed
        : static_cast<std::uint32_t>(m_labyrinth.runSeed() * 1664525u + 1013904223u + static_cast<std::uint32_t>(m_metaTotalRuns * 97));

    ++m_metaTotalRuns;
    m_enemiesDisruptedThisRun = 0;
    m_ghostClearsThisRun = 0;
    m_noiseLevel = 0.0f;
    m_roomPeakNoise = 0.0f;
    m_noiseSilenceGraceTimer = 0.0f;
    m_noiseIntrusionCooldown = 0.0f;
    m_runTimeSec = 0.0f;
    m_loreDisplayTimer = 0.0f;
    m_showMinimap = false;
    m_acoustics.setMaxReflections(cfg::kDefaultMaxReflections);
    m_acoustics.setActiveFrequency(AcousticFrequency::Mid);

    m_labyrinth.generateRun(nextSeed, cfg::kDefaultRunRoomCount);
    m_player.reset(m_labyrinth.currentRoom().playerDefaultSpawn);
    enterRoom(0, m_labyrinth.currentRoom().playerDefaultSpawn, true);
    m_state = AppState::Playing;
}

void Game::saveCheckpoint() {
    SaveData data{};
    data.runSeed = m_labyrinth.runSeed();
    data.targetRoomCount = static_cast<int>(m_labyrinth.targetRoomCount());
    data.checkpointRoomId = m_labyrinth.currentRoomId();
    data.playerPosX = m_player.position().x;
    data.playerPosY = m_player.position().y;
    data.playerHealth = m_player.health();
    data.playerMaxHealth = m_player.maxHealth();
    data.playerStamina = m_player.stamina();
    data.decoyCount = m_player.decoyCount();
    data.echoShards = m_player.echoShards();

    data.masterVolume = m_audio ? m_audio->masterVolume() : 80.0f;
    data.musicVolume = m_audio ? m_audio->musicVolume() : 75.0f;
    data.sfxVolume = m_audio ? m_audio->sfxVolume() : 85.0f;
    data.ambientVolume = m_audio ? m_audio->ambientVolume() : 80.0f;
    data.rayDensityScale = m_acoustics.rayDensityScale();
    data.maxReflections = static_cast<int>(m_acoustics.maxReflections());
    data.ambientVisibility = m_ambientVisibility;
    data.runTimeSec = m_runTimeSec;

    data.totalRuns = m_metaTotalRuns;
    data.totalVictories = m_metaTotalVictories;
    data.bossesDefeated = m_metaBossesDefeated;

    data.activatedSwitches = m_labyrinth.activatedSwitches();
    data.collectedLoreIds = m_labyrinth.collectedLoreIds();
    data.visitedRooms = m_labyrinth.visitedRooms();
    data.clearedRooms = m_labyrinth.clearedRooms();
    for (RelicId r : m_player.relics()) {
        data.collectedRelics.push_back(static_cast<int>(r));
    }

    SaveSystem::saveToFile(m_saveFilePath, data);
}

bool Game::loadCheckpoint() {
    SaveData data{};
    if (!SaveSystem::loadFromFile(m_saveFilePath, data)) {
        return false;
    }

    if (m_audio) {
        m_audio->setMasterVolume(data.masterVolume);
        m_audio->setMusicVolume(data.musicVolume);
        m_audio->setSfxVolume(data.sfxVolume);
        m_audio->setAmbientVolume(data.ambientVolume);
    }
    m_acoustics.setRayDensityScale(data.rayDensityScale);
    m_acoustics.setMaxReflections(static_cast<std::uint8_t>(data.maxReflections));
    m_ambientVisibility = data.ambientVisibility;
    m_runTimeSec = data.runTimeSec;
    m_metaTotalRuns = data.totalRuns;
    m_metaTotalVictories = data.totalVictories;
    m_metaBossesDefeated = data.bossesDefeated;

    m_labyrinth.generateRun(data.runSeed, static_cast<std::size_t>(data.targetRoomCount));
    m_labyrinth.restoreState(
        data.activatedSwitches,
        data.collectedLoreIds,
        data.visitedRooms,
        data.clearedRooms
    );

    m_player.reset({data.playerPosX, data.playerPosY});
    std::vector<RelicId> loadedRelics;
    for (int id : data.collectedRelics) {
        const auto rId = static_cast<RelicId>(id);
        loadedRelics.push_back(rId);
        if (rId == RelicId::SubharmonicVisor) {
            m_labyrinth.revealSecretRoomsOnMap();
        }
    }
    m_player.setRelics(loadedRelics);
    m_player.setMaxHealth(data.playerMaxHealth);
    m_player.setHealth(data.playerHealth);
    m_player.setStamina(data.playerStamina);
    m_player.setDecoyCount(data.decoyCount);
    m_player.setEchoShards(data.echoShards);

    m_noiseLevel = 0.0f;
    m_roomPeakNoise = 0.0f;
    enterRoom(data.checkpointRoomId, {data.playerPosX, data.playerPosY}, true);
    m_state = AppState::Playing;
    m_statusBannerText = "CHECKPOINT RESTORED — SEED #" + std::to_string(data.runSeed);
    m_statusBannerTimer = 3.0f;
    return true;
}

void Game::registerNoiseAction(NoiseAction action, float dt, sf::Vector2f stimulusOrigin) {
    const bool isFootstep =
        (action == NoiseAction::StepSneak || action == NoiseAction::StepWalk || action == NoiseAction::StepSprint);
    float mult = m_player.noiseGainMultiplier(isFootstep);

    // Block 2.1: Standing inside a DampMoss shelter dampens footstep noise in half
    const auto& room = m_labyrinth.currentRoom();
    if (isFootstep && AcousticSystem::isInDampMossShelter(m_player.position(), room.walls, room.floorMaterial)) {
        mult *= 0.50f;
    }

    m_noiseLevel = applyNoiseAction(m_noiseLevel, action, dt, mult);
    m_roomPeakNoise = std::max(m_roomPeakNoise, m_noiseLevel);

    if (action == NoiseAction::DirectionalPing ||
        action == NoiseAction::RadialPulse ||
        action == NoiseAction::DecoyImpact ||
        action == NoiseAction::CombatHit ||
        action == NoiseAction::StepSprint ||
        action == NoiseAction::WhistlePing ||
        action == NoiseAction::StompImpact ||
        action == NoiseAction::GlassShatter ||
        action == NoiseAction::WaterSplash ||
        action == NoiseAction::EchoSwarmAmplify) {
        m_noiseSilenceGraceTimer = 0.35f;
    }

    const bool isGroundVib =
        (action == NoiseAction::StepSprint ||
         action == NoiseAction::RadialPulse ||
         action == NoiseAction::StompImpact ||
         action == NoiseAction::GlassShatter ||
         action == NoiseAction::WaterSplash);

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
    m_cameraShakeTimer = 0.28f;
    registerNoiseAction(NoiseAction::StompImpact, 1.0f, m_player.position());

    auto& room = m_labyrinth.currentRoomMut();
    const sf::Vector2f pPos = m_player.position();

    // Block 3.2: Stomp pushes nearby enemies back and staggers them
    int pushedEnemies = 0;
    for (auto& enemy : m_enemies) {
        if (enemy.isActive()) {
            if (enemy.applyStompKnockback(pPos, cfg::kStompRadius, cfg::kStompKnockback, room.walls)) {
                ++pushedEnemies;
            }
        }
    }

    // Block 3.2 & 2.3: Stomp shatters any Brittle Glass walls within Stomp radius
    bool shatteredAnyGlass = false;
    for (auto& w : room.walls) {
        if (!w.isOpen && w.material == SurfaceMaterial::Glass) {
            const sf::Vector2f c = closestPointOnSegGame(pPos, w.a, w.b);
            if (distSq2D(pPos, c) <= cfg::kStompRadius * cfg::kStompRadius) {
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

    if (m_player.movedLastFrame()) {
        NoiseAction stepAct = NoiseAction::StepWalk;
        if (m_player.activeStance() == MovementStance::Sneak) {
            stepAct = NoiseAction::StepSneak;
        } else if (m_player.activeStance() == MovementStance::Sprint) {
            stepAct = NoiseAction::StepSprint;
        }
        registerNoiseAction(stepAct, dt, m_player.position());
    }

    // Silence decay (-10/sec, or -20/sec inside a DampMoss shelter per Block 2.1!)
    if ((!m_player.movedLastFrame() || m_player.activeStance() == MovementStance::Sneak) &&
        m_noiseSilenceGraceTimer <= 0.0f) {
        const auto& curRoom = m_labyrinth.currentRoom();
        const bool inMossShelter = AcousticSystem::isInDampMossShelter(
            m_player.position(),
            curRoom.walls,
            curRoom.floorMaterial
        );
        const float decayMult = inMossShelter ? 2.0f : 1.0f;
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

    // Block 1.3: If the player is making loud/scream noise in a cleared room with open doors,
    // an enemy from an adjacent room can hear the leaked noise (> 30) and breach through the doorway
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

                        EnemyAI intruder;
                        intruder.configurePatrol(
                            {entryPos, m_player.position(), {640.0f, 400.0f}},
                            (m_noiseLevel >= cfg::noise::kThresholdScream) ? EnemyVariant::Phantom : EnemyVariant::Stalker
                        );
                        intruder.applyNoiseTierStimulus(m_player.position(), classifyNoiseTier(m_noiseLevel), true);
                        m_enemies.push_back(std::move(intruder));

                        if (m_audio) m_audio->playEffect(SoundEffectId::EnemyHuntRoar, 0.95f, 0.92f);
                        m_cameraShakeTimer = 0.35f;
                        m_statusBannerText = "NOISE BREACH! NEIGHBORING HUNTER ATTRACTED THROUGH OPEN DOORWAY!";
                        m_statusBannerTimer = 3.8f;
                    }
                    break;
                }
            }
        }
    }
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
    m_cameraShakeTimer = std::max(0.0f, m_cameraShakeTimer - dt);
    m_shaleCrackCooldown = std::max(0.0f, m_shaleCrackCooldown - dt);

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
    m_labyrinth.updateAnimations(dt);

    WaveSignature emittedSig{};
    if (m_player.update(dt, moveInput, stance, m_mouseWorldPos, room.walls, m_acoustics, emittedSig)) {
        if (m_audio) m_audio->updateListener(m_player.position(), m_player.facingAngleRad());

        if (emittedSig == WaveSignature::FootstepSneak ||
            emittedSig == WaveSignature::FootstepWalk ||
            emittedSig == WaveSignature::FootstepSprint) {
            SurfaceMaterial floorMat = m_player.detectFloorMaterial(room.walls, room.floorMaterial);

            // Check if player stepped into a WaterPuddle prop
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

            // Block 2.4: Steps on Water add +15 noise and emit 2x-distance concentric water ripples!
            if (steppedInPuddle && emittedSig != WaveSignature::FootstepSneak && !m_player.hasRelic(RelicId::MossTreadBoots)) {
                m_acoustics.emitWaterRipples(m_player.position(), 240.0f);
                registerNoiseAction(NoiseAction::WaterSplash, 1.0f, m_player.position());
            }
        } else {
            if (m_audio) m_audio->playWaveSignature(emittedSig);
            if (emittedSig == WaveSignature::DecoyImpact) {
                registerNoiseAction(NoiseAction::DecoyImpact, 1.0f, m_mouseWorldPos);
            } else if (emittedSig == WaveSignature::EchoTrapPulse) {
                // Block 3.3: Echo Trap Mine lures enemies to the trap position while the player is away!
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

    // Update continuous Noise Meter & cross-room propagation
    updateNoiseSystem(dt);

    if (m_audio) m_audio->updateListener(m_player.position(), m_player.facingAngleRad());

    // Ambient room emitters
    for (auto& emitter : room.ambientEmitters) {
        emitter.timer += dt;
        if (emitter.timer >= emitter.intervalSec) {
            emitter.timer = 0.0f;
            m_acoustics.emitPulse(
                emitter.position,
                265.0f,
                emitter.isSteamVent ? 260.0f : 210.0f,
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

    const std::vector<int> newlyUnlockedBySonar = m_acoustics.update(
        dt,
        room.walls,
        &room.resonators,
        m_player.resonatorChargeMultiplier()
    );

    // Block 2.3: Check if any Brittle Glass walls were shattered by high-power sonar rays (+40 noise)
    sf::Vector2f glassPos = m_player.position();
    const int shatteredCount = m_acoustics.consumeShatteredGlassCount(&glassPos);
    if (shatteredCount > 0) {
        registerNoiseAction(NoiseAction::GlassShatter, 1.0f, glassPos);
        m_acoustics.emitPulse(glassPos, 430.0f, 480.0f, 1.20f, 72, WaveSignature::GlassShatterWave, 0.0f, 6.2831853f, true);
        if (m_audio) m_audio->playWaveSignature(WaveSignature::GlassShatterWave);
        m_cameraShakeTimer = 0.25f;
        m_statusBannerText = "BRITTLE GLASS SHATTERED BY RESONANCE (+40 NOISE)!";
        m_statusBannerTimer = 3.0f;
    }

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
                m_player.addEchoShards(room.shardReward);
                room.shardReward = 0;
                m_statusBannerText = "HARMONIC PUZZLE SOLVED (+20 ECHO SHARDS)!";
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

    for (auto& enemy : m_enemies) {
        if (!enemy.isActive()) continue;
        anyActiveBefore = true;

        bool reflectedHit = false;
        bool highFreqHit = false;
        const float impactEnergy = m_acoustics.consumeRayImpactAt(
            enemy.position(),
            enemy.hitRadius(),
            reflectedHit,
            &highFreqHit
        );
        if (impactEnergy > 0.0f) {
            const float dmg = impactEnergy * 11.5f * m_player.sonarDamageMultiplier(reflectedHit, highFreqHit);
            if (m_player.hasRelic(RelicId::EchoSiphonCoil)) {
                m_player.restoreStamina(5.5f);
            }
            // Block 1.1: Combat hit adds +10 noise
            registerNoiseAction(NoiseAction::CombatHit, 1.0f, enemy.position());

            const bool defeated = enemy.applyAcousticDamage(
                dmg,
                reflectedHit,
                m_player.position(),
                room.walls,
                highFreqHit
            );
            if (defeated) {
                ++m_enemiesDisruptedThisRun;
                if (enemy.variant() == EnemyVariant::AbyssalBoss) {
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
            m_cameraShakeTimer = 0.30f;
        } else if (prevState != EnemyState::SearchLastKnown && enemy.state() == EnemyState::SearchLastKnown) {
            const float occ = AcousticSystem::computeTransmissionFactor(enemy.position(), m_player.position(), room.walls);
            if (m_audio) m_audio->playSpatial(SoundEffectId::EnemySearchClick, enemy.position(), occ, 0.85f, 1.0f);
        }

        // Block 4.4: Check if EchoSwarm repeated & amplified a player sound nearby
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

        const bool meleeHit = enemy.hasCaughtPlayer();
        const bool boltHit = enemy.consumeBoltHitOnPlayer(m_player.position(), cfg::kPlayerRadius);
        if (meleeHit || boltHit) {
            if (meleeHit) {
                enemy.clearContactState();
            }
            if (m_player.applyDamage(1, m_acoustics)) {
                if (m_audio) m_audio->playEffect(SoundEffectId::PlayerCaught, 0.95f, 1.0f);
                m_cameraShakeTimer = 0.40f;
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

    // Check if wave or entire combat room was just cleared
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
            const int shardsEarned = Labyrinth::computeClearShardReward(room.shardReward, m_roomPeakNoise, isGhostClear);
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
                std::string bonusMsg = "GHOST CLEAR (PEAK NOISE <= 30)! +" + std::to_string(shardsEarned) + " SHARDS (x1.5)";
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
        m_audio->updateAdaptiveAudio(
            dt,
            danger0to1,
            anyPursuing,
            room.isCheckpoint,
            m_player.stamina()
        );
    }

    m_acoustics.clearRecentEvents();

    // Check 4-directional room portals
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
