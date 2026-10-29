#pragma once
// 내 PC에만 해당하는 설정 (조준점 색·모양). 로직·다른 플레이어와 무관하다.
#include <glm/glm.hpp>

enum class CrosshairColor { White, Black, Green };
enum class CrosshairShape { Cross, Circle };

struct Settings {
    CrosshairColor color = CrosshairColor::White;
    CrosshairShape shape = CrosshairShape::Cross;
    float infoTimer = 0.0f;    // > 0이면 화면에 설정 정보 표시 (M·2·3 키)

    void cycleColor() {
        color = color == CrosshairColor::White ? CrosshairColor::Black
              : color == CrosshairColor::Black ? CrosshairColor::Green
              : CrosshairColor::White;
    }
    void cycleShape() {
        shape = shape == CrosshairShape::Cross ? CrosshairShape::Circle : CrosshairShape::Cross;
    }
    glm::vec3 colorVec() const {
        switch (color) {
        case CrosshairColor::Black: return glm::vec3(0.0f);
        case CrosshairColor::Green: return glm::vec3(0.0f, 1.0f, 0.0f);
        default:                    return glm::vec3(1.0f);
        }
    }
    const char* colorName() const {
        return color == CrosshairColor::White ? "White" : color == CrosshairColor::Black ? "Black" : "Green";
    }
    const char* shapeName() const { return shape == CrosshairShape::Cross ? "Cross" : "Circle"; }
};
