#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <string>

namespace echo {

enum class TextAlign : std::uint8_t {
    Left = 0,
    Center = 1,
    Right = 2
};

class UiRenderer {
public:
    UiRenderer();

    // Attempts to load a system or bundled font; if none found, seamlessly uses built-in vector glyphs
    void initialize(const std::string& exeDir = "");

    bool hasSystemFont() const noexcept { return m_fontLoaded; }

    void drawText(
        sf::RenderTarget& target,
        const std::string& text,
        sf::Vector2f position,
        float pixelHeight,
        sf::Color color,
        TextAlign align = TextAlign::Left,
        bool boldGlow = false
    ) const;

    void drawPanel(
        sf::RenderTarget& target,
        sf::Vector2f topLeft,
        sf::Vector2f size,
        sf::Color fillColor,
        sf::Color borderColor,
        float borderThickness = 1.5f
    ) const;

    void drawProgressBar(
        sf::RenderTarget& target,
        sf::Vector2f topLeft,
        sf::Vector2f size,
        float ratio0to1,
        sf::Color backColor,
        sf::Color fillColor,
        sf::Color borderColor
    ) const;

private:
    void drawVectorText(
        sf::RenderTarget& target,
        const std::string& text,
        sf::Vector2f position,
        float pixelHeight,
        sf::Color color,
        TextAlign align,
        bool boldGlow
    ) const;

    sf::Font m_font;
    bool m_fontLoaded{false};
};

} // namespace echo
