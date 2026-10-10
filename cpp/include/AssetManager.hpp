#pragma once

#include "Config.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace echo {

enum class ModelId : std::uint8_t {
    SurveyorRig = 0,
    ListenerStalker = 1,
    ListenerWarden = 2,
    ResonatorCrystal = 3,
    HarmonicLever = 4,
    SurveyorTerminal = 5,
    VentTurbine = 6,
    Count = 7
};

struct MeshFace {
    std::uint16_t v0{0};
    std::uint16_t v1{0};
    std::uint16_t v2{0};
    sf::Vector3f normal{0.0f, 0.0f, 1.0f};
};

struct Mesh3D {
    std::string name;
    std::vector<sf::Vector3f> vertices;
    std::vector<MeshFace> faces;
    bool loadedFromDisk{false};
};

class AssetManager {
public:
    AssetManager();

    // Loads Wavefront .obj 3D models from assets/models/ (with procedural fallback)
    // and generates procedural 64x64 RGBA material images/textures once at startup.
    void initialize(const std::string& exeDir = "", bool uploadGpuTextures = true);

    bool isInitialized() const noexcept { return m_initialized; }

    const Mesh3D& getMesh(ModelId id) const noexcept;
    const sf::Image& getMaterialImage(SurfaceMaterial mat) const noexcept;
    const sf::Texture* getMaterialTexture(SurfaceMaterial mat) const noexcept;

    std::size_t loadedDiskModelCount() const noexcept { return m_loadedDiskModelCount; }
    std::size_t totalTriangleCount() const noexcept;

    // Projects and renders a 3D mesh in 2.5D oblique top-down perspective with dynamic normal shading
    void renderMesh25D(
        sf::RenderTarget& target,
        ModelId id,
        sf::Vector2f worldPos,
        float yawRadians,
        float scale,
        sf::Color baseColor,
        sf::Color rimColor,
        float illumination0to1,
        sf::Vector2f lightSourcePos,
        float pitchTiltRadians = 0.0f
    ) const;

private:
    bool loadObjFile(const std::string& filePath, Mesh3D& outMesh);
    void buildFallbackMesh(ModelId id, Mesh3D& outMesh);
    void buildProceduralTextures(bool uploadGpuTextures);

    std::array<Mesh3D, static_cast<std::size_t>(ModelId::Count)> m_meshes;
    std::array<sf::Image, 6> m_materialImages;
    std::vector<sf::Texture> m_materialTextures;
    std::size_t m_loadedDiskModelCount{0};
    bool m_initialized{false};
};

} // namespace echo
