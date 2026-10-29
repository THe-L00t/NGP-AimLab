#pragma once
// 명중 파편 효과. 순수하게 보여 주기용이라 클라이언트에만 있다 (판정과 무관).
// 물리 계산만 하고 그리기는 Renderer가 한다.
#include <glm/glm.hpp>
#include <random>
#include <vector>

class DebrisSystem {
public:
    struct Piece {
        glm::vec3 pos;
        glm::vec3 vel;
        glm::vec3 rot;      // 회전 각도(도)
        glm::vec3 spin;     // 회전 속도(도/초)
        glm::vec3 color;
        float size;
        float life;         // 남은 수명(초)
    };

    DebrisSystem() : rng_(std::random_device{}()) {}

    void targetBurst(const glm::vec3& pos, const glm::vec3& color);   // 원본과 같은 4조각
    void burst(const glm::vec3& pos, const glm::vec3& color, int count, float size, float speed);
    void update(float dt, float floorY);
    void clear() { pieces_.clear(); }
    const std::vector<Piece>& pieces() const { return pieces_; }

private:
    float randf(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng_); }
    void add(const glm::vec3& pos, const glm::vec3& vel, const glm::vec3& color, float size);

    std::vector<Piece> pieces_;
    std::mt19937 rng_;
};
