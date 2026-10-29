#include "Scene/LobbyScene.h"
#include "AimMath.h"
#include "Core/App.h"
#include "Core/Palette.h"
#include "Scene/GameScene.h"
#include "Scene/LoginScene.h"
#include "Scene/ResultScene.h"
#include "Scene/SceneCommon.h"
#include <cmath>
#include <iostream>

void LobbyScene::onEnter(App& app) {
    // 자리마다 서 있는 방향이 다르므로(원 위에 빙 둘러섬), 들어올 때 맵 중앙을 보게 한다
    float yaw = 0.0f, pitch = 0.0f;
    aim::lookAngles(aim::slotEyePosition(app.session().localSlot()), aim::arenaCenter(), yaw, pitch);
    app.camera().setAngles(yaw, pitch);
}

void LobbyScene::update(App& app, float) {
    Session& session = app.session();
    if (session.state() != Session::State::LoggedIn) {
        app.scenes().change(std::make_unique<LoginScene>());
        return;
    }
    const Snapshot& s = session.snapshot();
    const int me = session.localSlot();

    // 로직이 게임을 시작하면 게임 씬으로
    if (s.phase == Phase::RoundIntro || s.phase == Phase::RoundPlaying || s.phase == Phase::RoundSummary) {
        app.scenes().change(std::make_unique<GameScene>());
        return;
    }
    if (s.phase == Phase::FinalResult) {
        app.scenes().change(std::make_unique<ResultScene>());
        return;
    }

    scene::updateLook(app);
    scene::handleCommonKeys(app, true);

    if (app.input().keyPressed('r')) {
        const bool ready = !s.players[me].ready;
        session.setReady(ready);
        std::cout << (ready ? "준비 완료" : "준비 해제") << "\n";
    }

    const int joined = s.joinedCount();
    if (joined != lastJoined_) {
        std::cout << "대기실 인원: " << joined << " / " << (int)s.requiredPlayers << "\n";
        lastJoined_ = joined;
    }
}

void LobbyScene::render(App& app) {
    Renderer& r = app.renderer();
    const Snapshot& s = app.session().snapshot();
    const int me = app.session().localSlot();
    scene::drawWorld(app);

    r.begin2D();
    scene::drawNameTags(app);
    const float w = (float)r.width();
    const float h = (float)r.height();

    // ---- 왼쪽 위: 참가자 목록 ----
    const float px = 20.0f, pw = 330.0f, ph = 60.0f + 32.0f * cfg::kMaxPlayers;
    const float py = h - 20.0f - ph;
    scene::drawPanel(app, px, py, pw, ph);
    const int joined = s.joinedCount();
    r.drawText(px + 15.0f, py + ph - 30.0f, scene::format("LOBBY   Players %d / %d", joined, (int)s.requiredPlayers),
               palette::kYellow);
    for (int k = 0; k < cfg::kMaxPlayers; ++k) {
        const float y = py + ph - 65.0f - 32.0f * k;
        const PlayerInfo& p = s.players[k];
        if (!p.joined) {
            r.drawText(px + 15.0f, y, scene::format("%d. (empty)", k + 1), palette::kGray);
            continue;
        }
        r.drawText(px + 15.0f, y, scene::format("%d. %s%s", k + 1, p.name, k == me ? " (you)" : ""),
                   palette::slotColor(k));
        r.drawText(px + pw - 120.0f, y, p.ready ? "READY" : "NOT READY", p.ready ? palette::kGreen : palette::kRed);
    }

    // ---- 가운데 위: 상태 안내 ----
    std::string status;
    glm::vec3 statusColor = palette::kWhite;
    bool allReady = true;
    for (const auto& p : s.players) {
        if (p.joined && !p.ready) allReady = false;
    }
    if (s.phase == Phase::LobbyCountdown) {
        const int sec = (int)std::ceil(s.phaseTimeLeft);
        r.drawTextCentered(w / 2.0f, h / 2.0f + 60.0f, scene::format("Game starts in %d", sec < 1 ? 1 : sec),
                           palette::kYellow, Font::Large);
    } else if (joined < s.requiredPlayers) {
        status = scene::format("Waiting for players... (%d / %d)", joined, (int)s.requiredPlayers);
    } else if (me >= 0 && !s.players[me].ready) {
        status = "Press R when you are ready";
        statusColor = palette::kGreen;
    } else if (!allReady) {
        status = "Waiting for the others to get ready...";
    }
    if (!status.empty()) r.drawTextCentered(w / 2.0f, h - 50.0f, status, statusColor);

    scene::drawCrosshair(app);
    scene::drawInfoOverlay(app);
    scene::drawHint(app, "R: ready / not ready    2: crosshair color    3: crosshair shape    M: info    Esc: release mouse    Q: quit");
}
