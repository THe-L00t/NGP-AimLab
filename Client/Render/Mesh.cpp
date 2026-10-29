#include "Render/Mesh.h"
#include <cmath>

void Mesh::create(const std::vector<float>& vertices, int stride,
                  const std::vector<unsigned>& indices, GLenum primitive) {
    destroy();
    primitive_ = primitive;
    indexed_ = !indices.empty();
    count_ = indexed_ ? (GLsizei)indices.size() : (GLsizei)(vertices.size() / stride);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    if (indexed_) {
        glGenBuffers(1, &ebo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);
    }

    const GLsizei bytes = stride * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, bytes, (void*)0);
    glEnableVertexAttribArray(0);
    if (stride >= 6) {
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, bytes, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    if (stride >= 8) {   // 텍스처 좌표 (총 모델)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, bytes, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
    glBindVertexArray(0);
}

void Mesh::destroy() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ebo_) glDeleteBuffers(1, &ebo_);
    vao_ = vbo_ = ebo_ = 0;
    count_ = 0;
}

void Mesh::draw() const {
    if (!vao_) return;
    glBindVertexArray(vao_);
    if (indexed_) glDrawElements(primitive_, count_, GL_UNSIGNED_INT, nullptr);
    else glDrawArrays(primitive_, 0, count_);
    glBindVertexArray(0);
}

namespace MeshGen {

namespace {
constexpr float kPi = 3.14159265358979f;

void push(std::vector<float>& v, float x, float y, float z, float nx, float ny, float nz) {
    v.insert(v.end(), { x, y, z, nx, ny, nz });
}
}

void sphere(int sectors, int stacks, std::vector<float>& v, std::vector<unsigned>& idx) {
    v.clear();
    idx.clear();
    const float sectorStep = 2.0f * kPi / sectors;
    const float stackStep = kPi / stacks;
    for (int i = 0; i <= stacks; ++i) {
        const float stackAngle = kPi / 2.0f - i * stackStep;
        const float xy = std::cos(stackAngle);
        const float z = std::sin(stackAngle);
        for (int j = 0; j <= sectors; ++j) {
            const float a = j * sectorStep;
            const float x = xy * std::cos(a);
            const float y = xy * std::sin(a);
            push(v, x, y, z, x, y, z);  // 반지름 1 → 위치가 곧 법선
        }
    }
    for (int i = 0; i < stacks; ++i) {
        unsigned k1 = i * (sectors + 1);
        unsigned k2 = k1 + sectors + 1;
        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) idx.insert(idx.end(), { k1, k2, k1 + 1 });
            if (i != stacks - 1) idx.insert(idx.end(), { k1 + 1, k2, k2 + 1 });
        }
    }
}

void box(const float c[3], const float s[3], std::vector<float>& v, std::vector<unsigned>& idx) {
    const float hx = s[0] / 2, hy = s[1] / 2, hz = s[2] / 2;
    const float x0 = c[0] - hx, x1 = c[0] + hx;
    const float y0 = c[1] - hy, y1 = c[1] + hy;
    const float z0 = c[2] - hz, z1 = c[2] + hz;
    struct Face { float n[3]; float p[4][3]; };
    const Face faces[6] = {
        { { 0, 0, 1 },  { { x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 } } },
        { { 0, 0, -1 }, { { x1, y0, z0 }, { x0, y0, z0 }, { x0, y1, z0 }, { x1, y1, z0 } } },
        { { 0, 1, 0 },  { { x0, y1, z1 }, { x1, y1, z1 }, { x1, y1, z0 }, { x0, y1, z0 } } },
        { { 0, -1, 0 }, { { x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 } } },
        { { 1, 0, 0 },  { { x1, y0, z1 }, { x1, y0, z0 }, { x1, y1, z0 }, { x1, y1, z1 } } },
        { { -1, 0, 0 }, { { x0, y0, z0 }, { x0, y0, z1 }, { x0, y1, z1 }, { x0, y1, z0 } } },
    };
    for (const Face& f : faces) {
        const unsigned base = (unsigned)(v.size() / 6);
        for (const auto& p : f.p) push(v, p[0], p[1], p[2], f.n[0], f.n[1], f.n[2]);
        idx.insert(idx.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
    }
}

void cube(std::vector<float>& v, std::vector<unsigned>& idx) {
    v.clear();
    idx.clear();
    const float c[3] = { 0, 0, 0 };
    const float s[3] = { 1, 1, 1 };
    box(c, s, v, idx);
}

void room(float w, float h, float d, float floorY, std::vector<float>& v) {
    // 원본 ShootingBox와 같은 상자. 법선은 방 안쪽을 향한다.
    v.clear();
    const float hw = w / 2, hd = d / 2;
    const float y0 = floorY, y1 = floorY + h;
    auto quad = [&](float ax, float ay, float az, float bx, float by, float bz,
                    float cx, float cy, float cz, float dx, float dy, float dz,
                    float nx, float ny, float nz) {
        push(v, ax, ay, az, nx, ny, nz); push(v, bx, by, bz, nx, ny, nz); push(v, cx, cy, cz, nx, ny, nz);
        push(v, ax, ay, az, nx, ny, nz); push(v, cx, cy, cz, nx, ny, nz); push(v, dx, dy, dz, nx, ny, nz);
    };
    quad(-hw, y0, -hd, hw, y0, -hd, hw, y0, hd, -hw, y0, hd, 0, 1, 0);     // 바닥
    quad(-hw, y1, -hd, -hw, y1, hd, hw, y1, hd, hw, y1, -hd, 0, -1, 0);    // 천장
    quad(-hw, y0, -hd, -hw, y1, -hd, hw, y1, -hd, hw, y0, -hd, 0, 0, 1);   // Z- 벽
    quad(-hw, y0, hd, hw, y0, hd, hw, y1, hd, -hw, y1, hd, 0, 0, -1);      // Z+ 벽
    quad(-hw, y0, -hd, -hw, y0, hd, -hw, y1, hd, -hw, y1, -hd, 1, 0, 0);   // X- 벽 (뒤)
    quad(hw, y0, -hd, hw, y1, -hd, hw, y1, hd, hw, y0, hd, -1, 0, 0);      // X+ 벽 (정면)
}

void quad01(std::vector<float>& v) {
    v = { 0, 0, 0, 1, 0, 0, 1, 1, 0,
          0, 0, 0, 1, 1, 0, 0, 1, 0 };
}

void cross(std::vector<float>& v) {
    v = { -1, 0, 0, 1, 0, 0,
          0, -1, 0, 0, 1, 0 };
}

void circle(int segments, std::vector<float>& v) {
    v.clear();
    for (int i = 0; i < segments; ++i) {
        const float a = 2.0f * kPi * i / segments;
        v.insert(v.end(), { std::cos(a), std::sin(a), 0.0f });
    }
}

} // namespace MeshGen
