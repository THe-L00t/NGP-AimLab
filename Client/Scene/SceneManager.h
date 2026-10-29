#pragma once
// 현재 씬 보관 + 전환. 전환 요청은 다음 프레임 시작에 적용한다
// (씬의 update 도중에 자기 자신이 지워지지 않게).
#include "Scene/Scene.h"
#include <memory>

class SceneManager {
public:
    void change(std::unique_ptr<Scene> next) { pending_ = std::move(next); }
    void applyPending(App& app);
    Scene* current() const { return current_.get(); }

private:
    std::unique_ptr<Scene> current_;
    std::unique_ptr<Scene> pending_;
};
