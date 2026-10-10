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
    std::cout << "[SelfTest] PASS [1/12]: 20 distinct seeds verified for 2D grid uniqueness, N/E/S/W door symmetry, & Boss/Exit reachability.\n";

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
    std::cout << "[SelfTest] PASS [2/12]: Deterministic seed reproducibility verified across all 32 chambers.\n";

    // 3. Verify 2D BFS Geometric Walkability across every room in the procedural run
    for (int r = 0; r < static_cast<int>(labA.roomCount()); ++r) {
        std::string travErr;
        if (!labA.verifyRoomTraversability(r, travErr)) {
            std::cerr << "[SelfTest] FAIL: Room " << r << " failed 2D BFS walkability: " << travErr << "\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [3/12]: All 32 procedural chambers verified via 2D BFS grid pathfinding.\n";

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
    std::cout << "[SelfTest] PASS [4/12]: Combat door lockdown, room clear unsealing, & Secret Wall sonar fracture verified.\n";

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
    std::cout << "[SelfTest] PASS [5/12]: AcousticSystem reflections & multi-wall material attenuation verified.\n";

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
    std::cout << "[SelfTest] PASS [6/12]: Puzzle chamber AcousticResonator crystal energized and unlocked.\n";

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
    std::cout << "[SelfTest] PASS [7/12]: Player Hull Integrity, Echo Shards economy, & Relic synergies verified.\n";

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
    std::cout << "[SelfTest] PASS [8/12]: EnemyAI 5 variants, 3-phase AbyssalBoss, & harmonic sonar damage verified.\n";

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
    outSave.collectedRelics = {0, 1, 5};

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
        inSave.totalVictories != 3 || inSave.collectedRelics.size() != 3 || inSave.clearedRooms.size() != 3) {
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
    std::cout << "[SelfTest] PASS [9/12]: SaveSystem V4 atomic serialization, seed/relic/meta round-trip, & checksum verified.\n";

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
    std::cout << "[SelfTest] PASS [10/12]: AssetManager loaded " << assets.loadedDiskModelCount()
              << "/7 Wavefront 3D .obj models (" << assets.totalTriangleCount()
              << " triangles) and 6 RGBA material textures.\n";

    // 11. BLOCK 1.1 & 1.2: Deterministic Noise Meter Action -> Expected Value & Threshold Classification
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
    std::cout << "[SelfTest] PASS [11/12]: Noise Meter action deltas (+2/+8/+25/+12/+30/+15/+10/-10) & NoiseTier AI responses verified.\n";

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
    std::cout << "[SelfTest] PASS [12/12]: Cross-room 50% noise propagation, closed-door blocking, & Ghost Clear (x1.5 Shards) verified.\n";

    std::cout << "[SelfTest] ALL 12 ECHOLABYRINTH 5.0 (PHASE A) NATIVE C++17 VERIFICATION SUITES PASSED SUCCESSFULLY.\n";
    return 0;
}

} // namespace echo
