#pragma once
// 게임 상태를 주고받는 자료형.
// World(로직)가 Snapshot을 만들고, 클라이언트는 Snapshot만 보고 화면을 그린다.
// 네트워크를 붙일 때는 이 구조체들을 패킷으로 옮기면 된다.
#include "GameConfig.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

// 게임 진행 단계. 로직(나중엔 서버)이 결정하고, 클라는 이 값을 보고 씬을 바꾼다.
enum class Phase : uint8_t {
    Lobby,           // 대기실: 인원 모으기·준비
    LobbyCountdown,  // 전원 준비 → 시작 카운트다운
    RoundIntro,      // 라운드 시작 전 카운트다운
    RoundPlaying,    // 라운드 진행 중
    RoundSummary,    // 라운드 결과 표시
    FinalResult      // 3라운드 종료 후 최종 결과
};

enum class RoundType : uint8_t { GridShot, Tracking, Boss };

constexpr RoundType kRoundOrder[cfg::kRoundCount] = {
    RoundType::GridShot, RoundType::Tracking, RoundType::Boss
};

inline const char* roundName(RoundType r) {
    switch (r) {
    case RoundType::GridShot: return "GRIDSHOT";
    case RoundType::Tracking: return "TRACKING";
    case RoundType::Boss:     return "BOSS";
    }
    return "?";
}

// 화면 효과용 일회성 사건 (파편 튀기기 등)
enum class EventKind : uint8_t {
    TargetHit,     // GridShot 명중
    TargetKilled,  // Tracking 처치
    BossHit,       // pos = 맞은 지점
    BossTeleport,  // pos = 순간이동 직전 위치
    BossKilled,
    BossShieldUp,      // 보호막 켜짐 (트래킹 페이즈 시작)
    BossShieldBroken   // 보호막 깨짐
};
constexpr int kEventKindCount = 7;

struct GameEvent {
    EventKind kind;
    int8_t slot;        // 일으킨 플레이어 (-1 = 없음)
    glm::vec3 pos;
};

struct PlayerInfo {
    bool joined = false;
    bool ready = false;
    char name[cfg::kNicknameBufSize] = {};
    float yaw = 0.0f;
    float pitch = 0.0f;
    uint16_t hits[cfg::kRoundCount] = {};   // 라운드별로 «먼저 맞힌» 횟수 = 개인 기여도

    int totalHits() const {
        int sum = 0;
        for (uint16_t h : hits) sum += h;
        return sum;
    }
};

struct TargetInfo {
    uint16_t id = 0;
    glm::vec3 pos{ 0.0f };
    glm::vec3 vel{ 0.0f };   // 그리는 쪽에서 틱 사이 위치를 보정할 때 쓴다
    float radius = 0.0f;
    uint8_t progress[cfg::kMaxPlayers] = {};   // Tracking: 플레이어별 조준 진행도 (0~255 = 0~kTrackHoldTime)
};

struct BossInfo {
    bool alive = false;
    glm::vec3 pos{ 0.0f };
    glm::vec3 vel{ 0.0f };
    float radius = 0.0f;
    int16_t hp = 0;
    int16_t maxHp = 0;
    bool shield = false;            // 보호막 중 (총알이 막힘, 좌클릭을 누른 채 따라가야 깨짐)
    uint8_t shieldProgress = 0;     // 보호막 게이지 0~255
    uint8_t shieldsLeft = 0;        // 아직 남은 보호막 수
};

struct Snapshot {
    Phase phase = Phase::Lobby;
    uint8_t roundIndex = 0;            // 0~2
    uint8_t requiredPlayers = 1;       // 이 인원이 모두 준비해야 시작
    float phaseTimeLeft = 0.0f;        // 현재 단계의 남은 시간(초)
    uint16_t teamScore = 0;            // 팀 공용 점수 (모든 라운드 합)
    uint16_t roundTeamScore[cfg::kRoundCount] = {};
    uint16_t trackingRemaining = 0;    // Tracking 라운드 남은 처치 수
    uint16_t roundGoal = 0;            // 통과 점수 (GridShot 명중 수 / Tracking 처치 수, Boss는 0 = 처치가 조건)
    bool roundCleared = false;         // 진행 중: 이미 목표를 넘겼나 / 결과 단계: 이 라운드를 통과했나
    int8_t failedRound = -1;           // 이번 판에서 통과하지 못한 라운드 (-1 = 없음)
    PlayerInfo players[cfg::kMaxPlayers];
    BossInfo boss;
    std::vector<TargetInfo> targets;

    RoundType round() const {
        return kRoundOrder[roundIndex < cfg::kRoundCount ? roundIndex : 0];
    }

    int joinedCount() const {
        int n = 0;
        for (const auto& p : players) if (p.joined) ++n;
        return n;
    }

    bool inMatch() const {
        return phase == Phase::RoundIntro || phase == Phase::RoundPlaying ||
               phase == Phase::RoundSummary || phase == Phase::FinalResult;
    }
};
