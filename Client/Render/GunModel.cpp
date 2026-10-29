#include "Render/GunModel.h"
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
// 1인칭 총 배치 (눈 좌표계: 오른쪽 +X, 위 +Y, 정면 -Z)
constexpr float kGunLength = 0.45f;                        // 총 전체 길이 (화면 오른쪽 아래 표적을 너무 가리지 않게)
const glm::vec3 kGunPosition(0.34f, -0.33f, -0.9f);        // 총 가운데 위치 (오른쪽 아래)

std::string directoryOf(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
}

std::string trim(const std::string& s) {
    const size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}
}

void GunModel::load(const std::string& objPath) {
    if (loadObj(objPath)) {
        fromFile_ = true;
        return;
    }
    std::cout << "총 모델 없음(" << objPath << ") → 임시 상자 모양으로 표시\n";
    fromFile_ = false;
    buildFallback();
}

void GunModel::buildFallback() {
    // 눈 좌표계(카메라가 -Z를 봄)에 바로 만든다: 몸통 + 총열 + 손잡이
    std::vector<float> v;
    std::vector<unsigned> idx;
    const float bodyC[3] = { 0.32f, -0.30f, -0.95f },  bodyS[3] = { 0.10f, 0.12f, 0.42f };
    const float barrelC[3] = { 0.32f, -0.27f, -1.30f }, barrelS[3] = { 0.05f, 0.05f, 0.30f };
    const float gripC[3] = { 0.32f, -0.40f, -0.82f },   gripS[3] = { 0.08f, 0.16f, 0.09f };
    MeshGen::box(bodyC, bodyS, v, idx);
    MeshGen::box(barrelC, barrelS, v, idx);
    MeshGen::box(gripC, gripS, v, idx);
    mesh_.create(v, 6, idx, GL_TRIANGLES);
    viewTransform_ = glm::mat4(1.0f);
    tagAnchor_ = glm::vec3(0.32f, -0.08f, -1.0f);
}

bool GunModel::loadObj(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::vector<glm::vec3> positions, normals;
    std::vector<glm::vec2> texcoords;
    std::vector<float> out;   // 위치3 + 법선3 + UV2, 삼각형 목록
    std::string mtlName;

    auto resolve = [](int idx, size_t count) -> int {
        // OBJ 인덱스는 1부터, 음수는 뒤에서부터
        if (idx > 0) return idx - 1;
        if (idx < 0) return (int)count + idx;
        return -1;
    };

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "v") {
            glm::vec3 p;
            iss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (type == "vn") {
            glm::vec3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (type == "vt") {
            glm::vec2 t;
            iss >> t.x >> t.y;
            texcoords.push_back(t);
        } else if (type == "mtllib") {
            std::string rest;
            std::getline(iss, rest);
            mtlName = trim(rest);
        } else if (type == "f") {
            // 다각형 면도 받는다 (부채꼴로 삼각형 분할). 형식: v, v/vt, v//vn, v/vt/vn
            struct Corner { int p = -1, t = -1, n = -1; };
            std::vector<Corner> face;
            std::string token;
            while (iss >> token) {
                Corner c;
                std::string a, b, n;
                std::istringstream ts(token);
                std::getline(ts, a, '/');
                std::getline(ts, b, '/');
                std::getline(ts, n, '/');
                if (!a.empty()) c.p = resolve(std::stoi(a), positions.size());
                if (!b.empty()) c.t = resolve(std::stoi(b), texcoords.size());
                if (!n.empty()) c.n = resolve(std::stoi(n), normals.size());
                face.push_back(c);
            }
            for (size_t k = 1; k + 1 < face.size(); ++k) {
                const Corner tri[3] = { face[0], face[k], face[k + 1] };
                glm::vec3 p[3];
                bool ok = true;
                for (int j = 0; j < 3; ++j) {
                    if (tri[j].p < 0 || tri[j].p >= (int)positions.size()) ok = false;
                    else p[j] = positions[tri[j].p];
                }
                if (!ok) continue;
                glm::vec3 flat = glm::cross(p[1] - p[0], p[2] - p[0]);
                flat = glm::length(flat) > 1e-8f ? glm::normalize(flat) : glm::vec3(0, 1, 0);
                for (int j = 0; j < 3; ++j) {
                    const glm::vec3 nrm = (tri[j].n >= 0 && tri[j].n < (int)normals.size()) ? normals[tri[j].n] : flat;
                    const glm::vec2 uv = (tri[j].t >= 0 && tri[j].t < (int)texcoords.size()) ? texcoords[tri[j].t] : glm::vec2(0.0f);
                    out.insert(out.end(), { p[j].x, p[j].y, p[j].z, nrm.x, nrm.y, nrm.z, uv.x, uv.y });
                }
            }
        }
    }
    if (out.empty()) {
        std::cerr << "OBJ 파일에 면이 없습니다: " << path << "\n";
        return false;
    }
    mesh_.create(out, 8, {}, GL_TRIANGLES);
    std::cout << "총 모델 로드: " << path << " (삼각형 " << out.size() / 24 << "개)\n";

    // ---- 크기·방향 자동 맞춤 ----
    glm::vec3 bmin(1e9f), bmax(-1e9f);
    for (const auto& p : positions) {
        bmin = glm::min(bmin, p);
        bmax = glm::max(bmax, p);
    }
    const glm::vec3 size = bmax - bmin;
    const glm::vec3 center = (bmin + bmax) * 0.5f;
    const bool alongX = size.x >= size.z;   // 총열 축
    const float length = alongX ? size.x : size.z;

    // 아래쪽 30%(손잡이)가 몰린 쪽 = 뒤
    double gripSum = 0.0;
    int gripCount = 0;
    for (const auto& p : positions) {
        if (p.y < bmin.y + 0.3f * size.y) {
            gripSum += alongX ? p.x : p.z;
            ++gripCount;
        }
    }
    const float gripOffset = gripCount > 0 ? (float)(gripSum / gripCount) - (alongX ? center.x : center.z) : 0.0f;
    const bool muzzlePositive = gripOffset < 0.0f;   // 손잡이가 - 쪽이면 총구는 + 쪽

    // 총구 방향을 정면(-Z)으로: +X→-Z는 Y축 +90°, -X→-Z는 -90°, +Z→-Z는 180°, -Z는 그대로
    const float angle = alongX ? (muzzlePositive ? 90.0f : -90.0f) : (muzzlePositive ? 180.0f : 0.0f);
    const float scale = length > 1e-6f ? kGunLength / length : 1.0f;

    viewTransform_ = glm::translate(glm::mat4(1.0f), kGunPosition);
    viewTransform_ = glm::rotate(viewTransform_, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    viewTransform_ = glm::scale(viewTransform_, glm::vec3(scale));
    viewTransform_ = glm::translate(viewTransform_, -center);
    tagAnchor_ = kGunPosition + glm::vec3(0.0f, size.y * scale * 0.5f + 0.08f, 0.0f);
    std::cout << "  자동 맞춤: 총열 " << (alongX ? "X" : "Z") << "축, 총구 " << (muzzlePositive ? "+" : "-")
              << " 방향, 회전 " << angle << "°, 배율 " << scale << "\n";

    loadMaterialTexture(path, mtlName);
    return true;
}

void GunModel::loadMaterialTexture(const std::string& objPath, const std::string& mtlName) {
    const std::string dir = directoryOf(objPath);
    std::string texName;
    if (!mtlName.empty()) {
        std::ifstream mtl(dir + mtlName);
        std::string line;
        while (std::getline(mtl, line)) {
            std::istringstream iss(line);
            std::string key;
            iss >> key;
            if (key == "map_Kd") {
                std::string rest;
                std::getline(iss, rest);
                texName = trim(rest);
                break;
            }
        }
    }
    if (texName.empty()) {
        // MTL이 없으면 같은 이름의 PNG를 찾아본다 (Gun.obj → Gun.png)
        const size_t dot = objPath.find_last_of('.');
        const std::string stem = objPath.substr(dir.size(), dot == std::string::npos ? std::string::npos : dot - dir.size());
        texName = stem + ".png";
    }
    if (!texture_.loadFromFile(dir + texName)) {
        std::cout << "  총 텍스처 없음 → 단색으로 표시\n";
    }
}
