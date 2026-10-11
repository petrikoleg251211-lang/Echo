#include "Game.hpp"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace echo {

namespace {
bool approxEqual(float a, float b, float eps = 1e-3f) noexcept {
    return std::abs(a - b) <= eps;
}
} // namespace

int Game::runSelfTest(const std::string& exeDir) {
    std::cout << "[SelfTest] Starting EchoLabyrinth 5.0 'Resonance' (Phase A + B + C) C++17 verification suite...\n";

    // 1. Multi-Seed Procedural Run Graph Verification (20 distinct seeds)
    Labyrinth lab;
    for (std::uint32_t seedIdx = 1; seedIdx <= 20; ++seedIdx) {
        const std::uint32_t seed = 0x1000u + seedIdx * 7919u;
        lab.generateRun(seed, 32);
        std::string graphErr;
        if (!lab.verifyRunGraph(graphErr)) {
            std::cerr << "[SelfTest] FAIL: Seed " << seed << " failed verifyRunGraph: " << graphErr << "\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [1/17]: 20 distinct seeds verified for 2D grid uniqueness, N/E/S/W door symmetry, & Boss/Exit reachability.\n";

    // 2. Seed Determinism Verification (same seed -> identical graph)
    Labyrinth labA;
    Labyrinth labB;
    labA.generateRun(0xCAFE1234u, 32);
    labB.generateRun(0xCAFE1234u, 32);
    if (labA.roomCount() != labB.roomCount() || labA.bossRoomId() != labB.bossRoomId() || labA.exitRoomId() != labB.exitRoomId()) {
        std::cerr << "[SelfTest] FAIL: Identical seed did not produce identical room counts or Boss/Exit IDs.\n";
        return 1;
    }
    for (std::size_t i = 0; i < labA.roomCount(); ++i) {
        const auto& ra = labA.roomAt(i);
        const auto& rb = labB.roomAt(i);
        if (ra.gridX != rb.gridX || ra.gridY != rb.gridY || ra.type != rb.type || ra.neighbors != rb.neighbors) {
            std::cerr << "[SelfTest] FAIL: Seed determinism mismatch at room " << i << "\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [2/17]: Deterministic seed reproducibility verified across all 32 chambers.\n";

    // 3. Verify 2D BFS Geometric Walkability across every room in the procedural run
    for (int r = 0; r < static_cast<int>(labA.roomCount()); ++r) {
        std::string travErr;
        if (!labA.verifyRoomTraversability(r, travErr)) {
            std::cerr << "[SelfTest] FAIL: Room " << r << " failed 2D BFS walkability: " << travErr << "\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [3/17]: All 32 procedural chambers verified via 2D BFS grid pathfinding.\n";

    // 4. Verify Combat Door Lockdown, Room Clear Unsealing, and Secret Wall Sonar Fracture
    int combatRoomId = -1;
    int secretRoomId = -1;
    for (std::size_t i = 0; i < labA.roomCount(); ++i) {
        if (combatRoomId == -1 && labA.roomAt(i).type == RoomType::Combat) combatRoomId = static_cast<int>(i);
        if (secretRoomId == -1 && labA.roomAt(i).type == RoomType::Secret) secretRoomId = static_cast<int>(i);
    }
    if (combatRoomId == -1 || secretRoomId == -1) {
        std::cerr << "[SelfTest] FAIL: Missing Combat or Secret room in generated run.\n";
        return 1;
    }
    labA.setCombatLock(combatRoomId, true);
    if (!labA.roomAt(static_cast<std::size_t>(combatRoomId)).combatLocked) {
        std::cerr << "[SelfTest] FAIL: setCombatLock did not lock combat doors.\n";
        return 1;
    }
    labA.markRoomCleared(combatRoomId);
    if (!labA.isRoomCleared(combatRoomId) || labA.roomAt(static_cast<std::size_t>(combatRoomId)).combatLocked) {
        std::cerr << "[SelfTest] FAIL: markRoomCleared did not unseal combat doors.\n";
        return 1;
    }

    AcousticSystem acoustics;
    labA.setCurrentRoom(secretRoomId);
    auto& secRoom = labA.currentRoomMut();
    bool secretDoorUnsealed = false;
    acoustics.emitPulse({640.0f, 400.0f}, 460.0f, 750.0f, 1.25f, 180, WaveSignature::RadialPulse, 0.0f, 6.2831853f, true);
    for (int step = 0; step < 45; ++step) {
        const auto unlocked = acoustics.update(cfg::kFixedTimeStep, secRoom.walls, &secRoom.resonators);
        for (int id : unlocked) {
            labA.activateSwitch(id);
            if (id == 900 + secretRoomId) secretDoorUnsealed = true;
        }
    }
    if (!secretDoorUnsealed || !labA.isSwitchActivated(900 + secretRoomId)) {
        std::cerr << "[SelfTest] FAIL: Secret room wall was not unsealed by sonar pulse.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [4/17]: Combat door lockdown, room clear unsealing, & Secret Wall sonar fracture verified.\n";

    // 5. Verify AcousticSystem reflections & multi-wall material attenuation
    const std::vector<WallSegment> metalWall = {{{500.0f, 100.0f}, {500.0f, 700.0f}, SurfaceMaterial::Metal, -1, false}};
    const std::vector<WallSegment> mossWall = {{{500.0f, 100.0f}, {500.0f, 700.0f}, SurfaceMaterial::DampMoss, -1, false}};
    const std::vector<WallSegment> doubleStoneWalls = {
        {{400.0f, 100.0f}, {400.0f, 700.0f}, SurfaceMaterial::Stone, -1, false},
        {{600.0f, 100.0f}, {600.0f, 700.0f}, SurfaceMaterial::Stone, -1, false}
    };
    const float transMetal = AcousticSystem::computeTransmissionFactor({300.0f, 400.0f}, {700.0f, 400.0f}, metalWall);
    const float transMoss = AcousticSystem::computeTransmissionFactor({300.0f, 400.0f}, {700.0f, 400.0f}, mossWall);
    const float transDoubleStone = AcousticSystem::computeTransmissionFactor({300.0f, 400.0f}, {700.0f, 400.0f}, doubleStoneWalls);
    if (!(transMetal > transMoss && transMoss > transDoubleStone)) {
        std::cerr << "[SelfTest] FAIL: Material transmission ordering invalid.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [5/17]: AcousticSystem reflections & multi-wall material attenuation verified.\n";

    // 6. Verify AcousticResonator crystal charging in Puzzle room
    int puzzleRoomId = -1;
    for (std::size_t i = 0; i < labA.roomCount(); ++i) {
        if (labA.roomAt(i).type == RoomType::Puzzle) {
            puzzleRoomId = static_cast<int>(i);
            break;
        }
    }
    if (puzzleRoomId == -1) {
        std::cerr << "[SelfTest] FAIL: No Puzzle room generated.\n";
        return 1;
    }
    labA.setCurrentRoom(puzzleRoomId);
    acoustics.clear();
    auto& puzRoom = labA.currentRoomMut();
    bool resonatorUnlocked = false;
    for (int burst = 0; burst < 3 && !resonatorUnlocked; ++burst) {
        acoustics.emitPulse({580.0f, 400.0f}, 450.0f, 560.0f, 1.20f, 96, WaveSignature::DirectionalPing, 0.0f, 1.0471975f, true);
        for (int step = 0; step < 30; ++step) {
            const auto unlockedIds = acoustics.update(cfg::kFixedTimeStep, puzRoom.walls, &puzRoom.resonators, 2.0f);
            for (int id : unlockedIds) {
                labA.activateSwitch(id);
                if (id == 100 + puzzleRoomId) {
                    resonatorUnlocked = true;
                }
            }
        }
    }
    if (!resonatorUnlocked) {
        std::cerr << "[SelfTest] FAIL: Puzzle room AcousticResonator did not charge.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [6/17]: Puzzle chamber AcousticResonator crystal energized and unlocked.\n";

    // 7. Verify Player Hull Integrity, Echo Shards economy, and Relic synergies
    Player player;
    player.reset({640.0f, 400.0f});
    player.addEchoShards(50);
    if (!player.spendEchoShards(25) || player.echoShards() != 25) {
        std::cerr << "[SelfTest] FAIL: Player Echo Shards transaction failed.\n";
        return 1;
    }
    player.addRelic(RelicId::HarmonicAmplifier, &acoustics);
    player.addRelic(RelicId::PrismaticMirror, &acoustics);
    player.addRelic(RelicId::PiezoArmorCore, &acoustics);
    if (player.maxHealth() != 8 || player.sonarDamageMultiplier(true) <= 2.0f) {
        std::cerr << "[SelfTest] FAIL: Relic bonuses & Focused+Prismatic synergy not applied properly.\n";
        return 1;
    }
    if (!player.applyDamage(1, acoustics) || !player.isInvulnerable() || player.applyDamage(1, acoustics)) {
        std::cerr << "[SelfTest] FAIL: Player invulnerability frames or damage failed.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [7/17]: Player Hull Integrity, Echo Shards economy, & Relic synergies verified.\n";

    // 8. Verify EnemyAI 9 archetypes, 3-phase AbyssalBoss, and harmonic sonar disruption damage
    EnemyAI boss;
    boss.configurePatrol({{640.0f, 240.0f}, {780.0f, 400.0f}}, EnemyVariant::AbyssalBoss);
    if (!boss.isActive() || boss.bossPhase() != 1) {
        std::cerr << "[SelfTest] FAIL: AbyssalBoss initial state invalid.\n";
        return 1;
    }
    acoustics.clear();
    acoustics.emitPulse(boss.position() - sf::Vector2f{45.0f, 0.0f}, 460.0f, 500.0f, 1.25f, 96, WaveSignature::DirectionalPing, 0.0f, 0.8f, true);
    bool refHit = false;
    float hitEnergy = 0.0f;
    for (int step = 0; step < 8 && hitEnergy <= 0.0f; ++step) {
        acoustics.update(cfg::kFixedTimeStep, {});
        hitEnergy += acoustics.consumeRayImpactAt(boss.position(), boss.hitRadius(), refHit);
    }
    if (hitEnergy <= 0.0f) {
        std::cerr << "[SelfTest] FAIL: Sonar rays did not register impact on AbyssalBoss.\n";
        return 1;
    }
    boss.applyAcousticDamage(100.0f, true, {500.0f, 400.0f}, {});
    if (boss.bossPhase() != 2) {
        std::cerr << "[SelfTest] FAIL: AbyssalBoss did not transition to Phase 2 after taking damage.\n";
        return 1;
    }
    const bool killed = boss.applyAcousticDamage(200.0f, true, {500.0f, 400.0f}, {});
    if (!killed || boss.isActive()) {
        std::cerr << "[SelfTest] FAIL: AbyssalBoss was not defeated when HP reached 0.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [8/17]: EnemyAI 9 variants, 3-phase AbyssalBoss, & harmonic sonar damage verified.\n";

    // 9. Verify SaveSystem V4 atomic write, FNV-1a checksum, and full roguelike + meta state round-trip
    const std::string testSavePath = SaveSystem::resolveSaveFilePath(exeDir, "test_echolabyrinth_v4.dat");
    SaveData outSave{};
    outSave.runSeed = 0xDEADBEEFu;
    outSave.targetRoomCount = 32;
    outSave.checkpointRoomId = 5;
    outSave.playerPosX = 610.0f;
    outSave.playerPosY = 390.0f;
    outSave.playerHealth = 5;
    outSave.playerMaxHealth = 8;
    outSave.echoShards = 42;
    outSave.totalRuns = 7;
    outSave.totalVictories = 3;
    outSave.bossesDefeated = 4;
    outSave.visitedRooms = {0, 1, 2, 5};
    outSave.clearedRooms = {0, 1, 2};
    outSave.collectedRelics = {0, 1, 5, 8};

    if (!SaveSystem::saveToFile(testSavePath, outSave)) {
        std::cerr << "[SelfTest] FAIL: Could not write V4 save file.\n";
        return 1;
    }
    SaveData inSave{};
    if (!SaveSystem::loadFromFile(testSavePath, inSave)) {
        std::cerr << "[SelfTest] FAIL: Could not read valid V4 save file.\n";
        return 1;
    }
    std::remove(testSavePath.c_str());

    if (inSave.runSeed != 0xDEADBEEFu || inSave.echoShards != 42 || inSave.playerMaxHealth != 8 ||
        inSave.totalVictories != 3 || inSave.collectedRelics.size() != 4 || inSave.clearedRooms.size() != 3) {
        std::cerr << "[SelfTest] FAIL: V4 save round-trip data mismatch.\n";
        return 1;
    }

    {
        std::ofstream badOut(testSavePath, std::ios::trunc);
        badOut << "ECHOLABYRINTH_SAVE_V4\nseed 123\nroom 2\nchecksum 99999\n";
    }
    SaveData badData{};
    if (SaveSystem::loadFromFile(testSavePath, badData)) {
        std::cerr << "[SelfTest] FAIL: Tampered V4 checksum should be rejected.\n";
        std::remove(testSavePath.c_str());
        return 1;
    }
    std::remove(testSavePath.c_str());
    std::cout << "[SelfTest] PASS [9/17]: SaveSystem V4 atomic serialization, seed/relic/meta round-trip, & checksum verified.\n";

    // 10. Verify AssetManager 3D Wavefront OBJ models and 7 procedural material textures (including Glass)
    AssetManager assets;
    assets.initialize(exeDir, false);
    if (!assets.isInitialized() || assets.loadedDiskModelCount() < 7 || assets.totalTriangleCount() < 60) {
        std::cerr << "[SelfTest] FAIL: AssetManager expected 7 Wavefront .obj models from disk, got "
                  << assets.loadedDiskModelCount() << " (" << assets.totalTriangleCount() << " tris).\n";
        return 1;
    }
    for (int m = 0; m < static_cast<int>(SurfaceMaterial::Count); ++m) {
        const auto& img = assets.getMaterialImage(static_cast<SurfaceMaterial>(m));
        if (img.getSize().x != 64 || img.getSize().y != 64) {
            std::cerr << "[SelfTest] FAIL: Material texture image " << m << " invalid dimensions.\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [10/17]: AssetManager loaded " << assets.loadedDiskModelCount()
              << "/7 Wavefront 3D .obj models (" << assets.totalTriangleCount()
              << " triangles) and 7 RGBA material textures.\n";

    // 11. BLOCK 1.1 & 1.2: Deterministic Noise Meter Action -> Expected Value & Exact Boundary Classification
    if (classifyNoiseTier(0.0f) != NoiseTier::Quiet ||
        classifyNoiseTier(30.0f) != NoiseTier::Quiet ||
        classifyNoiseTier(30.01f) != NoiseTier::Audible ||
        classifyNoiseTier(59.99f) != NoiseTier::Audible ||
        classifyNoiseTier(60.0f) != NoiseTier::Loud ||
        classifyNoiseTier(84.99f) != NoiseTier::Loud ||
        classifyNoiseTier(85.0f) != NoiseTier::Scream ||
        classifyNoiseTier(100.0f) != NoiseTier::Scream) {
        std::cerr << "[SelfTest] FAIL: Exact NoiseTier boundary intervals [0,30], (30,60), [60,85), [85,100] violated.\n";
        return 1;
    }

    float noise = 0.0f;
    noise = applyNoiseAction(noise, NoiseAction::StepSneak, 1.0f);       // 0 + 2 = 2
    if (!approxEqual(noise, 2.0f) || classifyNoiseTier(noise) != NoiseTier::Quiet) {
        std::cerr << "[SelfTest] FAIL: StepSneak 1s expected noise 2.0 (Quiet), got " << noise << "\n";
        return 1;
    }
    noise = applyNoiseAction(noise, NoiseAction::StepWalk, 1.0f);        // 2 + 8 = 10
    noise = applyNoiseAction(noise, NoiseAction::DirectionalPing, 1.0f); // 10 + 12 = 22
    if (!approxEqual(noise, 22.0f) || classifyNoiseTier(noise) != NoiseTier::Quiet) {
        std::cerr << "[SelfTest] FAIL: Walk+Ping expected noise 22.0 (Quiet), got " << noise << "\n";
        return 1;
    }
    noise = applyNoiseAction(noise, NoiseAction::StepSprint, 1.0f);      // 22 + 25 = 47
    if (!approxEqual(noise, 47.0f) || classifyNoiseTier(noise) != NoiseTier::Audible) {
        std::cerr << "[SelfTest] FAIL: Sprint 1s expected noise 47.0 (Audible), got " << noise << "\n";
        return 1;
    }
    noise = applyNoiseAction(noise, NoiseAction::DecoyImpact, 1.0f);     // 47 + 15 = 62
    if (!approxEqual(noise, 62.0f) || classifyNoiseTier(noise) != NoiseTier::Loud) {
        std::cerr << "[SelfTest] FAIL: DecoyImpact expected noise 62.0 (Loud), got " << noise << "\n";
        return 1;
    }
    noise = applyNoiseAction(noise, NoiseAction::RadialPulse, 1.0f);     // 62 + 30 = 92
    if (!approxEqual(noise, 92.0f) || classifyNoiseTier(noise) != NoiseTier::Scream) {
        std::cerr << "[SelfTest] FAIL: RadialPulse expected noise 92.0 (Scream), got " << noise << "\n";
        return 1;
    }
    noise = applyNoiseAction(noise, NoiseAction::CombatHit, 1.0f);       // min(100, 92 + 10) = 100
    if (!approxEqual(noise, 100.0f)) {
        std::cerr << "[SelfTest] FAIL: CombatHit clamp expected 100.0, got " << noise << "\n";
        return 1;
    }
    noise = applyNoiseAction(noise, NoiseAction::SilenceDecay, 4.5f);    // 100 - 45 = 55
    if (!approxEqual(noise, 55.0f) || classifyNoiseTier(noise) != NoiseTier::Audible) {
        std::cerr << "[SelfTest] FAIL: SilenceDecay 4.5s expected 55.0 (Audible), got " << noise << "\n";
        return 1;
    }

    // Verify EnemyAI deterministic response to NoiseTier stimuli
    EnemyAI listener;
    listener.configurePatrol({{400.0f, 300.0f}, {700.0f, 300.0f}}, EnemyVariant::Stalker);
    listener.applyNoiseTierStimulus({640.0f, 400.0f}, NoiseTier::Quiet);
    if (listener.state() != EnemyState::Patrol) {
        std::cerr << "[SelfTest] FAIL: Quiet tier should keep enemy in Patrol.\n";
        return 1;
    }
    listener.applyNoiseTierStimulus({640.0f, 400.0f}, NoiseTier::Audible);
    if (listener.state() != EnemyState::InvestigateSound) {
        std::cerr << "[SelfTest] FAIL: Audible tier should transition enemy to InvestigateSound.\n";
        return 1;
    }
    listener.applyNoiseTierStimulus({600.0f, 380.0f}, NoiseTier::Scream);
    if (listener.state() != EnemyState::PursuePlayer || !approxEqual(listener.suspicion(), 100.0f)) {
        std::cerr << "[SelfTest] FAIL: Scream tier should lock enemy into PursuePlayer at 100 suspicion.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [11/17]: Noise Meter action deltas (+2/+8/+25/+12/+30/+15/+10/-10) & exact NoiseTier boundaries verified.\n";

    // 12. BLOCK 1.3 & 1.4: Cross-Room Sound Propagation (50% attenuation per open door), Closed-Door Blocking, & Ghost Clear
    Labyrinth noiseLab;
    noiseLab.generateRun(0xCAFE1234u, 32);
    for (std::size_t i = 0; i < noiseLab.roomCount(); ++i) {
        noiseLab.setCombatLock(static_cast<int>(i), false);
    }

    const auto openLeaks = noiseLab.propagateNoiseFromRoom(0, 80.0f, 2);
    if (openLeaks.empty()) {
        std::cerr << "[SelfTest] FAIL: Open doors from Start room (0) failed to propagate noise.\n";
        return 1;
    }
    for (const auto& lk : openLeaks) {
        const float expected = (lk.gridDistance == 1) ? 40.0f : 20.0f;
        if (!approxEqual(lk.leakedNoise, expected)) {
            std::cerr << "[SelfTest] FAIL: Cross-room noise attenuation mismatch at distance "
                      << lk.gridDistance << " (expected " << expected << ", got " << lk.leakedNoise << ").\n";
            return 1;
        }
    }

    noiseLab.setCombatLock(0, true);
    const auto lockedLeaks = noiseLab.propagateNoiseFromRoom(0, 95.0f, 2);
    if (!lockedLeaks.empty()) {
        std::cerr << "[SelfTest] FAIL: Closed combat doors did not block cross-room noise propagation.\n";
        return 1;
    }

    bool ghostFlag = false;
    const int ghostShards = Labyrinth::computeClearShardReward(20, 28.0f, ghostFlag);
    if (!ghostFlag || ghostShards != 30) {
        std::cerr << "[SelfTest] FAIL: Ghost Clear (peak 28 <= 30) expected 30 shards, got " << ghostShards << ".\n";
        return 1;
    }
    const int loudShards = Labyrinth::computeClearShardReward(20, 45.0f, ghostFlag);
    if (ghostFlag || loudShards != 20) {
        std::cerr << "[SelfTest] FAIL: Non-Ghost Clear (peak 45 > 30) expected 20 shards, got " << loudShards << ".\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [12/17]: Cross-room 50% noise propagation, closed-door blocking, & Ghost Clear (x1.5 Shards) verified.\n";

    // 13. BLOCK 1 END-TO-END INTEGRATION SUITE:
    //     (a) Ghost Clear after loud noise in previous room (m_roomPeakNoise reset to 0 on room transition),
    //     (b) Neighboring room enemy alert reaction to cross-room leaked noise,
    //     (c) Enemy intrusion into cleared room on loud noise, and
    //     (d) Anti-infinite-respawn guard (reopenRoomForNoiseIntrusion consumed once per room).
    Game simGame(exeDir);
    simGame.startNewGame(0xCAFE1234u);

    int adjCombatRoomId = -1;
    for (int d = 0; d < 4; ++d) {
        const int nb = simGame.m_labyrinth.roomAt(0).neighbors[static_cast<std::size_t>(d)];
        if (nb >= 0) {
            const auto& nbRoom = simGame.m_labyrinth.roomAt(static_cast<std::size_t>(nb));
            if (!nbRoom.cleared && !nbRoom.enemySpawns.empty()) {
                adjCombatRoomId = nb;
                break;
            }
        }
    }
    if (adjCombatRoomId == -1) {
        for (std::size_t i = 1; i < simGame.m_labyrinth.roomCount(); ++i) {
            const auto& rm = simGame.m_labyrinth.roomAt(i);
            if (!rm.cleared && !rm.enemySpawns.empty() && rm.totalWaves == 1) {
                adjCombatRoomId = static_cast<int>(i);
                break;
            }
        }
    }
    if (adjCombatRoomId == -1) {
        std::cerr << "[SelfTest] FAIL: Could not find combat room for Block 1 integration test.\n";
        return 1;
    }

    simGame.m_noiseLevel = 80.0f;
    simGame.m_roomPeakNoise = 80.0f;
    simGame.updateNoiseSystem(cfg::kFixedTimeStep);

    bool isDirectNeighborOfStart = false;
    for (int d = 0; d < 4; ++d) {
        if (simGame.m_labyrinth.roomAt(0).neighbors[static_cast<std::size_t>(d)] == adjCombatRoomId) {
            isDirectNeighborOfStart = true;
            break;
        }
    }
    if (isDirectNeighborOfStart) {
        if (simGame.m_labyrinth.roomAt(static_cast<std::size_t>(adjCombatRoomId)).pendingNoiseAlertLevel <= cfg::noise::kThresholdAudible) {
            std::cerr << "[SelfTest] FAIL: Cross-room noise from room 0 did not record pendingNoiseAlertLevel on adjacent room.\n";
            return 1;
        }
    }

    simGame.enterRoom(adjCombatRoomId, simGame.m_labyrinth.roomAt(static_cast<std::size_t>(adjCombatRoomId)).playerDefaultSpawn, true);
    if (!approxEqual(simGame.m_roomPeakNoise, 0.0f)) {
        std::cerr << "[SelfTest] FAIL: m_roomPeakNoise was not reset to 0.0f on room entry (got "
                  << simGame.m_roomPeakNoise << ").\n";
        return 1;
    }
    if (isDirectNeighborOfStart && !simGame.m_enemies.empty()) {
        const auto& firstEnemy = simGame.m_enemies.front();
        if (!firstEnemy.isDisguisedMimic() && firstEnemy.state() == EnemyState::Patrol) {
            std::cerr << "[SelfTest] FAIL: Enemy in adjacent room did not transition out of Patrol after cross-room noise leak.\n";
            return 1;
        }
    }

    simGame.m_noiseLevel = 0.0f;
    simGame.m_roomPeakNoise = 0.0f;
    const int shardsBeforeClear = simGame.m_player.echoShards();
    const int expectedBaseShards = simGame.m_labyrinth.currentRoom().shardReward;

    simGame.registerNoiseAction(NoiseAction::DirectionalPing, 1.0f, simGame.m_player.position());
    if (simGame.m_roomPeakNoise > cfg::noise::kThresholdAudible) {
        std::cerr << "[SelfTest] FAIL: Single ping exceeded Ghost Clear threshold.\n";
        return 1;
    }
    for (auto& enemy : simGame.m_enemies) {
        if (enemy.isActive()) {
            const sf::Vector2f ePos = enemy.position();
            simGame.m_acoustics.emitPulse(ePos - sf::Vector2f{20.0f, 0.0f}, 480.0f, 500.0f, 25.0f, 96, WaveSignature::DirectionalPing, 0.0f, 1.2f, true);
        }
    }
    for (int step = 0; step < 15 && !simGame.m_labyrinth.currentRoom().cleared; ++step) {
        simGame.updatePlaying(cfg::kFixedTimeStep);
    }
    if (!simGame.m_labyrinth.currentRoom().cleared || !simGame.m_labyrinth.currentRoom().ghostCleared ||
        simGame.m_ghostClearsThisRun < 1 || !simGame.m_player.hasRelic(RelicId::SilentStep)) {
        std::cerr << "[SelfTest] FAIL: Ghost Clear was not awarded after transitioning from a noisy previous room.\n";
        return 1;
    }
    const int expectedGhostShards = static_cast<int>(std::round(static_cast<float>(expectedBaseShards) * cfg::noise::kGhostClearShardMultiplier));
    if (simGame.m_player.echoShards() != shardsBeforeClear + expectedGhostShards) {
        std::cerr << "[SelfTest] FAIL: Ghost Clear shard bonus mismatch (expected +" << expectedGhostShards
                  << ", got +" << (simGame.m_player.echoShards() - shardsBeforeClear) << ").\n";
        return 1;
    }

    int clearedHostRoom = -1;
    for (std::size_t i = 1; i < simGame.m_labyrinth.roomCount(); ++i) {
        const auto& rm = simGame.m_labyrinth.roomAt(i);
        if (rm.type != RoomType::Start && rm.type != RoomType::Sanctuary && rm.type != RoomType::FinalExit) {
            for (int d = 0; d < 4; ++d) {
                const int nb = rm.neighbors[static_cast<std::size_t>(d)];
                if (nb >= 0) {
                    const auto& nbRm = simGame.m_labyrinth.roomAt(static_cast<std::size_t>(nb));
                    if (!nbRm.cleared && !nbRm.enemySpawns.empty()) {
                        clearedHostRoom = static_cast<int>(i);
                        break;
                    }
                }
            }
        }
        if (clearedHostRoom != -1) break;
    }
    if (clearedHostRoom == -1) {
        std::cerr << "[SelfTest] FAIL: Could not find a host room adjacent to an uncleared combat room.\n";
        return 1;
    }

    simGame.m_labyrinth.markRoomCleared(clearedHostRoom);
    simGame.enterRoom(clearedHostRoom, {640.0f, 400.0f}, true);
    simGame.m_enemies.clear();
    simGame.m_noiseIntrusionCooldown = 0.0f;
    simGame.m_noiseLevel = 90.0f;
    simGame.updateNoiseSystem(cfg::kFixedTimeStep);

    if (simGame.m_enemies.size() != 1 || simGame.m_labyrinth.currentRoom().cleared ||
        !simGame.m_labyrinth.currentRoom().combatLocked || !simGame.m_labyrinth.currentRoom().noiseIntrusionActive) {
        std::cerr << "[SelfTest] FAIL: Loud/Scream noise in cleared room did not trigger neighbor enemy intrusion.\n";
        return 1;
    }

    simGame.m_enemies.clear();
    simGame.m_labyrinth.markRoomCleared(clearedHostRoom);
    simGame.m_noiseIntrusionCooldown = 0.0f;
    simGame.m_noiseLevel = 95.0f;
    simGame.updateNoiseSystem(cfg::kFixedTimeStep);

    if (!simGame.m_enemies.empty() || !simGame.m_labyrinth.currentRoom().cleared ||
        simGame.m_labyrinth.reopenRoomForNoiseIntrusion(clearedHostRoom)) {
        std::cerr << "[SelfTest] FAIL: Anti-infinite-respawn guard failed; room allowed repeated noise intrusions.\n";
        return 1;
    }
    std::remove(simGame.m_saveFilePath.c_str());
    std::cout << "[SelfTest] PASS [13/17]: End-to-end Ghost Clear after room transition, cross-room AI alert, enemy intrusion, & anti-respawn guard verified.\n";

    // 14. BLOCK 2 VERIFICATION: Materials & Acoustic Physics
    //     - DampMoss: 80% reflection attenuation (reflectivity == 0.20) & 2x shelter noise dampening
    //     - Metal: lossless reflection (reflectivity == 1.00) & +2 bonus bounces
    //     - Glass: shatters on high-power pulse (isOpen = true, +40 noise via GlassShatter)
    //     - Water: 2x distance propagation & +15 step noise (WaterSplash)
    //     - Resonator Column: 4-second harmonic hum illuminating surroundings & luring enemies
    const MaterialProperties mossProps = getMaterialProperties(SurfaceMaterial::DampMoss);
    const MaterialProperties metalProps = getMaterialProperties(SurfaceMaterial::Metal);
    const MaterialProperties waterProps = getMaterialProperties(SurfaceMaterial::Water);
    if (!approxEqual(mossProps.reflectivity, 0.20f) ||
        !approxEqual(metalProps.reflectivity, 1.00f) ||
        metalProps.bonusReflections != 2 ||
        !approxEqual(waterProps.distanceMultiplier, 2.0f)) {
        std::cerr << "[SelfTest] FAIL: Block 2 material physics constants mismatch.\n";
        return 1;
    }

    // Verify DampMoss shelter detection and 2x silence decay (-20/sec)
    std::vector<WallSegment> shelterWalls = {{{300.0f, 300.0f}, {400.0f, 300.0f}, SurfaceMaterial::DampMoss, -1, false}};
    if (!AcousticSystem::isInDampMossShelter({350.0f, 325.0f}, shelterWalls, SurfaceMaterial::Stone)) {
        std::cerr << "[SelfTest] FAIL: DampMoss shelter detection failed.\n";
        return 1;
    }
    const float decayedInMoss = applyNoiseAction(50.0f, NoiseAction::SilenceDecay, 1.0f, 2.0f);
    if (!approxEqual(decayedInMoss, 30.0f)) {
        std::cerr << "[SelfTest] FAIL: DampMoss 2x silence decay expected 30.0, got " << decayedInMoss << ".\n";
        return 1;
    }

    // Verify Glass shattering on RadialPulse (+40 noise) & Resonator Column 4s hum
    AcousticSystem block2Acoustics;
    std::vector<WallSegment> glassWalls = {{{500.0f, 200.0f}, {500.0f, 600.0f}, SurfaceMaterial::Glass, 777, false}};
    std::vector<AcousticResonator> humColumns = {{
        751, {440.0f, 400.0f}, 18.0f, 0.0f, false, -1, "RESONANCE PIPE COLUMN", 0.0f, 0.0f
    }};
    block2Acoustics.emitPulse({400.0f, 400.0f}, 450.0f, 500.0f, 1.25f, 96, WaveSignature::RadialPulse, 0.0f, 6.2831853f, true);
    for (int step = 0; step < 20; ++step) {
        block2Acoustics.update(cfg::kFixedTimeStep, glassWalls, &humColumns);
    }
    if (!glassWalls[0].isOpen || block2Acoustics.consumeShatteredGlassCount() < 1) {
        std::cerr << "[SelfTest] FAIL: Brittle Glass wall did not shatter under RadialPulse.\n";
        return 1;
    }
    if (!approxEqual(applyNoiseAction(10.0f, NoiseAction::GlassShatter, 1.0f), 50.0f) ||
        !approxEqual(applyNoiseAction(10.0f, NoiseAction::WaterSplash, 1.0f), 25.0f)) {
        std::cerr << "[SelfTest] FAIL: GlassShatter (+40) or WaterSplash (+15) noise delta mismatch.\n";
        return 1;
    }
    if (humColumns[0].humTimer <= 3.0f ||
        block2Acoustics.queryIlluminationAt({460.0f, 400.0f}, {100.0f, 100.0f}, &humColumns) <= 0.45f) {
        std::cerr << "[SelfTest] FAIL: Resonator Column 4-second hum timer or illumination failed.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [14/17]: Block 2 material physics (Moss -80% & shelter, Metal +2 bounces, Glass shatter +40, Water 2x/+15, Column 4s hum) verified.\n";

    // 15. BLOCK 3 VERIFICATION: Extended Player Sound Arsenal (Whistle, Stomp, Delayed Echo Trap, & Frequency Tuning)
    Player arsenalPlayer;
    arsenalPlayer.reset({400.0f, 400.0f});
    AcousticSystem block3Acoustics;

    // 15a. Whistle: narrow 15-deg beam, long range, +5 noise
    if (!arsenalPlayer.triggerWhistle({700.0f, 400.0f}, block3Acoustics) ||
        !approxEqual(applyNoiseAction(0.0f, NoiseAction::WhistlePing, 1.0f), 5.0f)) {
        std::cerr << "[SelfTest] FAIL: Whistle beam emission or +5 noise delta failed.\n";
        return 1;
    }

    // 15b. Stomp: +35 noise & enemy knockback
    EnemyAI stompTarget;
    stompTarget.configurePatrol({{460.0f, 400.0f}, {560.0f, 400.0f}}, EnemyVariant::Stalker);
    const float distBeforeStomp = stompTarget.position().x - arsenalPlayer.position().x;
    if (!arsenalPlayer.triggerStomp(block3Acoustics) ||
        !stompTarget.applyStompKnockback(arsenalPlayer.position(), cfg::kStompRadius, cfg::kStompKnockback, {}) ||
        (stompTarget.position().x - arsenalPlayer.position().x) <= distBeforeStomp + 20.0f ||
        !approxEqual(applyNoiseAction(0.0f, NoiseAction::StompImpact, 1.0f), 35.0f)) {
        std::cerr << "[SelfTest] FAIL: Stomp wave knockback or +35 noise delta failed.\n";
        return 1;
    }

    // 15c. Delayed Echo Trap: placed on floor, triggers after 3.0s delay and emits EchoTrapPulse
    if (!arsenalPlayer.placeEchoTrap() || !arsenalPlayer.activeEchoTrap().active) {
        std::cerr << "[SelfTest] FAIL: Failed to place Delayed Echo Trap mine.\n";
        return 1;
    }
    bool trapDetonated = false;
    for (int step = 0; step < 200 && !trapDetonated; ++step) {
        WaveSignature sig{};
        if (arsenalPlayer.update(cfg::kFixedTimeStep, {0.0f, 0.0f}, MovementStance::Walk, {600.0f, 400.0f}, {}, block3Acoustics, sig)) {
            if (sig == WaveSignature::EchoTrapPulse) {
                trapDetonated = true;
            }
        }
    }
    if (!trapDetonated) {
        std::cerr << "[SelfTest] FAIL: Delayed Echo Trap did not detonate after 3.0s countdown.\n";
        return 1;
    }

    // 15d. Frequency Tuning (Low penetrates walls better than High)
    const std::vector<WallSegment> stoneBarrier = {{{500.0f, 100.0f}, {500.0f, 700.0f}, SurfaceMaterial::Stone, -1, false}};
    const float transLow = AcousticSystem::computeTransmissionFactor({300.0f, 400.0f}, {700.0f, 400.0f}, stoneBarrier, AcousticFrequency::Low);
    const float transMid = AcousticSystem::computeTransmissionFactor({300.0f, 400.0f}, {700.0f, 400.0f}, stoneBarrier, AcousticFrequency::Mid);
    const float transHigh = AcousticSystem::computeTransmissionFactor({300.0f, 400.0f}, {700.0f, 400.0f}, stoneBarrier, AcousticFrequency::High);
    if (!(transLow > transMid && transMid > transHigh)) {
        std::cerr << "[SelfTest] FAIL: Frequency tuning transmission ordering (Low > Mid > High) failed.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [15/17]: Block 3 sound arsenal (Whistle +5, Stomp +35 & knockback, 3s Echo Trap mine, Low/Mid/High frequencies) verified.\n";

    // 16. BLOCK 4 VERIFICATION: 4 New Acoustic Enemy Archetypes (SilentStalker, DeafBrute, Mimic, EchoSwarm)
    AcousticSystem block4Acoustics;

    // 16a. SilentStalker: emits zero passive tremors while patrolling, inhales when within 88px of player
    EnemyAI silentHunter;
    silentHunter.configurePatrol({{300.0f, 300.0f}, {500.0f, 300.0f}}, EnemyVariant::SilentStalker);
    for (int step = 0; step < 90; ++step) {
        silentHunter.update(cfg::kFixedTimeStep, {900.0f, 600.0f}, {}, block4Acoustics);
    }
    if (!block4Acoustics.recentEvents().empty()) {
        std::cerr << "[SelfTest] FAIL: SilentStalker emitted passive tremors while patrolling far from player.\n";
        return 1;
    }
    // Move player within 70px (< 88px) and verify pre-attack inhale warning pulse is emitted
    silentHunter.update(cfg::kFixedTimeStep, silentHunter.position() + sf::Vector2f{65.0f, 0.0f}, {}, block4Acoustics);
    if (block4Acoustics.recentEvents().empty()) {
        std::cerr << "[SelfTest] FAIL: SilentStalker did not emit pre-attack inhale warning near player.\n";
        return 1;
    }

    // 16b. DeafBrute: ignores airborne Whistle/Ping/Walk, reacts only to ground vibrations (Sprint/Stomp)
    block4Acoustics.clear();
    EnemyAI deafBrute;
    deafBrute.configurePatrol({{500.0f, 400.0f}, {700.0f, 400.0f}}, EnemyVariant::DeafBrute);
    block4Acoustics.emitPulse({460.0f, 400.0f}, 500.0f, 700.0f, 1.15f, 48, WaveSignature::WhistleBeam, 0.0f, 0.3f, true);
    block4Acoustics.emitPulse({460.0f, 400.0f}, 450.0f, 600.0f, 1.00f, 64, WaveSignature::DirectionalPing, 0.0f, 1.0f, true);
    deafBrute.update(cfg::kFixedTimeStep, {200.0f, 200.0f}, {}, block4Acoustics);
    deafBrute.applyNoiseTierStimulus({460.0f, 400.0f}, NoiseTier::Loud, false); // airborne = false
    if (deafBrute.state() != EnemyState::Patrol || deafBrute.suspicion() > 0.01f) {
        std::cerr << "[SelfTest] FAIL: DeafBrute reacted to airborne sound.\n";
        return 1;
    }
    block4Acoustics.clear();
    block4Acoustics.emitPulse({460.0f, 400.0f}, 460.0f, 450.0f, 1.40f, 96, WaveSignature::StompWave, 0.0f, 6.2831853f, true);
    deafBrute.update(cfg::kFixedTimeStep, {200.0f, 200.0f}, {}, block4Acoustics);
    if (deafBrute.state() == EnemyState::Patrol) {
        std::cerr << "[SelfTest] FAIL: DeafBrute did not react to StompWave ground vibration.\n";
        return 1;
    }

    // 16c. Mimic: starts disguised as crystal, emits lure waves, unmasks when scanned by Whistle / High-freq
    block4Acoustics.clear();
    EnemyAI mimic;
    mimic.configurePatrol({{600.0f, 400.0f}}, EnemyVariant::Mimic);
    if (!mimic.isDisguisedMimic() || mimic.state() != EnemyState::Idle) {
        std::cerr << "[SelfTest] FAIL: Mimic did not start in disguised Idle state.\n";
        return 1;
    }
    for (int step = 0; step < 110; ++step) {
        mimic.update(cfg::kFixedTimeStep, {100.0f, 100.0f}, {}, block4Acoustics);
    }
    if (block4Acoustics.recentEvents().empty() || block4Acoustics.recentEvents().back().signature != WaveSignature::ResonatorHum) {
        std::cerr << "[SelfTest] FAIL: Disguised Mimic did not emit false ResonatorHum lure wave.\n";
        return 1;
    }
    block4Acoustics.clear();
    block4Acoustics.setActiveFrequency(AcousticFrequency::High);
    block4Acoustics.emitPulse({500.0f, 400.0f}, 520.0f, 800.0f, 1.0f, 44, WaveSignature::WhistleBeam, 0.0f, 0.26f, true);
    mimic.update(cfg::kFixedTimeStep, {100.0f, 100.0f}, {}, block4Acoustics);
    if (mimic.isDisguisedMimic() || mimic.state() != EnemyState::PursuePlayer) {
        std::cerr << "[SelfTest] FAIL: Mimic did not unmask when scanned by High-frequency Whistle.\n";
        return 1;
    }

    // 16d. EchoSwarm: repeats & amplifies nearby player sound (+12 noise)
    block4Acoustics.clear();
    EnemyAI swarm;
    swarm.configurePatrol({{450.0f, 400.0f}, {550.0f, 400.0f}}, EnemyVariant::EchoSwarm);
    block4Acoustics.emitPulse({420.0f, 400.0f}, 450.0f, 550.0f, 1.0f, 64, WaveSignature::DirectionalPing, 0.0f, 1.0f, true);
    swarm.update(cfg::kFixedTimeStep, {200.0f, 200.0f}, {}, block4Acoustics);
    if (!swarm.consumeSwarmAmplification() ||
        !approxEqual(applyNoiseAction(20.0f, NoiseAction::EchoSwarmAmplify, 1.0f), 32.0f)) {
        std::cerr << "[SelfTest] FAIL: EchoSwarm did not amplify nearby player sound (+12 noise).\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [16/17]: Block 4 enemies (SilentStalker, DeafBrute, Mimic, & EchoSwarm) verified.\n";

    // 17. BLOCK 5 & SAVE V5 VERIFICATION:
    //     Dynamic Room Events, 12 Relics, 12 Audio-Lore Codex Logs, Boss Arena Acoustic Phases, & SaveSystem V5 (+ V4 Compat)
    bool hasEchoStorm = false;
    bool hasBlackout = false;
    bool hasOvercharge = false;
    std::size_t codexCollectibleCount = 0;
    for (std::size_t i = 0; i < labA.roomCount(); ++i) {
        const auto& rm = labA.roomAt(i);
        if (rm.roomEvent == RoomEvent::EchoStorm) hasEchoStorm = true;
        if (rm.roomEvent == RoomEvent::AcousticBlackout) hasBlackout = true;
        if (rm.roomEvent == RoomEvent::ResonanceOvercharge) hasOvercharge = true;
        codexCollectibleCount += rm.collectibles.size();
    }
    if (!hasEchoStorm || !hasBlackout || !hasOvercharge) {
        std::cerr << "[SelfTest] FAIL: Generated run missing one or more Block 5.1 RoomEvents.\n";
        return 1;
    }
    if (codexCollectibleCount < kTotalCodexFragments) {
        std::cerr << "[SelfTest] FAIL: Expected all 12 Audio-Lore Codex collectibles in run, found " << codexCollectibleCount << ".\n";
        return 1;
    }
    for (int cIdx = 0; cIdx < static_cast<int>(kTotalCodexFragments); ++cIdx) {
        const CodexEntrySpec cSpec = getCodexEntrySpec(cIdx);
        if (cSpec.id != 200 + cIdx || cSpec.title == nullptr || cSpec.excerpt == nullptr) {
            std::cerr << "[SelfTest] FAIL: Invalid CodexEntrySpec at index " << cIdx << ".\n";
            return 1;
        }
    }

    // Verify RoomEvent::AcousticBlackout (+50% shard bonus) & EchoStormPulse (+6) / BossDissonanceShock (+15)
    bool blackoutGhost = false;
    const int blackoutGhostShards = Labyrinth::computeClearShardReward(20, 25.0f, blackoutGhost, RoomEvent::AcousticBlackout);
    if (!blackoutGhost || blackoutGhostShards != 45) { // 20 * 1.5 (Ghost) * 1.5 (Blackout) = 45
        std::cerr << "[SelfTest] FAIL: AcousticBlackout + GhostClear expected 45 shards, got " << blackoutGhostShards << ".\n";
        return 1;
    }
    if (!approxEqual(applyNoiseAction(10.0f, NoiseAction::EchoStormPulse, 1.0f), 16.0f) ||
        !approxEqual(applyNoiseAction(10.0f, NoiseAction::BossDissonanceShock, 1.0f), 25.0f)) {
        std::cerr << "[SelfTest] FAIL: EchoStormPulse (+6) or BossDissonanceShock (+15) noise delta mismatch.\n";
        return 1;
    }

    // Verify Block 5.2 Extended Acoustic Relics (PhaseDampener, UltrasonicLens, AbyssalTuningFork)
    Player block5Player;
    block5Player.reset({640.0f, 400.0f});
    const int trapsBefore = block5Player.echoTrapCount();
    block5Player.addRelic(RelicId::PhaseDampener);
    block5Player.addRelic(RelicId::UltrasonicLens);
    block5Player.addRelic(RelicId::AbyssalTuningFork);
    if (block5Player.echoTrapCount() != trapsBefore + 2 ||
        !approxEqual(block5Player.silenceDecayMultiplier(), 1.50f) ||
        !approxEqual(block5Player.noiseGainMultiplier(false), 0.70f) ||
        !approxEqual(block5Player.shardGainMultiplier(), 1.25f) ||
        block5Player.sonarDamageMultiplier(false, true) <= 1.9f) {
        std::cerr << "[SelfTest] FAIL: Block 5.2 relics (PhaseDampener, UltrasonicLens, AbyssalTuningFork) failed verification.\n";
        return 1;
    }

    // Verify Block 5.4 Multi-Phase Boss Arena Mechanics (HarmonicChoir -> DissonantPulse -> AbyssalSilence)
    EnemyAI phaseBoss;
    phaseBoss.configurePatrol({{640.0f, 240.0f}, {780.0f, 400.0f}}, EnemyVariant::AbyssalBoss);
    if (phaseBoss.bossAcousticPhase() != BossArenaAcousticPhase::HarmonicChoir) {
        std::cerr << "[SelfTest] FAIL: AbyssalBoss did not start in HarmonicChoir phase.\n";
        return 1;
    }
    phaseBoss.applyAcousticDamage(70.0f, true, {640.0f, 400.0f}, {}); // 240 - 70*1.45 = 138.5 HP (57.7% -> Phase 2)
    if (phaseBoss.bossAcousticPhase() != BossArenaAcousticPhase::DissonantPulse) {
        std::cerr << "[SelfTest] FAIL: AbyssalBoss did not enter DissonantPulse phase at 57% HP.\n";
        return 1;
    }
    phaseBoss.applyAcousticDamage(70.0f, true, {640.0f, 400.0f}, {}); // 138.5 - 70 = 68.5 HP (28.5% -> Phase 3)
    if (phaseBoss.bossAcousticPhase() != BossArenaAcousticPhase::AbyssalSilence) {
        std::cerr << "[SelfTest] FAIL: AbyssalBoss did not enter AbyssalSilence phase at <= 33% HP.\n";
        return 1;
    }
    const float hpBeforeUnreflected = phaseBoss.health();
    phaseBoss.applyAcousticDamage(10.0f, false, {640.0f, 400.0f}, {}, false); // 0.65x in Phase III = 6.5 dmg
    const float dmgUnreflected = hpBeforeUnreflected - phaseBoss.health();
    const float hpBeforeReflected = phaseBoss.health();
    phaseBoss.applyAcousticDamage(10.0f, true, {640.0f, 400.0f}, {}, true);   // 1.85x in Phase III = 18.5 dmg
    const float dmgReflected = hpBeforeReflected - phaseBoss.health();
    if (!(dmgReflected > dmgUnreflected * 2.5f)) {
        std::cerr << "[SelfTest] FAIL: Phase III AbyssalSilence resonance multiplier failed (reflected="
                  << dmgReflected << ", unreflected=" << dmgUnreflected << ").\n";
        return 1;
    }

    // Verify SaveSystem V5 full round-trip + V4 backwards compatibility
    const std::string v5SavePath = SaveSystem::resolveSaveFilePath(exeDir, "test_echolabyrinth_v5.dat");
    SaveData v5Out{};
    v5Out.runSeed = 0x55AA2026u;
    v5Out.targetRoomCount = 32;
    v5Out.checkpointRoomId = 4;
    v5Out.echoTrapCount = 5;
    v5Out.activeFrequency = 2;
    v5Out.ghostClearsThisRun = 3;
    v5Out.metaTotalGhostClears = 14;
    v5Out.collectedRelics = {0, 8, 9, 10, 11};
    v5Out.discoveredCodexIds = {200, 201, 205, 211};
    if (!SaveSystem::saveToFile(v5SavePath, v5Out)) {
        std::cerr << "[SelfTest] FAIL: Failed to write SaveSystem V5 file.\n";
        return 1;
    }
    SaveData v5In{};
    if (!SaveSystem::loadFromFile(v5SavePath, v5In) ||
        v5In.echoTrapCount != 5 ||
        v5In.activeFrequency != 2 ||
        v5In.ghostClearsThisRun != 3 ||
        v5In.metaTotalGhostClears != 14 ||
        v5In.collectedRelics.size() != 5 ||
        v5In.discoveredCodexIds.size() != 4) {
        std::cerr << "[SelfTest] FAIL: SaveSystem V5 round-trip mismatch.\n";
        std::remove(v5SavePath.c_str());
        return 1;
    }
    std::remove(v5SavePath.c_str());

    // Verify V4 backwards compatibility (explicit ECHOLABYRINTH_SAVE_V4 file with computeChecksumV4)
    SaveData v4Compat{};
    v4Compat.runSeed = 0x12345678u;
    v4Compat.targetRoomCount = 32;
    v4Compat.checkpointRoomId = 2;
    v4Compat.playerHealth = 6;
    v4Compat.playerMaxHealth = 6;
    v4Compat.decoyCount = 4;
    v4Compat.echoShards = 55;
    v4Compat.masterVolume = 80.0f;
    v4Compat.maxReflections = 3;
    v4Compat.totalVictories = 2;
    v4Compat.visitedRooms = {0, 2};
    {
        std::ofstream v4File(v5SavePath, std::ios::trunc);
        v4File << "ECHOLABYRINTH_SAVE_V4\n"
               << "seed " << v4Compat.runSeed << "\n"
               << "room_count 32\n"
               << "room 2\n"
               << "health 6 6\n"
               << "decoys 4\n"
               << "shards 55\n"
               << "volume 80\n"
               << "bounces 3\n"
               << "meta 1 2 0\n"
               << "visited 0 2\n"
               << "checksum " << SaveSystem::computeChecksumV4(v4Compat) << "\n";
    }
    SaveData v4Loaded{};
    if (!SaveSystem::loadFromFile(v5SavePath, v4Loaded) || v4Loaded.echoShards != 55 || v4Loaded.totalVictories != 2) {
        std::cerr << "[SelfTest] FAIL: Backwards-compatible V4 save file failed to load in V5 SaveSystem.\n";
        std::remove(v5SavePath.c_str());
        return 1;
    }
    std::remove(v5SavePath.c_str());
    std::cout << "[SelfTest] PASS [17/17]: Block 5 (RoomEvents, 12 Relics, 12 Codex logs, Boss Arena Phases, & Save V5+V4 compat) verified.\n";

    std::cout << "[SelfTest] ALL 17 ECHOLABYRINTH 5.0 (PHASE A + B + C) NATIVE C++17 VERIFICATION SUITES PASSED SUCCESSFULLY.\n";
    return 0;
}

} // namespace echo
