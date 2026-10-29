#include "Render/Camera.h"
#include "AimMath.h"
#include <glm/gtc/matrix_transform.hpp>

void Camera::setAngles(float yawDeg, float pitchDeg) {
    yaw_ = yawDeg;
    pitch_ = aim::clampPitch(pitchDeg);
    update();
}

void Camera::addLook(float dxPixels, float dyPixels) {
    yaw_ += dxPixels * sensitivity;
    pitch_ = aim::clampPitch(pitch_ + dyPixels * sensitivity);
    // yaw가 끝없이 커지지 않게 -180~180으로 유지
    while (yaw_ > 180.0f) yaw_ -= 360.0f;
    while (yaw_ < -180.0f) yaw_ += 360.0f;
    update();
}

void Camera::update() {
    front_ = aim::direction(yaw_, pitch_);   // 로직의 조준 방향과 같은 식
    right_ = glm::normalize(glm::cross(front_, glm::vec3(0.0f, 1.0f, 0.0f)));
    up_ = glm::normalize(glm::cross(right_, front_));
}

glm::mat4 Camera::view() const {
    return glm::lookAt(position_, position_ + front_, up_);
}
