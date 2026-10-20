# Mixer: ambiente Windows para o codigo-fonte do Blender.
[CmdletBinding()]
param(
    [ValidateSet('Dependencies', 'Configure', 'Build', 'Smoke')]
    [string]$Action = 'Configure',
    [ValidatePattern('^[M-Zm-z]$')]
    [string]$Drive = 'M',
    [ValidateRange(1, 64)]
    [int]$Jobs = 2
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$driveName = $Drive.ToUpperInvariant() + ':'
$mappedHere = $false

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program terminou com codigo $LASTEXITCODE."
    }
}

try {
    # Nao reutilizar silenciosamente uma unidade que aponte para outro local.
    if (Test-Path ($driveName + '\')) {
        throw "A unidade $driveName ja esta ocupada. Escolha outra com -Drive e use a mesma nas proximas compilacoes."
    }
    Invoke-Checked 'subst.exe' @($driveName, $repoRoot)
    $mappedHere = $true
    $sourceDir = $driveName + '\'
    $buildDir = $sourceDir + 'build\windows'
    Push-Location $sourceDir
    try {
        if ($Action -eq 'Dependencies') {
            Invoke-Checked 'git' @('lfs', 'install', '--local')
            # O espelho GitHub oficial nao hospeda os objetos LFS do Blender.
            # Obter apenas os assets de compilacao, deixando testes volumosos para depois.
            Invoke-Checked 'git' @('-c', 'lfs.url=https://projects.blender.org/blender/blender.git/info/lfs', 'lfs', 'pull', '--exclude=tests/**', 'upstream')
            Invoke-Checked 'git' @('config', '--local', 'submodule.lib/windows_x64.update', 'checkout')
            $oldSmudge = $env:GIT_LFS_SKIP_SMUDGE
            try {
                $env:GIT_LFS_SKIP_SMUDGE = '1'
                Invoke-Checked 'git' @('submodule', 'update', '--init', '--progress', 'lib/windows_x64')
            } finally {
                $env:GIT_LFS_SKIP_SMUDGE = $oldSmudge
            }
            Invoke-Checked 'git' @('-C', 'lib/windows_x64', 'lfs', 'pull')
        } else {
            $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
            if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer/vswhere nao encontrado.' }
            $vsPath = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
            if (-not $vsPath) { throw 'Visual Studio 2022 com ferramentas C++ x64 nao encontrado.' }
            $cmake = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
            if (-not (Test-Path -LiteralPath $cmake)) {
                $cmake = (Get-Command cmake -ErrorAction Stop).Source
            }
            if ($Action -eq 'Configure') {
                if (-not (Test-Path -LiteralPath 'lib/windows_x64/.git')) { throw 'Execute primeiro -Action Dependencies.' }
                Invoke-Checked $cmake @('-S', $sourceDir, '-B', $buildDir, '-G', 'Visual Studio 17 2022', '-A', 'x64', '-DWITH_WINDOWS_BUNDLE_CRT=ON')
            } elseif ($Action -eq 'Build') {
                if (-not (Test-Path -LiteralPath ($buildDir + '\CMakeCache.txt'))) { throw 'Execute primeiro -Action Configure.' }
                Invoke-Checked $cmake @('--build', $buildDir, '--config', 'Release', '--target', 'INSTALL', '--parallel', "$Jobs")
            } else {
                $blender = $buildDir + '\bin\Release\blender.exe'
                if (-not (Test-Path -LiteralPath $blender)) { throw 'Executavel ausente. Execute -Action Build primeiro.' }
                Invoke-Checked $blender @('--background', '--factory-startup', '--python-exit-code', '1', '--python-expr', "import bpy; assert bpy.app.version[:2] == (5, 2); print('MIXER_SMOKE_OK', bpy.app.version_string)")
            }
        }
    } finally {
        Pop-Location
    }
} finally {
    if ($mappedHere) { & subst.exe $driveName /D }
}
