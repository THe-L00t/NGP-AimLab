#pragma once
// 최종 결과: 팀 점수 + 개인별 라운드 기여도 + MVP. 시간이 지나면 로직이 대기실로 돌려보낸다.
#include "Scene/Scene.h"

class ResultScene : public Scene {
public:
    void onEnter(App& app) override;
    void update(App& app, float dt) override;
    void render(App& app) override;
    const char* name() const override { return "Result"; }
};
