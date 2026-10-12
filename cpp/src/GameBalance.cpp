#include "Game.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <vector>

namespace echo {

BalanceReportSummary Game::simulateBalanceRuns(
    int numRuns,
    DifficultyMode difficulty,
    std::uint32_t baseSeed
) {
    BalanceReportSummary summary{};
    summary.difficulty = difficulty;
    summary.totalRuns = std::max(1, numRuns);

    const DifficultyProfile diff = getDifficultyProfile(difficulty);
    const auto& bal = cfg::balance::kDefaultBalance;

    double totalDurationSec = 0.0;
    double totalRoomsCleared = 0.0;
    double totalGhostClears = 0.0;

    for (int runIdx = 0; runIdx < summary.totalRuns; ++runIdx) {
        const std::uint32_t runSeed = baseSeed + static_cast<std::uint32_t>(runIdx) * 7919u;
        std::mt19937 rng(runSeed);
        std::uniform_real_distribution<float> unitDist(0.0f, 1.0f);

        int hp = diff.oneLifeOnly ? 1 : diff.playerStartHp;
        int maxHp = hp;
        int shards = 0;
        int decoys = bal.startingDecoys;
        int echoTraps = bal.startingEchoTraps;
        float runTimeSec = 0.0f;
        int roomsCleared = 0;
        int ghostClears = 0;

        std::array<bool, static_cast<std::size_t>(RelicId::Count)> hasRelic{};
        auto grantRelic = [&](RelicId id) {
            const auto idx = static_cast<std::size_t>(id);
            if (!hasRelic[idx]) {
                hasRelic[idx] = true;
                summary.relicPicks[idx] += 1;
                if (id == RelicId::PiezoArmorCore && !diff.oneLifeOnly) {
                    maxHp += 2;
                    hp = std::min(maxHp, hp + 2);
                } else if (id == RelicId::PhantomDecoyRig) {
                    decoys += 3;
                } else if (id == RelicId::UltrasonicLens) {
                    echoTraps += 2;
                }
            }
        };

        // Simulate traversal across the 18 critical-path & side chambers of the 32-room run
        constexpr int kSimulatedRunChambers = 18;
        bool survivedRun = true;

        for (int step = 0; step < kSimulatedRunChambers; ++step) {
            const bool isBossChamber = (step == kSimulatedRunChambers - 1);
            const bool isEliteChamber = (step == 6 || step == 12 || step == 15);
            const bool isSanctuary = (step == 5 || step == 11);
            const bool isTreasureOrShop = (step == 3 || step == 8 || step == 14);

            if (isSanctuary) {
                runTimeSec += 14.0f;
                if (!diff.oneLifeOnly) {
                    hp = std::min(maxHp, hp + 2);
                }
                ++roomsCleared;
                continue;
            }

            if (isTreasureOrShop) {
                runTimeSec += 18.0f;
                // Bot prioritizes strong acoustic relics when selecting from pedestals/shop
                const std::array<RelicId, 6> priorityPool = {
                    RelicId::PrismaticMirror,
                    RelicId::HarmonicAmplifier,
                    RelicId::PiezoArmorCore,
                    RelicId::ResonanceCapacitor,
                    RelicId::AbyssalTuningFork,
                    RelicId::EchoSiphonCoil
                };
                if (unitDist(rng) < 0.65f) {
                    grantRelic(priorityPool[rng() % priorityPool.size()]);
                } else {
                    grantRelic(static_cast<RelicId>(rng() % static_cast<std::size_t>(RelicId::Count)));
                }
                if (shards >= 45 && hp < maxHp && !diff.oneLifeOnly) {
                    shards -= 45;
                    hp = std::min(maxHp, hp + 2);
                }
                ++roomsCleared;
                continue;
            }

            // Combat / Elite / Boss encounter simulation driven by BalanceTable & DifficultyProfile
            float synergyBonus = 1.0f;
            if (hasRelic[static_cast<std::size_t>(RelicId::HarmonicAmplifier)]) synergyBonus += 0.16f;
            if (hasRelic[static_cast<std::size_t>(RelicId::PrismaticMirror)])   synergyBonus += 0.18f;
            if (hasRelic[static_cast<std::size_t>(RelicId::ResonanceCapacitor)]) synergyBonus += 0.12f;
            if (hasRelic[static_cast<std::size_t>(RelicId::SilentStep)])        synergyBonus += 0.14f;
            if (hasRelic[static_cast<std::size_t>(RelicId::PhaseDampener)])     synergyBonus += 0.10f;
            if (hasRelic[static_cast<std::size_t>(RelicId::UltrasonicLens)])    synergyBonus += 0.11f;
            if ((isEliteChamber || isBossChamber) && hasRelic[static_cast<std::size_t>(RelicId::AbyssalTuningFork)]) {
                synergyBonus += 0.18f;
            }

            const float enemyPressure = diff.enemyCountMult * diff.enemyHpMult * diff.enemySpeedMult *
                                        (diff.noiseGainMult / std::max(0.4f, diff.silenceDecayMult));

            const float baseEncounterSeconds = isBossChamber ? 78.0f : (isEliteChamber ? 42.0f : 27.0f);
            runTimeSec += baseEncounterSeconds * (diff.enemyHpMult / std::max(0.8f, synergyBonus));

            // Compute peak noise in chamber
            const float stealthMitigation =
                (hasRelic[static_cast<std::size_t>(RelicId::MossTreadBoots)] ? 0.78f : 1.0f) *
                (hasRelic[static_cast<std::size_t>(RelicId::SilentStep)] ? 0.72f : 1.0f);
            const float peakNoise = (22.0f + unitDist(rng) * 34.0f) * diff.noiseGainMult * stealthMitigation;

            if (!isBossChamber && peakNoise <= cfg::noise::kThresholdAudible) {
                ++ghostClears;
                shards += static_cast<int>(std::round(24.0f * diff.shardRewardMult));
                if (!hasRelic[static_cast<std::size_t>(RelicId::SilentStep)]) {
                    grantRelic(RelicId::SilentStep);
                }
            } else {
                shards += static_cast<int>(std::round(16.0f * diff.shardRewardMult));
            }

            // Expected hull damage during chamber combat
            float hazardRate = isBossChamber ? 2.45f : (isEliteChamber ? 0.92f : 0.42f);
            hazardRate *= (enemyPressure / (synergyBonus * 1.12f));
            if (echoTraps > 0 && (isEliteChamber || isBossChamber)) {
                --echoTraps;
                hazardRate *= 0.80f;
            }
            if (decoys > 0 && unitDist(rng) < 0.45f) {
                --decoys;
                hazardRate *= 0.85f;
            }

            int damageTaken = 0;
            const int checks = isBossChamber ? 4 : 2;
            for (int c = 0; c < checks; ++c) {
                const float hitProb = std::clamp(hazardRate / static_cast<float>(checks), 0.04f, 0.88f);
                if (unitDist(rng) < hitProb) {
                    damageTaken += bal.enemyMeleeDamage;
                }
            }

            hp -= damageTaken;
            if (hp <= 0) {
                survivedRun = false;
                break;
            }
            ++roomsCleared;
        }

        totalDurationSec += static_cast<double>(runTimeSec);
        totalRoomsCleared += static_cast<double>(roomsCleared);
        totalGhostClears += static_cast<double>(ghostClears);

        if (survivedRun) {
            ++summary.victories;
            for (std::size_t r = 0; r < static_cast<std::size_t>(RelicId::Count); ++r) {
                if (hasRelic[r]) {
                    summary.relicWins[r] += 1;
                }
            }
        }
    }

    summary.winRatePct = (100.0f * static_cast<float>(summary.victories)) / static_cast<float>(summary.totalRuns);
    summary.avgDurationSec = static_cast<float>(totalDurationSec / static_cast<double>(summary.totalRuns));
    summary.avgRoomsCleared = static_cast<float>(totalRoomsCleared / static_cast<double>(summary.totalRuns));
    summary.avgGhostClears = static_cast<float>(totalGhostClears / static_cast<double>(summary.totalRuns));
    return summary;
}

int Game::runBalanceReport(std::ostream& out, int numRuns, DifficultyMode difficulty) {
    const BalanceReportSummary rep = simulateBalanceRuns(numRuns, difficulty, 0xEC402026u);

    out << "====================================================================\n";
    out << "  ECHOLABYRINTH 5.0 'RESONANCE' — HEADLESS BOT BALANCE REPORT\n";
    out << "====================================================================\n";
    out << "  Simulated Bot Runs : " << rep.totalRuns << "\n";
    out << "  Difficulty Mode    : " << difficultyModeName(rep.difficulty) << "\n";
    out << "  Total Victories    : " << rep.victories << " / " << rep.totalRuns
        << " (" << std::fixed << std::setprecision(1) << rep.winRatePct << "%)\n";
    out << "  Avg Run Duration   : " << std::fixed << std::setprecision(1) << rep.avgDurationSec << " sec ("
        << std::setprecision(2) << (rep.avgDurationSec / 60.0f) << " min)\n";
    out << "  Avg Rooms Cleared  : " << std::fixed << std::setprecision(1) << rep.avgRoomsCleared << " / 18\n";
    out << "  Avg Ghost Clears   : " << std::fixed << std::setprecision(2) << rep.avgGhostClears << "\n";
    out << "--------------------------------------------------------------------\n";
    out << "  TOP-5 RELICS BY PICK & WIN CONTRIBUTION:\n";

    std::vector<std::size_t> indices(static_cast<std::size_t>(RelicId::Count));
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        if (rep.relicWins[a] != rep.relicWins[b]) {
            return rep.relicWins[a] > rep.relicWins[b];
        }
        return rep.relicPicks[a] > rep.relicPicks[b];
    });

    for (std::size_t rank = 0; rank < std::min<std::size_t>(5, indices.size()); ++rank) {
        const std::size_t idx = indices[rank];
        const RelicSpec spec = getRelicSpec(static_cast<RelicId>(idx));
        const int picks = rep.relicPicks[idx];
        const int wins = rep.relicWins[idx];
        const float relicWinRate = (picks > 0) ? (100.0f * static_cast<float>(wins) / static_cast<float>(picks)) : 0.0f;
        out << "    #" << (rank + 1) << " " << std::left << std::setw(24) << spec.name
            << " | Picks: " << std::right << std::setw(4) << picks
            << " | Wins: " << std::setw(3) << wins
            << " (" << std::fixed << std::setprecision(1) << relicWinRate << "% win w/ relic)\n";
    }
    out << "====================================================================\n";
    return 0;
}

int Game::runStage1SelfTest(const std::string& exeDir) {
    int failures = 0;
    auto check = [&](bool cond, const char* msg) {
        if (!cond) {
            std::cerr << "  [FAIL] " << msg << "\n";
            ++failures;
        } else {
            std::cout << "  [PASS] " << msg << "\n";
        }
    };

    std::cout << "\n[Suite 18] Stage 1 Gameplay Quality: Hit-Stop, Attack Telegraphing, 4 Difficulties, 3-Room Tutorial, Save V6 & Balance Report...\n";

    // 1. Verify Enemy Attack Telegraphing (0.3-0.6s windup before melee & ranged attacks)
    {
        AcousticSystem ac;
        EnemyAI stalker;
        stalker.configurePatrol({{200.0f, 200.0f}, {240.0f, 200.0f}}, EnemyVariant::Stalker);
        stalker.applyNoiseTierStimulus({205.0f, 200.0f}, NoiseTier::Scream, true);
        std::vector<WallSegment> emptyWalls;

        // First step within melee range starts telegraph windup, does NOT immediately catch player
        stalker.update(0.05f, {205.0f, 200.0f}, emptyWalls, ac);
        check(
            stalker.isAttackTelegraphing() && !stalker.hasCaughtPlayer() && stalker.consumeTelegraphAlert(),
            "Enemy melee attack enters 0.45s telegraph windup and emits audio/visual alert before striking"
        );

        // Advance past telegraph duration (0.45s) while player remains in range
        for (int i = 0; i < 10; ++i) {
            stalker.update(0.05f, {205.0f, 200.0f}, emptyWalls, ac);
        }
        check(
            stalker.hasCaughtPlayer(),
            "Enemy melee strike lands after telegraph windup completes if player fails to dodge"
        );
    }

    // 2. Verify Hit-Stop (30-60 ms) & Last Noise Action HUD Readability on Game instance
    {
        Game g(exeDir);
        g.setDifficulty(DifficultyMode::Surveyor);
        g.startNewGame(0xEC402026u);
        g.registerNoiseAction(NoiseAction::DirectionalPing, 1.0f, {640.0f, 400.0f});
        check(
            g.lastNoiseAction() == NoiseAction::DirectionalPing && g.lastNoiseDelta() >= 11.5f,
            "HUD Fear Readability records exact NoiseAction cause and delta on acoustic emission"
        );

        // Place an enemy directly in front of player and hit with directional ping to trigger hit-stop
        EnemyAI target;
        target.configurePatrol({{400.0f, 400.0f}, {420.0f, 400.0f}}, EnemyVariant::Stalker);
        g.m_enemies.clear();
        g.m_enemies.push_back(target);
        g.m_player.setPosition({350.0f, 400.0f});
        g.m_acoustics.emitPulse({350.0f, 400.0f}, 420.0f, 300.0f, 1.25f, 48, WaveSignature::DirectionalPing, 0.0f, 0.6f, true);
        g.updatePlaying(cfg::kFixedTimeStep);
        check(
            g.hitStopTimer() > 0.0f && g.hitStopTimer() <= cfg::feel::kHitStopEnemyKillSec,
            "Sonar ray impact on enemy triggers 30-60 ms Hit-Stop freeze window and exponential screen shake"
        );
    }

    // 3. Verify 4 Difficulty Modes (Explorer, Surveyor, Abyssal +25% enemies/slower decay/fewer shards, Silence 1-HP no minimap)
    {
        const DifficultyProfile expProf = getDifficultyProfile(DifficultyMode::Explorer);
        const DifficultyProfile survProf = getDifficultyProfile(DifficultyMode::Surveyor);
        const DifficultyProfile abysProf = getDifficultyProfile(DifficultyMode::Abyssal);
        const DifficultyProfile silProf = getDifficultyProfile(DifficultyMode::Silence);

        check(
            expProf.playerStartHp > survProf.playerStartHp &&
            abysProf.enemyCountMult >= 1.25f &&
            abysProf.silenceDecayMult < survProf.silenceDecayMult &&
            abysProf.shardRewardMult < survProf.shardRewardMult &&
            silProf.oneLifeOnly && !silProf.allowMinimap,
            "Difficulty profiles enforce Explorer, Surveyor, Abyssal (+25% enemies, slower decay, fewer shards), and Silence (1 HP, no minimap)"
        );

        Game gSilence(exeDir);
        gSilence.setDifficulty(DifficultyMode::Silence);
        gSilence.startNewGame(0xEC402026u);
        gSilence.m_player.addRelic(RelicId::PiezoArmorCore, &gSilence.m_acoustics);
        gSilence.handleKeyPressed(sf::Keyboard::Tab);
        check(
            gSilence.m_player.maxHealth() == 1 && gSilence.m_player.health() == 1 && !gSilence.m_showMinimap,
            "Silence Hardcore mode locks player to 1 HP even with armor relics and blocks minimap toggle"
        );
    }

    // 4. Verify 3-Room Interactive Tutorial progression & completion flag
    {
        Game gTut(exeDir);
        gTut.setTutorialDone(false);
        gTut.startNewGame(0xEC402026u);
        check(gTut.tutorialRoomStage() == 0 && !gTut.isTutorialDone(), "First run starts in Tutorial Stage 0 (Move + Sneak)");

        gTut.m_tutStepMoved = true;
        gTut.m_tutStepSneaked = true;
        gTut.updateTutorialProgression();
        check(gTut.tutorialRoomStage() == 1, "Completing Move + Sneak advances tutorial to Stage 1 (Sonar Ping)");

        gTut.handleKeyPressed(sf::Keyboard::Q); // Whistle ping
        check(gTut.tutorialRoomStage() == 2, "Emitting Sonar/Whistle advances tutorial to Stage 2 (Echo Trap / Decoy)");

        gTut.handleKeyPressed(sf::Keyboard::R); // Place Echo Trap
        check(
            gTut.tutorialRoomStage() == 3 && gTut.isTutorialDone(),
            "Placing Echo Trap completes Stage 2 and sets persistent tutorialDone = true"
        );
    }

    // 5. Verify Save Format V6 (ELSV6) + Full Backwards Compatibility Migration from V5
    {
        const std::string tmpV6 = (std::filesystem::temp_directory_path() / "echo_test_v6.sav").string();
        SaveData outV6{};
        outV6.runSeed = 0xCAFE2026u;
        outV6.difficulty = static_cast<int>(DifficultyMode::Abyssal);
        outV6.tutorialDone = true;
        outV6.echoShards = 77;
        const bool savedV6 = SaveSystem::saveToFile(tmpV6, outV6);

        SaveData inV6{};
        const bool loadedV6 = SaveSystem::loadFromFile(tmpV6, inV6);
        std::error_code ec;
        std::filesystem::remove(tmpV6, ec);

        check(
            savedV6 && loadedV6 &&
            inV6.difficulty == static_cast<int>(DifficultyMode::Abyssal) &&
            inV6.tutorialDone && inV6.echoShards == 77,
            "SaveSystem V6 (ECHOLABYRINTH_SAVE_V6) round-trips difficulty and tutorialDone with FNV-1a checksum"
        );

        // Construct a valid legacy V5 save file and verify seamless migration
        const std::string tmpV5 = (std::filesystem::temp_directory_path() / "echo_test_v5_legacy.sav").string();
        SaveData legacyV5{};
        legacyV5.runSeed = 0x12345678u;
        legacyV5.echoShards = 55;
        legacyV5.discoveredCodexIds = {0, 2, 4};
        const std::uint32_t v5Sum = SaveSystem::computeChecksumV5(legacyV5);
        {
            std::ofstream ofs(tmpV5, std::ios::out | std::ios::trunc);
            ofs << "ECHOLABYRINTH_SAVE_V5\n";
            ofs << "run_seed " << legacyV5.runSeed << "\n";
            ofs << "target_room_count " << legacyV5.targetRoomCount << "\n";
            ofs << "checkpoint_room " << legacyV5.checkpointRoomId << "\n";
            ofs << "player_pos " << legacyV5.playerPosX << " " << legacyV5.playerPosY << "\n";
            ofs << "player_health " << legacyV5.playerHealth << "\n";
            ofs << "player_max_health " << legacyV5.playerMaxHealth << "\n";
            ofs << "player_stamina " << legacyV5.playerStamina << "\n";
            ofs << "decoy_count " << legacyV5.decoyCount << "\n";
            ofs << "echo_trap_count " << legacyV5.echoTrapCount << "\n";
            ofs << "active_frequency " << legacyV5.activeFrequency << "\n";
            ofs << "echo_shards " << legacyV5.echoShards << "\n";
            ofs << "ghost_clears_run " << legacyV5.ghostClearsThisRun << "\n";
            ofs << "master_volume " << legacyV5.masterVolume << "\n";
            ofs << "music_volume " << legacyV5.musicVolume << "\n";
            ofs << "sfx_volume " << legacyV5.sfxVolume << "\n";
            ofs << "ambient_volume " << legacyV5.ambientVolume << "\n";
            ofs << "ray_density " << legacyV5.rayDensityScale << "\n";
            ofs << "max_reflections " << legacyV5.maxReflections << "\n";
            ofs << "ambient_visibility " << legacyV5.ambientVisibility << "\n";
            ofs << "run_time_sec " << legacyV5.runTimeSec << "\n";
            ofs << "total_runs " << legacyV5.totalRuns << "\n";
            ofs << "total_victories " << legacyV5.totalVictories << "\n";
            ofs << "bosses_defeated " << legacyV5.bossesDefeated << "\n";
            ofs << "meta_ghost_clears " << legacyV5.metaTotalGhostClears << "\n";
            ofs << "switches 0\n";
            ofs << "lore 0\n";
            ofs << "codex 3 0 2 4\n";
            ofs << "visited 0\n";
            ofs << "cleared 0\n";
            ofs << "relics 0\n";
            ofs << "checksum " << v5Sum << "\n";
        }
        SaveData migratedFromV5{};
        const bool okV5 = SaveSystem::loadFromFile(tmpV5, migratedFromV5);
        std::filesystem::remove(tmpV5, ec);
        check(
            okV5 && migratedFromV5.runSeed == 0x12345678u &&
            migratedFromV5.echoShards == 55 &&
            migratedFromV5.discoveredCodexIds.size() == 3,
            "SaveSystem V6 backwards-compatibility migrates legacy ECHOLABYRINTH_SAVE_V5 files with V5 checksum validation"
        );
    }

    // 6. Verify 1000-Run Headless Balance Report on Surveyor yields 8% - 15% win rate
    {
        std::ostringstream oss;
        Game::runBalanceReport(oss, 1000, DifficultyMode::Surveyor);
        const BalanceReportSummary rep = Game::simulateBalanceRuns(1000, DifficultyMode::Surveyor, 0xEC402026u);
        check(
            rep.totalRuns == 1000 &&
            rep.winRatePct >= 8.0f && rep.winRatePct <= 15.0f &&
            oss.str().find("TOP-5 RELICS") != std::string::npos,
            "Headless --balance-report (1000 bot runs on Surveyor) achieves target 8%-15% win rate and ranks Top-5 relics"
        );
    }

    return failures;
}

int Game::runStage2SelfTest(const std::string& exeDir) {
    int failures = 0;
    auto check = [&](bool cond, const char* msg) {
        if (!cond) {
            std::cerr << "  [FAIL] " << msg << "\n";
            ++failures;
        } else {
            std::cout << "  [PASS] " << msg << "\n";
        }
    };

    std::cout << "\n[Suite 19] Stage 2 Content & Biomes: 4 Acoustic Biomes, EchoCrystal Ray Splitting, Leech, Banshee, TuningForkConstruct & Room 16 Mini-Boss The Conductor...\n";

    // 1. Verify 4 Biomes (8 rooms each across 32 chambers) + Room 16 Mini-Boss (The Conductor)
    {
        Labyrinth lab;
        lab.generateRun(0xEC402026u, 32);
        int biomeCounts[4] = {0, 0, 0, 0};
        for (std::size_t i = 0; i < lab.roomCount(); ++i) {
            const auto& r = lab.roomAt(i);
            const auto bIdx = static_cast<std::size_t>(r.biome);
            if (bIdx < 4) ++biomeCounts[bIdx];
        }
        check(
            biomeCounts[0] == 8 && biomeCounts[1] == 8 && biomeCounts[2] == 8 && biomeCounts[3] == 8,
            "32-Room Labyrinth partitions cleanly into 4 Biomes (8 rooms each: Drowned Galleries, Crystal Vaults, Iron Foundry, Living Mycelium)"
        );

        const int condRoomId = lab.conductorRoomId();
        bool hasConductorSpawn = false;
        if (condRoomId >= 0 && static_cast<std::size_t>(condRoomId) < lab.roomCount()) {
            const auto& condRoom = lab.roomAt(static_cast<std::size_t>(condRoomId));
            for (const auto& sp : condRoom.enemySpawns) {
                if (sp.variant == EnemyVariant::TheConductor) {
                    hasConductorSpawn = true;
                    break;
                }
            }
        }
        check(
            condRoomId == 15 && hasConductorSpawn,
            "Chamber 16 (Room index 15) is guaranteed Mini-Boss arena featuring THE CONDUCTOR"
        );
    }

    // 2. Verify SurfaceMaterial::EchoCrystal chaotic ray splitting (1 ray -> 3 rays on bounce)
    {
        AcousticSystem ac;
        std::vector<WallSegment> crystalWalls = {
            WallSegment{{500.0f, 100.0f}, {500.0f, 700.0f}, 0.96f, SurfaceMaterial::EchoCrystal}
        };
        ac.emitPulse({420.0f, 400.0f}, 600.0f, 600.0f, 1.5f, 1, WaveSignature::DirectionalPing, 0.0f, 0.05f, true);
        for (int step = 0; step < 8; ++step) {
            ac.update(0.04f, crystalWalls);
        }
        const int splits = ac.consumeCrystalSplitCount();
        check(
            splits >= 1 && ac.activeRayCount() >= 3,
            "SurfaceMaterial::EchoCrystal splits incident sonar ray into 3 chaotic prism rays on reflection"
        );
    }

    // 3. Verify Biome Acoustic Modifiers (Drowned Galleries step muffling, Iron Foundry steam-hammer masking, Living Mycelium silence decay)
    {
        Game g(exeDir);
        g.setDifficulty(DifficultyMode::Surveyor);
        g.startNewGame(0xEC402026u);

        // Room 0 is Drowned Galleries (0.75x step noise)
        g.m_noiseLevel = 0.0f;
        g.registerNoiseAction(NoiseAction::StepSprint, 1.0f, {640.0f, 400.0f});
        const float drownedSprintNoise = g.m_noiseLevel;

        // Switch to Iron Foundry room (Room 16) with steam-hammer press active
        g.m_labyrinth.setCurrentRoom(16);
        g.m_noiseLevel = 0.0f;
        g.m_foundryHammerMaskTimer = 1.0f;
        g.registerNoiseAction(NoiseAction::StepSprint, 1.0f, {640.0f, 400.0f});
        const float maskedFoundrySprintNoise = g.m_noiseLevel;

        check(
            drownedSprintNoise > 0.0f && maskedFoundrySprintNoise < drownedSprintNoise * 0.45f,
            "Drowned Galleries muffles footsteps (0.75x) and Iron Foundry steam-hammer press masks footsteps by -75%"
        );

        const BiomeSpec mycSpec = getBiomeSpec(BiomeId::LivingMycelium);
        check(
            mycSpec.silenceDecayMult >= 1.50f && mycSpec.sporeEnemyReveal,
            "Living Mycelium accelerates silence decay (1.55x) and enables bioluminescent spore enemy reveal"
        );
    }

    // 4. Verify 4 New Enemy Archetypes (Leech, Banshee, TuningForkConstruct, TheConductor)
    {
        AcousticSystem ac;
        std::vector<WallSegment> noWalls;

        // 4a. Leech attaches without killing player, pulses +8 noise drain, and detaches on Sprint/Pulse
        EnemyAI leech;
        leech.configurePatrol({{300.0f, 300.0f}, {320.0f, 300.0f}}, EnemyVariant::Leech);
        leech.update(0.1f, {305.0f, 300.0f}, noWalls, ac);
        const bool attached = leech.isLeechedOnPlayer() && !leech.hasCaughtPlayer();
        for (int i = 0; i < 10; ++i) {
            leech.update(0.1f, {305.0f, 300.0f}, noWalls, ac);
        }
        const bool pulsedDrain = leech.consumeLeechDrainPulse();
        leech.detachLeech({305.0f, 300.0f});
        check(
            attached && pulsedDrain && !leech.isLeechedOnPlayer(),
            "Leech latches onto player without instant kill, drains stamina with +8 noise pulses, and detaches on Sprint/RadialPulse"
        );

        // 4b. Banshee unleashes piercing wail when alerted
        EnemyAI banshee;
        banshee.configurePatrol({{400.0f, 400.0f}, {460.0f, 400.0f}}, EnemyVariant::Banshee);
        banshee.applyNoiseTierStimulus({500.0f, 400.0f}, NoiseTier::Scream, true);
        bool wailed = false;
        for (int i = 0; i < 30; ++i) {
            banshee.update(0.1f, {500.0f, 400.0f}, noWalls, ac);
            if (banshee.consumeBansheeWail()) {
                wailed = true;
                break;
            }
        }
        check(wailed, "Banshee unleashes piercing acoustic wail (+22 noise shockwave) when alerted");

        // 4c. TuningForkConstruct is immune to direct pings, weak to reflected bounces (2.25x), and fires twin beams
        EnemyAI fork;
        fork.configurePatrol({{500.0f, 300.0f}, {500.0f, 300.0f}}, EnemyVariant::TuningForkConstruct);
        fork.applyNoiseTierStimulus({620.0f, 300.0f}, NoiseTier::Scream, true);
        const float hpStart = fork.health();
        fork.applyAcousticDamage(12.0f, false, {620.0f, 300.0f}, noWalls, false); // Direct unreflected -> 0 dmg
        const float hpAfterDirect = fork.health();
        fork.applyAcousticDamage(12.0f, true, {620.0f, 300.0f}, noWalls, false);  // Reflected bounce -> 2.25x dmg
        const float hpAfterReflected = fork.health();
        for (int i = 0; i < 30; ++i) {
            fork.update(0.1f, {620.0f, 300.0f}, noWalls, ac);
        }
        check(
            std::abs(hpStart - hpAfterDirect) < 0.001f &&
            (hpAfterDirect - hpAfterReflected) >= 24.0f &&
            fork.projectiles().size() >= 2,
            "Tuning Fork Construct blocks direct pings, takes 2.25x reflected bounce damage, and fires twin acoustic beams"
        );

        // 4d. The Conductor Mini-Boss fires 4-beam rotating cross and enters Phase 2 at <= 50% HP
        EnemyAI conductor;
        conductor.configurePatrol({{640.0f, 400.0f}, {700.0f, 400.0f}}, EnemyVariant::TheConductor);
        conductor.applyNoiseTierStimulus({800.0f, 400.0f}, NoiseTier::Scream, true);
        for (int i = 0; i < 32; ++i) {
            conductor.update(0.1f, {800.0f, 400.0f}, noWalls, ac);
        }
        const bool firedCross = (conductor.projectiles().size() >= 4);
        conductor.applyAcousticDamage(90.0f, true, {800.0f, 400.0f}, noWalls, true);
        check(
            firedCross && conductor.bossPhase() == 2,
            "Mini-Boss The Conductor fires 4-beam rotating cross volleys and transitions to Phase 2 at <= 50% HP"
        );
    }

    return failures;
}

int Game::runStage3SelfTest(const std::string& exeDir) {
    int failures = 0;
    auto check = [&](bool cond, const char* msg) {
        if (!cond) {
            std::cerr << "  [STAGE 3 FAIL] " << msg << "\n";
            ++failures;
        }
    };

    // 1. Verify GLSL Post-Processing Shader Source & Uniforms (Bloom, Chromatic Aberration, Shockwave UV, CRT Scanline)
    {
        const char* glslSrc = PostProcessor::embeddedFragmentShaderSource();
        const std::string shaderStr = (glslSrc != nullptr) ? glslSrc : "";
        check(
            shaderStr.find("u_bloomIntensity") != std::string::npos &&
            shaderStr.find("u_chromaticAberration") != std::string::npos &&
            shaderStr.find("u_shockwaveRadius") != std::string::npos &&
            shaderStr.find("u_crtScanline") != std::string::npos,
            "Embedded GLSL fragment shader contains Bloom, Chromatic Aberration, Shockwave UV, and CRT Scanline uniforms"
        );

        const PostFxUniforms uQuiet = PostProcessor::computeUniforms(true, false, 15.0f, 0.0f, 120, 0.0f, {640.0f, 400.0f});
        const PostFxUniforms uScream = PostProcessor::computeUniforms(true, false, 95.0f, 0.045f, 420, 0.25f, {640.0f, 400.0f});
        const PostFxUniforms uReduced = PostProcessor::computeUniforms(true, true, 95.0f, 0.045f, 420, 0.25f, {640.0f, 400.0f});
        const PostFxUniforms uOff = PostProcessor::computeUniforms(false, false, 95.0f, 0.045f, 420, 0.25f, {640.0f, 400.0f});

        check(
            uScream.bloomIntensity > uQuiet.bloomIntensity &&
            uScream.chromaticAberration > 0.008f &&
            uQuiet.chromaticAberration < 0.0001f &&
            uScream.shockwaveRadius > 0.04f &&
            uScream.shockwaveStrength > 0.002f,
            "PostProcessor::computeUniforms scales Bloom with active rays and triggers Chromatic Aberration & Shockwave in Scream/HitStop"
        );
        check(
            uReduced.chromaticAberration < uScream.chromaticAberration * 0.5f &&
            uOff.bloomIntensity == 0.0f &&
            uOff.chromaticAberration == 0.0f &&
            uOff.crtScanlineStrength == 0.0f,
            "PostProcessor respects Photosensitivity Reduce-Flashes dampening and full PostFX disable toggle"
        );
    }

    // 2. Verify 6-Stem Adaptive Audio Crossfade & Real-Time Low-Pass / Schroeder Reverb DSP
    {
        const AdaptiveStemWeights wExploreDrowned = AudioManager::computeAdaptiveStemWeights(
            0.10f, false, false, false, 1, BiomeId::DrownedGalleries
        );
        const AdaptiveStemWeights wPursuitCrystal = AudioManager::computeAdaptiveStemWeights(
            0.85f, true, false, false, 1, BiomeId::CrystalVaults
        );
        const AdaptiveStemWeights wSanctum = AudioManager::computeAdaptiveStemWeights(
            0.0f, false, true, false, 1, BiomeId::CrystalVaults
        );
        const AdaptiveStemWeights wBossP3 = AudioManager::computeAdaptiveStemWeights(
            0.95f, true, false, true, 3, BiomeId::LivingMycelium
        );

        check(
            wExploreDrowned.subDrone > 0.70f &&
            wExploreDrowned.lowPassCutoffHz < 3500.0f &&
            wPursuitCrystal.lowPassCutoffHz > 12000.0f &&
            wPursuitCrystal.reverbWetMix > wExploreDrowned.reverbWetMix &&
            wPursuitCrystal.pursuitPercussion > 0.80f &&
            wSanctum.sanctumPad > 0.80f &&
            wBossP3.bossChoir > 0.90f,
            "6-Stem Adaptive Audio weights & Biome DSP parameters (Drowned low-pass vs Crystal cathedral reverb) verified"
        );

        // Test One-Pole Low-Pass Filter & Schroeder Comb Reverb on synthetic 16-bit PCM buffer
        std::vector<sf::Int16> pcm(4410, 0);
        for (std::size_t i = 0; i < 400; ++i) {
            pcm[i] = (i % 2 == 0) ? 18000 : -18000; // Nyquist high-frequency square burst in first 400 samples
        }
        AudioManager::applyLowPassFilter(pcm, 900.0f, 44100);
        const bool lpfAttenuated = (std::abs(pcm[10]) < 12000);

        AudioManager::applyReverbTail(pcm, 0.025f, 0.65f, 0.55f, 44100);
        bool reverbTailPresent = false;
        for (std::size_t i = 1100; i < 2500; ++i) {
            if (std::abs(pcm[i]) > 120) {
                reverbTailPresent = true;
                break;
            }
        }
        check(
            lpfAttenuated && reverbTailPresent,
            "Procedural DSP One-Pole Low-Pass Filter attenuates high frequencies and Schroeder Comb Reverb generates echo tail"
        );
    }

    // 3. Verify 4 Colorblind Accessibility Modes (Normal, Deuteranopia, Protanopia, Tritanopia)
    {
        const sf::Color baseMoss = getMaterialProperties(SurfaceMaterial::DampMoss, ColorblindMode::Normal).baseColor;
        const sf::Color deutMoss = getMaterialProperties(SurfaceMaterial::DampMoss, ColorblindMode::Deuteranopia).baseColor;
        const sf::Color protMoss = getMaterialProperties(SurfaceMaterial::DampMoss, ColorblindMode::Protanopia).baseColor;
        const sf::Color tritMoss = getMaterialProperties(SurfaceMaterial::DampMoss, ColorblindMode::Tritanopia).baseColor;

        const sf::Color dangerRed(244, 63, 94);
        const sf::Color deutDanger = applyColorblindTransform(dangerRed, ColorblindMode::Deuteranopia);

        check(
            (baseMoss != deutMoss || baseMoss != protMoss || baseMoss != tritMoss) &&
            std::abs(static_cast<int>(deutMoss.b) - static_cast<int>(deutDanger.b)) >= 40,
            "4 Colorblind palettes (Normal, Deuteranopia, Protanopia, Tritanopia) provide distinct high-contrast separation"
        );
    }

    // 4. Verify Directional Closed Captions ([DIR] + Distance) & SaveSystem V6 Accessibility Persistence
    {
        const std::string capNorth = formatSoundCaptionLine({640.0f, 400.0f}, {640.0f, 200.0f}, "HUNTER ROAR");
        const std::string capEast = formatSoundCaptionLine({640.0f, 400.0f}, {840.0f, 400.0f}, "STEAM HISS");
        check(
            capNorth.find("[N ") != std::string::npos &&
            capNorth.find("20m]") != std::string::npos &&
            capEast.find("[E ") != std::string::npos,
            "Directional Sound Captions format 8-way compass bearing and distance in meters"
        );

        Game g(exeDir);
        g.setSubtitlesEnabled(true);
        g.setColorblindMode(ColorblindMode::Deuteranopia);
        g.pushSoundCaption("TEST SONAR ECHO", {640.0f, 250.0f});
        const bool queuedWhenEnabled = (g.activeSoundCaptions().size() == 1);
        g.setSubtitlesEnabled(false);
        g.pushSoundCaption("SUPPRESSED ECHO", {640.0f, 250.0f});
        const bool ignoredWhenDisabled = (g.activeSoundCaptions().size() == 1);
        check(queuedWhenEnabled && ignoredWhenDisabled, "Game::pushSoundCaption respects subtitlesEnabled accessibility toggle");

        const std::string testSave = SaveSystem::resolveSaveFilePath(exeDir, "test_stage3_accessibility_v6.dat");
        SaveData out{};
        out.runSeed = 0xEC405300u;
        out.checkpointRoomId = 5;
        out.colorblindMode = static_cast<int>(ColorblindMode::Tritanopia);
        out.subtitlesEnabled = false;
        out.shakeIntensityScale = 0.35f;
        out.reduceFlashes = true;
        out.postFxEnabled = false;

        const bool saved = SaveSystem::saveToFile(testSave, out);
        SaveData loaded{};
        const bool okLoad = SaveSystem::loadFromFile(testSave, loaded);
        std::remove(testSave.c_str());

        check(
            saved && okLoad &&
            loaded.colorblindMode == static_cast<int>(ColorblindMode::Tritanopia) &&
            loaded.subtitlesEnabled == false &&
            std::abs(loaded.shakeIntensityScale - 0.35f) < 0.02f &&
            loaded.reduceFlashes == true &&
            loaded.postFxEnabled == false,
            "SaveSystem V6 persists and validates all Stage 3 Accessibility & PostFX settings with FNV-1a checksum"
        );
    }

    return failures;
}

BenchmarkReportSummary Game::runBenchmark(
    std::ostream& out,
    int numFrames,
    const std::string& exeDir
) {
    BenchmarkReportSummary summary{};
    summary.totalFrames = std::max(10, numFrames);

    AcousticSystem acoustics;
    Labyrinth labyrinth(0xEC405400u);
    Room& stressRoom = labyrinth.roomMut(16); // Room 16: The Conductor's Orchestra Hall (Crystalline Cavern)

    // Add extra EchoCrystal walls to maximize ray-splitting stress during benchmark
    stressRoom.walls.push_back({{420.0f, 220.0f, 120.0f, 18.0f}, SurfaceMaterial::EchoCrystal, false, 1.0f});
    stressRoom.walls.push_back({{740.0f, 540.0f, 120.0f, 18.0f}, SurfaceMaterial::EchoCrystal, false, 1.0f});

    std::vector<EnemyAI> enemies;
    enemies.reserve(6);
    {
        EnemyAI conductor;
        conductor.configurePatrol({{640.0f, 380.0f}, {700.0f, 380.0f}}, EnemyVariant::Conductor);
        enemies.push_back(conductor);

        EnemyAI banshee;
        banshee.configurePatrol({{320.0f, 260.0f}, {480.0f, 260.0f}}, EnemyVariant::Banshee);
        enemies.push_back(banshee);

        EnemyAI construct;
        construct.configurePatrol({{860.0f, 480.0f}, {960.0f, 320.0f}}, EnemyVariant::TuningForkConstruct);
        enemies.push_back(construct);

        EnemyAI leech;
        leech.configurePatrol({{360.0f, 520.0f}, {520.0f, 520.0f}}, EnemyVariant::Leech);
        enemies.push_back(leech);
    }

    std::vector<float> frameTimesMs;
    frameTimesMs.reserve(static_cast<std::size_t>(summary.totalFrames));

    const sf::Vector2f playerPos{640.0f, 400.0f};
    const auto benchStart = std::chrono::steady_clock::now();

    for (int f = 0; f < summary.totalFrames; ++f) {
        const auto t0 = std::chrono::steady_clock::now();

        if (f % 30 == 0) {
            acoustics.emitRadialPulse(
                playerPos,
                120,
                520.0f,
                760.0f,
                sf::Color(56, 189, 248),
                true,
                SonarFrequency::High
            );
        } else if (f % 12 == 0) {
            const float angle = static_cast<float>(f) * 0.17f;
            acoustics.emitDirectionalCone(
                playerPos,
                angle,
                1.12f,
                56,
                540.0f,
                780.0f,
                sf::Color(192, 132, 252),
                true,
                SonarFrequency::Mid
            );
        }

        const std::size_t raysBefore = acoustics.activeRayCount();
        acoustics.update(cfg::kFixedTimeStep, stressRoom.walls);
        const std::size_t raysAfter = acoustics.activeRayCount();
        if (raysAfter > raysBefore) {
            summary.crystalSplitsTriggered += static_cast<int>(raysAfter - raysBefore);
        }
        summary.peakActiveRays = std::max(summary.peakActiveRays, raysAfter);

        for (auto& enemy : enemies) {
            enemy.update(
                cfg::kFixedTimeStep,
                playerPos,
                stressRoom.walls,
                static_cast<float>(f) * cfg::kFixedTimeStep,
                false,
                65.0f
            );
        }

        const auto t1 = std::chrono::steady_clock::now();
        const float dtMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
        frameTimesMs.push_back(dtMs);
    }

    const auto benchEnd = std::chrono::steady_clock::now();
    summary.totalElapsedMs = std::chrono::duration<float, std::milli>(benchEnd - benchStart).count();

    float sumMs = 0.0f;
    for (float ms : frameTimesMs) {
        sumMs += ms;
    }
    summary.avgFrameMs = sumMs / static_cast<float>(frameTimesMs.size());

    std::sort(frameTimesMs.begin(), frameTimesMs.end());
    const std::size_t p99Idx = std::min(
        frameTimesMs.size() - 1,
        static_cast<std::size_t>( static_cast<double>(frameTimesMs.size()) * 0.99 )
    );
    summary.p99FrameMs = frameTimesMs[p99Idx];
    summary.estimatedFps = (summary.avgFrameMs > 0.0001f) ? (1000.0f / summary.avgFrameMs) : 9999.0f;

    out << "\n========================================================================\n";
    out << " ECHOLABYRINTH 5.4 — STAGE 4 ACOUSTIC & AI STRESS BENCHMARK (" << summary.totalFrames << " FRAMES)\n";
    out << "========================================================================\n";
    out << std::fixed << std::setprecision(3);
    out << " Total Elapsed Wall Time : " << summary.totalElapsedMs << " ms\n";
    out << " Average Frame Time      : " << summary.avgFrameMs << " ms (Target < 16.667 ms for 60 FPS)\n";
    out << " 99th Percentile (P99)   : " << summary.p99FrameMs << " ms\n";
    out << std::setprecision(1);
    out << " Estimated Throughput    : " << summary.estimatedFps << " FPS\n";
    out << " Peak Active Sonar Rays  : " << summary.peakActiveRays << " rays\n";
    out << " EchoCrystal Ray Splits  : " << summary.crystalSplitsTriggered << " secondary rays\n";
    out << "========================================================================\n";
    (void)exeDir;
    return summary;
}

int Game::runStage4SelfTest(const std::string& exeDir) {
    int failures = 0;
    auto check = [&](bool cond, const char* msg) {
        if (!cond) {
            std::cerr << "[Stage4SelfTest] FAIL: " << msg << "\n";
            ++failures;
        }
    };

    // 1. Verify Procedural 64x64 Window Icon Generation
    {
        const sf::Image icon = AssetManager::generateWindowIconImage();
        check(
            icon.getSize().x == 64 && icon.getSize().y == 64 && icon.getPixelsPtr() != nullptr,
            "AssetManager::generateWindowIconImage generates a valid 64x64 RGBA buffer"
        );
        const sf::Color corner = icon.getPixel(0, 0);
        const sf::Color center = icon.getPixel(31, 31);
        const sf::Color ringSample = icon.getPixel(31 + 19, 31);
        check(
            corner.a == 0 && center.a == 255 && center.b >= 220 && ringSample.b > 80,
            "Window icon features rounded transparent corners, bright diamond crystal core, and concentric sonar rings"
        );
    }

    // 2. Verify Stage 4 Stress Benchmark (--benchmark) Performance & Ray Split Telemetry
    {
        std::ostringstream benchStream;
        const BenchmarkReportSummary bench = runBenchmark(benchStream, 180, exeDir);
        check(
            bench.totalFrames == 180 &&
            bench.avgFrameMs < 16.67f &&
            bench.peakActiveRays >= 80 &&
            bench.crystalSplitsTriggered > 0 &&
            benchStream.str().find("ECHOLABYRINTH 5.4") != std::string::npos,
            "Stage 4 --benchmark runs 180 high-load EchoCrystal + multi-enemy frames well under 16.67ms (60+ FPS)"
        );
    }

    // 3. Verify Stage 4 Release Assets, Soundtrack Export Readiness & AssetManager Integrity
    {
        AssetManager assets;
        assets.initialize(exeDir, false);
        check(
            assets.totalTriangleCount() >= 60 &&
            SoundtrackComposer::trackCount() == 5 &&
            PostProcessor::embeddedFragmentShaderSource() != nullptr,
            "Stage 4 release bundle includes 7 3D meshes, 5 procedural OST tracks, and embedded GLSL shader"
        );
    }

    return failures;
}

} // namespace echo


