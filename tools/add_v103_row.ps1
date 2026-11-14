# tools/add_v103_row.ps1 - add v1.03 row to README version matrix
$ErrorActionPreference = 'Stop'
$f = 'README.md'
$t = [IO.File]::ReadAllText($f, [Text.UTF8Encoding]::new($false))

$old = '| **v1.02 · 炒股完整版** | 标签 `v1.02`，即 `main` | v1.02 全部内容 + K 线图 + 鼠标点击交易 + 股力成长系统（学股市知识），测试 121 项 | Releases 下载 zip（含 exe），或直接克隆 `main` |'
$new = $old + [char]10 + '| **v1.03 · v2 时间系统版** | 标签 `v1.03`，即 `main` | v1.02 全部内容 + 24 小时制 / 昏迷 / 睡觉 / 失眠 + 社交三选一选项池 + 猝死隐藏结局，测试 121 项 | Releases 下载 zip（含 exe），或直接克隆 `main` |'

if ($t.Contains($old)) {
    $t = $t.Replace($old, $new)
    [IO.File]::WriteAllText($f, $t, [Text.UTF8Encoding]::new($false))
    Write-Host 'ROW-ADDED'
} elseif ($t.Contains('v1.03')) {
    Write-Host 'ROW-ALREADY-PRESENT'
} else {
    # 表格行措辞可能被改过：退化为在"版本（两个可玩的发行版）"标题后整段追加
    $anchor = '或直接克隆 `main` |'
    $idx = $t.LastIndexOf($anchor)
    if ($idx -lt 0) { Write-Host 'ANCHOR-NOT-FOUND'; exit 1 }
    $row = [char]10 + '| **v1.03 · v2 时间系统版** | 标签 `v1.03`，即 `main` | v1.02 全部内容 + 24 小时制 / 昏迷 / 睡觉 / 失眠 + 社交三选一选项池 + 猝死隐藏结局，测试 121 项 | Releases 下载 zip（含 exe），或直接克隆 `main` |'
    $t = $t.Insert($idx + $anchor.Length, $row)
    [IO.File]::WriteAllText($f, $t, [Text.UTF8Encoding]::new($false))
    Write-Host 'ROW-APPENDED-FALLBACK'
}
