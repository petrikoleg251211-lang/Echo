#include "UiRenderer.hpp"
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <vector>

namespace echo {

namespace {

struct StrokeSeg {
    float x1, y1, x2, y2;
};

// Clean 14-segment geometric vector glyph table in normalized [0..1]x[0..1] glyph box
void appendGlyphStrokes(char ch, std::vector<StrokeSeg>& out) {
    const char u = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    switch (u) {
        case 'A':
            out.push_back({0.0f, 1.0f, 0.5f, 0.0f});
            out.push_back({0.5f, 0.0f, 1.0f, 1.0f});
            out.push_back({0.2f, 0.65f, 0.8f, 0.65f});
            break;
        case 'B':
            out.push_back({0.0f, 0.0f, 0.0f, 1.0f});
            out.push_back({0.0f, 0.0f, 0.75f, 0.0f});
            out.push_back({0.75f, 0.0f, 0.95f, 0.25f});
            out.push_back({0.95f, 0.25f, 0.75f, 0.5f});
            out.push_back({0.0f, 0.5f, 0.75f, 0.5f});
            out.push_back({0.75f, 0.5f, 1.0f, 0.75f});
            out.push_back({1.0f, 0.75f, 0.75f, 1.0f});
            out.push_back({0.75f, 1.0f, 0.0f, 1.0f});
            break;
        case 'C':
            out.push_back({0.95f, 0.15f, 0.7f, 0.0f});
            out.push_back({0.7f, 0.0f, 0.2f, 0.0f});
            out.push_back({0.2f, 0.0f, 0.0f, 0.25f});
            out.push_back({0.0f, 0.25f, 0.0f, 0.75f});
            out.push_back({0.0f, 0.75f, 0.2f, 1.0f});
            out.push_back({0.2f, 1.0f, 0.7f, 1.0f});
            out.push_back({0.7f, 1.0f, 0.95f, 0.85f});
            break;
        case 'D':
            out.push_back({0.0f, 0.0f, 0.0f, 1.0f});
            out.push_back({0.0f, 0.0f, 0.65f, 0.0f});
            out.push_back({0.65f, 0.0f, 1.0f, 0.3f});
            out.push_back({1.0f, 0.3f, 1.0f, 0.7f});
            out.push_back({1.0f, 0.7f, 0.65f, 1.0f});
            out.push_back({0.65f, 1.0f, 0.0f, 1.0f});
            break;
        case 'E':
            out.push_back({0.0f, 0.0f, 0.0f, 1.0f});
            out.push_back({0.0f, 0.0f, 0.95f, 0.0f});
            out.push_back({0.0f, 0.5f, 0.75f, 0.5f});
            out.push_back({0.0f, 1.0f, 0.95f, 1.0f});
            break;
        case 'F':
            out.push_back({0.0f, 0.0f, 0.0f, 1.0f});
            out.push_back({0.0f, 0.0f, 0.95f, 0.0f});
            out.push_back({0.0f, 0.5f, 0.75f, 0.5f});
            break;
        case 'G':
            out.push_back({0.95f, 0.15f, 0.7f, 0.0f});
            out.push_back({0.7f, 0.0f, 0.2f, 0.0f});
            out.push_back({0.2f, 0.0f, 0.0f, 0.25f});
            out.push_back({0.0f, 0.25f, 0.0f, 0.75f});
            out.push_back({0.0f, 0.75f, 0.2f, 1.0f});
            out.push_back({0.2f, 1.0f, 0.85f, 1.0f});
            out.push_back({0.85f, 1.0f, 0.95f, 0.55f});
            out.push_back({0.95f, 0.55f, 0.5f, 0.55f});
            break;
        case 'H':
            out.push_back({0.0f, 0.0f, 0.0f, 1.0f});
            out.push_back({1.0f, 0.0f, 1.0f, 1.0f});
            out.push_back({0.0f, 0.5f, 1.0f, 0.5f});
            break;
        case 'I':
            out.push_back({0.2f, 0.0f, 0.8f, 0.0f});
            out.push_back({0.5f, 0.0f, 0.5f, 1.0f});
            out.push_back({0.2f, 1.0f, 0.8f, 1.0f});
            break;
        case 'J':
            out.push_back({0.85f, 0.0f, 0.85f, 0.8f});
            out.push_back({0.85f, 0.8f, 0.55f, 1.0f});
            out.push_back({0.55f, 1.0f, 0.2f, 1.0f});
            out.push_back({0.2f, 1.0f, 0.0f, 0.75f});
            break;
        case 'K':
            out.push_back({0.0f, 0.0f, 0.0f, 1.0f});
            out.push_back({0.95f, 0.0f, 0.0f, 0.55f});
            out.push_back({0.25f, 0.42f, 1.0f, 1.0f});
            break;
        case 'L':
            out.push_back({0.0f, 0.0f, 0.0f, 1.0f});
            out.push_back({0.0f, 1.0f, 0.95f, 1.0f});
            break;
        case 'M':
            out.push_back({0.0f, 1.0f, 0.0f, 0.0f});
            out.push_back({0.0f, 0.0f, 0.5f, 0.55f});
            out.push_back({0.5f, 0.55f, 1.0f, 0.0f});
            out.push_back({1.0f, 0.0f, 1.0f, 1.0f});
            break;
        case 'N':
            out.push_back({0.0f, 1.0f, 0.0f, 0.0f});
            out.push_back({0.0f, 0.0f, 1.0f, 1.0f});
            out.push_back({1.0f, 1.0f, 1.0f, 0.0f});
            break;
        case 'O':
            out.push_back({0.2f, 0.0f, 0.8f, 0.0f});
            out.push_back({0.8f, 0.0f, 1.0f, 0.25f});
            out.push_back({1.0f, 0.25f, 1.0f, 0.75f});
            out.push_back({1.0f, 0.75f, 0.8f, 1.0f});
            out.push_back({0.8f, 1.0f, 0.2f, 1.0f});
            out.push_back({0.2f, 1.0f, 0.0f, 0.75f});
            out.push_back({0.0f, 0.75f, 0.0f, 0.25f});
            out.push_back({0.0f, 0.25f, 0.2f, 0.0f});
            break;
        case 'P':
            out.push_back({0.0f, 1.0f, 0.0f, 0.0f});
            out.push_back({0.0f, 0.0f, 0.8f, 0.0f});
            out.push_back({0.8f, 0.0f, 1.0f, 0.25f});
            out.push_back({1.0f, 0.25f, 0.8f, 0.5f});
            out.push_back({0.8f, 0.5f, 0.0f, 0.5f});
            break;
        case 'Q':
            appendGlyphStrokes('O', out);
            out.push_back({0.55f, 0.65f, 1.0f, 1.05f});
            break;
        case 'R':
            appendGlyphStrokes('P', out);
            out.push_back({0.45f, 0.5f, 1.0f, 1.0f});
            break;
        case 'S':
            out.push_back({0.95f, 0.15f, 0.75f, 0.0f});
            out.push_back({0.75f, 0.0f, 0.2f, 0.0f});
            out.push_back({0.2f, 0.0f, 0.0f, 0.22f});
            out.push_back({0.0f, 0.22f, 0.15f, 0.45f});
            out.push_back({0.15f, 0.45f, 0.85f, 0.55f});
            out.push_back({0.85f, 0.55f, 1.0f, 0.78f});
            out.push_back({1.0f, 0.78f, 0.8f, 1.0f});
            out.push_back({0.8f, 1.0f, 0.2f, 1.0f});
            out.push_back({0.2f, 1.0f, 0.0f, 0.85f});
            break;
        case 'T':
            out.push_back({0.0f, 0.0f, 1.0f, 0.0f});
            out.push_back({0.5f, 0.0f, 0.5f, 1.0f});
            break;
        case 'U':
            out.push_back({0.0f, 0.0f, 0.0f, 0.8f});
            out.push_back({0.0f, 0.8f, 0.2f, 1.0f});
            out.push_back({0.2f, 1.0f, 0.8f, 1.0f});
            out.push_back({0.8f, 1.0f, 1.0f, 0.8f});
            out.push_back({1.0f, 0.8f, 1.0f, 0.0f});
            break;
        case 'V':
            out.push_back({0.0f, 0.0f, 0.5f, 1.0f});
            out.push_back({0.5f, 1.0f, 1.0f, 0.0f});
            break;
        case 'W':
            out.push_back({0.0f, 0.0f, 0.22f, 1.0f});
            out.push_back({0.22f, 1.0f, 0.5f, 0.45f});
            out.push_back({0.5f, 0.45f, 0.78f, 1.0f});
            out.push_back({0.78f, 1.0f, 1.0f, 0.0f});
            break;
        case 'X':
            out.push_back({0.0f, 0.0f, 1.0f, 1.0f});
            out.push_back({1.0f, 0.0f, 0.0f, 1.0f});
            break;
        case 'Y':
            out.push_back({0.0f, 0.0f, 0.5f, 0.5f});
            out.push_back({1.0f, 0.0f, 0.5f, 0.5f});
            out.push_back({0.5f, 0.5f, 0.5f, 1.0f});
            break;
        case 'Z':
            out.push_back({0.0f, 0.0f, 1.0f, 0.0f});
            out.push_back({1.0f, 0.0f, 0.0f, 1.0f});
            out.push_back({0.0f, 1.0f, 1.0f, 1.0f});
            break;
        case '0':
            appendGlyphStrokes('O', out);
            out.push_back({0.15f, 0.82f, 0.85f, 0.18f});
            break;
        case '1':
            out.push_back({0.25f, 0.2f, 0.55f, 0.0f});
            out.push_back({0.55f, 0.0f, 0.55f, 1.0f});
            out.push_back({0.2f, 1.0f, 0.9f, 1.0f});
            break;
        case '2':
            out.push_back({0.05f, 0.2f, 0.25f, 0.0f});
            out.push_back({0.25f, 0.0f, 0.8f, 0.0f});
            out.push_back({0.8f, 0.0f, 1.0f, 0.25f});
            out.push_back({1.0f, 0.25f, 0.0f, 1.0f});
            out.push_back({0.0f, 1.0f, 1.0f, 1.0f});
            break;
        case '3':
            out.push_back({0.05f, 0.0f, 0.85f, 0.0f});
            out.push_back({0.85f, 0.0f, 1.0f, 0.25f});
            out.push_back({1.0f, 0.25f, 0.75f, 0.5f});
            out.push_back({0.35f, 0.5f, 0.75f, 0.5f});
            out.push_back({0.75f, 0.5f, 1.0f, 0.75f});
            out.push_back({1.0f, 0.75f, 0.8f, 1.0f});
            out.push_back({0.8f, 1.0f, 0.05f, 1.0f});
            break;
        case '4':
            out.push_back({0.75f, 1.0f, 0.75f, 0.0f});
            out.push_back({0.75f, 0.0f, 0.0f, 0.65f});
            out.push_back({0.0f, 0.65f, 1.0f, 0.65f});
            break;
        case '5':
            out.push_back({0.95f, 0.0f, 0.05f, 0.0f});
            out.push_back({0.05f, 0.0f, 0.05f, 0.48f});
            out.push_back({0.05f, 0.48f, 0.8f, 0.48f});
            out.push_back({0.8f, 0.48f, 1.0f, 0.72f});
            out.push_back({1.0f, 0.72f, 0.8f, 1.0f});
            out.push_back({0.8f, 1.0f, 0.05f, 1.0f});
            break;
        case '6':
            out.push_back({0.85f, 0.0f, 0.2f, 0.0f});
            out.push_back({0.2f, 0.0f, 0.0f, 0.3f});
            out.push_back({0.0f, 0.3f, 0.0f, 0.8f});
            out.push_back({0.0f, 0.8f, 0.2f, 1.0f});
            out.push_back({0.2f, 1.0f, 0.8f, 1.0f});
            out.push_back({0.8f, 1.0f, 1.0f, 0.75f});
            out.push_back({1.0f, 0.75f, 0.8f, 0.5f});
            out.push_back({0.8f, 0.5f, 0.0f, 0.5f});
            break;
        case '7':
            out.push_back({0.0f, 0.0f, 1.0f, 0.0f});
            out.push_back({1.0f, 0.0f, 0.35f, 1.0f});
            break;
        case '8':
            appendGlyphStrokes('O', out);
            out.push_back({0.1f, 0.5f, 0.9f, 0.5f});
            break;
        case '9':
            out.push_back({1.0f, 0.5f, 0.2f, 0.5f});
            out.push_back({0.2f, 0.5f, 0.0f, 0.25f});
            out.push_back({0.0f, 0.25f, 0.2f, 0.0f});
            out.push_back({0.2f, 0.0f, 0.8f, 0.0f});
            out.push_back({0.8f, 0.0f, 1.0f, 0.25f});
            out.push_back({1.0f, 0.25f, 1.0f, 0.8f});
            out.push_back({1.0f, 0.8f, 0.75f, 1.0f});
            out.push_back({0.75f, 1.0f, 0.15f, 1.0f});
            break;
        case '-':
            out.push_back({0.15f, 0.5f, 0.85f, 0.5f});
            break;
        case '+':
            out.push_back({0.15f, 0.5f, 0.85f, 0.5f});
            out.push_back({0.5f, 0.2f, 0.5f, 0.8f});
            break;
        case ':':
            out.push_back({0.45f, 0.25f, 0.55f, 0.35f});
            out.push_back({0.45f, 0.75f, 0.55f, 0.85f});
            break;
        case '.':
            out.push_back({0.4f, 0.9f, 0.6f, 1.0f});
            break;
        case ',':
            out.push_back({0.5f, 0.85f, 0.35f, 1.1f});
            break;
        case '/':
            out.push_back({0.15f, 1.0f, 0.85f, 0.0f});
            break;
        case '[':
        case '(':
            out.push_back({0.7f, 0.0f, 0.3f, 0.0f});
            out.push_back({0.3f, 0.0f, 0.3f, 1.0f});
            out.push_back({0.3f, 1.0f, 0.7f, 1.0f});
            break;
        case ']':
        case ')':
            out.push_back({0.3f, 0.0f, 0.7f, 0.0f});
            out.push_back({0.7f, 0.0f, 0.7f, 1.0f});
            out.push_back({0.7f, 1.0f, 0.3f, 1.0f});
            break;
        case '%':
            out.push_back({0.1f, 0.95f, 0.9f, 0.05f});
            out.push_back({0.15f, 0.1f, 0.35f, 0.3f});
            out.push_back({0.65f, 0.7f, 0.85f, 0.9f});
            break;
        case '#':
            out.push_back({0.3f, 0.05f, 0.3f, 0.95f});
            out.push_back({0.7f, 0.05f, 0.7f, 0.95f});
            out.push_back({0.1f, 0.35f, 0.9f, 0.35f});
            out.push_back({0.1f, 0.65f, 0.9f, 0.65f});
            break;
        case '>':
            out.push_back({0.2f, 0.15f, 0.8f, 0.5f});
            out.push_back({0.8f, 0.5f, 0.2f, 0.85f});
            break;
        case '<':
            out.push_back({0.8f, 0.15f, 0.2f, 0.5f});
            out.push_back({0.2f, 0.5f, 0.8f, 0.85f});
            break;
        case '|':
            out.push_back({0.5f, 0.0f, 0.5f, 1.0f});
            break;
        default:
            break;
    }
}

} // namespace

UiRenderer::UiRenderer() = default;

void UiRenderer::initialize(const std::string& exeDir) {
    std::vector<std::string> candidatePaths;
    if (!exeDir.empty()) {
        candidatePaths.push_back(exeDir + "/assets/font.ttf");
        candidatePaths.push_back(exeDir + "/font.ttf");
    }
    candidatePaths.push_back("assets/font.ttf");
    candidatePaths.push_back("../assets/font.ttf");
    // Windows 10/11 standard fonts
    candidatePaths.push_back("C:/Windows/Fonts/consola.ttf");
    candidatePaths.push_back("C:/Windows/Fonts/segoeui.ttf");
    candidatePaths.push_back("C:/Windows/Fonts/arial.ttf");
    // Linux fallback fonts
    candidatePaths.push_back("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf");
    candidatePaths.push_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    candidatePaths.push_back("/usr/share/fonts/TTF/DejaVuSans.ttf");

    for (const auto& path : candidatePaths) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec) && m_font.loadFromFile(path)) {
            m_fontLoaded = true;
            return;
        }
    }
}

void UiRenderer::drawText(
    sf::RenderTarget& target,
    const std::string& text,
    sf::Vector2f position,
    float pixelHeight,
    sf::Color color,
    TextAlign align,
    bool boldGlow
) const {
    if (text.empty()) return;

    if (m_fontLoaded) {
        sf::Text sfText;
        sfText.setFont(m_font);
        sfText.setString(text);
        sfText.setCharacterSize(static_cast<unsigned int>(std::max(10.0f, pixelHeight)));
        sfText.setFillColor(color);

        const sf::FloatRect bounds = sfText.getLocalBounds();
        float xOffset = 0.0f;
        if (align == TextAlign::Center) {
            xOffset = -bounds.width * 0.5f;
        } else if (align == TextAlign::Right) {
            xOffset = -bounds.width;
        }
        sfText.setPosition(
             std::round(position.x + xOffset - bounds.left),
             std::round(position.y - bounds.top)
        );

        if (boldGlow) {
            sf::Text glow = sfText;
            sf::Color glowCol = color;
            glowCol.a = static_cast<sf::Uint8>(color.a / 3);
            glow.setFillColor(glowCol);
            glow.move(1.0f, 1.0f);
            target.draw(glow, sf::RenderStates(sf::BlendAdd));
        }

        target.draw(sfText);
        return;
    }

    drawVectorText(target, text, position, pixelHeight, color, align, boldGlow);
}

void UiRenderer::drawVectorText(
    sf::RenderTarget& target,
    const std::string& text,
    sf::Vector2f position,
    float pixelHeight,
    sf::Color color,
    TextAlign align,
    bool boldGlow
) const {
    const float charW = pixelHeight * 0.62f;
    const float spacing = pixelHeight * 0.24f;
    const float advance = charW + spacing;
    const float totalW = text.empty() ? 0.0f : (static_cast<float>(text.size()) * advance - spacing);

    float startX = position.x;
    if (align == TextAlign::Center) startX -= totalW * 0.5f;
    else if (align == TextAlign::Right) startX -= totalW;

    sf::VertexArray lines(sf::Lines);
    std::vector<StrokeSeg> segs;
    segs.reserve(12);

    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == ' ') continue;
        segs.clear();
        appendGlyphStrokes(c, segs);
        const float gx = startX + static_cast<float>(i) * advance;
        const float gy = position.y;

        for (const auto& s : segs) {
            const sf::Vector2f p1{gx + s.x1 * charW, gy + s.y1 * pixelHeight};
            const sf::Vector2f p2{gx + s.x2 * charW, gy + s.y2 * pixelHeight};
            lines.append(sf::Vertex(p1, color));
            lines.append(sf::Vertex(p2, color));
            if (boldGlow) {
                sf::Color glowCol = color;
                glowCol.a = static_cast<sf::Uint8>(color.a / 2);
                lines.append(sf::Vertex(p1 + sf::Vector2f{1.0f, 0.0f}, glowCol));
                lines.append(sf::Vertex(p2 + sf::Vector2f{1.0f, 0.0f}, glowCol));
            }
        }
    }

    target.draw(lines);
}

void UiRenderer::drawPanel(
    sf::RenderTarget& target,
    sf::Vector2f topLeft,
    sf::Vector2f size,
    sf::Color fillColor,
    sf::Color borderColor,
    float borderThickness
) const {
    sf::RectangleShape rect(size);
    rect.setPosition(topLeft);
    rect.setFillColor(fillColor);
    rect.setOutlineThickness(borderThickness);
    rect.setOutlineColor(borderColor);
    target.draw(rect);
}

void UiRenderer::drawProgressBar(
    sf::RenderTarget& target,
    sf::Vector2f topLeft,
    sf::Vector2f size,
    float ratio0to1,
    sf::Color backColor,
    sf::Color fillColor,
    sf::Color borderColor
) const {
    drawPanel(target, topLeft, size, backColor, borderColor, 1.0f);
    const float clamped = std::clamp(ratio0to1, 0.0f, 1.0f);
    if (clamped > 0.005f) {
        sf::RectangleShape bar({(size.x - 4.0f) * clamped, size.y - 4.0f});
        bar.setPosition({topLeft.x + 2.0f, topLeft.y + 2.0f});
        bar.setFillColor(fillColor);
        target.draw(bar);
    }
}

} // namespace echo
