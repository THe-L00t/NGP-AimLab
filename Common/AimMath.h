#pragma once
// 조준·명중 계산. 클라(그리기)와 로직(판정)이 같은 식을 써야 하므로 공용으로 둔다.
#include "GameConfig.h"
#include <glm/glm.hpp>
#include <cmath>

namespace aim {

// yaw/pitch(도) → 바라보는 방향. yaw 0 = +X, yaw가 커지면 +Z 쪽(오른쪽)으로 돈다.
inline glm::vec3 direction(float yawDeg, float pitchDeg) {
    const float y = glm::radians(yawDeg);
    const float p = glm::radians(pitchDeg);
    return glm::normalize(glm::vec3(std::cos(y) * std::cos(p),
                                    std::sin(p),
                                    std::sin(y) * std::cos(p)));
}

inline float clampPitch(float pitchDeg) {
    if (pitchDeg > 89.0f) return 89.0f;
    if (pitchDeg < -89.0f) return -89.0f;
    return pitchDeg;
}

// 광선-구 교차. dir은 정규화된 방향이어야 한다.
// 맞으면 true, t = 광선 위 거리. 등 뒤(t < 0)에 있는 구는 맞지 않은 것으로 본다.
inline bool raySphere(const glm::vec3& origin, const glm::vec3& dir,
                      const glm::vec3& center, float radius, float& t) {
    const glm::vec3 oc = origin - center;
    const float b = glm::dot(oc, dir);
    const float c = glm::dot(oc, oc) - radius * radius;
    const float disc = b * b - c;
    if (disc < 0.0f) return false;
    const float s = std::sqrt(disc);
    t = -b - s;               // 가까운 교점
    if (t < 0.0f) t = -b + s; // 구 안에서 쏜 경우 먼 교점
    return t >= 0.0f;
}

// 플레이어 자리별 눈 위치: 맵 중앙을 둘러싼 원 위에 같은 간격 (3명 = 정삼각형).
// 0번은 -X 쪽에 서서 +X(yaw 0)를 보면 중앙이고, 나머지는 360°/최대 인원씩 돌아간 자리.
inline glm::vec3 slotEyePosition(int slot) {
    if (slot < 0 || slot >= cfg::kMaxPlayers) slot = 0;
    const float a = glm::radians(180.0f + 360.0f * (float)slot / (float)cfg::kMaxPlayers);
    return glm::vec3(cfg::kArenaRadius * std::cos(a), cfg::kEyeHeight, cfg::kArenaRadius * std::sin(a));
}

// 맵 중앙 (대기실·로그인 화면에서 바라보는 점)
inline glm::vec3 arenaCenter() { return glm::vec3(0.0f, cfg::kArenaCenterY, 0.0f); }

// 눈 위치에서 한 점을 바라보는 yaw/pitch(도)
inline void lookAngles(const glm::vec3& eye, const glm::vec3& target, float& yawDeg, float& pitchDeg) {
    const glm::vec3 d = target - eye;
    yawDeg = glm::degrees(std::atan2(d.z, d.x));
    pitchDeg = glm::degrees(std::atan2(d.y, std::sqrt(d.x * d.x + d.z * d.z)));
}

} // namespace aim
