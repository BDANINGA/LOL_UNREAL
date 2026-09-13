param(
    [ValidateSet('status', 'exec')][string]$Action = 'status',
    [string]$ScriptFile,
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.7'
)
$ErrorActionPreference = 'Stop'
$pythonExe = Join-Path $EngineRoot 'Engine/Binaries/ThirdParty/Python3/Win64/python.exe'
$bridgeArgs = @('-B', (Join-Path $PSScriptRoot 'unreal_bridge.py'), $Action, '--engine', $EngineRoot)
if ($ScriptFile) { $bridgeArgs += @('--file', $ScriptFile) }
& $pythonExe @bridgeArgs
exit $LASTEXITCODE
