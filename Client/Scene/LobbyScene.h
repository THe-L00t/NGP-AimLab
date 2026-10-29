#pragma once
// 대기실: 인원이 모일 때까지 대기, R로 준비/해제, 전원 준비 → 카운트다운 → 게임 시작.
// 다른 플레이어가 들어오면 그 사람 자리에 총이 생긴다. 마우스로 시점을 돌릴 수 있다.
#include "Scene/Scene.h"

class LobbyScene : public Scene {
public:
    void onEnter(App& app) override;   // 내 자리에서 맵 중앙을 보게 한다
    void update(App& app, float dt) override;
    void render(App& app) override;
    const char* name() const override { return "Lobby"; }

private:
    int lastJoined_ = -1;
};
