#pragma once
// GPU 버퍼(VAO/VBO/EBO) 묶음 + 기본 도형 생성.
// 구·큐브 같은 도형은 하나만 만들어 두고 모델 행렬로 크기·위치를 바꿔 여러 번 그린다.
#include "Render/GL.h"
#include <vector>

class Mesh {
public:
    // stride: 정점당 float 수. 3 = 위치만(unlit), 6 = 위치+법선(lit), 8 = 위치+법선+텍스처 좌표(텍스처 모델)
    void create(const std::vector<float>& vertices, int stride,
                const std::vector<unsigned>& indices, GLenum primitive);
    void destroy();
    void draw() const;
    bool valid() const { return vao_ != 0; }

private:
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei count_ = 0;
    GLenum primitive_ = GL_TRIANGLES;
    bool indexed_ = false;
};

// 도형 생성 함수 (정점 데이터만 만든다 — GPU 업로드는 Mesh::create)
namespace MeshGen {
void sphere(int sectors, int stacks, std::vector<float>& v, std::vector<unsigned>& i);   // 반지름 1, 위치+법선
void cube(std::vector<float>& v, std::vector<unsigned>& i);                               // 한 변 1, 가운데 원점, 위치+법선
void room(float w, float h, float d, float floorY, std::vector<float>& v);                // 안쪽을 보는 상자, 위치+법선
void box(const float center[3], const float size[3], std::vector<float>& v, std::vector<unsigned>& i);  // 큐브를 원하는 곳에 추가
void quad01(std::vector<float>& v);     // (0,0)~(1,1) 사각형, 위치만, 삼각형 2개
void cross(std::vector<float>& v);      // 십자 선 4점 (GL_LINES)
void circle(int segments, std::vector<float>& v);  // 반지름 1 원 (GL_LINE_LOOP)
}
