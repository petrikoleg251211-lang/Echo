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

sf::Vector2f closestPointOnSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) noexcept {
    const sf::Vector2f ab = b - a;
    const float lenSq = ab.x * ab.x + ab.y * ab.y;
    if (lenSq < 1e-6f) return a;
    const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lenSq, 0.0f, 1.0f);
    return a + ab * t;
}

float distSq(sf::Vector2f a, sf::Vector2f b) noexcept {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return dx * dx + dy * dy;
}

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
            walls.push_back({
                {585.0f, top},
                {695.0f, top},
                doorMaterials[static_cast<std::size_t>(CardinalDir::North)],
                gateId,
                doorInitiallyOpen[static_cast<std::size_t>(CardinalDir::North)]
            });
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
            walls.push_back({
                {585.0f, bottom},
                {695.0f, bottom},
                doorMaterials[static_cast<std::size_t>(CardinalDir::South)],
                gateId,
                doorInitiallyOpen[static_cast<std::size_t>(CardinalDir::South)]
            });
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
            walls.push_back({
                {left, 345.0f},
                {left, 455.0f},
                doorMaterials[static_cast<std::size_t>(CardinalDir::West)],
                gateId,
                doorInitiallyOpen[static_cast<std::size_t>(CardinalDir::West)]
            });
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
            walls.push_back({
                {right, 345.0f},
                {right, 455.0f},
                doorMaterials[static_cast<std::size_t>(CardinalDir::East)],
                gateId,
                doorInitiallyOpen[static_cast<std::size_t>(CardinalDir::East)]
            });
        }
    } else {
        walls.push_back({{right, top}, {right, bottom}, mat, -1, false});
    }
}
} // namespace

Labyrinth::Labyrinth() {
    generateRun(m_runSeed, m_targetRoomCount);
}

const char* Labyrinth::roomTypeName(RoomType type) noexcept {
    switch (type) {
        case RoomType::Start:     return "ENTRY CISTERN";
        case RoomType::Combat:    return "HUNTING CHAMBER";
        case RoomType::Treasure:  return "RELIC VAULT";
        case RoomType::Secret:    return "CONCEALED CAVERN";
        case RoomType::Puzzle:    return "RESONANCE PUZZLE";
        case RoomType::Sanctuary: return "ACOUSTIC SANCTUM";
        case RoomType::Shop:      return "QUARTERMASTER";
        case RoomType::Elite:     return "WARDEN GARRISON";
        case RoomType::Boss:      return "CHOIR SOVEREIGN";
        case RoomType::FinalExit: return "ASCENT CONDUIT";
    }
    return "CHAMBER";
}

char Labyrinth::roomTypeIcon(RoomType type) noexcept {
    switch (type) {
        case RoomType::Start:     return 'S';
        case RoomType::Combat:    return 'C';
        case RoomType::Treasure:  return 'T';
        case RoomType::Secret:    return '?';
        case RoomType::Puzzle:    return 'P';
        case RoomType::Sanctuary: return '+';
        case RoomType::Shop:      return '$';
        case RoomType::Elite:     return '!';
        case RoomType::Boss:      return 'B';
        case RoomType::FinalExit: return '^';
    }
    return '.';
}

sf::Vector2f Labyrinth::spawnPositionForEntry(CardinalDir enteredFromDir) noexcept {
    switch (enteredFromDir) {
        case CardinalDir::North: return {640.0f, 152.0f};
        case CardinalDir::South: return {640.0f, 648.0f};
        case CardinalDir::West:  return {162.0f, 400.0f};
        case CardinalDir::East:  return {1118.0f, 400.0f};
        default:                 return {640.0f, 400.0f};
    }
}

int Labyrinth::findRoomAtGrid(int gx, int gy) const noexcept {
    for (std::size_t i = 0; i < m_rooms.size(); ++i) {
        if (m_rooms[i].gridX == gx && m_rooms[i].gridY == gy) {
            return static_cast<int>(i);
        }
    }
    return -1;
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

void Labyrinth::updateAnimations(float dt) noexcept {
    if (m_rooms.empty()) return;
    auto& room = m_rooms[m_currentRoomIndex];
    for (auto& mech : room.mechanisms) {
        const float target = mech.activated ? 1.0f : 0.0f;
        mech.animProgress += (target - mech.animProgress) * std::min(1.0f, dt * 7.5f);
    }
    for (auto& prop : room.props) {
        if (prop.type == PropType::VentTurbine) {
            prop.rotationDeg = std::fmod(prop.rotationDeg + dt * 65.0f, 360.0f);
        }
    }
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

    // 7. Construct full RoomDefinition geometry, 4-directional doors, props, relics, and enemy waves
    m_rooms.resize(nodes.size());
    constexpr std::size_t kPedestalPoolSize = 8;
    std::size_t relicRotationCursor = static_cast<std::size_t>(m_runSeed % kPedestalPoolSize);

    const SurfaceMaterial materials[5] = {
        SurfaceMaterial::Stone,
        SurfaceMaterial::Concrete,
        SurfaceMaterial::Metal,
        SurfaceMaterial::Water,
        SurfaceMaterial::DampMoss
    };

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto& gn = nodes[i];
        RoomDefinition room;
        room.id = static_cast<int>(i);
        room.gridX = gn.gx;
        room.gridY = gn.gy;
        room.distFromStart = gn.distFromStart;
        room.type = gn.type;
        room.neighbors = gn.neighbors;
        room.playerDefaultSpawn = {640.0f, 400.0f};
        room.isCheckpoint = (gn.type == RoomType::Start || gn.type == RoomType::Sanctuary || gn.type == RoomType::FinalExit);
        room.isExitRoom = (gn.type == RoomType::FinalExit);

        std::ostringstream titleOss;
        titleOss << "CHAMBER " << (i + 1) << " [" << gn.gx << "," << gn.gy << "] - " << roomTypeName(gn.type);
        room.name = titleOss.str();

        if (gn.distFromStart <= 2) room.sectorTitle = "UPPER DRAINAGE RING";
        else if (gn.distFromStart <= 4) room.sectorTitle = "FLOODED AQUEDUCT";
        else if (gn.distFromStart <= 6) room.sectorTitle = "INDUSTRIAL FOUNDRY";
        else room.sectorTitle = "ABYSSAL RESONANCE CORE";

        const SurfaceMaterial primaryMat = materials[(i + m_runSeed) % 5];
        const SurfaceMaterial secondaryMat = materials[(i * 3 + m_runSeed) % 5];
        room.floorMaterial = primaryMat;

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

        addFourWayRoomPerimeter(room.walls, primaryMat, gn.neighbors, doorGateIds, doorMats, doorOpen);

        room.props.push_back({PropType::PipeConduit, {135.0f, 112.0f}, {540.0f, 112.0f}, 1.0f, 0.0f});
        room.props.push_back({PropType::PipeConduit, {740.0f, 688.0f}, {1145.0f, 688.0f}, 1.0f, 0.0f});
        room.props.push_back({PropType::VentTurbine, {195.0f, 155.0f}, {0.0f, 0.0f}, 20.0f, static_cast<float>(i * 29)});
        room.props.push_back({PropType::VentTurbine, {1085.0f, 645.0f}, {0.0f, 0.0f}, 20.0f, static_cast<float>(i * 47)});
        room.props.push_back({PropType::WaterPuddle, {250.0f, 560.0f}, {0.0f, 0.0f}, 34.0f, 0.0f});
        room.props.push_back({PropType::WaterPuddle, {1010.0f, 235.0f}, {0.0f, 0.0f}, 38.0f, 0.0f});
        if (room.isCheckpoint) {
            room.props.push_back({PropType::EmergencyBeacon, {520.0f, 320.0f}, {0.0f, 0.0f}, 1.0f, 0.0f});
        }

        switch (gn.type) {
            case RoomType::Start: {
                room.cleared = true;
                room.visited = true;
                room.discovered = true;
                room.objectiveText = "EXPLORE VIA N/E/S/W DOORS. KEEP NOISE <= 30 FOR GHOST CLEAR (x1.5 SHARDS & SILENT STEP).";
                addRectObstacle(room.walls, 320.0f, 210.0f, 95.0f, 95.0f, SurfaceMaterial::Stone);
                addRectObstacle(room.walls, 865.0f, 210.0f, 95.0f, 95.0f, SurfaceMaterial::Stone);
                addRectObstacle(room.walls, 320.0f, 495.0f, 95.0f, 95.0f, SurfaceMaterial::Stone);
                addRectObstacle(room.walls, 865.0f, 495.0f, 95.0f, 95.0f, SurfaceMaterial::Stone);
                room.collectibles.push_back({
                    200,
                    {640.0f, 280.0f},
                    false,
                    "SURVEYOR BRIEFING - RESONANCE PROTOCOL",
                    "SOUND IS SIGHT, WEAPON, AND RISK. OPEN DOORS LEAK 50% NOISE TO NEIGHBORS; CLEAR CHAMBERS UNDER 30 NOISE FOR GHOST CLEAR."
                });
                break;
            }

            case RoomType::Combat: {
                room.cleared = false;
                room.shardReward = 12 + (gn.distFromStart * 2);
                room.objectiveText = "DISRUPT ALL HUNTERS TO UNSEAL EXITS. KEEP PEAK NOISE <= 30 FOR GHOST CLEAR BONUS!";

                const int layout = static_cast<int>((i + rng.nextU32()) % 3);
                if (layout == 0) {
                    addRectObstacle(room.walls, 340.0f, 210.0f, 110.0f, 110.0f, SurfaceMaterial::DampMoss);
                    addRectObstacle(room.walls, 830.0f, 210.0f, 110.0f, 110.0f, primaryMat);
                    addRectObstacle(room.walls, 340.0f, 480.0f, 110.0f, 110.0f, primaryMat);
                    addRectObstacle(room.walls, 830.0f, 480.0f, 110.0f, 110.0f, SurfaceMaterial::DampMoss);
                    // Block 2.3: Breakable Brittle Glass partition in center-north lane
                    room.walls.push_back({{580.0f, 265.0f}, {700.0f, 265.0f}, SurfaceMaterial::Glass, 700 + static_cast<int>(i), false});
                } else if (layout == 1) {
                    addRectObstacle(room.walls, 460.0f, 230.0f, 90.0f, 110.0f, SurfaceMaterial::Metal);
                    addRectObstacle(room.walls, 730.0f, 460.0f, 90.0f, 110.0f, SurfaceMaterial::DampMoss);
                    room.hazards.push_back({{420.0f, 520.0f}, 42.0f, "BRITTLE SHALE"});
                    room.ambientEmitters.push_back({{820.0f, 250.0f}, 3.6f, 0.5f, true});
                    // Block 2.5: Resonance Pipe Column that hums for 4s when struck by sonar
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

                // Block 4: Deterministic selection across both classic and new acoustic enemy variants
                const EnemyVariant combatPool[7] = {
                    EnemyVariant::Stalker,
                    EnemyVariant::Shrieker,
                    EnemyVariant::SilentStalker,
                    EnemyVariant::DeafBrute,
                    EnemyVariant::Mimic,
                    EnemyVariant::EchoSwarm,
                    EnemyVariant::Phantom
                };
                const EnemyVariant v1 = (gn.distFromStart <= 2)
                    ? combatPool[(i + m_runSeed) % 3]
                    : combatPool[(i + gn.distFromStart + m_runSeed) % 7];
                room.enemyVariant = v1;
                room.enemyPatrolNodes = {
                    {480.0f, 200.0f},
                    {800.0f, 200.0f},
                    {800.0f, 600.0f},
                    {480.0f, 600.0f}
                };
                room.enemySpawns.push_back({v1, room.enemyPatrolNodes, 0});

                if (gn.distFromStart >= 3) {
                    const EnemyVariant v2 = combatPool[(i * 3 + m_runSeed + 2) % 7];
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
                addRectObstacle(room.walls, 370.0f, 220.0f, 90.0f, 90.0f, SurfaceMaterial::Metal);
                addRectObstacle(room.walls, 820.0f, 220.0f, 90.0f, 90.0f, SurfaceMaterial::Metal);
                addRectObstacle(room.walls, 370.0f, 490.0f, 90.0f, 90.0f, SurfaceMaterial::Metal);
                addRectObstacle(room.walls, 820.0f, 490.0f, 90.0f, 90.0f, SurfaceMaterial::Metal);

                const auto rId = static_cast<RelicId>(relicRotationCursor % kPedestalPoolSize);
                ++relicRotationCursor;
                room.relicPedestals.push_back({rId, {640.0f, 400.0f}, 0, false});
                break;
            }

            case RoomType::Secret: {
                room.cleared = true;
                room.shardReward = 25;
                room.objectiveText = "CONCEALED ACOUSTIC CAVERN DISCOVERED! CLAIM THE ANCIENT RELIC & ECHO CACHE [F].";
                const auto rId = static_cast<RelicId>(relicRotationCursor % kPedestalPoolSize);
                ++relicRotationCursor;
                room.relicPedestals.push_back({rId, {640.0f, 360.0f}, 0, false});
                room.collectibles.push_back({
                    200 + static_cast<int>(i),
                    {640.0f, 480.0f},
                    false,
                    "HIDDEN SURVEYOR CACHE",
                    "RESONANT CAVERN UNSEALED BY SONAR WAVES. CONTAINS HIGH-PURITY ECHO SHARDS AND DECOY STONES."
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
                // Block 2.2: Polished Metal mirrors with lossless reflection & +2 bounces
                room.walls.push_back({{340.0f, 190.0f}, {430.0f, 280.0f}, SurfaceMaterial::Metal, -1, false});
                room.walls.push_back({{340.0f, 610.0f}, {430.0f, 520.0f}, SurfaceMaterial::Metal, -1, false});
                room.walls.push_back({{850.0f, 280.0f}, {940.0f, 190.0f}, SurfaceMaterial::Metal, -1, false});
                room.walls.push_back({{850.0f, 520.0f}, {940.0f, 610.0f}, SurfaceMaterial::Metal, -1, false});
                // Block 2.3: Brittle Glass baffle that can be shattered by RadialPulse/Stomp (+40 noise) or bypassed via Metal mirrors
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
                const auto rId = static_cast<RelicId>(relicRotationCursor % kPedestalPoolSize);
                ++relicRotationCursor;
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
                room.totalWaves = 2;
                room.shardReward = 28;
                room.objectiveText = "ELITE GARRISON: DEFEAT THE ARMORED WARDEN / DEAF BRUTE & REINFORCEMENTS TO CLAIM RELIC.";

                addRectObstacle(room.walls, 360.0f, 230.0f, 105.0f, 120.0f, SurfaceMaterial::Metal);
                addRectObstacle(room.walls, 815.0f, 230.0f, 105.0f, 120.0f, SurfaceMaterial::DampMoss);
                addRectObstacle(room.walls, 360.0f, 450.0f, 105.0f, 120.0f, SurfaceMaterial::DampMoss);
                addRectObstacle(room.walls, 815.0f, 450.0f, 105.0f, 120.0f, SurfaceMaterial::Metal);
                room.hazards.push_back({{640.0f, 240.0f}, 44.0f, "BRITTLE SHALE"});
                room.hazards.push_back({{640.0f, 560.0f}, 44.0f, "BRITTLE SHALE"});

                const EnemyVariant elitePrimary = (i % 2 == 0) ? EnemyVariant::DeafBrute : EnemyVariant::Warden;
                const EnemyVariant eliteSecondary = (i % 2 == 0) ? EnemyVariant::SilentStalker : EnemyVariant::EchoSwarm;
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

                const auto rId = static_cast<RelicId>(relicRotationCursor % kPedestalPoolSize);
                ++relicRotationCursor;
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

void Labyrinth::setCurrentRoom(int roomId) noexcept {
    if (roomId >= 0 && static_cast<std::size_t>(roomId) < m_rooms.size()) {
        m_currentRoomIndex = static_cast<std::size_t>(roomId);
        markRoomVisited(roomId);
    }
}

void Labyrinth::setCombatLock(int roomId, bool locked) noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return;
    auto& room = m_rooms[static_cast<std::size_t>(roomId)];
    room.combatLocked = locked;
    const int gateId = 800 + roomId;
    for (auto& w : room.walls) {
        if (w.doorId == gateId) {
            w.isOpen = !locked;
        }
    }
    for (auto& p : room.portals) {
        if (!p.isSecretDoor && !p.isVictoryExit) {
            if (locked) {
                p.requiredSwitchId = gateId;
            } else if (p.requiredSwitchId == gateId) {
                p.requiredSwitchId = -1;
            }
        }
    }
}

void Labyrinth::markRoomCleared(int roomId) noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return;
    auto& room = m_rooms[static_cast<std::size_t>(roomId)];
    room.cleared = true;
    room.combatLocked = false;
    room.noiseIntrusionActive = false;
    room.pendingNoiseAlertLevel = 0.0f;
    if (std::find(m_clearedRooms.begin(), m_clearedRooms.end(), roomId) == m_clearedRooms.end()) {
        m_clearedRooms.push_back(roomId);
    }
    activateSwitch(800 + roomId);
}

bool Labyrinth::isRoomCleared(int roomId) const noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return false;
    return m_rooms[static_cast<std::size_t>(roomId)].cleared;
}

bool Labyrinth::isDoorOpenTowards(int roomId, CardinalDir dir) const noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return false;
    const auto& room = m_rooms[static_cast<std::size_t>(roomId)];
    const int nbId = room.neighbors[static_cast<std::size_t>(dir)];
    if (nbId < 0 || static_cast<std::size_t>(nbId) >= m_rooms.size()) return false;

    const auto& nbRoom = m_rooms[static_cast<std::size_t>(nbId)];
    // Closed combat doors on either side block sound transmission completely
    if (room.combatLocked || nbRoom.combatLocked) {
        return false;
    }

    // Check if the portal in this cardinal direction is sealed by a switch/secret/boss lock
    for (const auto& portal : room.portals) {
        if (portal.isVictoryExit) continue;
        if (portal.direction == dir && portal.targetRoomId == nbId) {
            if (portal.requiredSwitchId != -1 && !isSwitchActivated(portal.requiredSwitchId)) {
                return false;
            }
        }
    }
    return true;
}

std::vector<RoomNoiseLeak> Labyrinth::propagateNoiseFromRoom(
    int sourceRoomId,
    float sourceNoise,
    int maxGridSteps
) const {
    std::vector<RoomNoiseLeak> leaks;
    if (sourceRoomId < 0 || static_cast<std::size_t>(sourceRoomId) >= m_rooms.size() || sourceNoise <= 0.0f || maxGridSteps <= 0) {
        return leaks;
    }

    struct QueueEntry {
        int roomId;
        int dist;
        float noise;
    };

    std::vector<int> bestDist(m_rooms.size(), 9999);
    std::queue<QueueEntry> q;
    bestDist[static_cast<std::size_t>(sourceRoomId)] = 0;
    q.push({sourceRoomId, 0, sourceNoise});

    while (!q.empty()) {
        const QueueEntry cur = q.front();
        q.pop();

        if (cur.dist >= maxGridSteps) continue;

        for (int d = 0; d < 4; ++d) {
            const auto dir = static_cast<CardinalDir>(d);
            if (!isDoorOpenTowards(cur.roomId, dir)) {
                continue; // Sealed/combat-locked doors block cross-room sound
            }
            const int nb = m_rooms[static_cast<std::size_t>(cur.roomId)].neighbors[static_cast<std::size_t>(d)];
            if (nb < 0 || static_cast<std::size_t>(nb) >= m_rooms.size()) continue;

            const int nextDist = cur.dist + 1;
            if (nextDist < bestDist[static_cast<std::size_t>(nb)]) {
                bestDist[static_cast<std::size_t>(nb)] = nextDist;
                const float nextNoise = cur.noise * cfg::noise::kCrossRoomAttenuation;
                leaks.push_back({nb, nextDist, nextNoise, oppositeDir(dir)});
                q.push({nb, nextDist, nextNoise});
            }
        }
    }

    return leaks;
}

bool Labyrinth::reopenRoomForNoiseIntrusion(int roomId) noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return false;
    auto& room = m_rooms[static_cast<std::size_t>(roomId)];
    if (room.noiseIntrusionConsumed || room.noiseIntrusionActive || !room.cleared) {
        return false;
    }
    room.cleared = false;
    room.noiseIntrusionActive = true;
    room.noiseIntrusionConsumed = true;
    m_clearedRooms.erase(
        std::remove(m_clearedRooms.begin(), m_clearedRooms.end(), roomId),
        m_clearedRooms.end()
    );
    setCombatLock(roomId, true);
    return true;
}

int Labyrinth::computeClearShardReward(int baseShards, float roomPeakNoise, bool& outIsGhostClear) noexcept {
    if (baseShards <= 0) {
        outIsGhostClear = false;
        return 0;
    }
    if (roomPeakNoise <= cfg::noise::kThresholdAudible) {
        outIsGhostClear = true;
        return static_cast<int>(std::round(static_cast<float>(baseShards) * cfg::noise::kGhostClearShardMultiplier));
    }
    outIsGhostClear = false;
    return baseShards;
}

void Labyrinth::activateSwitch(int switchId) noexcept {
    if (!isSwitchActivated(switchId)) {
        m_activatedSwitches.push_back(switchId);
    }
    for (auto& room : m_rooms) {
        for (auto& mech : room.mechanisms) {
            if (mech.id == switchId) mech.activated = true;
        }
        for (auto& res : room.resonators) {
            if (res.id == switchId) {
                res.activated = true;
                res.energy = 1.0f;
            }
        }
        for (auto& wall : room.walls) {
            if (wall.doorId == switchId) wall.isOpen = true;
        }
        for (auto& portal : room.portals) {
            if (portal.requiredSwitchId == switchId && !portal.isVictoryExit) {
                portal.requiredSwitchId = -1;
            }
        }
    }
    if (switchId >= 900 && static_cast<std::size_t>(switchId - 900) < m_rooms.size()) {
        m_rooms[static_cast<std::size_t>(switchId - 900)].discovered = true;
    }
}

bool Labyrinth::isSwitchActivated(int switchId) const noexcept {
    return std::find(m_activatedSwitches.begin(), m_activatedSwitches.end(), switchId) != m_activatedSwitches.end();
}

void Labyrinth::markLoreCollected(int loreId) noexcept {
    if (!isLoreCollected(loreId)) {
        m_collectedLoreIds.push_back(loreId);
    }
    for (auto& room : m_rooms) {
        for (auto& lore : room.collectibles) {
            if (lore.id == loreId) lore.collected = true;
        }
    }
}

bool Labyrinth::isLoreCollected(int loreId) const noexcept {
    return std::find(m_collectedLoreIds.begin(), m_collectedLoreIds.end(), loreId) != m_collectedLoreIds.end();
}

void Labyrinth::markRoomVisited(int roomId) noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return;
    if (!isRoomVisited(roomId)) {
        m_visitedRooms.push_back(roomId);
    }
    auto& room = m_rooms[static_cast<std::size_t>(roomId)];
    room.visited = true;
    room.discovered = true;

    for (int d = 0; d < 4; ++d) {
        const int nb = room.neighbors[static_cast<std::size_t>(d)];
        if (nb >= 0 && static_cast<std::size_t>(nb) < m_rooms.size()) {
            if (m_rooms[static_cast<std::size_t>(nb)].type != RoomType::Secret) {
                m_rooms[static_cast<std::size_t>(nb)].discovered = true;
            }
        }
    }
}

bool Labyrinth::isRoomVisited(int roomId) const noexcept {
    return std::find(m_visitedRooms.begin(), m_visitedRooms.end(), roomId) != m_visitedRooms.end();
}

void Labyrinth::revealAllRoomsOnMap() noexcept {
    for (auto& r : m_rooms) r.discovered = true;
}

void Labyrinth::revealSecretRoomsOnMap() noexcept {
    for (auto& r : m_rooms) {
        if (r.type == RoomType::Secret) r.discovered = true;
    }
}

void Labyrinth::restoreState(
    const std::vector<int>& switches,
    const std::vector<int>& loreIds,
    const std::vector<int>& visitedRooms,
    const std::vector<int>& clearedRooms
) {
    m_activatedSwitches.clear();
    m_collectedLoreIds.clear();
    m_visitedRooms.clear();
    m_clearedRooms.clear();

    for (int sw : switches) activateSwitch(sw);
    for (int lore : loreIds) markLoreCollected(lore);
    for (int rm : visitedRooms) {
        if (rm >= 0 && static_cast<std::size_t>(rm) < m_rooms.size()) {
            markRoomVisited(rm);
        }
    }
    for (int cr : clearedRooms) {
        if (cr >= 0 && static_cast<std::size_t>(cr) < m_rooms.size()) {
            markRoomCleared(cr);
        }
    }
    if (m_visitedRooms.empty()) {
        markRoomVisited(0);
    }
}

bool Labyrinth::verifyRoomTraversability(int roomId, std::string& outError) const {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) {
        outError = "Invalid room ID";
        return false;
    }

    const RoomDefinition& room = m_rooms[static_cast<std::size_t>(roomId)];
    constexpr float kStep = 10.0f;
    constexpr float kMinX = 45.0f;
    constexpr float kMaxX = 1235.0f;
    constexpr float kMinY = 42.0f;
    constexpr float kMaxY = 758.0f;
    constexpr int kCols = static_cast<int>((kMaxX - kMinX) / kStep) + 1;
    constexpr int kRows = static_cast<int>((kMaxY - kMinY) / kStep) + 1;
    constexpr float kClearance = cfg::kPlayerRadius + 2.0f;

    auto runBfs = [&](bool gatesOpen) {
        std::vector<bool> reachable(static_cast<std::size_t>(kCols * kRows), false);

        auto isWalkable = [&](float wx, float wy) {
            for (const auto& w : room.walls) {
                if (w.isOpen || (gatesOpen && w.doorId != -1)) continue;
                const sf::Vector2f c = closestPointOnSegment({wx, wy}, w.a, w.b);
                if (distSq({wx, wy}, c) < kClearance * kClearance) {
                    return false;
                }
            }
            return true;
        };

        const int startC = std::clamp(static_cast<int>(std::round((room.playerDefaultSpawn.x - kMinX) / kStep)), 0, kCols - 1);
        const int startR = std::clamp(static_cast<int>(std::round((room.playerDefaultSpawn.y - kMinY) / kStep)), 0, kRows - 1);

        std::queue<std::pair<int, int>> q;
        if (isWalkable(kMinX + static_cast<float>(startC) * kStep, kMinY + static_cast<float>(startR) * kStep)) {
            reachable[static_cast<std::size_t>(startR * kCols + startC)] = true;
            q.push({startC, startR});
        }

        const int dc[4] = {1, -1, 0, 0};
        const int dr[4] = {0, 0, 1, -1};

        while (!q.empty()) {
            const auto [c, r] = q.front();
            q.pop();
            for (int d = 0; d < 4; ++d) {
                const int nc = c + dc[d];
                const int nr = r + dr[d];
                if (nc < 0 || nc >= kCols || nr < 0 || nr >= kRows) continue;
                const std::size_t idx = static_cast<std::size_t>(nr * kCols + nc);
                if (reachable[idx]) continue;
                const float wx = kMinX + static_cast<float>(nc) * kStep;
                const float wy = kMinY + static_cast<float>(nr) * kStep;
                if (isWalkable(wx, wy)) {
                    reachable[idx] = true;
                    q.push({nc, nr});
                }
            }
        }
        return reachable;
    };

    auto isPointReachableWithin = [&](const std::vector<bool>& grid, sf::Vector2f target, float interactRadius) {
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                if (!grid[static_cast<std::size_t>(r * kCols + c)]) continue;
                const sf::Vector2f pt{kMinX + static_cast<float>(c) * kStep, kMinY + static_cast<float>(r) * kStep};
                if (distSq(pt, target) <= interactRadius * interactRadius) {
                    return true;
                }
            }
        }
        return false;
    };

    const auto openGrid = runBfs(true);
    for (const auto& mech : room.mechanisms) {
        if (!isPointReachableWithin(openGrid, mech.position, 48.0f)) {
            outError = "Mechanism unreachable in " + room.name;
            return false;
        }
    }
    for (const auto& lore : room.collectibles) {
        if (!isPointReachableWithin(openGrid, lore.position, 48.0f)) {
            outError = "Lore collectible unreachable in " + room.name;
            return false;
        }
    }
    for (const auto& ped : room.relicPedestals) {
        if (!isPointReachableWithin(openGrid, ped.position, 48.0f)) {
            outError = "Relic pedestal unreachable in " + room.name;
            return false;
        }
    }
    for (const auto& portal : room.portals) {
        if (!isPointReachableWithin(openGrid, portal.position, portal.radius + 6.0f)) {
            outError = "Portal (" + portal.label + ") unreachable in " + room.name;
            return false;
        }
    }

    return true;
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
