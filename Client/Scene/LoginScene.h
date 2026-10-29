#pragma once
// 로그인 화면: (서버 접속) → 닉네임 입력 → 입장. 배경에서 마우스로 시점을 돌려 볼 수 있다.
#include "Scene/Scene.h"
#include <string>

class LoginScene : public Scene {
public:
    void onEnter(App& app) override;
    void update(App& app, float dt) override;
    void render(App& app) override;
    const char* name() const override { return "Login"; }

private:
    std::string nickname_;
    float caretTimer_ = 0.0f;
};
