#pragma once
// 그리기 전담. 게임 상태를 «읽어서» 그리기만 하고, 게임 규칙은 전혀 모른다.
#include "Render/Camera.h"
#include "Render/GunModel.h"
#include "Render/Mesh.h"
#include "Render/Shader.h"
#include <glm/glm.hpp>
#include <string>

enum class Font { Small, Normal, Large };

class Renderer {
public:
    bool init(int width, int height);
    void shutdown();
    void resize(int width, int height);
    int width() const { return width_; }
    int height() const { return height_; }

    // ---- 3D ----
    void beginFrame(const Camera& cam);     // 화면 지우기 + 카메라·조명 설정
    void drawRoom();
    void drawSphere(const glm::vec3& center, float radius, const glm::vec3& color);
    void drawCube(const glm::mat4& model, const glm::vec3& color);
    void drawGunFirstPerson(const glm::vec3& color);
    void drawGunAt(const glm::mat4& eyeToWorld, const glm::vec3& color);   // 다른 플레이어의 총
    const glm::vec3& gunTagAnchor() const { return gun_.tagAnchor(); }    // 눈 좌표계, 총 바로 위
    // 카메라를 향하는 진행 바 (Tracking 타이머 바). fill = 0~1
    void drawBar3D(const glm::vec3& pos, float width, float height, float fill, const glm::vec3& color);
    bool worldToScreen(const glm::vec3& world, glm::vec2& screen) const;   // 화면 뒤쪽이면 false

    // ---- 2D (픽셀 좌표, 왼쪽 아래가 원점) ----
    void begin2D();
    void drawRect(float x, float y, float w, float h, const glm::vec4& color);
    void drawCrosshair(bool circle, const glm::vec3& color);
    void drawText(float x, float y, const std::string& text, const glm::vec3& color, Font font = Font::Normal);
    void drawTextCentered(float cx, float y, const std::string& text, const glm::vec3& color, Font font = Font::Normal);
    float textWidth(const std::string& text, Font font) const;

private:
    void useLit(const glm::mat4& view, const glm::vec3& lightPos, const glm::vec3& viewPos);
    void applyGunMaterial(const glm::vec3& tint);   // 총 텍스처(있으면) + 틴트 색
    void drawUnlit(const Mesh& mesh, const glm::mat4& mvp, const glm::vec4& color);
    static void* glutFont(Font font);

    Shader lit_;
    Shader unlit_;
    Mesh sphere_, cube_, room_, quad_, cross_, circle_;
    GunModel gun_;

    int width_ = 1200;
    int height_ = 800;
    glm::mat4 view_{ 1.0f };
    glm::mat4 proj_{ 1.0f };
    glm::mat4 ortho_{ 1.0f };
    glm::vec3 camPos_{ 0.0f }, camRight_{ 0.0f, 0.0f, 1.0f }, camUp_{ 0.0f, 1.0f, 0.0f }, camFront_{ 1.0f, 0.0f, 0.0f };
};
