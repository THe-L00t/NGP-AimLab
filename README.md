# AimLab — 3인 협동 에임 트레이너 (C++ / OpenGL)

CG 텀프로젝트(AimLab 모작)를 네트워크 멀티플레이로 확장하기 위해 다시 구성한 프로젝트.
**현재 상태: 네트워크 미연결 — 로컬 1인으로 로그인 → 대기실 → 3라운드 → 결과까지 동작.**

## 빌드·실행

1. `CG_Term_Project.sln`을 VS 2022 또는 VS 2026으로 연다 (툴셋 v143/v145 자동 선택, x64만).
2. 빌드하면 `x64\Debug\Client.exe` 옆에 DLL·`Shaders\`·`Resources\`가 복사된다.
3. F5 또는 `Client.exe` 직접 실행.
   - `Client.exe --bots 2`: 테스트용 봇 2명이 1·2번 자리에 앉는다 (쏘지 않고 두리번거리기만).
     네트워크 없이도 다른 플레이어의 총·이름표, 3명 기준 표적 수를 확인할 수 있다.
     VS에서는 프로젝트 속성 → 디버깅 → 명령 인수에 `--bots 2`.

라이브러리(freeglut·GLEW·GLM)는 `External/`에 들어 있어 따로 설치할 필요가 없다 → `External/README.md`.
총 모델은 `Client/Resources/Gun.obj` + `Gun.mtl` + `Gun.png`(텍스처)이고, 크기·방향은 자동으로 맞춘다 → `Client/Resources/README.md`.

## 게임 흐름

```
로그인(닉네임) → 대기실(R 준비, 인원 모두 준비 → 3초 카운트다운)
  → 1라운드 GridShot  (30초, 정지 표적 클릭. 팀 명중 ≥ 인원 × 20 이면 통과)
  → 2라운드 Tracking  (움직이는 표적을 좌클릭 누른 채 2초 따라가면 처치, 40초. 처치 ≥ 인원 × 4 이면 통과)
  → 3라운드 Boss      (순간이동 보스 HP를 0으로, 75초. HP 2/3·1/3에서 보호막 → 좌클릭 누른 채 따라가야 깨짐)
  → 최종 결과(10초, ALL CLEAR 또는 "몇 라운드에서 실패") → 대기실
```

- 통과 점수를 못 넘기면 그 라운드 결과(5초)를 보여 준 뒤 다음 라운드 없이 최종 결과로 간다.
- 보스 보호막: 켜지면 총알이 막히고 보스가 순간이동 없이 빠르게 움직인다. 좌클릭을 누른 채 보스를 조준하면 게이지가 차고
  (함께 조준하는 인원만큼 빨리), 다 차면 보호막이 깨지며 순간이동 후 다음 페이즈로 넘어간다. 아무도 조준하지 않으면 게이지가 조금씩 줄어든다.
- 플레이어는 맵 중앙을 둘러싼 반지름 14m 원 위에 같은 간격으로 선다(3명 = 정삼각형). 모두 중앙을 바라보고,
  표적·보스는 중앙 구역(GridShot 8×8×8, Tracking·Boss 12m 폭)에 생긴다. 대기실에 들어오거나 라운드가 시작되면 시점이 중앙으로 맞춰진다.
  화면과 명중 판정이 같은 눈 위치를 쓴다. 세로 시야각은 55°(`cfg::kFovYDeg`).

- 표적은 모두가 공유한다. **먼저 맞힌 사람**이 그 명중의 주인.
- 명중마다 **팀 공용 점수 +1**, 맞힌 사람의 **개인 맞힌 횟수 +1** (라운드별로 집계 → 기여도·MVP).
- 인원에 맞춰 표적 수를 늘린다 (`Common/GameConfig.h`에서 조정):

| | 1명 | 2명 | 3명 |
|---|---|---|---|
| GridShot 표적 | 3 | 4 | 5 |
| GridShot 통과 점수 (명중) | 20 | 40 | 60 |
| Tracking 동시 표적 / 총 처치 | 6 / 6 | 7 / 12 | 8 / 18 |
| Tracking 통과 점수 (처치) | 4 | 8 | 12 |
| Boss HP | 34 | 67 | 100 |
| 보호막 켜지는 HP | 22 · 11 | 44 · 22 | 66 · 33 |

`--bots`로 시험할 때 봇은 쏘지 않지만 인원에는 들어가므로, 통과 점수가 3명 기준이 된다.

## 조작

| 키 | 동작 |
|---|---|
| 마우스 | 시점 (로그인·대기실에서도 가능) |
| 좌클릭 | 사격 (Tracking과 보스 보호막은 누르고 있기) |
| R | 준비 / 준비 해제 (대기실) |
| 2 / 3 | 조준점 색 / 모양 (대기실·결과 화면) |
| M | 조준점 설정 표시 |
| Esc | 마우스 놓기 / 다시 잡기 |
| Q | 종료 (로그인 화면에서는 Ctrl+Q) |

## 폴더 구조

```
Common/     게임 로직. OpenGL 호출 없음 → 나중에 서버가 그대로 쓴다
  GameConfig.h   규칙 수치 (시간·표적 수·HP)
  GameTypes.h    Phase·Snapshot·GameEvent (로직 → 화면으로 넘기는 상태)
  AimMath.h      조준 방향·광선-구 판정·자리별 눈 위치
  World.*        대기실·준비·카운트다운·3라운드·점수 전부
Client/
  Core/      App(GLUT 창·메인 루프), Settings(조준점), Palette(색)
  Input/     InputManager — GLUT 입력 콜백은 여기만 받는다. 씬은 상태를 조회만 한다
  Scene/     SceneManager + Login / Lobby / Game / Result 씬
  Session/   Session 인터페이스 + LocalSession(같은 프로세스에서 World 실행)
  Render/    Renderer·Shader·Mesh·Camera·GunModel·Texture — 상태를 읽어서 그리기만 한다
  Effects/   명중 파편 (보여 주기용, 판정과 무관)
  Shaders/   lit(조명), unlit(단색: 조준점·UI·바)
  Resources/ Gun.obj 등
External/   freeglut 3.6.0 · GLEW 2.2.0 · GLM 1.0.1 · stb_image 2.30 (x64)
```

한 프레임: 입력 상태 갱신 → 세션 진행(로직 30Hz 고정 틱) → 사건(파편) → 씬 update → 내 시선 전달 → 그리기.

## 네트워크를 붙일 때 (계획)

씬은 `Session` 인터페이스만 쓰므로, 아래 두 가지를 만들고 `Core/App.cpp`에서 `LocalSession` 대신 `NetSession`을 만들면 된다.

1. **Server 프로젝트**: `Common/World`를 그대로 돌린다. 클라 접속을 받아 `join/leave/setReady/setAim/fire/setTrigger`를 호출하고,
   매 틱 `buildSnapshot`·`drainEvents` 결과를 모든 클라에 보낸다. 판정은 서버(World)가 한다.
2. **Client/Session/NetSession**: `Session`을 구현.

| Session 함수 | 네트워크로는 |
|---|---|
| `login(nickname)` | 클라 → 서버: 입장 요청. 응답으로 자리 번호 |
| `setReady` / `sendAim` / `fire` / `setTrigger` | 클라 → 서버 |
| `snapshot()` | 서버 → 클라: 매 틱 상태 (`Snapshot`) |
| `drainEvents()` | 서버 → 클라: 명중·순간이동 사건 (`GameEvent`) |
| `state()` | 접속 중 → 접속됨(닉네임 입력) → 입장 중 → 입장 완료 / 실패 |

- 필요 인원은 `World(requiredPlayers)` 생성자 인자 (서버는 3, 지금 로컬은 1).
- 다른 플레이어의 총·이름표 그리기는 이미 들어 있다 (`Snapshot.players`에 들어오면 바로 보인다).

## 원본 대비 고친 점

- 명중 판정: 등 뒤 표적도 맞던 문제, 겹치면 가장 가까운 표적이 아니라 목록 첫 표적이 맞던 문제
- 텍스트 UI가 셰이더가 켜진 채 그려지던 문제 (글자 전에 셰이더 해제)
- 원형 조준점이 화면 비율 때문에 찌그러지던 문제 (픽셀 단위로 그림)
- 결과 화면에서 파편이 공중에 멈추던 문제
- 표적 방향 전환 확률이 프레임률에 따라 달라지던 문제 (초 단위 확률로)
- 종료 시 GL 컨텍스트가 사라진 뒤 GPU 자원을 지우던 문제
- OBJ 로더: 사각형 이상 면·음수 인덱스 지원
