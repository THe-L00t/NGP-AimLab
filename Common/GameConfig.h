#pragma once
// 게임 규칙 수치 모음.
// OpenGL·윈도우와 무관하다 → 나중에 서버에서도 그대로 include 해서 쓴다.

namespace cfg {

// ---- 인원 ----
constexpr int kMaxPlayers = 3;          // 한 판 최대 인원
constexpr int kNicknameMaxLen = 12;     // 닉네임 최대 글자 수 (영문·숫자·_-)
constexpr int kNicknameBufSize = 16;    // 닉네임 저장 버퍼 (널 문자 포함)

// ---- 시뮬레이션 ----
constexpr float kTickInterval = 1.0f / 30.0f;   // 게임 로직 고정 틱 (30Hz)

// ---- 사격장(방) ----
constexpr float kRoomWidth = 40.0f;     // X
constexpr float kRoomHeight = 20.0f;    // Y
constexpr float kRoomDepth = 40.0f;     // Z
constexpr float kRoomFloorY = -2.0f;    // 바닥 높이
// 플레이어 배치: 맵 중앙을 둘러싼 원 위에 같은 간격으로 선다 (3명 = 정삼각형, 최대 인원을 4로 하면 정사각형).
// 모두 중앙을 바라보고, 표적·보스는 중앙에 생긴다.
// 화면 카메라와 명중 판정 광선이 모두 이 위치에서 출발한다 (aim::slotEyePosition).
constexpr float kArenaRadius = 14.0f;   // 맵 중앙에서 플레이어 눈까지 수평 거리
constexpr float kEyeHeight = 0.0f;      // 눈 높이 (바닥보다 2m 위)
constexpr float kArenaCenterY = 5.0f;   // 표적 구역 가운데 높이 (대기실에서 바라보는 점)

// ---- 카메라 ----
constexpr float kFovYDeg = 55.0f;       // 세로 시야각 (원래 45°). 화면 비율 3:2에서 가로는 약 76°

// ---- 단계별 시간(초) ----
constexpr float kLobbyCountdown = 3.0f;     // 전원 준비 → 게임 시작까지
constexpr float kRoundIntroTime = 3.0f;     // 라운드 시작 전 카운트다운
constexpr float kRoundSummaryTime = 5.0f;   // 라운드 결과 표시
constexpr float kFinalResultTime = 10.0f;   // 최종 결과 표시 → 대기실 복귀

// ---- 라운드 공통 ----
constexpr int kRoundCount = 3;
constexpr float kTargetRadius = 0.8f;

// ---- 1라운드 GridShot: 30초 동안 정지 표적 맞히기 ----
constexpr float kGridDuration = 30.0f;
constexpr int kGridTargetsBase = 3;             // 1명일 때 표적 수 (원본 값)
constexpr int kGridTargetsPerExtraPlayer = 1;   // 인원 1명 늘 때마다 추가 → 3명 = 5개
constexpr int kGridGoalPerPlayer = 20;          // 통과 조건: 30초 안에 팀 명중 ≥ 인원 × 20 (3명 = 60)

// ---- 2라운드 Tracking: 움직이는 표적을 2초간 계속 조준하면 처치 ----
constexpr float kTrackingDuration = 40.0f;
constexpr int kTrackingAliveBase = 6;           // 동시에 떠 있는 표적 수 (1명, 원본 값)
constexpr int kTrackingAlivePerExtraPlayer = 1; // → 3명 = 8개
constexpr int kTrackingKillsPerPlayer = 6;      // 라운드 총 처치 수 = 인원 × 6 → 3명 = 18개
constexpr int kTrackingGoalPerPlayer = 4;       // 통과 조건: 40초 안에 처치 ≥ 인원 × 4 (3명 = 12)
constexpr float kTrackHoldTime = 2.0f;          // 이 시간 동안 계속 조준하면 처치
constexpr float kTrackSpeedMin = 2.0f;
constexpr float kTrackSpeedMax = 5.0f;
constexpr float kTrackTurnRate = 0.3f;          // 초당 방향 전환 확률 (원본: 프레임당 0.5% × 60fps)

// ---- 3라운드 Boss: 순간이동하는 보스, 협동으로 HP 깎기 ----
// 통과 조건: 시간 안에 처치. 보호막 단계가 생겨서 60초에서 75초로 늘림
constexpr float kBossDuration = 75.0f;
constexpr int kBossHpFullParty = 100;           // 3명 기준 HP. 인원이 적으면 비례해서 줄인다
constexpr float kBossRadius = 1.6f;
constexpr float kBossSpeed = 3.0f;
constexpr float kBossTeleportMinDistance = 6.0f;
// HP 비율에 따른 순간이동 주기(초): 75% 초과 / 50% 초과 / 25% 초과 / 그 이하
constexpr float kBossTeleportInterval[4] = { 2.0f, 1.5f, 1.0f, 0.7f };

// 보호막 (트래킹 페이즈): HP가 2/3, 1/3 지점에 닿으면 보호막을 켠다.
// 보호막 중에는 총알이 막히고, 순간이동 없이 빠르게 움직인다.
// 좌클릭을 누른 채 보스를 조준하고 있으면 게이지가 차고, 다 차면 보호막이 깨지며 다음 페이즈로 넘어간다.
// 여럿이 함께 조준하면 그 인원만큼 빨리 찬다.
constexpr int kBossShieldCount = 2;
constexpr float kBossShieldHoldPerPlayer = 3.0f;  // 필요한 조준 시간 = 인원 × 3초 (3명이 같이 따라가면 3초)
constexpr float kBossShieldDecay = 0.5f;          // 아무도 조준하지 않으면 초당 0.5초씩 줄어든다
constexpr float kBossShieldSpeed = 4.5f;          // 보호막 중 이동 속도

// i번째 보호막이 켜지는 HP (i = 0 → 2/3, 1 → 1/3)
constexpr int bossShieldHp(int maxHp, int i) {
    return maxHp * (kBossShieldCount - i) / (kBossShieldCount + 1);
}

} // namespace cfg
