param(
    [ValidateSet("debug", "release")]
    [string]$config = "release",
    [string]$prefix = "D:\Qt\Qt5.14.2\5.14.2\mingw73_64",

    [switch]$configure,   # 仅配置
    [switch]$build,       # 配置+构建
    [switch]$rebuild,     # 清理+配置+构建
    [switch]$clean,       # 仅清理
    [switch]$test,        # 运行测试
    [switch]$publish,     # 发布模式（不生成控制台可执行文件）

    [Alias("h")]
    [switch]$help
)

# ---------- 帮助（File Code: UTF-8 with BOM） ----------
if ($PSBoundParameters.ContainsKey('help') -or $PSBoundParameters.ContainsKey('h')) {
    Write-Host @"
USAGE: .\build.ps1 [-config <debug|release>] [-configure] [-build] [-rebuild] [-clean] [-publish] [-help|-h]

PARAMS:
  -config       指定构建类型：debug 或 release （默认release）
  -prefix       指定额外搜索库路径

SWITCHES:
  -configure    仅生成 Ninja 构建系统，不编译
  -build        配置并编译（默认 release）
  -rebuild      先清理再配置+编译
  -clean        仅删除 build/<config>/ 目录
  -test         运行测试程序
  -publish      指定发布状态：1 发布模式 | 0 开发者模式（有控制台）

EXAMPLES:
  配置 Release        : .\build.ps1 -configure
  配置 Debug          : .\build.ps1 -configure -config debug
  编译 Debug          : .\build.ps1 -build -config debug
  重新编译 Release    : .\build.ps1 -rebuild -config release
  运行测试            : .\build.ps1 -test
  清理                : .\build.ps1 -clean
"@ -ForegroundColor Cyan
    exit 0
}

# 发布模式（强制 release）
if ($publish) {
    $config = "release"
    $IsPublish = $true
} else {
    $IsPublish = $false
}

# 固定路径
$SrcDir   = $PSScriptRoot                           # src 目录
$BuildDir = Join-Path $SrcDir "build\$config"       # 构建目录

# --- 工具检查 ---
function Test-Tools {
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw "cmake not found" }
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) { throw "ninja not found" }
}

# --- 清理 ---
function Clean-Build {
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
        Write-Host ">>> Cleaned $BuildDir" -ForegroundColor Yellow
    }
}

# --- 配置 ---
function Configure-Build {
    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
    }

    $CMakeArgs = @(
        "-S", $SrcDir
        "-B", $BuildDir
        "-G", "Ninja"
        "-DCMAKE_BUILD_TYPE=$config"
        "-DCMAKE_PREFIX_PATH=$prefix"
    )

    if ($IsPublish) {
        # 可追加发布专用选项，例如
        $CMakeArgs += "-Dpublish=1"
    }

    Write-Host ">>> Configuring $config ..." -ForegroundColor Cyan
    cmake @CMakeArgs
    if ($LASTEXITCODE -ne 0) { throw "Configuration failed" }
}

# --- 构建 ---
function Build-Project {
    Write-Host ">>> Building $config ..." -ForegroundColor Green
    cmake --build $BuildDir --config $config
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
}

# --- 运行测试 ---
function Run-Tests {
    Write-Host ">>> Running tests ..." -ForegroundColor Magenta
    $TestExes = Get-ChildItem -Path $BuildDir -Filter "test_*.exe" -File
    if ($TestExes.Count -eq 0) {
        Write-Host "No test executables found in $BuildDir" -ForegroundColor Yellow
        return
    }
    
    $FailedTests = 0
    foreach ($TestExe in $TestExes) {
        Write-Host "  Running $($TestExe.Name) ..." -ForegroundColor Cyan
        & $TestExe.FullName
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  FAILED: $($TestExe.Name)" -ForegroundColor Red
            $FailedTests++
        } else {
            Write-Host "  PASSED: $($TestExe.Name)" -ForegroundColor Green
        }
    }
    
    if ($FailedTests -gt 0) {
        throw "$FailedTests test(s) failed"
    }
}

# --- 主流程 ---
try {
    # 仅清理
    if ($clean) {
        Clean-Build
        exit 0
    }

    Test-Tools

    # 重建
    if ($rebuild) {
        Clean-Build
    }

    # 配置阶段（configure / build / rebuild 都需要）
    if ($configure -or $build -or $rebuild) {
        Configure-Build
    }

    # 构建阶段（build / rebuild 才需要）
    if ($build -or $rebuild) {
        Build-Project
    }

    # 测试阶段
    if ($test) {
        Run-Tests
    }

    Write-Host ">>> Done." -ForegroundColor Green
}
catch {
    Write-Error $_
    exit 1
}
