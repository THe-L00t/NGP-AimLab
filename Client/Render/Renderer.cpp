#include "Render/Renderer.h"
#include "GameConfig.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace {
const glm::vec3 kRoomColor(0.3f, 0.3f, 0.3f);          // 원본 ShootingBox 색
const glm::vec3 kLightPos(0.0f, 12.0f, 0.0f);          // 원본 Light의 주 조명
const glm::vec3 kLightColor(1.0f, 1.0f, 1.0f);
}

bool Renderer::init(int width, int height) {
    resize(width, height);

    if (!lit_.loadFromFiles("Shaders/lit.vert", "Shaders/lit.frag")) return false;
    if (!unlit_.loadFromFiles("Shaders/unlit.vert", "Shaders/unlit.frag")) return false;

    std::vector<float> v;
    std::vector<unsigned> idx;
    MeshGen::sphere(36, 18, v, idx);  sphere_.create(v, 6, idx, GL_TRIANGLES);
    MeshGen::cube(v, idx);            cube_.create(v, 6, idx, GL_TRIANGLES);
    MeshGen::room(cfg::kRoomWidth, cfg::kRoomHeight, cfg::kRoomDepth, cfg::kRoomFloorY, v);
    room_.create(v, 6, {}, GL_TRIANGLES);
    MeshGen::quad01(v);               quad_.create(v, 3, {}, GL_TRIANGLES);
    MeshGen::cross(v);                cross_.create(v, 3, {}, GL_LINES);
    MeshGen::circle(32, v);           circle_.create(v, 3, {}, GL_LINE_LOOP);

    gun_.load("Resources/Gun.obj");

    glEnable(GL_DEPTH_TEST);
    return true;
}

void Renderer::shutdown() {
    lit_.destroy();
    unlit_.destroy();
    sphere_.destroy();
    cube_.destroy();
    room_.destroy();
    quad_.destroy();
    cross_.destroy();
    circle_.destroy();
    gun_.destroy();
}

void Renderer::resize(int width, int height) {
    width_ = width > 0 ? width : 1;
    height_ = height > 0 ? height : 1;
    glViewport(0, 0, width_, height_);
    proj_ = glm::perspective(glm::radians(cfg::kFovYDeg), (float)width_ / (float)height_, 0.1f, 100.0f);
    ortho_ = glm::ortho(0.0f, (float)width_, 0.0f, (float)height_, -1.0f, 1.0f);
}

// ============================================================
// 3D
// ============================================================

void Renderer::beginFrame(const Camera& cam) {
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    view_ = cam.view();
    camPos_ = cam.position();
    camRight_ = cam.right();
    camUp_ = cam.up();
    camFront_ = cam.front();
}

void Renderer::useLit(const glm::mat4& view, const glm::vec3& lightPos, const glm::vec3& viewPos) {
    lit_.use();
    lit_.setMat4("view", view);
    lit_.setMat4("projection", proj_);
    lit_.setVec3("lightPos", lightPos);
    lit_.setVec3("lightColor", kLightColor);
    lit_.setVec3("viewPos", viewPos);
    lit_.setInt("useTexture", 0);   // 기본은 단색. 텍스처는 총을 그릴 때만 켠다
    lit_.setInt("diffuseMap", 0);
}

void Renderer::applyGunMaterial(const glm::vec3& tint) {
    if (gun_.hasTexture()) {
        gun_.texture().bind(0);
        lit_.setInt("useTexture", 1);
    }
    lit_.setVec3("objectColor", tint);
}

void Renderer::drawRoom() {
    useLit(view_, kLightPos, camPos_);
    lit_.setMat4("model", glm::mat4(1.0f));
    lit_.setVec3("objectColor", kRoomColor);
    room_.draw();
}

void Renderer::drawSphere(const glm::vec3& center, float radius, const glm::vec3& color) {
    useLit(view_, kLightPos, camPos_);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), center);
    model = glm::scale(model, glm::vec3(radius));
    lit_.setMat4("model", model);
    lit_.setVec3("objectColor", color);
    sphere_.draw();
}

void Renderer::drawCube(const glm::mat4& model, const glm::vec3& color) {
    useLit(view_, kLightPos, camPos_);
    lit_.setMat4("model", model);
    lit_.setVec3("objectColor", color);
    cube_.draw();
}

void Renderer::drawGunFirstPerson(const glm::vec3& color) {
    // 총은 카메라에 붙어 있으므로 view = 단위행렬. 조명도 눈 좌표계 기준 (원본과 같은 값)
    // 깊이를 지우고 그려서 가까운 물체(옆 사람 총 등)에 파묻히지 않게 한다
    glClear(GL_DEPTH_BUFFER_BIT);
    useLit(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f));
    lit_.setMat4("model", gun_.viewTransform());
    // 텍스처가 있으면 원래 색 그대로, 없으면 지정한 단색
    applyGunMaterial(gun_.hasTexture() ? glm::vec3(1.0f) : color);
    gun_.mesh().draw();
}

void Renderer::drawGunAt(const glm::mat4& eyeToWorld, const glm::vec3& color) {
    useLit(view_, kLightPos, camPos_);
    lit_.setMat4("model", eyeToWorld * gun_.viewTransform());
    // 텍스처가 있으면 자리 색을 반쯤 섞어서 누구 총인지 구분되게 한다
    applyGunMaterial(gun_.hasTexture() ? glm::mix(glm::vec3(1.0f), color, 0.5f) : color);
    gun_.mesh().draw();
}

void Renderer::drawUnlit(const Mesh& mesh, const glm::mat4& mvp, const glm::vec4& color) {
    unlit_.use();
    unlit_.setMat4("uMVP", mvp);
    unlit_.setVec4("uColor", color);
    mesh.draw();
}

void Renderer::drawBar3D(const glm::vec3& pos, float width, float height, float fill, const glm::vec3& color) {
    // 카메라를 정면으로 보게 회전 (빌보드)
    glm::mat4 billboard = glm::translate(glm::mat4(1.0f), pos);
    billboard[0] = glm::vec4(camRight_, 0.0f);
    billboard[1] = glm::vec4(camUp_, 0.0f);
    billboard[2] = glm::vec4(-camFront_, 0.0f);

    glDisable(GL_DEPTH_TEST);
    const glm::mat4 base = proj_ * view_ * billboard * glm::translate(glm::mat4(1.0f), glm::vec3(-width / 2.0f, 0.0f, 0.0f));
    drawUnlit(quad_, base * glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f)), glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));
    if (fill > 0.0f) {
        drawUnlit(quad_, base * glm::scale(glm::mat4(1.0f), glm::vec3(width * fill, height, 1.0f)), glm::vec4(color, 1.0f));
    }
    glEnable(GL_DEPTH_TEST);
}

bool Renderer::worldToScreen(const glm::vec3& world, glm::vec2& screen) const {
    const glm::vec4 clip = proj_ * view_ * glm::vec4(world, 1.0f);
    if (clip.w <= 0.01f) return false;   // 카메라 뒤
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f) return false;
    screen.x = (ndc.x * 0.5f + 0.5f) * width_;
    screen.y = (ndc.y * 0.5f + 0.5f) * height_;
    return true;
}

// ============================================================
// 2D
// ============================================================

void Renderer::begin2D() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::drawRect(float x, float y, float w, float h, const glm::vec4& color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    drawUnlit(quad_, ortho_ * model, color);
}

void Renderer::drawCrosshair(bool circle, const glm::vec3& color) {
    // 픽셀 단위로 그려서 창 비율이 달라도 찌그러지지 않는다
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(width_ / 2.0f, height_ / 2.0f, 0.0f));
    model = glm::scale(model, glm::vec3(circle ? 10.0f : 12.0f));
    glLineWidth(2.0f);
    drawUnlit(circle ? circle_ : cross_, ortho_ * model, glm::vec4(color, 1.0f));
}

void* Renderer::glutFont(Font font) {
    switch (font) {
    case Font::Small: return GLUT_BITMAP_HELVETICA_12;
    case Font::Large: return GLUT_BITMAP_TIMES_ROMAN_24;
    default:          return GLUT_BITMAP_HELVETICA_18;
    }
}

float Renderer::textWidth(const std::string& text, Font font) const {
    return (float)glutBitmapLength(glutFont(font), (const unsigned char*)text.c_str());
}

void Renderer::drawText(float x, float y, const std::string& text, const glm::vec3& color, Font font) {
    // 비트맵 글자는 고정 파이프라인 기능이라 셰이더를 꺼야 색·위치가 정확하다
    glUseProgram(0);
    glColor3f(color.r, color.g, color.b);
    glWindowPos2f(x, y);
    glutBitmapString(glutFont(font), (const unsigned char*)text.c_str());
}

void Renderer::drawTextCentered(float cx, float y, const std::string& text, const glm::vec3& color, Font font) {
    drawText(cx - textWidth(text, font) / 2.0f, y, text, color, font);
}
