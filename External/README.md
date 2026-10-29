# External — 빌드에 필요한 OpenGL 라이브러리 (x64 전용)

프로젝트가 이 폴더만 참조하므로, PC에 라이브러리를 따로 설치하지 않아도 빌드된다.

| 라이브러리 | 버전 | 출처 | 들어 있는 것 |
|---|---|---|---|
| GLEW | 2.2.0 | https://github.com/nigels-com/glew/releases/tag/glew-2.2.0 (`glew-2.2.0-win32.zip`, 공식 빌드) | `include/GL/glew.h` 등, `lib/glew32.lib`, `dll/glew32.dll` |
| GLM | 1.0.1 | https://github.com/g-truc/glm/releases/tag/1.0.1 (`glm-1.0.1-light.zip`) | `include/glm/` (헤더 전용) |
| freeglut | 3.6.0 | https://github.com/freeglut/freeglut/releases/tag/v3.6.0 (공식 소스) | `include/GL/freeglut*.h`, `lib/freeglut.lib`, `dll/freeglut.dll` |
| stb_image | 2.30 | https://github.com/nothings/stb (`stb_image.h`, 2026-10-29 받음) | `include/stb/stb_image.h` (헤더 하나, 총 텍스처 PNG 읽기용). 라이선스는 파일 끝 (MIT 또는 퍼블릭 도메인 중 선택) |

## freeglut 빌드 방법 (공식 Windows 바이너리 배포처가 접속 불가라 소스에서 직접 빌드함)

VS 2026 내장 CMake, Release x64, 공유 라이브러리(DLL)만:

```
cmake -S freeglut-3.6.0 -B build -G "Visual Studio 18 2026" -A x64 ^
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DFREEGLUT_BUILD_DEMOS=OFF ^
      -DFREEGLUT_BUILD_STATIC_LIBS=OFF -DINSTALL_PDB=OFF -DCMAKE_INSTALL_PREFIX=install
cmake --build build --config Release
cmake --install build --config Release
```

- `CMAKE_POLICY_VERSION_MINIMUM=3.5`: CMake 4.x가 freeglut의 오래된 최소 버전 선언을 거부해서 필요.
- 빌드 폴더 경로가 길면 MSBuild 추적 파일이 260자 제한에 걸리므로 짧은 경로에서 빌드할 것.

## 프로젝트 쪽 설정 메모

- `FREEGLUT_LIB_PRAGMAS=0`을 정의해서 freeglut 헤더의 자동 링크를 끈다.
  켜 두면 Debug 빌드에서 `freeglutd.lib`를 찾는데, 여기에는 Release 라이브러리만 있다.
  대신 `freeglut.lib; glew32.lib; opengl32.lib; glu32.lib`를 명시적으로 링크한다.
- `dll/`의 DLL은 빌드 후 이벤트로 실행 파일 폴더에 복사된다.
- 라이선스 원문은 `licenses/`에 있다.
