#pragma once
// 여러 씬이 같이 쓰는 처리 (시점 이동, 공통 키, 3D 장면 그리기, UI 조각)
#include <glm/glm.hpp>
#include <string>

class App;

namespace scene {

// 마우스 이동량을 카메라에 반영하고, 카메라를 내 자리 눈 위치에 둔다
void updateLook(App& app);

// 공통 키: Q 종료, M 설정 정보. allowSettings면 2(조준점 색)·3(조준점 모양)도 처리
void handleCommonKeys(App& app, bool allowSettings);

// 3D 장면: 방 + 표적 + 보스 + 파편 + 다른 플레이어 총 + 내 총
// bossFlash: 0~1, 보스가 맞았을 때 하얗게 번쩍이는 정도
void drawWorld(App& app, float bossFlash = 0.0f);

// 2D: 다른 플레이어 이름표 / 조준점 / 설정 정보 / 아래쪽 도움말 / 반투명 패널
void drawNameTags(App& app);
void drawCrosshair(App& app);
void drawInfoOverlay(App& app);
void drawHint(App& app, const std::string& text);
void drawPanel(App& app, float x, float y, float w, float h, float alpha = 0.55f);

std::string format(const char* fmt, ...);

} // namespace scene
