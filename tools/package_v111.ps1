# tools/package_v111.ps1 - stage and zip the v1.1.1 release bundle
$ErrorActionPreference = 'Stop'
$root = 'D:\面向对象程序设计\HuangpuSim'
$stage = Join-Path $root 'build-rel\stage_v111'
$zipPath = Join-Path $root 'huangpu-sim-v1.1.1-win64.zip'

if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stage 'data') | Out-Null

foreach ($exe in @('huangpu_sim.exe', 'huangpu_simulate.exe', 'huangpu_tests.exe')) {
    Copy-Item -LiteralPath (Join-Path $root "build-rel\$exe") -Destination $stage
}
foreach ($json in (Get-ChildItem -LiteralPath (Join-Path $root 'data') -Filter *.json)) {
    Copy-Item -LiteralPath $json.FullName -Destination (Join-Path $stage 'data')
}
Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination $stage
Copy-Item -LiteralPath (Join-Path $root 'run_demo.bat') -Destination $stage
Copy-Item -LiteralPath (Join-Path $root 'tools\release-notes\使用说明.txt') -Destination $stage

if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($stage, $zipPath)
Remove-Item -LiteralPath $stage -Recurse -Force

$z = Get-Item -LiteralPath $zipPath
Write-Host ('ZIP-OK: ' + $z.Name + '  ' + [math]::Round($z.Length / 1KB) + ' KB')
