# 마법진 귀환 연출

기존 기본 파란 원 나이아가라 대신 `ULOL_RecallVisualComponent`가 귀환 연출을 담당합니다.
서버의 기존 귀환 시작/종료 멀티캐스트에 연결하며, 귀환 시간(기본 8초)과 이동·피격 취소 판정은 유지합니다.

## 연출
- 시작: 0.65초에 걸쳐 바깥으로 펼쳐지는 마법진, 얇은 이중 원, 24개 문양, 역방향 회전 기하학 패턴.
- 진행: 청록색 빛줄기 3겹과 8개의 작은 빛 조각. 바닥 문양이 챔피언을 가리지 않도록 중심부를 비웁니다.
- 후반: 충전량에 따라 빛이 강해지고 두 번째 문양이 상승합니다.
- 완료: 도착 지점에 0.45초간 퍼지며 사라지는 문양.
- 취소: 취소 지점에서 0.18초간 사라집니다.
- 사망/소유 액터 종료: 즉시 정리. 시야로 숨겨진 챔피언의 연출도 숨깁니다.
- 비활성 상태에서는 컴포넌트 틱이 꺼지고, 전용 서버에서는 시각 요소를 만들지 않습니다.

## 구성
- `LOL_UNREAL/Source/LOL_UNREAL/Private/Component/LOL_RecallVisualComponent.cpp`: 배치, 시간별 연출, 정리.
- `/Game/VFX/Recall/M_RecallSigil`: 마법진.
- `/Game/VFX/Recall/M_RecallColumn`: 부드러운 세로 빛줄기.
- `/Game/VFX/Recall/M_RecallMote`: 작은 빛 조각.
- `create_recall_materials.py`: 위 세 머티리얼을 재생성하거나 조정하는 에디터 스크립트.

외부 텍스처나 새 플러그인 없이 엔진 기본 평면과 절차적 머티리얼을 사용합니다.
기존 `/Game/UI/recall/StaticMeshes/ns_recall` 에셋은 보존하며 현재 귀환 경로에서는 사용하지 않습니다.

## 검증
에디터 및 Win64 Development 게임 타깃 빌드 통과.
`prepare_recall_test.py`로 저장하지 않는 테스트 맵을 만들고, PIE에서 `runpy.run_path`로
`test_recall_visual.py`를 실행합니다. 실제 챔피언 RPC로 다음을 검사합니다:

- 시작/중복 요청 차단, 이동·피격 취소, 사망 시 제거
- 페이드 중 재시작, 시야 숨김/복귀
- 8초 이전 순간이동 없음, 완료 후 귀환 지점 복귀 및 컴포넌트 정리
- 비활성 상태의 틱 중단

결과와 화면 캡처는 `LOL_UNREAL/Saved/Validation/Recall`에 기록합니다.
테스트 바닥 머티리얼 `/Game/Developers/RecallPreview/M_RecallTestFloor`는 저장하지 않는 임시 자료이며 테스트 후 제거합니다.
외부 PC 다중 접속 시험은 별도입니다. 이전에 만들어 둔 패키지에는 재패키징 전까지 반영되지 않습니다.
