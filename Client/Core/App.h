#pragma once
// 프로그램 뼈대: GLUT 창·메인 루프를 돌리고, 각 시스템을 가지고 있다.
//
// 한 프레임 순서 (frame):
//   입력 상태 갱신 → 세션(로직) 진행 → 사건 → 효과 → 씬 전환 적용 → 씬 update → 조준 방향 보내기 → 입력 비우기
// 그리기(render)는 GLUT 화면 콜백에서 현재 씬의 render만 부른다.
#include "Core/Settings.h"
#include "Effects/DebrisSystem.h"
#include "GameTypes.h"
#include "Input/InputManager.h"
#include "Render/Camera.h"
#include "Render/Renderer.h"
#include "Scene/SceneManager.h"
#include "Session/Session.h"
#include <chrono>
#include <memory>
#include <vector>

class App {
public:
    int run(int argc, char** argv);
    void quit();

    InputManager& input() { return input_; }
    Renderer& renderer() { return renderer_; }
    Session& session() { return *session_; }
    Camera& camera() { return camera_; }
    DebrisSystem& debris() { return debris_; }
    Settings& settings() { return settings_; }
    SceneManager& scenes() { return scenes_; }
    const std::vector<GameEvent>& frameEvents() const { return frameEvents_; }   // 이번 프레임에 일어난 사건

private:
    static void displayCallback();
    static void reshapeCallback(int w, int h);
    static void timerCallback(int);
    static void closeCallback();

    void frame();
    void render();
    void handleEvents();
    void releaseGraphics();

    static App* s_app;

    InputManager input_;
    Renderer renderer_;
    std::unique_ptr<Session> session_;
    Camera camera_;
    DebrisSystem debris_;
    Settings settings_;
    SceneManager scenes_;
    std::vector<GameEvent> frameEvents_;

    std::chrono::steady_clock::time_point lastTime_;
    bool quitting_ = false;
    bool graphicsReleased_ = false;
};
