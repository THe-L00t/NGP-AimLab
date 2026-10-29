#pragma once
// 1인칭 카메라. yaw/pitch(도)로 시선을 표현한다 — 같은 값을 로직에 조준 방향으로 보낸다.
#include <glm/glm.hpp>

class Camera {
public:
    Camera() { update(); }

    void setPosition(const glm::vec3& p) { position_ = p; }
    void setAngles(float yawDeg, float pitchDeg);
    void addLook(float dxPixels, float dyPixels);   // 마우스 이동량(픽셀). dy는 위쪽이 +

    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
    const glm::vec3& position() const { return position_; }
    const glm::vec3& front() const { return front_; }
    const glm::vec3& right() const { return right_; }
    const glm::vec3& up() const { return up_; }
    glm::mat4 view() const;

    float sensitivity = 0.03f;   // 원본 값 (도/픽셀)

private:
    void update();

    glm::vec3 position_{ 0.0f };
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    glm::vec3 front_{ 1.0f, 0.0f, 0.0f };
    glm::vec3 right_{ 0.0f, 0.0f, 1.0f };
    glm::vec3 up_{ 0.0f, 1.0f, 0.0f };
};
