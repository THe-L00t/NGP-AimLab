#pragma once
// 네트워크 없이 같은 프로세스 안에서 World를 돌리는 세션 (혼자 플레이·테스트용).
// bots > 0이면 자리만 차지하는 테스트용 봇을 넣는다 (쏘지 않고 두리번거리기만 함)
// → 네트워크 없이도 다른 플레이어의 총·이름표·대기실 인원 표시를 확인할 수 있다.
#include "Session/Session.h"
#include "World.h"
#include <vector>

class LocalSession : public Session {
public:
    explicit LocalSession(int bots = 0);

    void update(float dt) override;
    State state() const override { return state_; }
    std::string statusMessage() const override { return message_; }
    void retry() override;
    void login(const std::string& nickname) override;

    int localSlot() const override { return slot_; }
    const Snapshot& snapshot() const override { return snapshot_; }
    float snapshotAge() const override { return accumulator_; }
    void drainEvents(std::vector<GameEvent>& out) override { world_.drainEvents(out); }

    void setReady(bool ready) override;
    void sendAim(float yawDeg, float pitchDeg) override;
    void fire(float yawDeg, float pitchDeg) override;
    void setTrigger(bool held, float yawDeg, float pitchDeg) override;

private:
    void updateBots();

    World world_;
    std::vector<int> botSlots_;
    float botClock_ = 0.0f;
    Snapshot snapshot_;
    float accumulator_ = 0.0f;   // 다음 틱까지 쌓인 시간
    int slot_ = -1;
    State state_ = State::Connected;   // 로컬은 접속 과정이 없다
    std::string message_;
};
