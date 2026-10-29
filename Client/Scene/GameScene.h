#pragma once
// 인게임: 1라운드 GridShot → 2라운드 Tracking → 3라운드 Boss.
// 라운드 시작 카운트다운·진행·라운드 결과까지 이 씬이 맡고, 단계는 로직(Snapshot.phase)을 따른다.
#include "GameTypes.h"
#include "Scene/Scene.h"

class GameScene : public Scene {
public:
    void onEnter(App& app) override;
    void onExit(App& app) override;
    void update(App& app, float dt) override;
    void render(App& app) override;
    const char* name() const override { return "Game"; }

private:
    void drawHud(App& app);
    void drawIntro(App& app);
    void drawSummary(App& app);

    Phase lastPhase_ = Phase::Lobby;
    bool triggerHeld_ = false;   // Tracking: 로직에 '누르고 있음'을 알린 상태
    float bossFlash_ = 0.0f;
};
