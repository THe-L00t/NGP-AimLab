#include "Scene/SceneManager.h"
#include <iostream>

void SceneManager::applyPending(App& app) {
    if (!pending_) return;
    if (current_) current_->onExit(app);
    current_ = std::move(pending_);
    std::cout << "[씬] " << current_->name() << "\n";
    current_->onEnter(app);
}
