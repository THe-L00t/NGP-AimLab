#pragma once
// 게임 로직 전체 (대기실 → 카운트다운 → 3라운드 → 최종 결과 → 대기실).
//
// - OpenGL·GLUT·윈도우 호출이 전혀 없다. 나중에 서버 프로젝트가 이 파일을 그대로 쓴다.
// - 판정은 전부 여기서 한다. 클라는 "어느 방향으로 쐈다 / 누르고 있다"만 알려 준다.
// - 표적은 모든 플레이어가 공유한다. 먼저 맞힌 사람이 그 명중의 주인이고,
//   명중할 때마다 팀 공용 점수 +1, 맞힌 사람의 개인 기여도(hits) +1.
#include "GameConfig.h"
#include "GameTypes.h"
#include <glm/glm.hpp>
#include <random>
#include <string>
#include <vector>

class World {
public:
    explicit World(int requiredPlayers);

    // ---- 접속 / 대기실 ----
    bool canJoin() const;                       // 대기실 단계이고 빈 자리가 있는가
    int join(const std::string& nickname);      // 배정된 자리 번호, 실패하면 -1
    void leave(int slot);
    void setReady(int slot, bool ready);

    // ---- 플레이어 입력(명령) ----
    void setAim(int slot, float yawDeg, float pitchDeg);              // 시선 (다른 사람 총 방향 표시 + Tracking 판정)
    void fire(int slot, float yawDeg, float pitchDeg);                // 클릭 한 번 (GridShot·Boss)
    void setTrigger(int slot, bool held, float yawDeg, float pitchDeg); // 누르고 있기 (Tracking)

    // ---- 진행 ----
    void tick(float dt);                        // cfg::kTickInterval 간격으로 호출
    void buildSnapshot(Snapshot& out) const;
    void drainEvents(std::vector<GameEvent>& out);

    Phase phase() const { return phase_; }
    int requiredPlayers() const { return required_; }
    int joinedCount() const;

    // 라운드의 표적(보스)이 나오는 구역의 가운데. 라운드 시작 때 시점을 여기로 맞춘다.
    static glm::vec3 areaCenter(RoundType round);

private:
    struct Player {
        bool joined = false;
        bool ready = false;
        std::string name;
        float yaw = 0.0f;
        float pitch = 0.0f;
        bool trigger = false;
        int hits[cfg::kRoundCount] = {};
    };

    struct Target {
        uint16_t id = 0;
        glm::vec3 pos{ 0.0f };
        glm::vec3 vel{ 0.0f };
        float radius = cfg::kTargetRadius;
        float progress[cfg::kMaxPlayers] = {};  // Tracking: 플레이어별 연속 조준 시간
    };

    struct Boss {
        bool alive = false;
        glm::vec3 pos{ 0.0f };
        glm::vec3 vel{ 0.0f };
        float radius = cfg::kBossRadius;
        int hp = 0;
        int maxHp = 0;
        float teleportTimer = 0.0f;
        bool shield = false;      // 보호막 중
        float shieldHold = 0.0f;  // 보호막 게이지 (모두의 조준 시간 합, 초)
        int shieldsDone = 0;      // 깨뜨린 보호막 수
    };

    struct Bounds {
        glm::vec3 min;
        glm::vec3 max;
    };

    // 단계 전환
    void enterPhase(Phase phase, float duration);
    bool lobbyReadyToStart() const;
    void startMatch();
    void startRound(int index);
    void endRound();
    void resetToLobby();
    bool roundFinished() const;
    bool roundCleared() const;   // 이번 라운드 통과 조건을 채웠나
    RoundType currentRound() const { return kRoundOrder[roundIndex_]; }
    bool validPlayer(int slot) const;

    // 라운드별 생성·갱신
    void spawnGridShot();
    void spawnTracking();
    void spawnBoss();
    Target makeTrackingTarget(int ignoreIndex);
    void updateTracking(float dt);
    void updateBoss(float dt);
    void hitBoss(int slot, const glm::vec3& point);
    void teleportBoss();
    int bossStage(int hp) const;
    void raiseShield();
    void breakShield();
    float shieldRequired() const;   // 보호막을 깨는 데 필요한 조준 시간 합

    // 보조
    void credit(int slot);                       // 명중 기록: 팀 점수 +1, 개인 +1
    int pickTarget(const glm::vec3& origin, const glm::vec3& dir) const;  // 가장 가까운 표적
    glm::vec3 randomFreePosition(const Bounds& b, float minDistance, int ignoreIndex);
    glm::vec3 randomIn(const Bounds& b);
    float randf(float lo, float hi);
    void clearEntities();
    int roundPlayerCount() const;

    static Bounds gridBounds();
    static Bounds trackingBounds();
    static Bounds bossBounds();
    static void bounceInside(glm::vec3& pos, glm::vec3& vel, float radius, const Bounds& b);
    static std::string sanitizeName(const std::string& raw, int slot);

    int required_;
    Phase phase_ = Phase::Lobby;
    float phaseTime_ = 0.0f;
    int roundIndex_ = 0;

    Player players_[cfg::kMaxPlayers];
    std::vector<Target> targets_;
    Boss boss_;
    uint16_t nextTargetId_ = 1;

    int teamScore_ = 0;
    int roundTeamScore_[cfg::kRoundCount] = {};
    int roundGoal_ = 0;          // 이번 라운드 통과 점수 (라운드 시작 때 인원으로 정함)
    bool lastRoundCleared_ = false;
    int failedRound_ = -1;       // 통과 못 한 라운드 (-1 = 없음)

    int trackQuota_ = 0;     // Tracking 라운드 총 처치 목표
    int trackSpawned_ = 0;   // 지금까지 만든 표적 수
    int trackKilled_ = 0;    // 지금까지 처치한 수

    std::mt19937 rng_;
    std::vector<GameEvent> events_;
};
