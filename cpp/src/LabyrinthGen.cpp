#include "Labyrinth.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
#include <sstream>

namespace echo {

namespace {
struct XorShift32 {
    std::uint32_t state;
    explicit XorShift32(std::uint32_t seed) noexcept
        : state(seed == 0 ? 0x9E3779B9u : seed) {}

    std::uint32_t nextU32() noexcept {
        std::uint32_t x = state;
        x ^= x << 13u;
        x ^= x >> 17u;
        x ^= x << 5u;
        state = x;
        return x;
    }

    int rangeInt(int minVal, int maxVal) noexcept {
        if (maxVal <= minVal) return minVal;
        return minVal + static_cast<int>(nextU32() % static_cast<std::uint32_t>(maxVal - minVal + 1));
    }
};

void addRectObstacle(
    std::vector<WallSegment>& walls,
    float x, float y, float w, float h,
    SurfaceMaterial mat
) {
    walls.push_back({{x, y}, {x + w, y}, mat, -1, false});
    walls.push_back({{x + w, y}, {x + w, y + h}, mat, -1, false});
    walls.push_back({{x + w, y + h}, {x, y + h}, mat, -1, false});
    walls.push_back({{x, y + h}, {x, y}, mat, -1, false});
}

void addFourWayRoomPerimeter(
    std::vector<WallSegment>& walls,
    SurfaceMaterial mat,
    const std::array<int, 4>& neighbors,
    const std::array<int, 4>& doorGateIds,
    const std::array<SurfaceMaterial, 4>& doorMaterials,
    const std::array<bool, 4>& doorInitiallyOpen
) {
    constexpr float left = 100.0f;
    constexpr float right = 1180.0f;
    constexpr float top = 90.0f;
    constexpr float bottom = 710.0f;

    // 1. NORTH WALL (y = top, vestibule x in [585, 695])
    if (neighbors[static_cast<std::size_t>(CardinalDir::North)] != -1) {
        walls.push_back({{left, top}, {585.0f, top}, mat, -1, false});
        walls.push_back({{585.0f, top}, {585.0f, 42.0f}, mat, -1, false});
        walls.push_back({{585.0f, 42.0f}, {695.0f, 42.0f}, mat, -1, false});
        walls.push_back({{695.0f, 42.0f}, {695.0f, top}, mat, -1, false});
        walls.push_back({{695.0f, top}, {right, top}, mat, -1, false});
        const int gateId = doorGateIds[static_cast<std::size_t>(CardinalDir::North)];
        if (gateId != -1) {
            walls.push_back({{585.0f, top}, {695.0f, top}, doorMaterials[static_cast<std::size_t>(CardinalDir::North)], gateId, doorInitiallyOpen[static_cast<std::size_t>(CardinalDir::North)]});
        }
    } else {
        walls.push_back({{left, top}, {right, top}, mat, -1, false});
    }

    // 2. SOUTH WALL (y = bottom, vestibule x in [585, 695])
    if (neighbors[static_cast<std::size_t>(CardinalDir::South)] != -1) {
        walls.push_back({{right, bottom}, {695.0f, bottom}, mat, -1, false});
        walls.push_back({{695.0f, bottom}, {695.0f, 758.0f}, mat, -1, false});
        walls.push_back({{695.0f, 758.0f}, {585.0f, 758.0f}, mat, -1, false});
        walls.push_back({{585.0f, 758.0f}, {585.0f, bottom}, mat, -1, false});
        walls.push_back({{585.0f, bottom}, {left, bottom}, mat, -1, false});
        const int gateId = doorGateIds[static_cast<std::size_t>(CardinalDir::South)];
        if (gateId != -1) {
            walls.push_back({{585.0f, bottom}, {695.0f, bottom}, doorMaterials[static_cast<std::size_t>(CardinalDir::South)], gateId, doorInitiallyOpen[static_cast<std::size_t>(CardinalDir::South)]});
        }
    } else {
        walls.push_back({{right, bottom}, {left, bottom}, mat, -1, false});
    }

    // 3. WEST WALL (x = left, vestibule y in [345, 455])
    if (neighbors[static_cast<std::size_t>(CardinalDir::West)] != -1) {
        walls.push_back({{left, top}, {left, 345.0f}, mat, -1, false});
        walls.push_back({{left, 345.0f}, {45.0f, 345.0f}, mat, -1, false});
        walls.push_back({{45.0f, 345.0f}, {45.0f, 455.0f}, mat, -1, false});
        walls.push_back({{45.0f, 455.0f}, {left, 455.0f}, mat, -1, false});
        walls.push_back({{left, 455.0f}, {left, bottom}, mat, -1, false});
        const int gateId = doorGateIds[static_cast<std::size_t>(CardinalDir::West)];
        if (gateId != -1) {
            walls.push_back({{left, 345.0f}, {left, 455.0f}, doorMaterials[static_cast<std::size_t>(CardinalDir::West)], gateId, doorInitiallyOpen[static_cast<std::size_t>(CardinalDir::West)]});
        }
    } else {
        walls.push_back({{left, bottom}, {left, top}, mat, -1, false});
    }

    // 4. EAST WALL (x = right, vestibule y in [345, 455])
    if (neighbors[static_cast<std::size_t>(CardinalDir::East)] != -1) {
        walls.push_back({{right, top}, {right, 345.0f}, mat, -1, false});
        walls.push_back({{right, 345.0f}, {1235.0f, 345.0f}, mat, -1, false});
        walls.push_back({{1235.0f, 345.0f}, {1235.0f, 455.0f}, mat, -1, false});
        walls.push_back({{1235.0f, 455.0f}, {right, 455.0f}, mat, -1, false});
        walls.push_back({{right, 455.0f}, {right, bottom}, mat, -1, false});
        const int gateId = doorGateIds[static_cast<std::size_t>(CardinalDir::East)];
        if (gateId != -1) {
            walls.push_back({{right, 345.0f}, {right, 455.0f}, doorMaterials[static_cast<std::size_t>(CardinalDir::East)], gateId, doorInitiallyOpen[static_cast<std::size_t>(CardinalDir::East)]});
        }
    } else {
        walls.push_back({{right, top}, {right, bottom}, mat, -1, false});
    }
}
} // namespace

Labyrinth::Labyrinth() {
    generateRun(m_runSeed, m_targetRoomCount);
}

void Labyrinth::resetAllProgress() {
    m_activatedSwitches.clear();
    m_collectedLoreIds.clear();
    m_visitedRooms.clear();
    m_clearedRooms.clear();
    m_currentRoomIndex = 0;
    generateRun(m_runSeed, m_targetRoomCount);
}

void Labyrinth::buildAllRooms() {
    generateRun(m_runSeed, m_targetRoomCount);
}

void Labyrinth::generateRun(std::uint32_t seed, std::size_t targetRoomCount) {
    m_runSeed = (seed == 0) ? 0xEC402026u : seed;
    m_targetRoomCount = std::clamp<std::size_t>(targetRoomCount, 15, 60);

    m_rooms.clear();
    m_activatedSwitches.clear();
    m_collectedLoreIds.clear();
    m_visitedRooms.clear();
    m_clearedRooms.clear();
    m_currentRoomIndex = 0;

    XorShift32 rng(m_runSeed);

    struct GridNode {
        int gx{0};
        int gy{0};
        RoomType type{RoomType::Combat};
        std::array<int, 4> neighbors{{-1, -1, -1, -1}};
        int distFromStart{0};
        int degree{0};
    };

    std::vector<GridNode> nodes;
    nodes.reserve(m_targetRoomCount);

    auto findNodeAt = [&](int gx, int gy) -> int {
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].gx == gx && nodes[i].gy == gy) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };

    auto linkNodes = [&](int a, CardinalDir dirFromA, int b) {
        const auto dIdx = static_cast<std::size_t>(dirFromA);
        const auto oppIdx = static_cast<std::size_t>(oppositeDir(dirFromA));
        if (nodes[a].neighbors[dIdx] == -1) {
            nodes[a].neighbors[dIdx] = b;
            ++nodes[a].degree;
        }
        if (nodes[b].neighbors[oppIdx] == -1) {
            nodes[b].neighbors[oppIdx] = a;
            ++nodes[b].degree;
        }
    };

    // 1. Place Start Room at (0, 0)
    nodes.push_back(GridNode{0, 0, RoomType::Start, {-1, -1, -1, -1}, 0, 0});

    // Reserve 2 slots at the end for FinalExit and Secret room so total count == m_targetRoomCount
    const std::size_t mainBodyTarget = (m_targetRoomCount > 4) ? (m_targetRoomCount - 2) : m_targetRoomCount;

    // 2. Grow connected branching 2D grid graph
    int guard = 0;
    while (nodes.size() < mainBodyTarget && guard < 10000) {
        ++guard;
        const int parentIdx = rng.rangeInt(0, static_cast<int>(nodes.size()) - 1);
        const CardinalDir dir = static_cast<CardinalDir>(rng.rangeInt(0, 3));
        const int nx = nodes[parentIdx].gx + dirDeltaX(dir);
        const int ny = nodes[parentIdx].gy + dirDeltaY(dir);

        const int existing = findNodeAt(nx, ny);
        if (existing != -1) {
            if (rng.rangeInt(0, 99) < 14) {
                linkNodes(parentIdx, dir, existing);
            }
            continue;
        }

        int adjacentCount = 0;
        for (int d = 0; d < 4; ++d) {
            const auto cd = static_cast<CardinalDir>(d);
            if (findNodeAt(nx + dirDeltaX(cd), ny + dirDeltaY(cd)) != -1) {
                ++adjacentCount;
            }
        }
        if (adjacentCount > 1 && rng.rangeInt(0, 99) < 72) {
            continue;
        }

        const int newIdx = static_cast<int>(nodes.size());
        nodes.push_back(GridNode{nx, ny, RoomType::Combat, {-1, -1, -1, -1}, 0, 0});
        linkNodes(parentIdx, dir, newIdx);
    }

    // 3. Compute exact shortest-path BFS distances from Start (0)
    auto recomputeBfsDistances = [&]() {
        for (auto& n : nodes) n.distFromStart = 9999;
        nodes[0].distFromStart = 0;
        std::queue<int> q;
        q.push(0);
        while (!q.empty()) {
            const int u = q.front();
            q.pop();
            for (int d = 0; d < 4; ++d) {
                const int v = nodes[u].neighbors[static_cast<std::size_t>(d)];
                if (v != -1 && nodes[v].distFromStart > nodes[u].distFromStart + 1) {
                    nodes[v].distFromStart = nodes[u].distFromStart + 1;
                    q.push(v);
                }
            }
        }
    };
    recomputeBfsDistances();

    // 4. Select Boss Room at maximum distance and attach FinalExit beyond it
    int bossIdx = -1;
    CardinalDir exitDirFromBoss = CardinalDir::East;
    int maxDist = -1;

    for (std::size_t i = 1; i < nodes.size(); ++i) {
        for (int d = 0; d < 4; ++d) {
            const auto cd = static_cast<CardinalDir>(d);
            const int ex = nodes[i].gx + dirDeltaX(cd);
            const int ey = nodes[i].gy + dirDeltaY(cd);
            if (findNodeAt(ex, ey) == -1) {
                if (nodes[i].distFromStart > maxDist) {
                    maxDist = nodes[i].distFromStart;
                    bossIdx = static_cast<int>(i);
                    exitDirFromBoss = cd;
                }
            }
        }
    }

    if (bossIdx == -1) {
        bossIdx = static_cast<int>(nodes.size() - 1);
    }
    nodes[bossIdx].type = RoomType::Boss;
    m_bossRoomId = bossIdx;

    if (nodes.size() < m_targetRoomCount) {
        const int ex = nodes[bossIdx].gx + dirDeltaX(exitDirFromBoss);
        const int ey = nodes[bossIdx].gy + dirDeltaY(exitDirFromBoss);
        const int exitIdx = static_cast<int>(nodes.size());
        nodes.push_back(GridNode{ex, ey, RoomType::FinalExit, {-1, -1, -1, -1}, nodes[bossIdx].distFromStart + 1, 0});
        linkNodes(bossIdx, exitDirFromBoss, exitIdx);
        m_exitRoomId = exitIdx;
    } else {
        m_exitRoomId = bossIdx;
    }

    // 5. Add a Secret Room attached by a single concealed doorway
    while (nodes.size() < m_targetRoomCount) {
        int hostIdx = rng.rangeInt(1, static_cast<int>(nodes.size()) - 1);
        if (nodes[hostIdx].type == RoomType::Boss || nodes[hostIdx].type == RoomType::FinalExit) {
            hostIdx = 0;
        }
        bool placed = false;
        for (int d = 0; d < 4; ++d) {
            const auto cd = static_cast<CardinalDir>((d + rng.rangeInt(0, 3)) % 4);
            const int sx = nodes[hostIdx].gx + dirDeltaX(cd);
            const int sy = nodes[hostIdx].gy + dirDeltaY(cd);
            if (findNodeAt(sx, sy) == -1) {
                const int secIdx = static_cast<int>(nodes.size());
                const RoomType rType = (nodes.size() + 1 == m_targetRoomCount) ? RoomType::Secret : RoomType::Combat;
                nodes.push_back(GridNode{sx, sy, rType, {-1, -1, -1, -1}, nodes[hostIdx].distFromStart + 1, 0});
                linkNodes(hostIdx, cd, secIdx);
                placed = true;
                break;
            }
        }
        if (!placed) {
            for (std::size_t i = 0; i < nodes.size() && !placed; ++i) {
                if (nodes[i].type == RoomType::FinalExit) continue;
                for (int d = 0; d < 4; ++d) {
                    const auto cd = static_cast<CardinalDir>(d);
                    const int sx = nodes[i].gx + dirDeltaX(cd);
                    const int sy = nodes[i].gy + dirDeltaY(cd);
                    if (findNodeAt(sx, sy) == -1) {
                        const int secIdx = static_cast<int>(nodes.size());
                        nodes.push_back(GridNode{sx, sy, RoomType::Secret, {-1, -1, -1, -1}, nodes[i].distFromStart + 1, 0});
                        linkNodes(static_cast<int>(i), cd, secIdx);
                        placed = true;
                        break;
                    }
                }
            }
        }
    }

    recomputeBfsDistances();

    // 6. Assign specialized RoomTypes
    std::vector<int> candidateIndices;
    for (int i = 1; i < static_cast<int>(nodes.size()); ++i) {
        if (nodes[i].type == RoomType::Combat) {
            candidateIndices.push_back(i);
        }
    }

    std::sort(candidateIndices.begin(), candidateIndices.end(), [&](int a, int b) {
        if (nodes[a].degree != nodes[b].degree) return nodes[a].degree < nodes[b].degree;
        return nodes[a].distFromStart > nodes[b].distFromStart;
    });

    const std::vector<RoomType> specialQuotas = {
        RoomType::Treasure,
        RoomType::Shop,
        RoomType::Sanctuary,
        RoomType::Puzzle,
        RoomType::Elite,
        RoomType::Treasure,
        RoomType::Sanctuary,
        RoomType::Puzzle,
        RoomType::Elite,
        RoomType::Secret
    };

    std::size_t quotaCursor = 0;
    for (int idx : candidateIndices) {
        if (quotaCursor >= specialQuotas.size()) break;
        const RoomType desired = specialQuotas[quotaCursor];
        if (desired == RoomType::Secret && nodes[idx].degree > 1) {
            continue;
        }
        nodes[idx].type = desired;
        ++quotaCursor;
    }

    bool hasSecret = false;
    for (const auto& n : nodes) {
        if (n.type == RoomType::Secret) hasSecret = true;
    }
    if (!hasSecret) {
        for (int idx : candidateIndices) {
            if (nodes[idx].degree == 1 && nodes[idx].type != RoomType::Boss && nodes[idx].type != RoomType::FinalExit) {
                nodes[idx].type = RoomType::Secret;
                break;
            }
        }
    }

    // Stage 2: Ensure Room 15 (16th Chamber, CHAMBER 16) is the Mini-Boss Elite Chamber (The Conductor)
    int conductorIdx = (nodes.size() > 15) ? 15 : static_cast<int>(nodes.size() / 2);
    if (nodes[static_cast<std::size_t>(conductorIdx)].type == RoomType::Boss ||
        nodes[static_cast<std::size_t>(conductorIdx)].type == RoomType::FinalExit ||
        nodes[static_cast<std::size_t>(conductorIdx)].type == RoomType::Secret ||
        conductorIdx == 0) {
        for (int offset = 1; offset < static_cast<int>(nodes.size()); ++offset) {
            const int cand = (15 + offset) % static_cast<int>(nodes.size());
            if (cand > 0 &&
                nodes[static_cast<std::size_t>(cand)].type != RoomType::Boss &&
                nodes[static_cast<std::size_t>(cand)].type != RoomType::FinalExit &&
                nodes[static_cast<std::size_t>(cand)].type != RoomType::Secret) {
                conductorIdx = cand;
                break;
            }
        }
    }
    nodes[static_cast<std::size_t>(conductorIdx)].type = RoomType::Elite;
    m_conductorRoomId = conductorIdx;

    // 7. Construct full RoomDefinition geometry, 4-directional doors, props, relics, codex logs, and enemy waves
    m_rooms.resize(nodes.size());
    std::size_t relicRotationCursor = static_cast<std::size_t>(m_runSeed % 11u);
    int codexLogCursor = 0;

    const RoomEvent eventPool[4] = {
        RoomEvent::None,
        RoomEvent::EchoStorm,
        RoomEvent::AcousticBlackout,
        RoomEvent::ResonanceOvercharge
    };
    std::size_t eventRotationCursor = 0;

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto& gn = nodes[i];
        RoomDefinition room;
        room.id = static_cast<int>(i);
        room.gridX = gn.gx;
        room.gridY = gn.gy;
        room.distFromStart = gn.distFromStart;
        room.type = gn.type;
        room.biome = biomeForRoomIndex(i);
        room.neighbors = gn.neighbors;
        room.playerDefaultSpawn = {640.0f, 400.0f};
        room.isCheckpoint = (gn.type == RoomType::Start || gn.type == RoomType::Sanctuary || gn.type == RoomType::FinalExit);
        room.isExitRoom = (gn.type == RoomType::FinalExit);

        const BiomeSpec bSpec = getBiomeSpec(room.biome);
        room.sectorTitle = bSpec.name;

        // Block 5.1: Deterministically assign dynamic acoustic RoomEvents to Combat, Elite, and Puzzle chambers
        if (gn.type == RoomType::Combat || gn.type == RoomType::Elite || gn.type == RoomType::Puzzle) {
            room.roomEvent = eventPool[eventRotationCursor % 4];
            ++eventRotationCursor;
        }

        std::ostringstream titleOss;
        titleOss << "CHAMBER " << (i + 1) << " [" << gn.gx << "," << gn.gy << "] - " << roomTypeName(gn.type);
        room.name = titleOss.str();

        const SurfaceMaterial primaryMat = ((i + m_runSeed) % 2 == 0) ? bSpec.primaryMaterial : bSpec.secondaryMaterial;
        const SurfaceMaterial secondaryMat = ((i + m_runSeed) % 2 == 0) ? bSpec.secondaryMaterial : bSpec.primaryMaterial;
        room.floorMaterial = bSpec.primaryMaterial;

        std::array<int, 4> doorGateIds{{-1, -1, -1, -1}};
        std::array<SurfaceMaterial, 4> doorMats{{
            SurfaceMaterial::ResonanceDoor,
            SurfaceMaterial::ResonanceDoor,
            SurfaceMaterial::ResonanceDoor,
            SurfaceMaterial::ResonanceDoor
        }};
        std::array<bool, 4> doorOpen{{true, true, true, true}};

        for (int d = 0; d < 4; ++d) {
            const int nb = gn.neighbors[static_cast<std::size_t>(d)];
            if (nb == -1) continue;

            if (gn.type == RoomType::Secret || nodes[nb].type == RoomType::Secret) {
                const int secId = (gn.type == RoomType::Secret) ? static_cast<int>(i) : nb;
                doorGateIds[static_cast<std::size_t>(d)] = 900 + secId;
                doorMats[static_cast<std::size_t>(d)] = SurfaceMaterial::Stone;
                doorOpen[static_cast<std::size_t>(d)] = false;
            } else if (gn.type == RoomType::Boss && nodes[nb].type == RoomType::FinalExit) {
                doorGateIds[static_cast<std::size_t>(d)] = 800 + static_cast<int>(i);
                doorMats[static_cast<std::size_t>(d)] = SurfaceMaterial::ResonanceDoor;
                doorOpen[static_cast<std::size_t>(d)] = false;
            } else if (gn.type == RoomType::Combat || gn.type == RoomType::Elite || gn.type == RoomType::Boss) {
                doorGateIds[static_cast<std::size_t>(d)] = 800 + static_cast<int>(i);
                doorMats[static_cast<std::size_t>(d)] = SurfaceMaterial::ResonanceDoor;
                doorOpen[static_cast<std::size_t>(d)] = true;
            }
        }

        const SurfaceMaterial perimeterMat = (room.biome == BiomeId::DrownedGalleries)
            ? SurfaceMaterial::Concrete
            : bSpec.primaryMaterial;
        addFourWayRoomPerimeter(room.walls, perimeterMat, gn.neighbors, doorGateIds, doorMats, doorOpen);

        room.props.push_back({PropType::PipeConduit, {135.0f, 112.0f}, {540.0f, 112.0f}, 1.0f, 0.0f});
        room.props.push_back({PropType::PipeConduit, {740.0f, 688.0f}, {1145.0f, 688.0f}, 1.0f, 0.0f});
        room.props.push_back({PropType::VentTurbine, {195.0f, 155.0f}, {0.0f, 0.0f}, 20.0f, static_cast<float>(i * 29)});
        room.props.push_back({PropType::VentTurbine, {1085.0f, 645.0f}, {0.0f, 0.0f}, 20.0f, static_cast<float>(i * 47)});
        room.props.push_back({PropType::WaterPuddle, {250.0f, 560.0f}, {0.0f, 0.0f}, 34.0f, 0.0f});
        room.props.push_back({PropType::WaterPuddle, {1010.0f, 235.0f}, {0.0f, 0.0f}, 38.0f, 0.0f});
        if (room.biome == BiomeId::DrownedGalleries) {
            room.props.push_back({PropType::WaterPuddle, {640.0f, 400.0f}, {0.0f, 0.0f}, 54.0f, 0.0f});
        } else if (room.biome == BiomeId::IronFoundry) {
            room.ambientEmitters.push_back({{320.0f, 220.0f}, 3.0f, 0.4f, true});
            room.ambientEmitters.push_back({{960.0f, 580.0f}, 3.4f, 1.2f, true});
        }
        if (room.isCheckpoint) {
            room.props.push_back({PropType::EmergencyBeacon, {520.0f, 320.0f}, {0.0f, 0.0f}, 1.0f, 0.0f});
        }

        switch (gn.type) {
            case RoomType::Start: {
                room.cleared = true;
                room.visited = true;
                room.discovered = true;
                room.objectiveText = "EXPLORE VIA N/E/S/W DOORS. KEEP NOISE <= 30 FOR GHOST CLEAR (x1.5 SHARDS & SILENT STEP).";
                addRectObstacle(room.walls, 320.0f, 210.0f, 95.0f, 95.0f, SurfaceMaterial::Concrete);
                addRectObstacle(room.walls, 865.0f, 210.0f, 95.0f, 95.0f, SurfaceMaterial::Water);
                addRectObstacle(room.walls, 320.0f, 495.0f, 95.0f, 95.0f, SurfaceMaterial::Water);
                addRectObstacle(room.walls, 865.0f, 495.0f, 95.0f, 95.0f, SurfaceMaterial::Concrete);
                const CodexEntrySpec c0 = getCodexEntrySpec(codexLogCursor++);
                room.collectibles.push_back({
                    c0.id,
                    {640.0f, 280.0f},
                    false,
                    c0.title,
                    c0.excerpt
                });
                break;
            }

            case RoomType::Combat: {
                room.cleared = false;
                room.shardReward = 12 + (gn.distFromStart * 2);
                room.objectiveText = "DISRUPT ALL HUNTERS TO UNSEAL EXITS. KEEP PEAK NOISE <= 30 FOR GHOST CLEAR BONUS!";

                const int layout = static_cast<int>((i + rng.nextU32()) % 3);
                if (room.biome == BiomeId::CrystalVaults) {
                    addRectObstacle(room.walls, 340.0f, 210.0f, 105.0f, 105.0f, SurfaceMaterial::EchoCrystal);
                    addRectObstacle(room.walls, 835.0f, 210.0f, 105.0f, 105.0f, SurfaceMaterial::EchoCrystal);
                    addRectObstacle(room.walls, 340.0f, 485.0f, 105.0f, 105.0f, SurfaceMaterial::Glass);
                    addRectObstacle(room.walls, 835.0f, 485.0f, 105.0f, 105.0f, SurfaceMaterial::EchoCrystal);
                    room.walls.push_back({{580.0f, 265.0f}, {700.0f, 265.0f}, SurfaceMaterial::Glass, 700 + static_cast<int>(i), false});
                } else if (layout == 0) {
                    addRectObstacle(room.walls, 340.0f, 210.0f, 110.0f, 110.0f, SurfaceMaterial::DampMoss);
                    addRectObstacle(room.walls, 830.0f, 210.0f, 110.0f, 110.0f, primaryMat);
                    addRectObstacle(room.walls, 340.0f, 480.0f, 110.0f, 110.0f, primaryMat);
                    addRectObstacle(room.walls, 830.0f, 480.0f, 110.0f, 110.0f, SurfaceMaterial::DampMoss);
                    room.walls.push_back({{580.0f, 265.0f}, {700.0f, 265.0f}, SurfaceMaterial::Glass, 700 + static_cast<int>(i), false});
                } else if (layout == 1) {
                    addRectObstacle(room.walls, 460.0f, 230.0f, 90.0f, 110.0f, primaryMat);
                    addRectObstacle(room.walls, 730.0f, 460.0f, 90.0f, 110.0f, SurfaceMaterial::DampMoss);
                    room.hazards.push_back({{420.0f, 520.0f}, 42.0f, "BRITTLE SHALE"});
                    room.ambientEmitters.push_back({{820.0f, 250.0f}, 3.6f, 0.5f, true});
                    room.resonators.push_back({
                        750 + static_cast<int>(i),
                        {640.0f, 400.0f},
                        16.0f,
                        0.0f,
                        false,
                        -1,
                        "RESONANCE PIPE COLUMN (4S HUM LURE)",
                        0.0f,
                        0.0f
                    });
                } else {
                    addRectObstacle(room.walls, 310.0f, 240.0f, 95.0f, 320.0f, secondaryMat);
                    addRectObstacle(room.walls, 875.0f, 240.0f, 95.0f, 320.0f, SurfaceMaterial::DampMoss);
                    room.walls.push_back({{580.0f, 535.0f}, {700.0f, 535.0f}, SurfaceMaterial::Glass, 700 + static_cast<int>(i), false});
                }

                if (codexLogCursor < static_cast<int>(kTotalCodexFragments)) {
                    const CodexEntrySpec cx = getCodexEntrySpec(codexLogCursor++);
                    room.collectibles.push_back({
                        cx.id,
                        {640.0f, 330.0f},
                        false,
                        cx.title,
                        cx.excerpt
                    });
                }

                const EnemyVariant combatPool[10] = {
                    EnemyVariant::Stalker,
                    EnemyVariant::Shrieker,
                    EnemyVariant::SilentStalker,
                    EnemyVariant::DeafBrute,
                    EnemyVariant::Mimic,
                    EnemyVariant::EchoSwarm,
                    EnemyVariant::Phantom,
                    EnemyVariant::Leech,
                    EnemyVariant::Banshee,
                    EnemyVariant::TuningForkConstruct
                };

                EnemyVariant v1 = combatPool[(i + gn.distFromStart + m_runSeed) % 10];
                if (room.biome == BiomeId::DrownedGalleries && (i % 2 == 1)) {
                    v1 = EnemyVariant::Leech;
                } else if (room.biome == BiomeId::CrystalVaults && (i % 2 == 0)) {
                    v1 = EnemyVariant::Banshee;
                } else if (room.biome == BiomeId::IronFoundry && (i % 2 == 1)) {
                    v1 = EnemyVariant::TuningForkConstruct;
                } else if (room.biome == BiomeId::LivingMycelium && (i % 3 == 0)) {
                    v1 = EnemyVariant::Leech;
                }

                room.enemyVariant = v1;
                room.enemyPatrolNodes = {
                    {480.0f, 200.0f},
                    {800.0f, 200.0f},
                    {800.0f, 600.0f},
                    {480.0f, 600.0f}
                };
                room.enemySpawns.push_back({v1, room.enemyPatrolNodes, 0});

                if (gn.distFromStart >= 3) {
                    const EnemyVariant v2 = combatPool[(i * 3 + m_runSeed + 2) % 10];
                    const int waveIdx = (gn.distFromStart >= 5 && (i % 2 == 1)) ? 1 : 0;
                    room.totalWaves = (waveIdx == 1) ? 2 : 1;
                    room.enemySpawns.push_back({
                        v2,
                        {{780.0f, 560.0f}, {500.0f, 560.0f}, {500.0f, 240.0f}, {780.0f, 240.0f}},
                        waveIdx
                    });
                }
                break;
            }

            case RoomType::Treasure: {
                room.cleared = true;
                room.objectiveText = "PRESS [F] NEAR THE CENTRAL PEDESTAL TO CLAIM AN ACOUSTIC RELIC.";
                addRectObstacle(room.walls, 370.0f, 220.0f, 90.0f, 90.0f, bSpec.primaryMaterial);
                addRectObstacle(room.walls, 820.0f, 220.0f, 90.0f, 90.0f, bSpec.primaryMaterial);
                addRectObstacle(room.walls, 370.0f, 490.0f, 90.0f, 90.0f, bSpec.primaryMaterial);
                addRectObstacle(room.walls, 820.0f, 490.0f, 90.0f, 90.0f, bSpec.primaryMaterial);

                const RelicId rId = getPedestalRelicFromPool(relicRotationCursor++);
                room.relicPedestals.push_back({rId, {640.0f, 400.0f}, 0, false});
                break;
            }

            case RoomType::Secret: {
                room.cleared = true;
                room.shardReward = 25;
                room.objectiveText = "CONCEALED ACOUSTIC CAVERN DISCOVERED! CLAIM THE ANCIENT RELIC & ECHO CACHE [F].";
                const RelicId rId = getPedestalRelicFromPool(relicRotationCursor++);
                room.relicPedestals.push_back({rId, {640.0f, 360.0f}, 0, false});
                const CodexEntrySpec cs = getCodexEntrySpec(codexLogCursor < static_cast<int>(kTotalCodexFragments) ? codexLogCursor++ : 11);
                room.collectibles.push_back({
                    cs.id,
                    {640.0f, 480.0f},
                    false,
                    cs.title,
                    cs.excerpt
                });
                break;
            }

            case RoomType::Puzzle: {
                room.cleared = true;
                room.shardReward = 20;
                room.objectiveText = "BOUNCE SONAR RAYS OFF THE 45-DEGREE STEEL MIRRORS (+2 BOUNCES) TO ENERGIZE THE CRYSTAL.";
                const int switchId = 100 + static_cast<int>(i);

                addRectObstacle(room.walls, 460.0f, 250.0f, 80.0f, 120.0f, SurfaceMaterial::Stone);
                addRectObstacle(room.walls, 460.0f, 430.0f, 80.0f, 120.0f, SurfaceMaterial::Stone);
                room.walls.push_back({{340.0f, 190.0f}, {430.0f, 280.0f}, SurfaceMaterial::Metal, -1, false});
                room.walls.push_back({{340.0f, 610.0f}, {430.0f, 520.0f}, SurfaceMaterial::Metal, -1, false});
                room.walls.push_back({{850.0f, 280.0f}, {940.0f, 190.0f}, SurfaceMaterial::Metal, -1, false});
                room.walls.push_back({{850.0f, 520.0f}, {940.0f, 610.0f}, SurfaceMaterial::Metal, -1, false});
                room.walls.push_back({{565.0f, 365.0f}, {565.0f, 435.0f}, SurfaceMaterial::Glass, 700 + static_cast<int>(i), false});

                room.resonators.push_back({
                    switchId,
                    {640.0f, 400.0f},
                    18.0f,
                    0.0f,
                    false,
                    switchId,
                    "HARMONIC RESONATOR CRYSTAL",
                    0.0f,
                    0.0f
                });
                room.mechanisms.push_back({
                    switchId,
                    {760.0f, 250.0f},
                    false,
                    0.0f,
                    "AUXILIARY CONDUIT LEVER",
                    switchId
                });
                break;
            }

            case RoomType::Sanctuary: {
                room.cleared = true;
                room.objectiveText = "SANCTUARY REACHED: INTERACT [F] WITH THE HARMONIC SHRINE TO RESTORE HULL INTEGRITY & SAVE.";
                addRectObstacle(room.walls, 380.0f, 230.0f, 95.0f, 95.0f, SurfaceMaterial::DampMoss);
                addRectObstacle(room.walls, 805.0f, 230.0f, 95.0f, 95.0f, SurfaceMaterial::DampMoss);
                room.mechanisms.push_back({
                    500 + static_cast<int>(i),
                    {640.0f, 400.0f},
                    false,
                    0.0f,
                    "SANCTUARY REPAIR SHRINE (+2 HP & SAVE)",
                    -1
                });
                break;
            }

            case RoomType::Shop: {
                room.cleared = true;
                room.objectiveText = "QUARTERMASTER EXCHANGE: PRESS [F] NEAR AN ITEM TO SPEND ECHO SHARDS.";
                const RelicId rId = getPedestalRelicFromPool(relicRotationCursor++);
                const RelicSpec spec = getRelicSpec(rId);

                room.relicPedestals.push_back({rId, {460.0f, 380.0f}, spec.shopCostShards, false});
                room.shopOffers.push_back({
                    600 + static_cast<int>(i) * 2,
                    {640.0f, 380.0f},
                    "HULL INTEGRITY REPAIR (+2 HP)",
                    15,
                    2,
                    0,
                    false
                });
                room.shopOffers.push_back({
                    601 + static_cast<int>(i) * 2,
                    {820.0f, 380.0f},
                    "ACOUSTIC DECOY & TRAP PACK (+3 DECOYS, +2 TRAPS)",
                    12,
                    0,
                    3,
                    false
                });
                break;
            }

            case RoomType::Elite: {
                room.cleared = false;
                const bool isConductorChamber = (static_cast<int>(i) == m_conductorRoomId);
                room.totalWaves = isConductorChamber ? 1 : 2;
                room.shardReward = isConductorChamber ? 40 : 28;
                room.objectiveText = isConductorChamber
                    ? "MINI-BOSS: DISRUPT THE CONDUCTOR WITH REFLECTED OR HIGH-FREQ [3] SONAR RAYS TO CLAIM RELIC!"
                    : "ELITE GARRISON: DEFEAT THE ARMORED WARDEN / TUNING FORK CONSTRUCT & REINFORCEMENTS TO CLAIM RELIC.";

                const SurfaceMaterial pillarMat = (room.biome == BiomeId::CrystalVaults)
                    ? SurfaceMaterial::EchoCrystal
                    : SurfaceMaterial::Metal;
                addRectObstacle(room.walls, 360.0f, 230.0f, 105.0f, 120.0f, pillarMat);
                addRectObstacle(room.walls, 815.0f, 230.0f, 105.0f, 120.0f, SurfaceMaterial::DampMoss);
                addRectObstacle(room.walls, 360.0f, 450.0f, 105.0f, 120.0f, SurfaceMaterial::DampMoss);
                addRectObstacle(room.walls, 815.0f, 450.0f, 105.0f, 120.0f, pillarMat);
                room.hazards.push_back({{640.0f, 240.0f}, 44.0f, "BRITTLE SHALE"});
                room.hazards.push_back({{640.0f, 560.0f}, 44.0f, "BRITTLE SHALE"});

                if (isConductorChamber) {
                    room.enemyVariant = EnemyVariant::TheConductor;
                    room.enemyPatrolNodes = {
                        {520.0f, 220.0f},
                        {760.0f, 220.0f},
                        {760.0f, 580.0f},
                        {520.0f, 580.0f}
                    };
                    room.enemySpawns.push_back({EnemyVariant::TheConductor, room.enemyPatrolNodes, 0});
                } else {
                    const EnemyVariant elitePrimary = (i % 3 == 0)
                        ? EnemyVariant::TuningForkConstruct
                        : ((i % 2 == 0) ? EnemyVariant::DeafBrute : EnemyVariant::Warden);
                    const EnemyVariant eliteSecondary = (i % 2 == 0) ? EnemyVariant::Banshee : EnemyVariant::Leech;
                    room.enemyVariant = elitePrimary;
                    room.enemyPatrolNodes = {
                        {520.0f, 200.0f},
                        {760.0f, 200.0f},
                        {760.0f, 600.0f},
                        {520.0f, 600.0f}
                    };
                    room.enemySpawns.push_back({elitePrimary, room.enemyPatrolNodes, 0});
                    room.enemySpawns.push_back({
                        eliteSecondary,
                        {{300.0f, 200.0f}, {980.0f, 200.0f}, {980.0f, 600.0f}, {300.0f, 600.0f}},
                        1
                    });
                }

                const RelicId rId = getPedestalRelicFromPool(relicRotationCursor++);
                room.relicPedestals.push_back({rId, {640.0f, 400.0f}, 0, false});
                break;
            }

            case RoomType::Boss: {
                room.cleared = false;
                room.totalWaves = 1;
                room.shardReward = 50;
                room.objectiveText = "BOSS: SHATTER THE CHOIR SOVEREIGN WITH SONAR PINGS [LMB] & PULSES [SPACE] TO UNSEAL THE ASCENT!";

                addRectObstacle(room.walls, 340.0f, 210.0f, 95.0f, 95.0f, SurfaceMaterial::Metal);
                addRectObstacle(room.walls, 845.0f, 210.0f, 95.0f, 95.0f, SurfaceMaterial::Metal);
                addRectObstacle(room.walls, 340.0f, 495.0f, 95.0f, 95.0f, SurfaceMaterial::Metal);
                addRectObstacle(room.walls, 845.0f, 495.0f, 95.0f, 95.0f, SurfaceMaterial::Metal);

                room.enemyVariant = EnemyVariant::AbyssalBoss;
                room.enemyPatrolNodes = {
                    {640.0f, 240.0f},
                    {780.0f, 400.0f},
                    {640.0f, 560.0f},
                    {500.0f, 400.0f}
                };
                room.enemySpawns.push_back({EnemyVariant::AbyssalBoss, room.enemyPatrolNodes, 0});
                break;
            }

            case RoomType::FinalExit: {
                room.cleared = true;
                room.objectiveText = "ENGAGE THE SURFACE LIFT INTERLOCK [F] AND STEP INTO THE ASCENT PORTAL TO COMPLETE THE RUN!";
                const int finalSwitch = 100 + static_cast<int>(i);
                addRectObstacle(room.walls, 380.0f, 200.0f, 100.0f, 120.0f, SurfaceMaterial::Metal);
                addRectObstacle(room.walls, 800.0f, 200.0f, 100.0f, 120.0f, SurfaceMaterial::Metal);
                room.mechanisms.push_back({
                    finalSwitch,
                    {640.0f, 490.0f},
                    false,
                    0.0f,
                    "SURFACE ASCENT INTERLOCK",
                    finalSwitch
                });
                room.portals.push_back({
                    {640.0f, 280.0f},
                    34.0f,
                    static_cast<int>(i),
                    {640.0f, 400.0f},
                    "SURFACE ASCENT LIFT",
                    finalSwitch,
                    CardinalDir::North,
                    false,
                    true
                });
                break;
            }
        }

        for (int d = 0; d < 4; ++d) {
            const int nb = gn.neighbors[static_cast<std::size_t>(d)];
            if (nb == -1) continue;
            const auto dir = static_cast<CardinalDir>(d);
            const CardinalDir entrySideInTarget = oppositeDir(dir);

            sf::Vector2f portalPos{640.0f, 400.0f};
            std::string dirLabel = "CONDUIT";
            switch (dir) {
                case CardinalDir::North:
                    portalPos = {640.0f, 66.0f};
                    dirLabel = "NORTH DOOR";
                    break;
                case CardinalDir::South:
                    portalPos = {640.0f, 734.0f};
                    dirLabel = "SOUTH DOOR";
                    break;
                case CardinalDir::West:
                    portalPos = {70.0f, 400.0f};
                    dirLabel = "WEST DOOR";
                    break;
                case CardinalDir::East:
                default:
                    portalPos = {1210.0f, 400.0f};
                    dirLabel = "EAST DOOR";
                    break;
            }

            const bool isSecret = (gn.type == RoomType::Secret || nodes[nb].type == RoomType::Secret);
            const int reqSw = doorGateIds[static_cast<std::size_t>(d)];
            std::string portalLabel = dirLabel + " -> " + roomTypeName(nodes[nb].type);
            if (isSecret) {
                portalLabel = "SECRET FRACTURE -> " + std::string(roomTypeName(nodes[nb].type));
            }

            room.portals.push_back({
                portalPos,
                28.0f,
                nb,
                spawnPositionForEntry(entrySideInTarget),
                portalLabel,
                (isSecret || (gn.type == RoomType::Boss && nodes[nb].type == RoomType::FinalExit)) ? reqSw : -1,
                dir,
                isSecret,
                false
            });
        }

        m_rooms[i] = std::move(room);
    }

    markRoomVisited(0);
    markRoomCleared(0);
}

bool Labyrinth::verifyRunGraph(std::string& outError) const {
    if (m_rooms.size() < 15) {
        outError = "Run room count below minimum 15";
        return false;
    }

    for (std::size_t i = 0; i < m_rooms.size(); ++i) {
        const auto& rm = m_rooms[i];
        for (std::size_t j = i + 1; j < m_rooms.size(); ++j) {
            if (rm.gridX == m_rooms[j].gridX && rm.gridY == m_rooms[j].gridY) {
                outError = "Duplicate grid coordinates between rooms " + std::to_string(i) + " and " + std::to_string(j);
                return false;
            }
        }

        for (int d = 0; d < 4; ++d) {
            const int nb = rm.neighbors[static_cast<std::size_t>(d)];
            if (nb == -1) continue;
            if (nb < 0 || static_cast<std::size_t>(nb) >= m_rooms.size()) {
                outError = "Doorway leads to out-of-range neighbor in room " + std::to_string(i);
                return false;
            }
            const auto cd = static_cast<CardinalDir>(d);
            const auto opp = oppositeDir(cd);
            const auto& nbRoom = m_rooms[static_cast<std::size_t>(nb)];
            if (nbRoom.neighbors[static_cast<std::size_t>(opp)] != static_cast<int>(i)) {
                outError = "Asymmetric doorway between rooms " + std::to_string(i) + " and " + std::to_string(nb);
                return false;
            }
            if (nbRoom.gridX != rm.gridX + dirDeltaX(cd) || nbRoom.gridY != rm.gridY + dirDeltaY(cd)) {
                outError = "Neighbor grid coordinate mismatch between rooms " + std::to_string(i) + " and " + std::to_string(nb);
                return false;
            }
        }
    }

    std::vector<bool> visited(m_rooms.size(), false);
    std::queue<int> q;
    visited[0] = true;
    q.push(0);
    std::size_t reachedCount = 0;

    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        ++reachedCount;
        for (int d = 0; d < 4; ++d) {
            const int v = m_rooms[static_cast<std::size_t>(u)].neighbors[static_cast<std::size_t>(d)];
            if (v != -1 && !visited[static_cast<std::size_t>(v)]) {
                visited[static_cast<std::size_t>(v)] = true;
                q.push(v);
            }
        }
    }

    if (reachedCount != m_rooms.size()) {
        outError = "Disconnected rooms detected: reached " + std::to_string(reachedCount) + " of " + std::to_string(m_rooms.size());
        return false;
    }

    if (m_bossRoomId <= 0 || !visited[static_cast<std::size_t>(m_bossRoomId)] ||
        m_rooms[static_cast<std::size_t>(m_bossRoomId)].type != RoomType::Boss) {
        outError = "Boss chamber missing or unreachable";
        return false;
    }

    if (m_exitRoomId <= 0 || !visited[static_cast<std::size_t>(m_exitRoomId)] ||
        m_rooms[static_cast<std::size_t>(m_exitRoomId)].type != RoomType::FinalExit) {
        outError = "FinalExit chamber missing or unreachable";
        return false;
    }

    return true;
}

} // namespace echo
