# 이 대화에서 Unreal Editor 제어

이 프로젝트는 Unreal Engine 5.7의 PythonScriptPlugin과 EditorScriptingUtilities를 사용합니다.
Codex가 로컬 스크립트를 실행하므로 별도 OpenAI API 키나 MCP 서버 설치는 필요하지 않습니다.

1. `LOL_UNREAL/LOL_UNREAL.uproject`를 Unreal Editor로 엽니다.
2. Codex에서 `Tools/unreal.ps1 status`로 연결을 확인합니다.
3. 이후 이 대화에 원하는 작업을 한국어로 요청합니다.

## 에이전트 실행 방법

PowerShell에서 프로젝트 루트를 작업 폴더로 사용합니다.

```powershell
./Tools/unreal.ps1 status
./Tools/unreal.ps1 exec -ScriptFile ./Tools/my_editor_task.py
```

`exec`는 UTF-8 Python 파일을 열린 에디터에서 실행합니다. 스크립트는 `import unreal`로 시작합니다.
에디터가 제공하는 Python API 범위에서 액터 조회/배치, 속성 변경, 에셋 조회/편집을 수행할 수 있습니다.
블루프린트 그래프 편집 등 Python API에 노출되지 않은 작업은 추가 도구나 C++ 구현이 필요합니다.
C++ 소스 변경은 직접 파일 편집 후 해당 변경에 맞는 빌드/에디터 재시작이 필요합니다.

연결은 127.0.0.1과 로컬 전용 멀티캐스트(TTL 0)를 사용합니다. 이 기능은 같은 PC의 프로세스가
에디터 Python을 실행할 수 있게 합니다. 사용을 중지하려면 DefaultEngine.ini의
`bRemoteExecution=False`로 변경하고 에디터를 다시 시작합니다.
다른 폴더에서 실행한 같은 이름의 프로젝트에는 명령을 보내지 않도록 전체 프로젝트 경로를 검사합니다.
에셋 저장은 요청된 작업 범위에 한해 명시적으로 수행하고, 연결 확인만으로 에셋을 저장하지 않습니다.

연결 실패 시 에디터 실행 여부와 프로젝트 설정 > Python > Enable Remote Execution을 확인합니다.
명령 실행 중 응답이 끊긴 경우 편집이 이미 적용됐을 수 있으므로 같은 변경을 자동 재전송하지 말고 먼저 상태를 확인합니다.
