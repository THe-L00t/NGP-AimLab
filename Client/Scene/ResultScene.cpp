#include "Scene/ResultScene.h"
#include "Core/App.h"
#include "Core/Palette.h"
#include "Scene/GameScene.h"
#include "Scene/LobbyScene.h"
#include "Scene/LoginScene.h"
#include "Scene/SceneCommon.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

void ResultScene::onEnter(App& app) {
    const Snapshot& s = app.session().snapshot();
    std::cout << "===== 최종 결과: "
              << (s.failedRound < 0 ? std::string("모두 통과") : "라운드 " + std::to_string(s.failedRound + 1) + " 실패")
              << " · 팀 점수 " << s.teamScore << " =====\n";
    for (int k = 0; k < cfg::kMaxPlayers; ++k) {
        const PlayerInfo& p = s.players[k];
        if (!p.joined) continue;
        std::cout << "  " << p.name << ": " << p.hits[0] << " / " << p.hits[1] << " / " << p.hits[2]
                  << "  (합계 " << p.totalHits() << ")\n";
    }
}

void ResultScene::update(App& app, float) {
    Session& session = app.session();
    if (session.state() != Session::State::LoggedIn) {
        app.scenes().change(std::make_unique<LoginScene>());
        return;
    }
    const Snapshot& s = session.snapshot();
    if (s.phase == Phase::Lobby || s.phase == Phase::LobbyCountdown) {
        app.scenes().change(std::make_unique<LobbyScene>());
        return;
    }
    if (s.phase != Phase::FinalResult) {
        app.scenes().change(std::make_unique<GameScene>());
        return;
    }
    scene::updateLook(app);
    scene::handleCommonKeys(app, true);
}

void ResultScene::render(App& app) {
    Renderer& r = app.renderer();
    const Snapshot& s = app.session().snapshot();
    scene::drawWorld(app);
    r.begin2D();
    scene::drawNameTags(app);

    // 개인 합계가 높은 순으로 정렬
    std::vector<int> order;
    for (int k = 0; k < cfg::kMaxPlayers; ++k) {
        if (s.players[k].joined) order.push_back(k);
    }
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return s.players[a].totalHits() > s.players[b].totalHits();
    });

    const float cx = r.width() / 2.0f, cy = r.height() / 2.0f;
    const float pw = 600.0f, ph = 230.0f + 30.0f * (float)order.size();
    const float left = cx - pw / 2.0f;
    scene::drawPanel(app, left, cy - ph / 2.0f, pw, ph, 0.7f);

    float y = cy + ph / 2.0f - 45.0f;
    const bool allClear = s.failedRound < 0;
    if (allClear) {
        r.drawTextCentered(cx, y, "ALL CLEAR", palette::kGreen, Font::Large);
    } else {
        const int fr = s.failedRound;
        r.drawTextCentered(cx, y, scene::format("FAILED AT ROUND %d - %s", fr + 1, roundName(kRoundOrder[fr])),
                           palette::kRed, Font::Large);
    }
    y -= 40.0f;
    r.drawTextCentered(cx, y, scene::format("TEAM SCORE  %d", s.teamScore), palette::kWhite, Font::Large);
    y -= 45.0f;

    // 표: 이름 | R1 | R2 | R3 | 합계
    const float col[5] = { left + 40.0f, left + 280.0f, left + 350.0f, left + 420.0f, left + 500.0f };
    const char* header[5] = { "PLAYER", "R1", "R2", "R3", "TOTAL" };
    for (int c = 0; c < 5; ++c) r.drawText(col[c], y, header[c], palette::kGray, Font::Small);
    y -= 30.0f;
    for (int k : order) {
        const PlayerInfo& p = s.players[k];
        r.drawText(col[0], y, p.name, palette::slotColor(k));
        for (int rr = 0; rr < cfg::kRoundCount; ++rr) {
            const bool played = allClear || rr <= s.failedRound;   // 실패한 뒤 라운드는 하지 않았다
            r.drawText(col[1 + rr], y, played ? scene::format("%d", p.hits[rr]) : std::string("-"),
                       played ? palette::kWhite : palette::kGray);
        }
        r.drawText(col[4], y, scene::format("%d", p.totalHits()), palette::kYellow);
        y -= 30.0f;
    }

    if (!order.empty()) {
        const PlayerInfo& mvp = s.players[order.front()];
        y -= 10.0f;
        r.drawTextCentered(cx, y, scene::format("MVP  %s  (%d hits)", mvp.name, mvp.totalHits()), palette::kGreen);
    }
    y -= 35.0f;
    const int sec = (int)std::ceil(s.phaseTimeLeft);
    r.drawTextCentered(cx, y, scene::format("Back to lobby in %d", sec < 1 ? 1 : sec), palette::kGray);

    scene::drawCrosshair(app);
    scene::drawInfoOverlay(app);
    scene::drawHint(app, "2: crosshair color    3: crosshair shape    M: info    Esc: release mouse    Q: quit");
}
