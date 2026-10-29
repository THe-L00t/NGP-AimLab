#include "Session/LocalSession.h"
#include "AimMath.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
int clampBots(int bots) { return std::clamp(bots, 0, cfg::kMaxPlayers - 1); }
}

LocalSession::LocalSession(int bots)
    : world_(1 + clampBots(bots)) {   // 필요 인원 = 나 + 봇
    const int count = clampBots(bots);
    if (count > 0) {
        // 내가 0번(가운데) 자리에 앉도록: 0번을 잠시 잡아 두고 봇을 1·2번에 앉힌 뒤 0번을 비운다
        const int hold = world_.join("hold");
        for (int i = 0; i < count; ++i) {
            const int slot = world_.join("Bot" + std::to_string(i + 1));
            if (slot >= 0) botSlots_.push_back(slot);
        }
        world_.leave(hold);
    }
    updateBots();
    world_.buildSnapshot(snapshot_);
    message_ = botSlots_.empty() ? "Offline mode" : "Offline mode (test bots: " + std::to_string(botSlots_.size()) + ")";
}

void LocalSession::updateBots() {
    for (size_t i = 0; i < botSlots_.size(); ++i) {
        const int slot = botSlots_[i];
        const float phase = (float)i * 1.7f;
        // 맵 중앙 근처를 천천히 두리번거린다 (총이 시선을 따라 움직이는지 보이게)
        float yaw = 0.0f, pitch = 0.0f;
        aim::lookAngles(aim::slotEyePosition(slot), aim::arenaCenter(), yaw, pitch);
        world_.setAim(slot, yaw + 25.0f * std::sin(botClock_ * 0.6f + phase), pitch + 8.0f * std::sin(botClock_ * 0.9f + phase));
        world_.setReady(slot, true);   // 대기실로 돌아오면 다시 준비 (결과 화면 뒤 준비가 풀리므로)
    }
}

void LocalSession::update(float dt) {
    // 서버와 같은 방식으로 고정 틱(30Hz)으로 진행한다
    accumulator_ += dt;
    bool ticked = false;
    while (accumulator_ >= cfg::kTickInterval) {
        botClock_ += cfg::kTickInterval;
        updateBots();
        world_.tick(cfg::kTickInterval);
        accumulator_ -= cfg::kTickInterval;
        ticked = true;
    }
    if (ticked) world_.buildSnapshot(snapshot_);
}

void LocalSession::retry() {
    state_ = State::Connected;
}

void LocalSession::login(const std::string& nickname) {
    if (slot_ >= 0) return;
    slot_ = world_.join(nickname);
    if (slot_ >= 0) {
        state_ = State::LoggedIn;
    } else {
        state_ = State::Failed;
        message_ = "The room is full or a game is in progress";
    }
    world_.buildSnapshot(snapshot_);
}

void LocalSession::setReady(bool ready) {
    world_.setReady(slot_, ready);
    world_.buildSnapshot(snapshot_);
}

void LocalSession::sendAim(float yawDeg, float pitchDeg) {
    world_.setAim(slot_, yawDeg, pitchDeg);
}

void LocalSession::fire(float yawDeg, float pitchDeg) {
    world_.fire(slot_, yawDeg, pitchDeg);
    world_.buildSnapshot(snapshot_);   // 맞은 표적이 바로 사라지게 즉시 반영
}

void LocalSession::setTrigger(bool held, float yawDeg, float pitchDeg) {
    world_.setTrigger(slot_, held, yawDeg, pitchDeg);
}
