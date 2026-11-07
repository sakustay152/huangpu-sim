# tools/make_source_doc.ps1 - generate docs/source bundle markdown
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File tools\make_source_doc.ps1
$ErrorActionPreference = 'Stop'
$root = 'D:\面向对象程序设计\HuangpuSim'
$outPath = Join-Path $root 'docs\源码汇编.md'

# ---- collect files: root build files, then include/src/main/tests/data ----
$files = @()
foreach ($n in @('README.md','build.bat','CMakeLists.txt','run_demo.bat')) {
    $p = Join-Path $root $n
    if (Test-Path -LiteralPath $p) { $files += Get-Item -LiteralPath $p }
}
foreach ($dir in @('include','src','main','tests','data')) {
    $files += Get-ChildItem -LiteralPath (Join-Path $root $dir) -Recurse -File | Sort-Object FullName
}

$lang = @{
    '.h'='cpp'; '.hpp'='cpp'; '.cpp'='cpp'; '.cc'='cpp'
    '.json'='json'; '.bat'='bat'; '.cmd'='bat'
    '.md'='markdown'; '.txt'='text'; '.csv'='text'
}

# ---- stats ----
$totalLines = 0
foreach ($f in $files) {
    $totalLines += ([System.IO.File]::ReadAllText($f.FullName) -split "`n").Count
}

$sb = [System.Text.StringBuilder]::new(4194304)
[void]$sb.AppendLine('# 《黄埔江上又一人》· 校园浮生录 —— 源码汇编')
[void]$sb.AppendLine()
[void]$sb.AppendLine('> 面向对象程序设计课程项目 · 模拟经营方向（类《北京浮生记》）')
[void]$sb.AppendLine('> C++17 · 不使用游戏引擎 · 仅 C++ 标准库 + 内置 nlohmann/json（third_party/，约 900KB 单头，不在本文档内）')
[void]$sb.AppendLine('> 生成时间：' + (Get-Date -Format 'yyyy-MM-dd HH:mm') + ' · 共 ' + $files.Count + ' 个文件 · 约 ' + $totalLines + ' 行')
[void]$sb.AppendLine('> 本文档由 tools/make_source_doc.ps1 从工程文件直接汇总生成，与磁盘内容逐字一致。')
[void]$sb.AppendLine()
[void]$sb.AppendLine('## 构建 / 运行 / 测试')
[void]$sb.AppendLine()
[void]$sb.AppendLine('```bat')
[void]$sb.AppendLine('REM 一键构建（MSVC 直编，无需 CMake / vcpkg）')
[void]$sb.AppendLine('build.bat')
[void]$sb.AppendLine('cd build')
[void]$sb.AppendLine('REM 游戏（控制台垂直切片）')
[void]$sb.AppendLine('huangpu_sim.exe')
[void]$sb.AppendLine('REM headless 模拟器（1000 局 x 3 策略，输出 CSV）')
[void]$sb.AppendLine('huangpu_simulate.exe --simulate 1000 --seed 42 --strategy all --out sim_result.csv')
[void]$sb.AppendLine('REM 规则测试（33 项断言）')
[void]$sb.AppendLine('huangpu_tests.exe')
[void]$sb.AppendLine('```')
[void]$sb.AppendLine()

# ---- table of contents ----
[void]$sb.AppendLine('## 文件清单（' + $files.Count + ' 个）')
[void]$sb.AppendLine()
foreach ($f in $files) {
    $rel = $f.FullName.Substring($root.Length + 1)
    [void]$sb.AppendLine('- `' + $rel + '`')
}
[void]$sb.AppendLine()

# ---- per-file content ----
foreach ($f in $files) {
    $rel = $f.FullName.Substring($root.Length + 1)
    $ext = $f.Extension.ToLower()
    $code = if ($lang.ContainsKey($ext)) { $lang[$ext] } else { 'text' }
    [void]$sb.AppendLine('## `' + $rel + '`')
    [void]$sb.AppendLine()
    [void]$sb.AppendLine('````' + $code)
    [void]$sb.AppendLine([System.IO.File]::ReadAllText($f.FullName).TrimEnd())
    [void]$sb.AppendLine('````')
    [void]$sb.AppendLine()
}

# ---- write UTF-8 (no BOM) ----
$utf8 = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($outPath, $sb.ToString(), $utf8)
Write-Host ('OK: ' + $files.Count + ' files, ~' + $totalLines + ' lines -> ' + $outPath)
Write-Host ('Size: ' + (Get-Item -LiteralPath $outPath).Length + ' bytes')
