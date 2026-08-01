# 라이트 배치 에디터 및 런타임 로더 가이드

## 1. 문서 목적

이 문서는 라이트 배치 에디터와 JSON 기반 런타임 라이트 로더의 구현 구조, 사용 방법, 문제 해결 과정 및 추후 작업을 기록한다.

현재 구현은 다음 기능을 제공한다.

- Directional, Point, Spot Light 생성
- ImGuizmo를 이용한 위치 및 방향 편집
- 실시간 조명 속성 변경
- 별칭 지정
- JSON 저장 및 로드
- Client 레벨 진입 시 JSON 기반 라이트 생성
- 레벨 해제 시 해당 배치 라이트 정리
- 코드에서 생성한 일반 라이트와 에디터 배치 라이트 분리

---

## 2. 전체 구조

```text
CLightPlacementEditor
├─ 라이트 생성 및 편집
├─ JSON 저장 및 로드
├─ 디버그 형상 출력
└─ 선택 라이트 기즈모 조작

CLightPlacementObject
├─ 레벨 진입 시 JSON 로드
├─ 실제 CLight 생성
├─ 배치 그룹 태깅
├─ 에디터 활성 그룹 동기화
└─ 레벨 해제 시 해당 그룹 라이트 제거

CLightManager
├─ 모든 일반 라이트 렌더링 관리
├─ 라이트 생성 및 삭제
└─ 배치 그룹 단위 제거

CLight
├─ 실제 조명 데이터
├─ 표시용 별칭
└─ 배치 그룹 태그
```

게임오브젝트 레이어는 다음과 같이 구분된다.

```text
Layer_LightPlacement
└─ CLightPlacementObject 런타임 로더

LightLayer
├─ 실제 Directional Light
├─ 실제 Point Light
├─ 실제 Spot Light
└─ 코드에서 생성한 일반 라이트
```

`Layer_LightPlacement`에는 실제 라이트가 아니라 JSON을 읽는 로더 오브젝트가 들어간다.

실제 `CLight`는 모두 기존과 동일하게 `LightLayer`에 생성된다.

---

## 3. 라이트 배치 데이터

공용 데이터는 다음 파일에 정의되어 있다.

```text
Engine/Public/LightPlacementData.h
```

### 3.1 LIGHT_PLACEMENT_ENTRY

라이트 하나의 저장 데이터를 담당한다.

```cpp
struct LIGHT_PLACEMENT_ENTRY final : public ISerializable
{
    std::string sName{};
    std::string sAlias{};
    LIGHT_TYPE eType{ LIGHT_TYPE::POINT };

    _float3 vPosition{};
    _float3 vDirection{ 0.f, -1.f, 0.f };
    _float3 vColor{ 1.f, 1.f, 1.f };

    _float fIntensity{ 10.f };
    _float fRange{ 10.f };
    _float fInnerAttenuation{ 20.f };
    _float fOuterAttenuation{ 30.f };

    _bool bActive{ true };
};
```

### 3.2 LIGHT_PLACEMENT_FILE

한 JSON 파일에 저장되는 전체 배치 라이트 데이터다.

```cpp
struct LIGHT_PLACEMENT_FILE final : public ISerializable
{
    uint32_t iVersion{ 1 };
    std::vector<LIGHT_PLACEMENT_ENTRY> lights{};
};
```

JSON 루트는 다음과 같다.

```json
{
    "LightPlacements": {
        "iVersion": 1,
        "lights": []
    }
}
```

기본 저장 경로는 다음과 같다.

```text
./Resources/json/Lights/<파일명>.json
```

파일명은 `.json` 확장자와 잘못된 경로 문자를 정규화한 후 배치 그룹 이름으로도 사용된다.

```text
Level_Terrain.json
→ PlacementGroup = "Level_Terrain"
```

배치 그룹은 파일명에서 파생되므로 JSON 내부에는 별도로 저장하지 않는다.

---

## 4. 라이트 배치 에디터

관련 파일:

```text
Engine/Public/LightPlacementEditor.h
Engine/Private/LightPlacementEditor.cpp
```

기존 `CLightManager::UpdateGUI()`는 `CLightPlacementEditor::UpdateGUI()`를 우선 호출한다.

기존 라이트 매니저 GUI 코드는 fallback 용도로 남아 있지만, 정상 초기화 상태에서는 새로운 라이트 배치 에디터가 사용된다.

### 4.1 생성 가능한 라이트

- Directional Light
- Point Light
- Spot Light

새 라이트는 활성 카메라의 전방 `Spawn Distance` 위치에 생성된다.

### 4.2 편집 가능한 속성

- Active
- Light Type
- Position
- Direction
- Color
- Intensity
- Range
- Spot Inner Angle
- Spot Outer Angle
- Alias

Point Light는 방향이 필요하지 않으므로 Rotate 기즈모를 사용하지 않는다.

### 4.3 기즈모

- Translate
- Rotate
- Local 모드
- World 모드
- Translation Snap
- Rotation Snap

현재 화면 피킹은 구현하지 않았다.

라이트 에디터의 목록에서 라이트를 선택한 뒤 기즈모로 편집한다.

---

## 5. 디버그 시각화

라이트 에디터는 `CDbgLineRender`를 이용해 라이트 형태를 직접 출력한다.

### 5.1 Point Light

- 중심 Cross
- Range 기반 Sphere

### 5.2 Spot Light

- 중심 Cross
- 진행 방향 Arrow
- Outer Angle과 Range를 이용한 Cone

기존 Frustum 형태보다 Spot Light의 실제 영향 형태를 쉽게 확인할 수 있도록 Cone을 사용한다.

### 5.3 Directional Light

- 중심 Cross
- 방향 Arrow

### 5.4 표시 옵션

- Visible
- Depth Test
- Show All
- Influence Range
- Direction

Point Light의 Range가 `0`이면 투영 행렬의 Near/Far 값이 같아져 DirectXMath assertion이 발생할 수 있다.

이를 방지하기 위해 최소 Range를 `0.02f`로 제한했다.

---

## 6. 라이트 별칭

`CLight`에 표시용 별칭을 추가했다.

```cpp
std::string m_sAlias{};
```

관련 API:

```cpp
void Set_LightAlias(std::string sAlias);
const std::string& Get_LightAlias() const;
```

별칭은 기존 오브젝트 태그를 대체하지 않는다.

표시 형식:

```text
별칭 없음:
Light_Clone0 [Point]

별칭 있음:
Light_Clone0 (복도 조명) [Point]
```

JSON 저장 시 `sAlias`에 기록되고 에디터 및 런타임 로더에서 복원된다.

기존 JSON에 `sAlias`가 없으면 기본 빈 문자열을 유지하므로 이전 데이터와 호환된다.

---

## 7. 레벨 프리셋 및 파일명 보호

레벨 파일 프리셋은 버튼이 아니라 콤보박스로 제공한다.

지원 프리셋:

```text
Level_CharlesRookwood
Level_BossCharlesRookwood
Level_Terrain
```

프리셋에 포함되지 않은 파일명은 `Custom`으로 표시한다.

파일명 오입력을 방지하기 위해 `Manual File Name` 체크가 활성화된 경우에만 `Light File` 입력창을 직접 편집할 수 있다.

콤보 선택은 활성 배치 그룹과 대상 파일명을 전환한다.

콤보 선택만으로 자동 저장이나 자동 로드를 실행하지 않는다.

---

## 8. 배치 라이트와 코드 라이트 분리

### 8.1 기존 문제

기존 구현은 `CLightManager`의 일반 라이트 전체를 대상으로 저장 및 삭제했다.

```text
Save
→ 일반 라이트 전체 저장

Clear
→ 일반 라이트 전체 제거

Load
→ 일반 라이트 전체 제거 후 JSON 내용 재생성
```

따라서 게임 코드에서 생성한 일반 라이트도 에디터의 `Load` 또는 `Clear`에 의해 제거될 수 있었다.

### 8.2 해결 방식

`CLight`에 배치 그룹 태그를 추가했다.

```cpp
std::string m_sPlacementGroup{};
```

관련 API:

```cpp
void Set_LightPlacementGroup(std::string sGroup);
const std::string& Get_LightPlacementGroup() const;
_bool Is_PlacementLight() const;
```

분류 방식:

```text
JSON 배치 라이트
→ PlacementGroup = "Level_Terrain"

코드에서 생성한 일반 라이트
→ PlacementGroup = ""

Effect Light
→ 기존 Effect Light Pool 사용
```

라이트 에디터는 현재 활성 그룹과 일치하는 라이트만 대상으로 다음 작업을 수행한다.

- 목록 표시
- 디버그 출력
- 생성
- 저장
- 로드
- 선택 삭제
- Clear

다른 배치 그룹과 코드에서 생성한 일반 라이트는 건드리지 않는다.

---

## 9. 그룹 단위 제거

`CLightManager`에 배치 그룹 단위 제거 API를 추가했다.

```cpp
size_t Remove_PlacementLightGroup(
    std::string_view sGroup);
```

동작 순서:

1. `m_LightHandleList` 순회
2. 유효한 `CLight` 확인
3. `PlacementGroup` 비교
4. 일치하는 핸들 수집
5. 기존 `Remove_Light()`로 제거

배치 그룹 제거는 매 프레임 실행되는 작업이 아니다.

라이트 수도 제한적이므로 별도 그룹별 핸들 컨테이너를 중복 관리하지 않고 필요할 때 전체 일반 라이트 목록을 순회한다.

이 방식은 핸들 등록 및 해제 누락으로 stale handle이 남는 위험을 줄인다.

---

## 10. 런타임 라이트 로더

관련 파일:

```text
Engine/Public/LightPlacementObject.h
Engine/Private/LightPlacementObject.cpp
```

### 10.1 초기화 데이터

```cpp
struct DESC : public CGameObject::GAMEOBJECT_DESC
{
    std::string sLightFileName{};
    std::string sPlacementGroup{};
    const LIGHT_PLACEMENT_FILE* pLightData{};
};
```

일반적인 사용 방식:

```cpp
CLightPlacementObject::DESC desc{};
desc.sObjectTag = "TerrainLightPlacement";
desc.sLightFileName = "Level_Terrain";
```

### 10.2 초기화 흐름

```text
JSON 읽기
→ 버전 확인
→ 라이트 데이터 검증
→ 타입별 CLight 생성
→ Alias 및 조명 속성 적용
→ PlacementGroup 태깅
→ 활성 그룹을 라이트 에디터에 통지
```

잘못된 데이터는 다음과 같이 보정한다.

- Direction 벡터 정규화
- 길이가 너무 짧은 Direction은 `{ 0, -1, 0 }` 사용
- Intensity 최소 `0`
- Range 최소 `0.02`
- Spot Outer Angle `0.1~75`
- Spot Inner Angle `0~Outer`

일부 라이트 생성에 실패하면 해당 생성 과정에서 이미 만든 라이트들을 제거하고 실패를 반환한다.

---

## 11. 런타임 로더와 에디터 자동 동기화

### 11.1 발생했던 문제

런타임 로더가 `Level_Lights` 그룹으로 라이트를 생성한 상태에서 GUI 프리셋을 `Level_Terrain`으로 변경하면 기존 라이트들이 에디터에서 사라졌다.

```text
로드된 라이트 그룹: Level_Lights
에디터 활성 그룹:   Level_Terrain
```

에디터는 활성 그룹만 표시하므로 기존 라이트가 필터링되었다.

또한 `Level_Terrain` 그룹에는 라이트가 없었기 때문에 Save 시 빈 JSON이 저장되었다.

당시 확인된 데이터:

```text
Level_Lights.json  → 라이트 6개
Level_Terrain.json → 라이트 0개
```

기존 데이터가 삭제된 것은 아니며 `Level_Lights.json`에 그대로 남아 있었다.

### 11.2 해결 방식

런타임 로더가 성공하면 해당 그룹을 에디터에 전달하도록 연결했다.

```text
CLightPlacementObject
→ CGameInstance
→ CLightManager
→ CLightPlacementEditor
→ SetActivePlacementGroup()
```

결과:

- GUI 콤보 자동 선택
- 파일명 자동 동기화
- 현재 로드된 라이트 그룹 즉시 표시
- 다른 빈 그룹에 저장하는 실수 방지

---

## 12. 런타임 로더 해제 방식

### 12.1 기존 문제

기존 로더는 생성 당시의 개별 라이트 핸들을 보관했다.

에디터에서 `Load`를 실행하면 기존 라이트가 제거되고 새로운 라이트가 생성되므로, 로더가 보관한 핸들이 오래된 핸들이 될 수 있었다.

```text
로더가 라이트 A 생성 및 핸들 보관
→ 에디터 Load
→ 라이트 A 제거
→ 새로운 라이트 B 생성
→ 로더는 삭제된 A 핸들만 보관
```

### 12.2 변경 후

로더 해제 시 개별 핸들이 아니라 자신의 배치 그룹을 기준으로 현재 라이트를 제거한다.

```text
CLightPlacementObject::Free()
→ 자신의 PlacementGroup 확인
→ Remove_PlacementLightGroup()
→ 현재 해당 그룹에 속한 라이트 전부 제거
```

에디터 재로드로 라이트 핸들이 교체되어도 현재 그룹의 라이트를 정상적으로 정리할 수 있다.

생성 도중 실패를 롤백하기 위한 임시 핸들 목록은 내부적으로 유지하지만 외부에는 노출하지 않는다.

---

## 13. Client Terrain 레벨 연결

`CLevelTerrain`에 런타임 로더 오브젝트를 배치했다.

관련 파일:

```text
Client/Private/LevelTerrain.cpp
```

현재 코드:

```cpp
CLightPlacementObject::DESC desc{};
desc.sObjectTag = "TerrainLightPlacement";
desc.sLightFileName = "Level_Terrain";

if (!CGameInstance::Get().AddGameObjectToLayer(
    ES_EngineProtoMajorType::PERMANENT,
    ES_EngineProtoGameObject::
        Prototype_GameObject_LightPlacement,
    "Layer_LightPlacement",
    &desc))
{
    return E_FAIL;
}
```

기존 하드코딩 Directional Light는 JSON 데이터와 중복되므로 제거했다.

---

## 14. Terrain JSON 데이터 이전

비어 있던 `Level_Terrain.json`에 기존 `Level_Lights.json`의 라이트 6개를 이전했다.

현재 상태:

```text
Level_Lights.json
→ 라이트 6개
→ 기존 데이터 백업

Level_Terrain.json
→ 라이트 6개
→ CLevelTerrain 런타임 사용
```

사용 경로:

```text
JUSIN_160_FINAL_TEAM_RESOURCE/json/Lights/Level_Terrain.json
```

확인된 데이터:

- 파일 버전: 1
- 라이트 개수: 6
- JSON 파싱 성공

---

## 15. 프로토타입 등록

엔진 공용 게임오브젝트 프로토타입에 런타임 로더를 등록했다.

```cpp
Prototype_GameObject_LightPlacement
```

`GameInstanceInitLoader`에서 다음 프로토타입을 `PERMANENT` 그룹에 등록한다.

```cpp
CLightPlacementObject::Create()
```

따라서 Client 레벨에서는 별도의 Client 전용 프로토타입 등록 없이 사용할 수 있다.

---

## 16. 현재 완료 범위

현재 라이트 배치 에디터의 핵심 기능은 완료된 상태다.

- Directional, Point, Spot Light 배치
- 기즈모 조작
- 실시간 속성 변경
- 별칭
- 타입별 디버그 형상
- 프리셋
- 파일명 입력 보호
- JSON 저장 및 로드
- 코드 라이트와 배치 라이트 분리
- 그룹 단위 수명 관리
- 런타임 로더
- Terrain 레벨 연동
- 로더와 에디터 자동 그룹 동기화

---

## 17. 추후 선택 작업

현재 필수 작업은 아니며 필요할 때 추가한다.

- 화면 마우스 피킹
- 선택 라이트 복제
- 목록 검색
- 라이트 타입 필터
- 선택 라이트 위치로 카메라 이동
- Undo/Redo
- 저장 후 수정 여부 표시
- 라이트별 Shadow On/Off
- 다중 선택 및 일괄 편집

`Level_CharlesRookwood`와 `Level_BossCharlesRookwood`는 현재 콤보 프리셋만 존재한다.

각 레벨에 다음 작업이 필요하다.

1. 해당 JSON 생성
2. 해당 레벨에 `CLightPlacementObject` 배치
3. 해당 레벨 파일명 지정
4. 레벨 진입, 재진입 및 해제 테스트

---

## 18. 최종 확인 항목

빌드 후 다음 항목을 확인한다.

1. Terrain 진입 시 라이트 6개가 생성되는지 확인
2. GUI 프리셋이 자동으로 `Level_Terrain`을 선택하는지 확인
3. Alias와 조명 속성을 변경하고 저장
4. Terrain 재진입 후 저장값 유지 확인
5. 반복 재진입 시 라이트 중복 여부 확인
6. Clear 실행 시 `Level_Terrain` 그룹만 제거되는지 확인
7. 코드에서 생성한 일반 라이트가 유지되는지 확인
8. 레벨 해제 시 `Level_Terrain` 그룹 라이트가 제거되는지 확인

---

## 19. 파일 및 빌드 주의사항

- 수정 파일은 UTF-8(BOM 없음), CRLF를 사용한다.
- `EngineSDK` 미러 헤더는 직접 수정하지 않는다.
- 엔진 빌드 후 빌드 이벤트를 통해 미러 헤더가 갱신된다.
- 배치 그룹 태깅은 라이트 생성 시 적용되므로 변경 후에는 레벨에 다시 진입해야 한다.
- 이 문서 작성 시점에는 정적 검사를 완료했지만 전체 프로젝트 빌드는 실행하지 않았다.
