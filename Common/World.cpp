#include "World.h"
#include "AimMath.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
constexpr float kPi = 3.14159265358979f;
}

World::World(int requiredPlayers)
    : required_(std::clamp(requiredPlayers, 1, cfg::kMaxPlayers)),
      rng_(std::random_device{}()) {
    resetToLobby();
}

// ============================================================
// 표적이 생기는 영역: 모두 맵 중앙. 플레이어는 이 구역을 둘러싸고 서서 안쪽을 쏜다.
// (플레이어는 중앙에서 kArenaRadius = 14m 떨어져 있으므로 가장 가까운 모서리도 6m 넘게 떨어진다)
// ============================================================

World::Bounds World::gridBounds() {
    // 중앙의 8 × 8 × 8 상자 (가운데 높이 5)
    return { { -4.0f, 1.0f, -4.0f }, { 4.0f, 9.0f, 4.0f } };
}

World::Bounds World::trackingBounds() {
    // 중앙의 12 × 11 × 12 상자 (움직이는 표적이 튕기며 돌아다님)
    return { { -6.0f, 0.0f, -6.0f }, { 6.0f, 11.0f, 6.0f } };
}

World::Bounds World::bossBounds() {
    return { { -6.0f, 1.0f, -6.0f }, { 6.0f, 11.0f, 6.0f } };
}

glm::vec3 World::areaCenter(RoundType round) {
    const Bounds b = round == RoundType::GridShot ? gridBounds()
                   : round == RoundType::Tracking ? trackingBounds()
                   : bossBounds();
    return (b.min + b.max) * 0.5f;
}

// ============================================================
// 접속 / 대기실
// ============================================================

bool World::validPlayer(int slot) const {
    return slot >= 0 && slot < cfg::kMaxPlayers && players_[slot].joined;
}

int World::joinedCount() const {
    int n = 0;
    for (const auto& p : players_) if (p.joined) ++n;
    return n;
}

bool World::canJoin() const {
    if (phase_ != Phase::Lobby && phase_ != Phase::LobbyCountdown) return false;
    return joinedCount() < cfg::kMaxPlayers;
}

std::string World::sanitizeName(const std::string& raw, int slot) {
    std::string name;
    for (char c : raw) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (ok) name += c;
        if ((int)name.size() >= cfg::kNicknameMaxLen) break;
    }
    if (name.empty()) name = "Player" + std::to_string(slot + 1);
    return name;
}

int World::join(const std::string& nickname) {
    if (!canJoin()) return -1;
    for (int s = 0; s < cfg::kMaxPlayers; ++s) {
        if (players_[s].joined) continue;
        players_[s] = Player{};
        players_[s].joined = true;
        players_[s].name = sanitizeName(nickname, s);
        return s;
    }
    return -1;
}

void World::leave(int slot) {
    if (!validPlayer(slot)) return;
    players_[slot] = Player{};
    for (auto& t : targets_) t.progress[slot] = 0.0f;
}

void World::setReady(int slot, bool ready) {
    if (!validPlayer(slot)) return;
    if (phase_ != Phase::Lobby && phase_ != Phase::LobbyCountdown) return;
    players_[slot].ready = ready;
}

// ============================================================
// 플레이어 입력
// ============================================================

void World::setAim(int slot, float yawDeg, float pitchDeg) {
    if (!validPlayer(slot)) return;
    players_[slot].yaw = yawDeg;
    players_[slot].pitch = aim::clampPitch(pitchDeg);
}

void World::fire(int slot, float yawDeg, float pitchDeg) {
    if (!validPlayer(slot)) return;
    setAim(slot, yawDeg, pitchDeg);
    if (phase_ != Phase::RoundPlaying) return;

    const glm::vec3 origin = aim::slotEyePosition(slot);
    const glm::vec3 dir = aim::direction(players_[slot].yaw, players_[slot].pitch);

    switch (currentRound()) {
    case RoundType::GridShot: {
        const int idx = pickTarget(origin, dir);
        if (idx < 0) return;
        events_.push_back({ EventKind::TargetHit, (int8_t)slot, targets_[idx].pos });
        credit(slot);
        // 같은 자리에 다시 생기지 않게 다른 곳으로 옮긴다
        Target& t = targets_[idx];
        t.id = nextTargetId_++;
        t.pos = randomFreePosition(gridBounds(), t.radius * 3.0f, idx);
        break;
    }
    case RoundType::Boss: {
        float t = 0.0f;
        if (boss_.alive && aim::raySphere(origin, dir, boss_.pos, boss_.radius, t)) {
            hitBoss(slot, origin + dir * t);
        }
        break;
    }
    case RoundType::Tracking:
        break;  // Tracking은 클릭이 아니라 누르고 있기(setTrigger)로 판정
    }
}

void World::setTrigger(int slot, bool held, float yawDeg, float pitchDeg) {
    if (!validPlayer(slot)) return;
    setAim(slot, yawDeg, pitchDeg);
    players_[slot].trigger = held;
    if (!held) {
        // 원본과 같게: 버튼을 떼면 진행도 초기화
        for (auto& t : targets_) t.progress[slot] = 0.0f;
    }
}

// ============================================================
// 진행
// ============================================================

void World::enterPhase(Phase phase, float duration) {
    phase_ = phase;
    phaseTime_ = duration;
}

bool World::lobbyReadyToStart() const {
    const int joined = joinedCount();
    if (joined < required_) return false;
    for (const auto& p : players_) {
        if (p.joined && !p.ready) return false;
    }
    return true;
}

void World::tick(float dt) {
    phaseTime_ -= dt;

    switch (phase_) {
    case Phase::Lobby:
        if (lobbyReadyToStart()) enterPhase(Phase::LobbyCountdown, cfg::kLobbyCountdown);
        break;

    case Phase::LobbyCountdown:
        if (!lobbyReadyToStart()) enterPhase(Phase::Lobby, 0.0f);   // 누가 준비 해제·퇴장 → 취소
        else if (phaseTime_ <= 0.0f) startMatch();
        break;

    case Phase::RoundIntro:
        if (phaseTime_ <= 0.0f) {
            const float duration = currentRound() == RoundType::GridShot ? cfg::kGridDuration
                                 : currentRound() == RoundType::Tracking ? cfg::kTrackingDuration
                                 : cfg::kBossDuration;
            enterPhase(Phase::RoundPlaying, duration);
        }
        break;

    case Phase::RoundPlaying:
        if (currentRound() == RoundType::Tracking) updateTracking(dt);
        else if (currentRound() == RoundType::Boss) updateBoss(dt);
        if (roundFinished()) endRound();
        break;

    case Phase::RoundSummary:
        if (phaseTime_ <= 0.0f) {
            // 통과 못 한 라운드가 있으면 다음 라운드 없이 게임을 끝낸다
            if (failedRound_ >= 0) enterPhase(Phase::FinalResult, cfg::kFinalResultTime);
            else startRound(roundIndex_ + 1);
        }
        break;

    case Phase::FinalResult:
        if (phaseTime_ <= 0.0f) resetToLobby();
        break;
    }

    // 게임 중에 전원이 나가면 대기실로
    if (phase_ != Phase::Lobby && phase_ != Phase::LobbyCountdown && joinedCount() == 0) {
        resetToLobby();
    }
}

void World::startMatch() {
    failedRound_ = -1;
    teamScore_ = 0;
    for (int& s : roundTeamScore_) s = 0;
    for (auto& p : players_) {
        for (int& h : p.hits) h = 0;
    }
    startRound(0);
}

void World::startRound(int index) {
    roundIndex_ = std::clamp(index, 0, cfg::kRoundCount - 1);
    clearEntities();
    for (auto& p : players_) p.trigger = false;

    switch (currentRound()) {
    case RoundType::GridShot: spawnGridShot(); break;
    case RoundType::Tracking: spawnTracking(); break;
    case RoundType::Boss:     spawnBoss(); break;
    }

    // 통과 점수는 라운드 시작 때 인원으로 정한다 (Boss는 처치가 조건이라 0)
    const int players = roundPlayerCount();
    roundGoal_ = currentRound() == RoundType::GridShot ? cfg::kGridGoalPerPlayer * players
               : currentRound() == RoundType::Tracking ? cfg::kTrackingGoalPerPlayer * players
               : 0;
    lastRoundCleared_ = false;
    enterPhase(Phase::RoundIntro, cfg::kRoundIntroTime);
}

void World::endRound() {
    lastRoundCleared_ = roundCleared();   // 엔티티를 지우기 전에 판정 (보스 생존 여부)
    if (!lastRoundCleared_) failedRound_ = roundIndex_;
    clearEntities();
    // 1·2라운드는 통과든 실패든 결과를 5초 보여 준다. 실패면 그 뒤 바로 최종 결과로 간다 (RoundSummary 참고)
    if (roundIndex_ < cfg::kRoundCount - 1) enterPhase(Phase::RoundSummary, cfg::kRoundSummaryTime);
    else enterPhase(Phase::FinalResult, cfg::kFinalResultTime);
}

bool World::roundCleared() const {
    switch (currentRound()) {
    case RoundType::GridShot: return roundTeamScore_[roundIndex_] >= roundGoal_;
    case RoundType::Tracking: return trackKilled_ >= roundGoal_;
    case RoundType::Boss:     return boss_.maxHp > 0 && !boss_.alive;
    }
    return false;
}

void World::resetToLobby() {
    clearEntities();
    for (auto& p : players_) {
        p.ready = false;
        p.trigger = false;
    }
    roundIndex_ = 0;
    enterPhase(Phase::Lobby, 0.0f);
}

bool World::roundFinished() const {
    if (phaseTime_ <= 0.0f) return true;
    switch (currentRound()) {
    case RoundType::Tracking: return trackKilled_ >= trackQuota_;
    case RoundType::Boss:     return !boss_.alive;
    default:                  return false;
    }
}

void World::clearEntities() {
    targets_.clear();
    boss_ = Boss{};
    trackQuota_ = trackSpawned_ = trackKilled_ = 0;
}

int World::roundPlayerCount() const {
    return std::max(1, joinedCount());
}

// ============================================================
// 1라운드 GridShot
// ============================================================

void World::spawnGridShot() {
    const int count = cfg::kGridTargetsBase + (roundPlayerCount() - 1) * cfg::kGridTargetsPerExtraPlayer;
    for (int i = 0; i < count; ++i) {
        Target t;
        t.id = nextTargetId_++;
        t.radius = cfg::kTargetRadius;
        t.pos = randomFreePosition(gridBounds(), t.radius * 3.0f, -1);
        targets_.push_back(t);
    }
}

// ============================================================
// 2라운드 Tracking
// ============================================================

World::Target World::makeTrackingTarget(int ignoreIndex) {
    Bounds b = trackingBounds();
    const float r = cfg::kTargetRadius;
    b.min += glm::vec3(r);
    b.max -= glm::vec3(r);

    Target t;
    t.id = nextTargetId_++;
    t.radius = r;
    t.pos = randomFreePosition(b, r * 3.0f, ignoreIndex);

    const float angle = randf(0.0f, 2.0f * kPi);
    const float speed = randf(cfg::kTrackSpeedMin, cfg::kTrackSpeedMax);
    t.vel = glm::vec3(std::cos(angle) * speed, randf(-1.0f, 1.0f) * speed * 0.3f, std::sin(angle) * speed);
    return t;
}

void World::spawnTracking() {
    const int players = roundPlayerCount();
    trackQuota_ = cfg::kTrackingKillsPerPlayer * players;
    const int alive = cfg::kTrackingAliveBase + (players - 1) * cfg::kTrackingAlivePerExtraPlayer;
    const int count = std::min(alive, trackQuota_);
    for (int i = 0; i < count; ++i) {
        targets_.push_back(makeTrackingTarget(-1));
        ++trackSpawned_;
    }
}

void World::updateTracking(float dt) {
    const Bounds bounds = trackingBounds();

    // 1) 이동 + 가끔 방향 전환 + 벽에 튕기기
    for (auto& t : targets_) {
        t.pos += t.vel * dt;
        if (randf(0.0f, 1.0f) < cfg::kTrackTurnRate * dt) {
            const float angle = randf(0.0f, 2.0f * kPi);
            const float speed = std::sqrt(t.vel.x * t.vel.x + t.vel.z * t.vel.z);
            t.vel.x = std::cos(angle) * speed;
            t.vel.z = std::sin(angle) * speed;
        }
        bounceInside(t.pos, t.vel, t.radius, bounds);
    }

    // 2) 플레이어마다 지금 조준 중인 표적 (버튼을 누르고 있을 때만)
    int aimed[cfg::kMaxPlayers];
    for (int s = 0; s < cfg::kMaxPlayers; ++s) {
        aimed[s] = -1;
        const Player& p = players_[s];
        if (!p.joined || !p.trigger) continue;
        aimed[s] = pickTarget(aim::slotEyePosition(s), aim::direction(p.yaw, p.pitch));
    }

    // 3) 계속 조준하면 진행도 증가, 벗어나면 초기화 (진행도는 사람마다 따로)
    for (size_t i = 0; i < targets_.size(); ++i) {
        for (int s = 0; s < cfg::kMaxPlayers; ++s) {
            if (aimed[s] == (int)i) targets_[i].progress[s] += dt;
            else targets_[i].progress[s] = 0.0f;
        }
    }

    // 4) 먼저 2초를 채운 사람이 처치. 남은 목표가 있으면 새 표적으로 교체
    for (size_t i = 0; i < targets_.size();) {
        int killer = -1;
        float best = 0.0f;
        for (int s = 0; s < cfg::kMaxPlayers; ++s) {
            if (targets_[i].progress[s] >= cfg::kTrackHoldTime && targets_[i].progress[s] > best) {
                best = targets_[i].progress[s];
                killer = s;
            }
        }
        if (killer < 0) {
            ++i;
            continue;
        }

        events_.push_back({ EventKind::TargetKilled, (int8_t)killer, targets_[i].pos });
        credit(killer);
        ++trackKilled_;

        if (trackSpawned_ < trackQuota_) {
            targets_[i] = makeTrackingTarget((int)i);
            ++trackSpawned_;
            ++i;
        } else {
            targets_.erase(targets_.begin() + i);
        }
    }
}

// ============================================================
// 3라운드 Boss
// ============================================================

void World::spawnBoss() {
    const int players = roundPlayerCount();
    boss_ = Boss{};
    boss_.alive = true;
    boss_.radius = cfg::kBossRadius;
    boss_.maxHp = (cfg::kBossHpFullParty * players + cfg::kMaxPlayers - 1) / cfg::kMaxPlayers;  // 올림
    boss_.hp = boss_.maxHp;

    Bounds b = bossBounds();
    b.min += glm::vec3(boss_.radius);
    b.max -= glm::vec3(boss_.radius);
    boss_.pos = randomIn(b);

    const float angle = randf(0.0f, 2.0f * kPi);
    boss_.vel = glm::vec3(std::cos(angle), randf(-0.3f, 0.3f), std::sin(angle)) * cfg::kBossSpeed;
    boss_.teleportTimer = cfg::kBossTeleportInterval[0];
}

int World::bossStage(int hp) const {
    if (boss_.maxHp <= 0) return 0;
    const float f = (float)hp / (float)boss_.maxHp;
    if (f > 0.75f) return 0;
    if (f > 0.50f) return 1;
    if (f > 0.25f) return 2;
    return 3;
}

void World::updateBoss(float dt) {
    if (!boss_.alive) return;
    boss_.pos += boss_.vel * dt;
    bounceInside(boss_.pos, boss_.vel, boss_.radius, bossBounds());

    if (boss_.shield) {
        // 트래킹 페이즈: 좌클릭을 누른 채 보스를 조준하고 있는 사람 수만큼 게이지가 찬다
        int trackers = 0;
        for (int s = 0; s < cfg::kMaxPlayers; ++s) {
            const Player& p = players_[s];
            if (!p.joined || !p.trigger) continue;
            float t = 0.0f;
            if (aim::raySphere(aim::slotEyePosition(s), aim::direction(p.yaw, p.pitch), boss_.pos, boss_.radius, t)) ++trackers;
        }
        if (trackers > 0) boss_.shieldHold += dt * (float)trackers;
        else boss_.shieldHold = std::max(0.0f, boss_.shieldHold - cfg::kBossShieldDecay * dt);
        if (boss_.shieldHold >= shieldRequired()) breakShield();
        return;   // 보호막 중에는 순간이동하지 않는다 (따라갈 수 있게)
    }

    boss_.teleportTimer -= dt;
    if (boss_.teleportTimer <= 0.0f) teleportBoss();
}

float World::shieldRequired() const {
    return cfg::kBossShieldHoldPerPlayer * (float)roundPlayerCount();
}

void World::raiseShield() {
    boss_.shield = true;
    boss_.shieldHold = 0.0f;
    // 순간이동 대신 빠르게 움직이며 도망간다
    const float angle = randf(0.0f, 2.0f * kPi);
    boss_.vel = glm::vec3(std::cos(angle), randf(-0.3f, 0.3f), std::sin(angle)) * cfg::kBossShieldSpeed;
    events_.push_back({ EventKind::BossShieldUp, -1, boss_.pos });
}

void World::breakShield() {
    boss_.shield = false;
    boss_.shieldHold = 0.0f;
    ++boss_.shieldsDone;
    events_.push_back({ EventKind::BossShieldBroken, -1, boss_.pos });
    teleportBoss();   // 다음 페이즈 시작: 바로 순간이동하고 다시 총으로 깎는다
}

void World::teleportBoss() {
    const glm::vec3 from = boss_.pos;
    Bounds b = bossBounds();
    b.min += glm::vec3(boss_.radius);
    b.max -= glm::vec3(boss_.radius);

    glm::vec3 to = randomIn(b);
    for (int i = 0; i < 20 && glm::length(to - from) < cfg::kBossTeleportMinDistance; ++i) {
        to = randomIn(b);
    }
    boss_.pos = to;

    const float angle = randf(0.0f, 2.0f * kPi);
    boss_.vel = glm::vec3(std::cos(angle), randf(-0.3f, 0.3f), std::sin(angle)) * cfg::kBossSpeed;
    boss_.teleportTimer = cfg::kBossTeleportInterval[bossStage(boss_.hp)];

    events_.push_back({ EventKind::BossTeleport, -1, from });
}

void World::hitBoss(int slot, const glm::vec3& point) {
    if (boss_.shield) return;   // 보호막 중에는 총알이 막힌다 (따라가서 깨야 한다)

    const int stageBefore = bossStage(boss_.hp);
    --boss_.hp;
    credit(slot);
    events_.push_back({ EventKind::BossHit, (int8_t)slot, point });

    if (boss_.hp <= 0) {
        boss_.hp = 0;
        boss_.alive = false;
        events_.push_back({ EventKind::BossKilled, (int8_t)slot, boss_.pos });
        return;
    }
    // HP가 2/3, 1/3 지점에 닿으면 보호막 (트래킹 페이즈)
    if (boss_.shieldsDone < cfg::kBossShieldCount && boss_.hp <= cfg::bossShieldHp(boss_.maxHp, boss_.shieldsDone)) {
        raiseShield();
        return;
    }
    // HP 25% 구간을 넘을 때마다 즉시 순간이동하고, 이후 순간이동이 더 잦아진다
    if (bossStage(boss_.hp) != stageBefore) teleportBoss();
}

// ============================================================
// 보조
// ============================================================

void World::credit(int slot) {
    ++teamScore_;
    ++roundTeamScore_[roundIndex_];
    if (slot >= 0 && slot < cfg::kMaxPlayers) ++players_[slot].hits[roundIndex_];
}

int World::pickTarget(const glm::vec3& origin, const glm::vec3& dir) const {
    int best = -1;
    float bestT = 0.0f;
    for (size_t i = 0; i < targets_.size(); ++i) {
        float t = 0.0f;
        if (!aim::raySphere(origin, dir, targets_[i].pos, targets_[i].radius, t)) continue;
        if (best < 0 || t < bestT) {
            best = (int)i;
            bestT = t;
        }
    }
    return best;
}

float World::randf(float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(rng_);
}

glm::vec3 World::randomIn(const Bounds& b) {
    return glm::vec3(randf(b.min.x, b.max.x), randf(b.min.y, b.max.y), randf(b.min.z, b.max.z));
}

glm::vec3 World::randomFreePosition(const Bounds& b, float minDistance, int ignoreIndex) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        const glm::vec3 p = randomIn(b);
        bool ok = true;
        for (size_t i = 0; i < targets_.size() && ok; ++i) {
            if ((int)i == ignoreIndex) continue;
            if (glm::length(p - targets_[i].pos) < minDistance) ok = false;
        }
        if (ok) return p;
    }
    return (b.min + b.max) * 0.5f;  // 자리를 못 찾으면 영역 가운데
}

void World::bounceInside(glm::vec3& pos, glm::vec3& vel, float radius, const Bounds& b) {
    for (int k = 0; k < 3; ++k) {
        if (pos[k] - radius < b.min[k]) {
            pos[k] = b.min[k] + radius;
            vel[k] = std::fabs(vel[k]);
        } else if (pos[k] + radius > b.max[k]) {
            pos[k] = b.max[k] - radius;
            vel[k] = -std::fabs(vel[k]);
        }
    }
}

// ============================================================
// 상태 내보내기
// ============================================================

void World::buildSnapshot(Snapshot& out) const {
    const bool moving = phase_ == Phase::RoundPlaying;  // 진행 중일 때만 움직인다 (그리는 쪽 보정용)

    out.phase = phase_;
    out.roundIndex = (uint8_t)roundIndex_;
    out.requiredPlayers = (uint8_t)required_;
    out.phaseTimeLeft = std::max(0.0f, phaseTime_);
    out.teamScore = (uint16_t)teamScore_;
    for (int r = 0; r < cfg::kRoundCount; ++r) out.roundTeamScore[r] = (uint16_t)roundTeamScore_[r];

    const bool trackingActive = currentRound() == RoundType::Tracking &&
                                (phase_ == Phase::RoundIntro || phase_ == Phase::RoundPlaying);
    out.trackingRemaining = trackingActive ? (uint16_t)std::max(0, trackQuota_ - trackKilled_) : 0;
    out.roundGoal = (uint16_t)roundGoal_;
    out.roundCleared = phase_ == Phase::RoundPlaying ? roundCleared() : lastRoundCleared_;
    out.failedRound = (int8_t)failedRound_;

    for (int s = 0; s < cfg::kMaxPlayers; ++s) {
        const Player& p = players_[s];
        PlayerInfo& info = out.players[s];
        info = PlayerInfo{};
        info.joined = p.joined;
        info.ready = p.ready;
        std::snprintf(info.name, sizeof(info.name), "%s", p.name.c_str());
        info.yaw = p.yaw;
        info.pitch = p.pitch;
        for (int r = 0; r < cfg::kRoundCount; ++r) info.hits[r] = (uint16_t)p.hits[r];
    }

    out.boss = BossInfo{};
    out.boss.alive = boss_.alive;
    out.boss.pos = boss_.pos;
    out.boss.vel = moving ? boss_.vel : glm::vec3(0.0f);
    out.boss.radius = boss_.radius;
    out.boss.hp = (int16_t)boss_.hp;
    out.boss.maxHp = (int16_t)boss_.maxHp;
    out.boss.shield = boss_.shield;
    out.boss.shieldProgress = boss_.shield
        ? (uint8_t)(std::min(1.0f, boss_.shieldHold / shieldRequired()) * 255.0f) : 0;
    out.boss.shieldsLeft = boss_.alive ? (uint8_t)(cfg::kBossShieldCount - boss_.shieldsDone) : 0;

    out.targets.resize(targets_.size());
    for (size_t i = 0; i < targets_.size(); ++i) {
        const Target& t = targets_[i];
        TargetInfo& info = out.targets[i];
        info.id = t.id;
        info.pos = t.pos;
        info.vel = moving ? t.vel : glm::vec3(0.0f);
        info.radius = t.radius;
        for (int s = 0; s < cfg::kMaxPlayers; ++s) {
            const float f = std::min(1.0f, t.progress[s] / cfg::kTrackHoldTime);
            info.progress[s] = (uint8_t)(f * 255.0f);
        }
    }
}

void World::drainEvents(std::vector<GameEvent>& out) {
    out.insert(out.end(), events_.begin(), events_.end());
    events_.clear();
}
