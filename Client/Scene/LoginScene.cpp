#include "Scene/LoginScene.h"
#include "AimMath.h"
#include "Core/App.h"
#include "Core/Palette.h"
#include "GameConfig.h"
#include "Scene/LobbyScene.h"
#include "Scene/SceneCommon.h"
#include <cmath>
#include <iostream>

namespace {
bool isNicknameChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
}
constexpr char kCtrlQ = 17;
}

void LoginScene::onEnter(App& app) {
    app.input().setMouseLook(true);
    // 입장 전에는 0번 자리에서 맵 중앙을 본다
    float yaw = 0.0f, pitch = 0.0f;
    aim::lookAngles(aim::slotEyePosition(app.session().localSlot()), aim::arenaCenter(), yaw, pitch);
    app.camera().setAngles(yaw, pitch);
}

void LoginScene::update(App& app, float dt) {
    Session& session = app.session();
    caretTimer_ += dt;
    scene::updateLook(app);

    if (session.state() == Session::State::LoggedIn) {
        std::cout << "입장 완료: " << session.snapshot().players[session.localSlot()].name << "\n";
        app.scenes().change(std::make_unique<LobbyScene>());
        return;
    }

    // 닉네임 입력 중에는 글자 키(Q 등)가 글자로 들어가야 하므로 종료는 Ctrl+Q
    for (char c : app.input().typedChars()) {
        if (c == kCtrlQ) {
            app.quit();
            return;
        }
        if (session.state() == Session::State::Failed) {
            if (c == '\r') session.retry();
            continue;
        }
        if (session.state() != Session::State::Connected) continue;

        if (c == '\b') {
            if (!nickname_.empty()) nickname_.pop_back();
        } else if (c == '\r' || c == '\n') {
            session.login(nickname_);
        } else if (isNicknameChar(c) && (int)nickname_.size() < cfg::kNicknameMaxLen) {
            nickname_ += c;
        }
    }
}

void LoginScene::render(App& app) {
    Renderer& r = app.renderer();
    Session& session = app.session();
    scene::drawWorld(app);

    r.begin2D();
    const float cx = r.width() / 2.0f;
    const float cy = r.height() / 2.0f;
    scene::drawPanel(app, cx - 260.0f, cy - 110.0f, 520.0f, 250.0f, 0.6f);

    r.drawTextCentered(cx, cy + 95.0f, "AIMLAB", palette::kYellow, Font::Large);
    r.drawTextCentered(cx, cy + 65.0f, "3-player co-op aim trainer", palette::kGray, Font::Small);

    switch (session.state()) {
    case Session::State::Connecting:
        r.drawTextCentered(cx, cy, "Connecting to server...", palette::kWhite);
        break;

    case Session::State::Connected: {
        r.drawTextCentered(cx, cy + 25.0f, "Enter your nickname", palette::kWhite);
        // 입력 칸
        r.drawRect(cx - 150.0f, cy - 25.0f, 300.0f, 34.0f, glm::vec4(1.0f, 1.0f, 1.0f, 0.12f));
        const bool caretOn = std::fmod(caretTimer_, 1.0f) < 0.5f;
        r.drawTextCentered(cx, cy - 14.0f, nickname_ + (caretOn ? "_" : " "), palette::kWhite);
        r.drawTextCentered(cx, cy - 55.0f, "A-Z, 0-9, _ -  (max 12)    Enter: join", palette::kGray, Font::Small);
        break;
    }
    case Session::State::LoggingIn:
        r.drawTextCentered(cx, cy, "Joining...", palette::kWhite);
        break;

    case Session::State::LoggedIn:
        break;

    case Session::State::Failed:
        r.drawTextCentered(cx, cy + 10.0f, session.statusMessage(), palette::kRed);
        r.drawTextCentered(cx, cy - 25.0f, "Press Enter to retry", palette::kWhite);
        break;
    }

    r.drawTextCentered(cx, cy - 95.0f, session.statusMessage(), palette::kGray, Font::Small);
    scene::drawNameTags(app);
    // 조준점은 그리지 않는다 (화면 가운데의 닉네임 입력 칸과 겹침)
    scene::drawHint(app, "Mouse: look around    Esc: release mouse    Ctrl+Q: quit");
}
