param([string]$BuildDir = 'build')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $version = (Get-Content VERSION -Raw).Trim()
    if ($version -notmatch '^\d+\.\d+\.\d+-rc\.\d+$') { throw 'Only candidate packaging is enabled' }
    if ($env:GITHUB_REF_TYPE -eq 'tag' -and $env:GITHUB_REF_NAME -ne "v$version") { throw 'Tag must match VERSION' }
    $commit = (git rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0) { throw 'Cannot identify source commit' }
    if (git status --porcelain --untracked-files=normal) { throw 'Source tree must be clean before packaging' }
    $exe = Join-Path $BuildDir 'Release/Calculator.exe'
    if (!(Test-Path $exe)) { throw "Missing executable: $exe" }
    $metadata = (Get-Item $exe).VersionInfo
    if ($metadata.FileVersion -ne $version -or $metadata.ProductVersion -ne $version) { throw 'Executable version mismatch' }
    $name = "Calculator-$version-windows-x64"
    New-Item -ItemType Directory -Force dist | Out-Null
    $stage = Join-Path 'dist' $name
    if (Test-Path $stage) { throw "Refusing to overwrite package stage: $stage" }
    New-Item -ItemType Directory $stage | Out-Null
    Copy-Item $exe $stage
    Copy-Item README.md,CHANGELOG.md,LICENSE $stage
    Copy-Item docs (Join-Path $stage 'docs') -Recurse
    $testLog = Join-Path $BuildDir 'Testing/Temporary/LastTest.log'
    if (!(Test-Path $testLog)) { throw 'Required test log missing; run ctest first' }
    Copy-Item $testLog (Join-Path $stage 'TEST-RESULTS.txt')
    $compiler = Get-ChildItem $BuildDir -Recurse -Filter CMakeCXXCompiler.cmake | Select-Object -First 1
    if (!$compiler) { throw 'Compiler metadata not found' }
    $toolchain = Get-Content $compiler.FullName | Where-Object { $_ -match 'CMAKE_CXX_COMPILER(_ID|_VERSION)? ' }
    # Capture all native output before selecting a line, avoiding a closed pipe
    # and a misleading nonzero LASTEXITCODE in the Actions PowerShell wrapper.
    $cmakeVersionLines = & cmake --version
    if ($LASTEXITCODE -ne 0) { throw 'Cannot capture CMake version' }
    $info = [ordered]@{
        version=$version; sourceCommit=$commit; repository='Denberg28/Calculator'; architecture='x64';
        releaseTier='test candidate'; signed=$false; runtime='MSVC static'; compiler=$toolchain;
        os=[Environment]::OSVersion.VersionString; cmake=$cmakeVersionLines[0];
        workflowRun=$env:GITHUB_RUN_ID; buildUtc=[DateTime]::UtcNow.ToString('o');
        automatedTests='ctest passed before packaging; package UI gate required before publication';
        desktopValidation='NOT TESTED: manual Windows 10/11, DPI, physical keyboard, clipboard, accessibility';
        executableSha256=(Get-FileHash $exe -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    $info | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $stage 'BUILD-INFO.json') -Encoding UTF8
    $zip = Join-Path 'dist' "$name.zip"
    if (Test-Path $zip) { throw "Refusing to replace package: $zip" }
    Compress-Archive -Path "$stage/*" -DestinationPath $zip
    $validation = Join-Path 'dist' 'package validation with spaces'
    if (Test-Path $validation) { throw 'Package validation directory already exists' }
    Expand-Archive -Path $zip -DestinationPath $validation
    if ((Get-FileHash (Join-Path $validation 'Calculator.exe')).Hash -ne (Get-FileHash $exe).Hash) { throw 'Packaged executable checksum mismatch' }
    & "$root/tests/windows-ui.ps1" -Exe (Join-Path $validation 'Calculator.exe')
    $checksum = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLowerInvariant()
    "$checksum  $name.zip" | Set-Content 'dist/SHA256SUMS.txt' -Encoding ASCII
    [ordered]@{status='PASS';sourceCommit=$commit;version=$version;zipSha256=$checksum;packageUi='PASS';extractionPathWithSpaces='PASS'} |
        ConvertTo-Json | Set-Content 'dist/package-build.json' -Encoding UTF8
    Write-Output "PASS: $zip, version, metadata, extraction, checksum and packaged Windows UI gate"
} finally { Pop-Location }

