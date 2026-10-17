# UI 아이콘 원본

## 언리얼 콘텐츠 브라우저
- `/Game/UI/Minimap/Icons/Blue`: 블루 타워·억제기·넥서스
- `/Game/UI/Minimap/Icons/Red`: 레드 타워·억제기·넥서스
- `/Game/UI/Minimap/Icons/Neutral`: 회색 원본 3종
- `/Game/UI/HUD/SkillLevel`: 스킬 레벨 활성·비활성 표시

미니맵 텍스처는 64×64, 스킬 레벨 표시는 9×9 투명 PNG입니다.
팀 색상이 이미 들어간 텍스처를 사용할 때 위젯 Tint는 흰색으로 두면 됩니다.

## 원본 파일 구성
- `Minimap/Blue`, `Minimap/Red`, `Minimap/Neutral`: 가져오기용 PNG
- `Minimap/HighResolution`: 팀 색상 편집 고해상도 결과, 언리얼에는 가져오지 않음
- `SkillLevel`: 스킬 레벨 PNG
- 각 폴더의 `*_preview.png`: 확인용 미리보기, 언리얼에는 가져오지 않음
- `import_manifest.json`: 원본과 언리얼 에셋 경로 대응

## 출처와 편집
회색 건물 아이콘과 스킬 레벨 표시는 로컬 League of Legends `UI.wad.client`에서 추출했습니다.
건물 원본: `assets/ux/minimap/icons/{tower,inhibitor,nexus}.tex`
스킬 표시: `assets/ux/lol/clarity_hudatlas.tex`, FullPip (1011,258)-(1020,267), EmptyPip (999,258)-(1008,267).
블루·레드 아이콘은 내장 image_gen으로 색상을 편집한 버전입니다. 원본과 형태·명암에 미세한 차이가 있습니다.
고해상도 편집 결과는 원본 아이콘의 표시 영역에 맞춰 64×64로 조정했습니다.

## 가져오기
프로젝트를 Unreal Editor에서 연 뒤 저장소 루트에서 실행:

```powershell
./Tools/unreal.ps1 exec -ScriptFile ./Tools/import_ui_icons.py
```

기본 실행은 기존 에셋을 덮어쓰지 않습니다. UI 압축 설정, UI 텍스처 그룹, 밉맵 없음, sRGB 및 Clamp 주소 방식을 사용합니다.
에셋만 추가하며 기존 HUD나 미니맵 위젯에 자동 연결하지 않습니다.
