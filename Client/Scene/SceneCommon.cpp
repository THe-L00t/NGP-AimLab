#include "Scene/SceneCommon.h"
#include "AimMath.h"
#include "Core/App.h"
#include "Core/Palette.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <iostream>

namespace scene {

std::string format(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return buf;
}

void updateLook(App& app) {
    const glm::vec2 d = app.input().lookDelta();
    if (d.x != 0.0f || d.y != 0.0f) app.camera().addLook(d.x, d.y);
    const int slot = app.session().localSlot();
    app.camera().setPosition(aim::slotEyePosition(slot >= 0 ? slot : 0));
}

void handleCommonKeys(App& app, bool allowSettings) {
    InputManager& in = app.input();
    Settings& st = app.settings();
    if (in.keyPressed('q')) {
        app.quit();
        return;
    }
    if (in.keyPressed('m')) {
        st.infoTimer = 3.0f;
        std::cout << "조준점: " << st.colorName() << " / " << st.shapeName() << "\n";
    }
    if (!allowSettings) return;   // 원본과 같게: 게임 중에는 조준점 설정 변경 불가
    if (in.keyPressed('2')) {
        st.cycleColor();
        st.infoTimer = 3.0f;
        std::cout << "조준점 색: " << st.colorName() << "\n";
    }
    if (in.keyPressed('3')) {
        st.cycleShape();
        st.infoTimer = 3.0f;
        std::cout << "조준점 모양: " << st.shapeName() << "\n";
    }
}

// 다른 플레이어의 눈 → 월드 변환 (그 사람의 총을 그 사람 시선 방향으로 그릴 때 쓴다)
static glm::mat4 eyeToWorld(int slot, const PlayerInfo& p) {
    const glm::vec3 eye = aim::slotEyePosition(slot);
    const glm::vec3 front = aim::direction(p.yaw, p.pitch);
    return glm::inverse(glm::lookAt(eye, eye + front, glm::vec3(0.0f, 1.0f, 0.0f)));
}

void drawWorld(App& app, float bossFlash) {
    Renderer& r = app.renderer();
    const Snapshot& s = app.session().snapshot();
    const int me = app.session().localSlot();
    // 스냅샷은 30Hz로 갱신되므로, 그 사이에는 속도로 위치를 보정해서 부드럽게 그린다
    const float age = std::min(app.session().snapshotAge(), 0.1f);

    r.beginFrame(app.camera());
    r.drawRoom();

    // 표적
    for (const TargetInfo& t : s.targets) {
        bool mine = me >= 0 && t.progress[me] > 0;
        bool others = false;
        for (int k = 0; k < cfg::kMaxPlayers; ++k) {
            if (k != me && t.progress[k] > 0) others = true;
        }
        const glm::vec3 color = mine ? palette::kTrackedMine : others ? palette::kTrackedOther : palette::kTarget;
        r.drawSphere(t.pos + t.vel * age, t.radius, color);
    }

    // 보스
    if (s.boss.alive) {
        const glm::vec3 base = s.boss.shield ? palette::kShield : palette::kBoss;   // 보호막 중이면 하늘색
        const glm::vec3 color = glm::mix(base, palette::kWhite, std::clamp(bossFlash, 0.0f, 1.0f));
        r.drawSphere(s.boss.pos + s.boss.vel * age, s.boss.radius, color);
    }

    // 파편
    for (const auto& p : app.debris().pieces()) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), p.pos);
        m = glm::rotate(m, glm::radians(p.rot.x), glm::vec3(1, 0, 0));
        m = glm::rotate(m, glm::radians(p.rot.y), glm::vec3(0, 1, 0));
        m = glm::rotate(m, glm::radians(p.rot.z), glm::vec3(0, 0, 1));
        m = glm::scale(m, glm::vec3(p.size));
        r.drawCube(m, p.color);
    }

    // 다른 플레이어의 총 (접속하면 내 옆에 생긴다)
    for (int k = 0; k < cfg::kMaxPlayers; ++k) {
        if (k == me || !s.players[k].joined) continue;
        r.drawGunAt(eyeToWorld(k, s.players[k]), palette::slotColor(k) * 0.6f);
    }

    // 내가 따라가는 표적 위의 타이머 바 (원본과 같게 남은 시간만큼, 초록 → 빨강)
    if (me >= 0) {
        for (const TargetInfo& t : s.targets) {
            if (t.progress[me] == 0) continue;
            const float done = t.progress[me] / 255.0f;
            const glm::vec3 pos = t.pos + t.vel * age + glm::vec3(0.0f, t.radius + 0.3f, 0.0f);
            r.drawBar3D(pos, t.radius * 2.0f, 0.1f, 1.0f - done, glm::vec3(done, 1.0f - done, 0.0f));
        }
    }

    r.drawGunFirstPerson(palette::kMyGun);
}

void drawNameTags(App& app) {
    Renderer& r = app.renderer();
    const Snapshot& s = app.session().snapshot();
    const int me = app.session().localSlot();
    for (int k = 0; k < cfg::kMaxPlayers; ++k) {
        if (k == me || !s.players[k].joined) continue;
        // 그 사람 총 바로 위에 띄운다 (총이 그 사람을 나타내는 유일한 모습이므로)
        glm::vec2 screen;
        const glm::vec3 tag = glm::vec3(eyeToWorld(k, s.players[k]) * glm::vec4(r.gunTagAnchor(), 1.0f));
        if (r.worldToScreen(tag, screen)) {
            r.drawTextCentered(screen.x, screen.y, s.players[k].name, palette::slotColor(k));
        }
    }
}

void drawCrosshair(App& app) {
    const Settings& st = app.settings();
    app.renderer().drawCrosshair(st.shape == CrosshairShape::Circle, st.colorVec());
}

void drawInfoOverlay(App& app) {
    const Settings& st = app.settings();
    if (st.infoTimer <= 0.0f) return;
    Renderer& r = app.renderer();
    const float cx = r.width() / 2.0f;
    const float y = r.height() / 2.0f + 120.0f;
    r.drawTextCentered(cx, y, format("Crosshair Color: %s", st.colorName()), palette::kWhite);
    r.drawTextCentered(cx, y - 30.0f, format("Crosshair Shape: %s", st.shapeName()), palette::kWhite);
}

void drawHint(App& app, const std::string& text) {
    Renderer& r = app.renderer();
    r.drawRect(0.0f, 0.0f, (float)r.width(), 30.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    r.drawTextCentered(r.width() / 2.0f, 10.0f, text, palette::kGray, Font::Small);
}

void drawPanel(App& app, float x, float y, float w, float h, float alpha) {
    app.renderer().drawRect(x, y, w, h, glm::vec4(0.0f, 0.0f, 0.0f, alpha));
}

} // namespace scene
