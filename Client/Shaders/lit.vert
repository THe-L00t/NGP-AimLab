#version 330 core
// 조명을 받는 물체 (방·표적·보스·파편·총). 원본 vertex.glsl + 텍스처 좌표 전달

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;   // 텍스처가 없는 메시는 이 속성이 꺼져 있어 (0,0)이 들어온다

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
}
