#include "Game.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace echo {

void Game::setDifficulty(DifficultyMode mode) noexcept {
    m_difficulty = mode;
}

void Game::cycleDifficulty(int delta) noexcept {
    const int count = 4;
    const int cur = static_cast<int>(m_difficulty);
    const int next = ((cur + delta) % count + count) % count;
    m_difficulty = static_cast<DifficultyMode>(next);
}

void Game::spawnWaveForCurrentRoom(int waveIndex) {
    m_enemies.clear();
    const auto& room = m_labyrinth.currentRoom();
    if (room.cleared) return;

    const DifficultyProfile diff = getDifficultyProfile(m_difficulty);
    int spawnedInWave = 0;

    for (const auto& sp : room.enemySpawns) {
        if (sp.waveIndex == waveIndex) {
            EnemyAI enemy;
            enemy.configurePatrol(sp.waypoints, sp.variant);
            enemy.applyDifficultyScaling(diff.enemyHpMult, diff.enemySpeedMult);
            m_enemies.push_back(std::move(enemy));
            ++spawnedInWave;
        }
    }

    // Stage 1.3: Abyssal (+25% enemy count) & Explorer (-20% enemy count when > 2)
    if (room.type != RoomType::Boss && spawnedInWave > 0) {
        if (diff.enemyCountMult > 1.05f) {
            const int extraCount = std::max(1, static_cast<int>(std::floor(static_cast<float>(spawnedInWave) * (diff.enemyCountMult - 1.0f) + 0.5f)));
            for (int i = 0; i < extraCount; ++i) {
                const auto& baseSp = room.enemySpawns[static_cast<std::size_t>(i) % room.enemySpawns.size()];
                std::vector<sf::Vector2f> shiftedWaypoints = baseSp.waypoints;
                for (auto& wp : shiftedWaypoints) {
                    wp.x = std::clamp(wp.x + 36.0f * static_cast<float>((i % 2 == 0) ? 1 : -1), 120.0f, cfg::kWorldWidth - 120.0f);
                    wp.y = std::clamp(wp.y + 28.0f * static_cast<float>((i % 2 == 0) ? -1 : 1), 120.0f, cfg::kWorldHeight - 120.0f);
                }
                EnemyAI extraEnemy;
                extraEnemy.configurePatrol(
                    shiftedWaypoints,
                    (baseSp.variant == EnemyVariant::AbyssalBoss || baseSp.variant == EnemyVariant::TheConductor)
                        ? EnemyVariant::Stalker
                        : baseSp.variant
                );
                extraEnemy.applyDifficultyScaling(diff.enemyHpMult, diff.enemySpeedMult);
                m_enemies.push_back(std::move(extraEnemy));
            }
        } else if (diff.enemyCountMult < 0.95f && m_enemies.size() > 2) {
            m_enemies.pop_back();
        }
    }
}

void Game::enterRoom(int roomId, sf::Vector2f spawnPos, bool preservePlayerBuild) {
    const int prevRoomId = m_labyrinth.currentRoomId();
    const bool isRoomTransition = preservePlayerBuild && (roomId != prevRoomId);

    m_labyrinth.setCurrentRoom(roomId);
    m_acoustics.clear();

    const DifficultyProfile diff = getDifficultyProfile(m_difficulty);

    if (preservePlayerBuild) {
        m_player.setPosition(spawnPos);
        if (isRoomTransition) {
            m_noiseLevel *= cfg::noise::kCrossRoomAttenuation;
        }
    } else {
        m_player.reset(spawnPos);
        m_player.setOneLifeLocked(diff.oneLifeOnly);
        if (!diff.oneLifeOnly) {
            m_player.setMaxHealth(diff.playerStartHp);
            m_player.setHealth(diff.playerStartHp);
        }
        m_noiseLevel = 0.0f;
    }

    auto& room = m_labyrinth.currentRoomMut();
    m_currentWaveIndex = 0;
    m_roomPeakNoise = 0.0f;
    m_noiseIntrusionCooldown = 4.0f;
    m_echoStormTimer = 0.0f;
    m_bossDissonanceTimer = 0.0f;
    m_foundryHammerTimer = 0.0f;
    m_foundryHammerMaskTimer = 0.0f;
    m_leechSprintDetachTimer = 0.0f;

    if (!room.cleared && room.noiseIntrusionActive) {
        m_enemies.clear();
        const sf::Vector2f entryPos = Labyrinth::spawnPositionForEntry(room.pendingNoiseAlertDir);
        EnemyAI intruder;
        intruder.configurePatrol(
            {entryPos, spawnPos, {640.0f, 400.0f}},
            EnemyVariant::Stalker
        );
        intruder.applyDifficultyScaling(diff.enemyHpMult, diff.enemySpeedMult);
        intruder.applyNoiseTierStimulus(spawnPos, NoiseTier::Loud, true);
        m_enemies.push_back(std::move(intruder));
        m_labyrinth.setCombatLock(roomId, true);
        m_statusBannerText = "INTRUDER STILL PROWLING CHAMBER — DISRUPT HUNTER TO UNSEAL!";
        m_statusBannerTimer = 3.2f;
    } else if (!room.cleared && !room.enemySpawns.empty()) {
        spawnWaveForCurrentRoom(0);
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
        m_statusBannerText = std::string(Labyrinth::roomTypeName(room.type));
        if (room.roomEvent != RoomEvent::None) {
            m_statusBannerText += " [" + std::string(roomEventName(room.roomEvent)) + "]";
        } else {
            m_statusBannerText += " — KEEP NOISE <= 30 FOR GHOST CLEAR!";
        }
        m_statusBannerTimer = 3.2f;
    } else {
        m_enemies.clear();
        m_labyrinth.setCombatLock(roomId, false);
        m_statusBannerText = room.name + "  //  " + room.sectorTitle;
        m_statusBannerTimer = 3.0f;
    }

    const float entryRange = (room.roomEvent == RoomEvent::AcousticBlackout) ? 200.0f : 360.0f;
    m_acoustics.emitPulse(spawnPos, 380.0f, entryRange, 0.72f, 56, WaveSignature::FootstepWalk, 0.0f, 6.2831853f, false);

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
    m_lastNoiseAction = NoiseAction::SilenceDecay;
    m_lastNoiseDelta = 0.0f;
    m_lastNoiseSourceTimer = 0.0f;
    m_hitStopTimer = 0.0f;
    m_echoStormTimer = 0.0f;
    m_bossDissonanceTimer = 0.0f;
    m_foundryHammerTimer = 0.0f;
    m_foundryHammerMaskTimer = 0.0f;
    m_leechSprintDetachTimer = 0.0f;
    m_runTimeSec = 0.0f;
    m_loreDisplayTimer = 0.0f;
    m_showMinimap = false;
    m_acoustics.setMaxReflections(cfg::kDefaultMaxReflections);
    m_acoustics.setActiveFrequency(AcousticFrequency::Mid);

    // Stage 1.4: Initialize 3-room onboarding tutorial on first run if not yet completed
    if (!m_tutorialDone) {
        m_tutorialRoomStage = 0;
        m_tutStepMoved = false;
        m_tutStepSneaked = false;
        m_tutStepPinged = false;
        m_tutStepTrappedOrDecoy = false;
    } else {
        m_tutorialRoomStage = 3;
    }

    const DifficultyProfile diff = getDifficultyProfile(m_difficulty);

    m_labyrinth.generateRun(nextSeed, cfg::kDefaultRunRoomCount);
    m_player.reset(m_labyrinth.currentRoom().playerDefaultSpawn);
    m_player.setOneLifeLocked(diff.oneLifeOnly);
    if (!diff.oneLifeOnly) {
        m_player.setMaxHealth(diff.playerStartHp);
        m_player.setHealth(diff.playerStartHp);
    }
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
    data.echoTrapCount = m_player.echoTrapCount();
    data.activeFrequency = static_cast<int>(m_player.activeFrequency());
    data.echoShards = m_player.echoShards();
    data.ghostClearsThisRun = m_ghostClearsThisRun;

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
    data.metaTotalGhostClears = m_metaTotalGhostClears;
    data.difficulty = static_cast<int>(m_difficulty);
    data.tutorialDone = m_tutorialDone;

    data.activatedSwitches = m_labyrinth.activatedSwitches();
    data.collectedLoreIds = m_labyrinth.collectedLoreIds();
    data.discoveredCodexIds = m_discoveredCodexIds;
    for (int id : data.collectedLoreIds) {
        if (std::find(data.discoveredCodexIds.begin(), data.discoveredCodexIds.end(), id) == data.discoveredCodexIds.end()) {
            data.discoveredCodexIds.push_back(id);
        }
    }
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
    m_metaTotalGhostClears = data.metaTotalGhostClears;
    m_ghostClearsThisRun = data.ghostClearsThisRun;
    m_difficulty = static_cast<DifficultyMode>(std::clamp(data.difficulty, 0, 3));
    m_tutorialDone = data.tutorialDone;
    m_tutorialRoomStage = m_tutorialDone ? 3 : 0;
    m_discoveredCodexIds = data.discoveredCodexIds;
    for (int id : data.collectedLoreIds) {
        if (std::find(m_discoveredCodexIds.begin(), m_discoveredCodexIds.end(), id) == m_discoveredCodexIds.end()) {
            m_discoveredCodexIds.push_back(id);
        }
    }

    const DifficultyProfile diff = getDifficultyProfile(m_difficulty);

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
    m_player.setOneLifeLocked(diff.oneLifeOnly);
    m_player.setMaxHealth(diff.oneLifeOnly ? 1 : data.playerMaxHealth);
    m_player.setHealth(diff.oneLifeOnly ? 1 : data.playerHealth);
    m_player.setStamina(data.playerStamina);
    m_player.setDecoyCount(data.decoyCount);
    m_player.setEchoTrapCount(data.echoTrapCount);
    m_player.setFrequency(static_cast<AcousticFrequency>(std::clamp(data.activeFrequency, 0, 2)), m_acoustics);
    m_player.setEchoShards(data.echoShards);

    m_noiseLevel = 0.0f;
    m_roomPeakNoise = 0.0f;
    m_showMinimap = false;
    enterRoom(data.checkpointRoomId, {data.playerPosX, data.playerPosY}, true);
    m_state = AppState::Playing;
    m_statusBannerText = "CHECKPOINT RESTORED — SEED #" + std::to_string(data.runSeed);
    m_statusBannerTimer = 3.0f;
    return true;
}

} // namespace echo
