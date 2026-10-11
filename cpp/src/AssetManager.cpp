#include "AssetManager.hpp"
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace echo {

namespace {
sf::Vector3f normalize3D(sf::Vector3f v) noexcept {
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq < 1e-6f) return {0.0f, 0.0f, 1.0f};
    const float inv = 1.0f / std::sqrt(lenSq);
    return {v.x * inv, v.y * inv, v.z * inv};
}

sf::Vector3f computeFaceNormal(sf::Vector3f a, sf::Vector3f b, sf::Vector3f c) noexcept {
    const sf::Vector3f u = b - a;
    const sf::Vector3f v = c - a;
    const sf::Vector3f crs{
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x
    };
    return normalize3D(crs);
}

float dot3D(sf::Vector3f a, sf::Vector3f b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

std::uint32_t texHash(unsigned int x, unsigned int y, std::uint32_t seed) noexcept {
    std::uint32_t h = (static_cast<std::uint32_t>(x) * 374761393u + static_cast<std::uint32_t>(y) * 668265263u) ^ seed;
    h = (h ^ (h >> 13u)) * 1274126177u;
    return h ^ (h >> 16u);
}
} // namespace

AssetManager::AssetManager() = default;

void AssetManager::initialize(const std::string& exeDir, bool uploadGpuTextures) {
    if (m_initialized) return;

    struct ModelSpec {
        ModelId id;
        const char* filename;
        const char* label;
    };

    const ModelSpec specs[static_cast<std::size_t>(ModelId::Count)] = {
        {ModelId::SurveyorRig,      "surveyor_rig.obj",      "SurveyorRig"},
        {ModelId::ListenerStalker,  "listener_stalker.obj",  "ListenerStalker"},
        {ModelId::ListenerWarden,   "listener_warden.obj",   "ListenerWarden"},
        {ModelId::ResonatorCrystal, "resonator_crystal.obj", "ResonatorCrystal"},
        {ModelId::HarmonicLever,    "harmonic_lever.obj",    "HarmonicLever"},
        {ModelId::SurveyorTerminal, "surveyor_terminal.obj", "SurveyorTerminal"},
        {ModelId::VentTurbine,      "vent_turbine.obj",      "VentTurbine"}
    };

    m_loadedDiskModelCount = 0;

    for (const auto& spec : specs) {
        auto& mesh = m_meshes[static_cast<std::size_t>(spec.id)];
        mesh.name = spec.label;

        std::vector<std::string> candidateDirs;
        if (!exeDir.empty()) {
            candidateDirs.push_back(exeDir + "/assets/models/" + spec.filename);
            candidateDirs.push_back(exeDir + "/../assets/models/" + spec.filename);
            candidateDirs.push_back(exeDir + "/../../assets/models/" + spec.filename);
        }
        candidateDirs.push_back(std::string("assets/models/") + spec.filename);
        candidateDirs.push_back(std::string("../assets/models/") + spec.filename);
        candidateDirs.push_back(std::string("cpp/assets/models/") + spec.filename);

        bool loaded = false;
        for (const auto& path : candidateDirs) {
            std::error_code ec;
            if (std::filesystem::exists(path, ec) && loadObjFile(path, mesh)) {
                mesh.loadedFromDisk = true;
                loaded = true;
                ++m_loadedDiskModelCount;
                break;
            }
        }

        if (!loaded || mesh.vertices.empty() || mesh.faces.empty()) {
            buildFallbackMesh(spec.id, mesh);
        }
    }

    buildProceduralTextures(uploadGpuTextures);
    m_initialized = true;
}

bool AssetManager::loadObjFile(const std::string& filePath, Mesh3D& outMesh) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    std::vector<sf::Vector3f> tempVerts;
    std::vector<sf::Vector3f> tempNormals;
    std::vector<MeshFace> tempFaces;

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string prefix;
        if (!(iss >> prefix)) continue;

        if (prefix == "v") {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            if (iss >> x >> y >> z) {
                tempVerts.push_back({x, y, z});
            }
        } else if (prefix == "vn") {
            float nx = 0.0f, ny = 0.0f, nz = 1.0f;
            if (iss >> nx >> ny >> nz) {
                tempNormals.push_back(normalize3D({nx, ny, nz}));
            }
        } else if (prefix == "f") {
            std::vector<int> vIndices;
            std::vector<int> nIndices;
            std::string token;
            while (iss >> token) {
                int vIdx = 0;
                int nIdx = 0;
                const auto slash1 = token.find('/');
                if (slash1 == std::string::npos) {
                    vIdx = std::stoi(token);
                } else {
                    vIdx = std::stoi(token.substr(0, slash1));
                    const auto slash2 = token.find('/', slash1 + 1);
                    if (slash2 != std::string::npos && slash2 + 1 < token.size()) {
                        nIdx = std::stoi(token.substr(slash2 + 1));
                    }
                }
                if (vIdx > 0 && static_cast<std::size_t>(vIdx) <= tempVerts.size()) {
                    vIndices.push_back(vIdx - 1);
                    nIndices.push_back(nIdx - 1);
                }
            }
            for (std::size_t i = 1; i + 1 < vIndices.size(); ++i) {
                MeshFace face;
                face.v0 = static_cast<std::uint16_t>(vIndices[0]);
                face.v1 = static_cast<std::uint16_t>(vIndices[i]);
                face.v2 = static_cast<std::uint16_t>(vIndices[i + 1]);
                if (nIndices[0] >= 0 && static_cast<std::size_t>(nIndices[0]) < tempNormals.size()) {
                    face.normal = tempNormals[static_cast<std::size_t>(nIndices[0])];
                } else {
                    face.normal = computeFaceNormal(tempVerts[face.v0], tempVerts[face.v1], tempVerts[face.v2]);
                }
                tempFaces.push_back(face);
            }
        }
    }

    if (tempVerts.empty() || tempFaces.empty()) return false;
    outMesh.vertices = std::move(tempVerts);
    outMesh.faces = std::move(tempFaces);
    return true;
}

void AssetManager::buildFallbackMesh(ModelId id, Mesh3D& outMesh) {
    outMesh.vertices = {
        { 0.0f,  0.0f, 1.2f},
        { 0.8f,  0.0f, 0.2f},
        { 0.0f,  0.7f, 0.2f},
        {-0.8f,  0.0f, 0.2f},
        { 0.0f, -0.7f, 0.2f}
    };
    if (id == ModelId::ListenerWarden) {
        for (auto& v : outMesh.vertices) v *= 1.25f;
    }
    outMesh.faces = {
        {0, 1, 2, normalize3D({ 0.6f,  0.6f, 0.5f})},
        {0, 2, 3, normalize3D({-0.6f,  0.6f, 0.5f})},
        {0, 3, 4, normalize3D({-0.6f, -0.6f, 0.5f})},
        {0, 4, 1, normalize3D({ 0.6f, -0.6f, 0.5f})}
    };
    outMesh.loadedFromDisk = false;
}

void AssetManager::buildProceduralTextures(bool uploadGpuTextures) {
    constexpr unsigned int kTexSize = 64;
    constexpr int kMatCount = static_cast<int>(SurfaceMaterial::Count);
    if (uploadGpuTextures) {
        m_materialTextures.resize(static_cast<std::size_t>(kMatCount));
    }

    for (int matIdx = 0; matIdx < kMatCount; ++matIdx) {
        const auto mat = static_cast<SurfaceMaterial>(matIdx);
        const MaterialProperties props = getMaterialProperties(mat);
        sf::Image& img = m_materialImages[static_cast<std::size_t>(matIdx)];
        img.create(kTexSize, kTexSize, props.fillTint);

        for (unsigned int y = 0; y < kTexSize; ++y) {
            for (unsigned int x = 0; x < kTexSize; ++x) {
                const std::uint32_t h = texHash(x, y, static_cast<std::uint32_t>(matIdx * 997));
                const int grain = static_cast<int>(h & 0x1Fu) - 16;

                int r = props.fillTint.r + grain;
                int g = props.fillTint.g + grain;
                int b = props.fillTint.b + grain;

                if (mat == SurfaceMaterial::Stone) {
                    if (y % 16 == 0 || ((x + (y / 16) * 16) % 32 == 0)) {
                        r -= 10; g -= 12; b -= 14;
                    }
                    if (((x * 3 + y * 5) % 47 == 0) && (h & 3u) == 0u) {
                        r += 18; g += 26; b += 34;
                    }
                } else if (mat == SurfaceMaterial::Concrete) {
                    if (y % 32 == 0 || x % 32 == 0) {
                        r -= 8; g -= 8; b -= 8;
                    }
                    if ((h & 0x3Fu) == 0u) {
                        r += 22; g += 24; b += 28;
                    }
                } else if (mat == SurfaceMaterial::Metal) {
                    const bool border = (x < 2 || x >= 62 || y < 2 || y >= 62);
                    const bool rivet = ((x == 6 || x == 57) && (y == 6 || y == 57));
                    if (border) {
                        r -= 12; g -= 10; b -= 10;
                    } else if (rivet) {
                        r += 45; g += 55; b += 65;
                    } else if ((x % 13 == 0) && (y > 18 && y < 52)) {
                        r += 24; g += 8; b -= 6;
                    }
                } else if (mat == SurfaceMaterial::Water) {
                    const float wave = std::sin(static_cast<float>(x) * 0.28f) * std::cos(static_cast<float>(y) * 0.28f);
                    const int delta = static_cast<int>(wave * 18.0f);
                    r += delta / 2;
                    g += delta;
                    b += delta;
                } else if (mat == SurfaceMaterial::DampMoss) {
                    if (((x * x + y * y + (h & 15u)) % 19) < 7) {
                        r -= 4;
                        g += 28;
                        b += 14;
                    }
                } else if (mat == SurfaceMaterial::ResonanceDoor) {
                    if (((x + y) % 16) < 3 || ((64 - x + y) % 16) < 3) {
                        r += 35; g += 22; b += 52;
                    }
                } else if (mat == SurfaceMaterial::Glass) {
                    if (((x + y * 2) % 21) < 2 || ((64 - x + y) % 29) == 0) {
                        r += 55; g += 60; b += 65;
                    }
                }

                img.setPixel(
                    x,
                    y,
                    sf::Color(
                        static_cast<sf::Uint8>(std::clamp(r, 0, 255)),
                        static_cast<sf::Uint8>(std::clamp(g, 0, 255)),
                        static_cast<sf::Uint8>(std::clamp(b, 0, 255)),
                        255
                    )
                );
            }
        }

        if (uploadGpuTextures) {
            m_materialTextures[static_cast<std::size_t>(matIdx)].loadFromImage(img);
            m_materialTextures[static_cast<std::size_t>(matIdx)].setRepeated(true);
            m_materialTextures[static_cast<std::size_t>(matIdx)].setSmooth(true);
        }
    }
}

const Mesh3D& AssetManager::getMesh(ModelId id) const noexcept {
    return m_meshes[static_cast<std::size_t>(id)];
}

const sf::Image& AssetManager::getMaterialImage(SurfaceMaterial mat) const noexcept {
    const std::size_t idx = std::min<std::size_t>(static_cast<std::size_t>(mat), m_materialImages.size() - 1);
    return m_materialImages[idx];
}

const sf::Texture* AssetManager::getMaterialTexture(SurfaceMaterial mat) const noexcept {
    if (m_materialTextures.empty()) return nullptr;
    const std::size_t idx = std::min<std::size_t>(static_cast<std::size_t>(mat), m_materialTextures.size() - 1);
    return &m_materialTextures[idx];
}

std::size_t AssetManager::totalTriangleCount() const noexcept {
    std::size_t total = 0;
    for (const auto& m : m_meshes) {
        total += m.faces.size();
    }
    return total;
}

void AssetManager::renderMesh25D(
    sf::RenderTarget& target,
    ModelId id,
    sf::Vector2f worldPos,
    float yawRadians,
    float scale,
    sf::Color baseColor,
    sf::Color rimColor,
    float illumination0to1,
    sf::Vector2f lightSourcePos,
    float pitchTiltRadians
) const {
    const Mesh3D& mesh = getMesh(id);
    if (mesh.vertices.empty() || mesh.faces.empty()) return;

    const float cosY = std::cos(yawRadians);
    const float sinY = std::sin(yawRadians);
    const float cosP = std::cos(pitchTiltRadians);
    const float sinP = std::sin(pitchTiltRadians);

    // Transform 3D vertices into world-aligned 2.5D space
    std::vector<sf::Vector3f> transformed(mesh.vertices.size());
    std::vector<sf::Vector2f> projected(mesh.vertices.size());

    for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
        const sf::Vector3f v = mesh.vertices[i];
        // Apply pitch tilt around local Y axis
        const float px = v.x * cosP + v.z * sinP;
        const float py = v.y;
        const float pz = -v.x * sinP + v.z * cosP;

        // Apply yaw rotation around Z axis
        const float rx = px * cosY - py * sinY;
        const float ry = px * sinY + py * cosY;
        const float rz = pz;

        transformed[i] = {rx, ry, rz};
        // Oblique 2.5D top-down projection: height Z elevates Y upward and slightly shifts X
        projected[i] = {
            worldPos.x + rx * scale - rz * scale * 0.18f,
            worldPos.y + ry * scale - rz * scale * 0.52f
        };
    }

    // Light direction in 3D space from lightSourcePos to worldPos
    const sf::Vector2f lDelta = lightSourcePos - worldPos;
    const sf::Vector3f lightDir = normalize3D({lDelta.x, lDelta.y, 140.0f});

    struct SortedFace {
        std::size_t faceIndex;
        float avgZ;
    };
    std::vector<SortedFace> order(mesh.faces.size());
    for (std::size_t i = 0; i < mesh.faces.size(); ++i) {
        const auto& f = mesh.faces[i];
        const float zAvg = (transformed[f.v0].z + transformed[f.v1].z + transformed[f.v2].z) / 3.0f;
        order[i] = {i, zAvg};
    }
    std::sort(order.begin(), order.end(), [](const SortedFace& a, const SortedFace& b) {
        return a.avgZ < b.avgZ;
    });

    sf::VertexArray tris(sf::Triangles);
    sf::VertexArray wire(sf::Lines);

    const float vis = std::clamp(illumination0to1, 0.08f, 1.0f);

    for (const auto& item : order) {
        const auto& face = mesh.faces[item.faceIndex];
        // Rotate normal by yaw
        const sf::Vector3f rn{
            face.normal.x * cosY - face.normal.y * sinY,
            face.normal.x * sinY + face.normal.y * cosY,
            face.normal.z
        };
        const float ndotl = std::clamp(dot3D(rn, lightDir) * 0.5f + 0.5f, 0.20f, 1.0f);
        const float shade = ndotl * (0.45f + 0.55f * vis);

        const sf::Color faceCol(
            static_cast<sf::Uint8>(std::clamp(static_cast<float>(baseColor.r) * shade + static_cast<float>(rimColor.r) * (ndotl * 0.28f), 0.0f, 255.0f)),
            static_cast<sf::Uint8>(std::clamp(static_cast<float>(baseColor.g) * shade + static_cast<float>(rimColor.g) * (ndotl * 0.28f), 0.0f, 255.0f)),
            static_cast<sf::Uint8>(std::clamp(static_cast<float>(baseColor.b) * shade + static_cast<float>(rimColor.b) * (ndotl * 0.28f), 0.0f, 255.0f)),
            static_cast<sf::Uint8>(std::clamp(static_cast<float>(baseColor.a) * vis, 35.0f, 255.0f))
        );

        const sf::Vector2f p0 = projected[face.v0];
        const sf::Vector2f p1 = projected[face.v1];
        const sf::Vector2f p2 = projected[face.v2];

        tris.append(sf::Vertex(p0, faceCol));
        tris.append(sf::Vertex(p1, faceCol));
        tris.append(sf::Vertex(p2, faceCol));

        sf::Color edgeCol = rimColor;
        edgeCol.a = static_cast<sf::Uint8>(std::clamp(vis * (90.0f + ndotl * 130.0f), 25.0f, 235.0f));
        wire.append(sf::Vertex(p0, edgeCol));
        wire.append(sf::Vertex(p1, edgeCol));
        wire.append(sf::Vertex(p1, edgeCol));
        wire.append(sf::Vertex(p2, edgeCol));
        wire.append(sf::Vertex(p2, edgeCol));
        wire.append(sf::Vertex(p0, edgeCol));
    }

    target.draw(tris);
    target.draw(wire);
}

} // namespace echo
