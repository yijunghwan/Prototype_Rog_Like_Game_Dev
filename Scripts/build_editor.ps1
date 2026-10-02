[CmdletBinding()]
param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'Action_RogueLike.uproject'
$buildScript = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$logDirectory = Join-Path $projectRoot 'Saved\Logs'

foreach ($requiredFile in @($projectFile, $buildScript)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "Required build file not found: $requiredFile"
    }
}

# Scope crash-dialog suppression to this launcher and its child processes.
# Windows-wide error-reporting settings and Unreal Editor are not changed.
# Child processes inherit SetErrorMode unless they explicitly override it:
# https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-seterrormode
if (-not ('ARBuild.NativeErrorMode' -as [type])) {
    Add-Type -TypeDefinition @'
using System.Runtime.InteropServices;
namespace ARBuild
{
    public static class NativeErrorMode
    {
        [DllImport("kernel32.dll")]
        public static extern uint GetErrorMode();
        [DllImport("kernel32.dll")]
        public static extern uint SetErrorMode(uint mode);
    }
}
'@
}

$previousErrorMode = [ARBuild.NativeErrorMode]::GetErrorMode()
# SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX.
[void][ARBuild.NativeErrorMode]::SetErrorMode($previousErrorMode -bor 0x0003)
$transcriptStarted = $false
$buildExitCode = 1
try {
    [void](New-Item -ItemType Directory -Path $logDirectory -Force)
    $consoleLog = Join-Path $logDirectory 'BuildEditor-console.log'
    $buildLog = Join-Path $logDirectory 'BuildEditor.log'
    [void](Start-Transcript -LiteralPath $consoleLog -Append)
    $transcriptStarted = $true

    & $buildScript Action_RogueLikeEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReload "-Log=$buildLog"
    $buildExitCode = $LASTEXITCODE
    if ($buildExitCode -ne 0) {
        Write-Warning "Editor build failed (exit code $buildExitCode). See $consoleLog and $buildLog (if generated)."
    }
}
finally {
    try {
        if ($transcriptStarted) {
            [void](Stop-Transcript)
        }
    }
    finally {
        [void][ARBuild.NativeErrorMode]::SetErrorMode($previousErrorMode)
    }
}

# Preserve failures for callers; suppressing a dialog does not fix a crash.
exit $buildExitCode
