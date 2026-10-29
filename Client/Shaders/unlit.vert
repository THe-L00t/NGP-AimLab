#version 330 core
// 단색 도형 (조준점·진행 바·UI 패널). 원본 crosshair/timerbar 셰이더를 하나로 합쳤다.

layout(location = 0) in vec3 aPos;

uniform mat4 uMVP;

void main()
{
    gl_Position = uMVP * vec4(aPos, 1.0);
}
