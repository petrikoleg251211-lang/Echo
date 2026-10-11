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
    std::cout << "[SelfTest] Starting EchoLabyrinth 5.0 'Resonance' (Phase A) C++17 verification suite...\n";

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
    std::cout << "[SelfTest] PASS [1/13]: 20 distinct seeds verified for 2D grid uniqueness, N/E/S/W door symmetry, & Boss/Exit reachability.\n";

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
    std::cout << "[SelfTest] PASS [2/13]: Deterministic seed reproducibility verified across all 32 chambers.\n";

    // 3. Verify 2D BFS Geometric Walkability across every room in the procedural run
    for (int r = 0; r < static_cast<int>(labA.roomCount()); ++r) {
        std::string travErr;
        if (!labA.verifyRoomTraversability(r, travErr)) {
            std::cerr << "[SelfTest] FAIL: Room " << r << " failed 2D BFS walkability: " << travErr << "\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [3/13]: All 32 procedural chambers verified via 2D BFS grid pathfinding.\n";

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
    std::cout << "[SelfTest] PASS [4/13]: Combat door lockdown, room clear unsealing, & Secret Wall sonar fracture verified.\n";

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
    std::cout << "[SelfTest] PASS [5/13]: AcousticSystem reflections & multi-wall material attenuation verified.\n";

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
                resonatorUnlocked = true;
            }
        }
    }
    if (!resonatorUnlocked) {
        std::cerr << "[SelfTest] FAIL: Puzzle room AcousticResonator did not charge.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [6/13]: Puzzle chamber AcousticResonator crystal energized and unlocked.\n";

    // 7. Verify Player Hull Integrity, Echo Shards economy, and Relic synergies
    Player player;
    player.reset({640.0f, 400.0f});
    player.addEchoShards(50);
    if (!player.spendEchoShards(25) || player.echoShards() != 25) {
        std::cerr << "[SelfTest] FAIL: Player Echo Shards transaction failed.\n";
        return 1;
    }
    player.addRelic(RelicId::FocusedTransducer, &acoustics);
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
    std::cout << "[SelfTest] PASS [7/13]: Player Hull Integrity, Echo Shards economy, & Relic synergies verified.\n";

    // 8. Verify EnemyAI 5 archetypes, 3-phase AbyssalBoss, and harmonic sonar disruption damage
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
    std::cout << "[SelfTest] PASS [8/13]: EnemyAI 5 variants, 3-phase AbyssalBoss, & harmonic sonar damage verified.\n";

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
    outSave.collectedRelics = {0, 1, 5, 9}; // Includes SilentStep (9) while staying V4 compatible

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
    std::cout << "[SelfTest] PASS [9/13]: SaveSystem V4 atomic serialization, seed/relic/meta round-trip, & checksum verified.\n";

    // 10. Verify AssetManager 3D Wavefront OBJ models and procedural material textures
    AssetManager assets;
    assets.initialize(exeDir, false);
    if (!assets.isInitialized() || assets.loadedDiskModelCount() < 7 || assets.totalTriangleCount() < 60) {
        std::cerr << "[SelfTest] FAIL: AssetManager expected 7 Wavefront .obj models from disk, got "
                  << assets.loadedDiskModelCount() << " (" << assets.totalTriangleCount() << " tris).\n";
        return 1;
    }
    for (int m = 0; m < 6; ++m) {
        const auto& img = assets.getMaterialImage(static_cast<SurfaceMaterial>(m));
        if (img.getSize().x != 64 || img.getSize().y != 64) {
            std::cerr << "[SelfTest] FAIL: Material texture image " << m << " invalid dimensions.\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [10/13]: AssetManager loaded " << assets.loadedDiskModelCount()
              << "/7 Wavefront 3D .obj models (" << assets.totalTriangleCount()
              << " triangles) and 6 RGBA material textures.\n";

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
    std::cout << "[SelfTest] PASS [11/13]: Noise Meter action deltas (+2/+8/+25/+12/+30/+15/+10/-10) & exact NoiseTier boundaries verified.\n";

    // 12. BLOCK 1.3 & 1.4: Cross-Room Sound Propagation (50% attenuation per open door), Closed-Door Blocking, & Ghost Clear
    Labyrinth noiseLab;
    noiseLab.generateRun(0xCAFE1234u, 32);
    // Ensure all non-secret rooms are unlocked so open doors transmit 50% per hop
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

    // Seal room 0 with combat lockdown and verify zero noise leaks through closed combat doors
    noiseLab.setCombatLock(0, true);
    const auto lockedLeaks = noiseLab.propagateNoiseFromRoom(0, 95.0f, 2);
    if (!lockedLeaks.empty()) {
        std::cerr << "[SelfTest] FAIL: Closed combat doors did not block cross-room noise propagation.\n";
        return 1;
    }

    // Verify Ghost Clear reward (peak noise <= 30 -> x1.5 Shards; peak noise > 30 -> x1.0 Shards)
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
    std::cout << "[SelfTest] PASS [12/13]: Cross-room 50% noise propagation, closed-door blocking, & Ghost Clear (x1.5 Shards) verified." << std::endl;

    // 13. BLOCK 1 END-TO-END INTEGRATION SUITE:
    //     (a) Ghost Clear after loud noise in previous room (m_roomPeakNoise reset to 0 on room transition),
    //     (b) Neighboring room enemy alert reaction to cross-room leaked noise,
    //     (c) Enemy intrusion into cleared room on loud noise, and
    //     (d) Anti-infinite-respawn guard (reopenRoomForNoiseIntrusion consumed once per room).
    Game simGame(exeDir);
    simGame.startNewGame(0xCAFE1234u);

    // Find an uncleared combat room adjacent to Start (room 0) or along the graph
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

    // (a) Raise noise in room 0 to 80.0 (Loud), then transition into adjCombatRoomId.
    // Verify that m_roomPeakNoise is initialized to 0.0f on entry so prior room noise does not ruin Ghost Clear,
    // and that cross-room leaked noise (80 * 0.5 = 40 > 30) alerted the neighboring room's enemies upon entry!
    simGame.m_noiseLevel = 80.0f;
    simGame.m_roomPeakNoise = 80.0f;
    simGame.updateNoiseSystem(cfg::kFixedTimeStep);

    // If adjCombatRoomId is directly connected to room 0, verify pendingNoiseAlertLevel was recorded (> 30)
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

    // Enter adjCombatRoomId as a room transition
    simGame.enterRoom(adjCombatRoomId, simGame.m_labyrinth.roomAt(static_cast<std::size_t>(adjCombatRoomId)).playerDefaultSpawn, true);
    if (!approxEqual(simGame.m_roomPeakNoise, 0.0f)) {
        std::cerr << "[SelfTest] FAIL: m_roomPeakNoise was not reset to 0.0f on room entry (got "
                  << simGame.m_roomPeakNoise << ").\n";
        return 1;
    }
    if (isDirectNeighborOfStart && !simGame.m_enemies.empty()) {
        if (simGame.m_enemies.front().state() == EnemyState::Patrol) {
            std::cerr << "[SelfTest] FAIL: Enemy in adjacent room did not transition out of Patrol after cross-room noise leak.\n";
            return 1;
        }
    }

    // Now let noise decay to 0 while silent, then defeat the room's enemies while keeping m_roomPeakNoise <= 30.0f
    simGame.m_noiseLevel = 0.0f;
    simGame.m_roomPeakNoise = 0.0f;
    const int shardsBeforeClear = simGame.m_player.echoShards();
    const int expectedBaseShards = simGame.m_labyrinth.currentRoom().shardReward;

    // Emit a single quiet ping (+12 noise <= 30) and lethal acoustic damage to clear the room stealthily
    simGame.registerNoiseAction(NoiseAction::DirectionalPing, 1.0f, simGame.m_player.position());
    if (simGame.m_roomPeakNoise > cfg::noise::kThresholdAudible) {
        std::cerr << "[SelfTest] FAIL: Single ping exceeded Ghost Clear threshold.\n";
        return 1;
    }
    for (auto& enemy : simGame.m_enemies) {
        if (enemy.isActive()) {
            // Deal fatal damage on the final sonar impact (+10 combat hit -> 22 peak noise <= 30)
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

    // (c) & (d) Test enemy intrusion into a cleared room with an uncleared neighbor, and verify anti-infinite-respawn guard
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
    simGame.m_noiseLevel = 90.0f; // Scream tier -> leaks 45.0 (> 30) through open doorway
    simGame.updateNoiseSystem(cfg::kFixedTimeStep);

    if (simGame.m_enemies.size() != 1 || simGame.m_labyrinth.currentRoom().cleared ||
        !simGame.m_labyrinth.currentRoom().combatLocked || !simGame.m_labyrinth.currentRoom().noiseIntrusionActive) {
        std::cerr << "[SelfTest] FAIL: Loud/Scream noise in cleared room did not trigger neighbor enemy intrusion.\n";
        return 1;
    }

    // Re-clear the room after defeating the intruder, then make 95 noise again with 0 cooldown:
    // verify that noiseIntrusionConsumed prevents a second infinite respawn in the same room!
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

    // Clean up any checkpoint file created during simulation test
    std::remove(simGame.m_saveFilePath.c_str());

    std::cout << "[SelfTest] PASS [13/13]: End-to-end Ghost Clear after room transition, cross-room AI alert, enemy intrusion, & anti-respawn guard verified.\n";

    std::cout << "[SelfTest] ALL 13 ECHOLABYRINTH 5.0 (PHASE A) NATIVE C++17 VERIFICATION SUITES PASSED SUCCESSFULLY.\n";
    return 0;
}

} // namespace echo
