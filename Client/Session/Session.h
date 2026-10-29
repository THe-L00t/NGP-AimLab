#pragma once
// 클라이언트 ↔ 게임 로직 사이의 창구.
//
// 씬들은 이 인터페이스만 쓴다. 지금은 같은 프로세스 안에서 World를 돌리는 LocalSession뿐이다.
// ▶ 네트워크를 붙일 때: 이 인터페이스를 구현하는 NetSession을 만들면 된다.
//    - login/setReady/sendAim/fire/setTrigger → 패킷으로 서버에 보내기
//    - 서버가 보낸 상태 → snapshot(), 사건 → drainEvents()
//    - 연결 과정 → state()가 Connecting → Connected → LoggingIn → LoggedIn
//   씬 코드는 바꿀 필요가 없다.
#include "GameTypes.h"
#include <string>
#include <vector>

class Session {
public:
    enum class State {
        Connecting,   // 서버 접속 중
        Connected,    // 접속됨 → 닉네임 입력 대기
        LoggingIn,    // 닉네임 보냄, 응답 대기
        LoggedIn,     // 자리 배정 받음
        Failed        // 접속·입장 실패 (statusMessage에 이유)
    };

    virtual ~Session() = default;

    virtual void update(float dt) = 0;          // 매 프레임 (로컬: 로직 진행 / 네트워크: 받은 패킷 처리)
    virtual State state() const = 0;
    virtual std::string statusMessage() const = 0;
    virtual void retry() = 0;                   // Failed 상태에서 다시 접속
    virtual void login(const std::string& nickname) = 0;

    virtual int localSlot() const = 0;          // 내 자리 번호 (입장 전 -1)
    virtual const Snapshot& snapshot() const = 0;
    virtual float snapshotAge() const = 0;      // 스냅샷이 만들어진 뒤 지난 시간(초) — 움직이는 표적 위치 보정용
    virtual void drainEvents(std::vector<GameEvent>& out) = 0;

    virtual void setReady(bool ready) = 0;
    virtual void sendAim(float yawDeg, float pitchDeg) = 0;
    virtual void fire(float yawDeg, float pitchDeg) = 0;
    virtual void setTrigger(bool held, float yawDeg, float pitchDeg) = 0;
};
