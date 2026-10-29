#pragma once
// 입력 관리자.
// GLUT 입력 콜백은 여기에만 등록되고, 받은 입력을 «상태»로 모아 둔다.
// 씬은 매 프레임 이 상태를 조회만 한다 (콜백 안에서 게임 로직을 돌리지 않는다).
#include <glm/glm.hpp>
#include <string>

enum class MouseButton { Left = 0, Middle = 1, Right = 2 };

class InputManager {
public:
    void install(int width, int height);   // 창을 만든 뒤 한 번 호출
    void beginFrame();                     // 프레임 시작: 창 활성 여부에 따라 마우스 붙잡기/놓기
    void endFrame();                       // 프레임 끝: '이번 프레임에만' 유효한 값 비우기
    void onReshape(int width, int height);

    // ---- 키보드 (영문은 대소문자 구분 없이 소문자로 조회) ----
    bool keyDown(unsigned char key) const;
    bool keyPressed(unsigned char key) const;    // 이번 프레임에 새로 눌림 (키 반복 제외)
    bool keyReleased(unsigned char key) const;
    const std::string& typedChars() const { return typed_; }  // 이번 프레임에 입력된 문자 (텍스트 입력용, 키 반복 포함)

    // ---- 마우스 ----
    bool mouseDown(MouseButton b) const { return mDown_[(int)b]; }
    bool mousePressed(MouseButton b) const { return mPressed_[(int)b]; }
    bool mouseReleased(MouseButton b) const { return mReleased_[(int)b]; }
    glm::vec2 lookDelta() const { return delta_; }   // 시점 이동량(픽셀). x: 오른쪽 +, y: 위쪽 +

    // ---- 마우스 시점 모드 (커서를 숨기고 창 가운데에 고정) ----
    void setMouseLook(bool enabled);
    bool mouseLookEnabled() const { return lookEnabled_; }
    bool mouseCaptured() const { return captured_; }

private:
    static void onKeyDown(unsigned char key, int x, int y);
    static void onKeyUp(unsigned char key, int x, int y);
    static void onMouse(int button, int state, int x, int y);
    static void onMotion(int x, int y);
    static unsigned char normalize(unsigned char key);

    void handleMotion(int x, int y);
    void setCaptured(bool captured);

    static InputManager* s_instance;

    bool down_[256] = {};
    bool pressed_[256] = {};
    bool released_[256] = {};
    bool mDown_[3] = {};
    bool mPressed_[3] = {};
    bool mReleased_[3] = {};
    std::string typed_;
    glm::vec2 delta_{ 0.0f };

    int width_ = 1200;
    int height_ = 800;
    bool lookEnabled_ = true;      // 사용자가 Esc로 켜고 끈다
    bool focused_ = false;         // 창이 활성 상태인가
    bool captured_ = false;        // 실제로 붙잡고 있는가 (= 켜져 있고 창이 활성)
    bool skipNextMotion_ = true;   // 붙잡은 직후 첫 이동은 커서가 튀므로 버린다
    void* hwnd_ = nullptr;         // 이 창 (활성 여부 확인용)
};
