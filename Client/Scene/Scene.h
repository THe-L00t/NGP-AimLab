#pragma once
// 씬 기반 클래스. 화면 하나(로그인·대기실·게임·결과)가 씬 하나다.
class App;

class Scene {
public:
    virtual ~Scene() = default;
    virtual void onEnter(App&) {}
    virtual void onExit(App&) {}
    virtual void update(App& app, float dt) = 0;   // 입력 처리 + 씬 전환 판단
    virtual void render(App& app) = 0;             // 그리기만
    virtual const char* name() const = 0;
};
