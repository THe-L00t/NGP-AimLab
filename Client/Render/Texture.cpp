#include "Render/Texture.h"
#include <iostream>

// stb_image: 헤더 하나짜리 이미지 디코더 (External/include/stb, 퍼블릭 도메인/MIT).
// 구현은 이 파일 한 곳에서만 만든다.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#pragma warning(push)
#pragma warning(disable : 4996 4244 4100)
#include <stb/stb_image.h>
#pragma warning(pop)

bool Texture::loadFromFile(const std::string& path) {
    destroy();
    // 이미지 파일은 위쪽 줄부터, OpenGL 텍스처는 아래쪽 줄부터 → 뒤집어서 읽는다 (OBJ의 UV와 맞추기)
    stbi_set_flip_vertically_on_load(1);
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        std::cerr << "텍스처를 읽을 수 없습니다: " << path << " (" << stbi_failure_reason() << ")\n";
        return false;
    }

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);

    std::cout << "텍스처 로드: " << path << " (" << width << "x" << height << ")\n";
    return true;
}

void Texture::destroy() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
        id_ = 0;
    }
}

void Texture::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}
