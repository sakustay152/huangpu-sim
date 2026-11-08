# tools/gh_make_releases.ps1 - create two GitHub releases for huangpu-sim
# Run from project root: powershell -NoProfile -ExecutionPolicy Bypass -File tools\gh_make_releases.ps1
$ErrorActionPreference = 'Stop'
$token = (Get-Content 'tools\.gh_token' -Raw).Trim()
$headers = @{
    Authorization = 'token ' + $token
    Accept        = 'application/vnd.github+json'
    'User-Agent'  = 'huangpu-sim-release'
}

function New-Release($tag, $name, $body) {
    $payload = @{
        tag_name = $tag
        name     = $name
        body     = $body
    } | ConvertTo-Json -Depth 3
    try {
        $r = Invoke-RestMethod -Method Post -Uri 'https://api.github.com/repos/sakustay152/huangpu-sim/releases' `
            -Headers $headers -Body ([System.Text.Encoding]::UTF8.GetBytes($payload)) -ContentType 'application/json; charset=utf-8' -TimeoutSec 30
        Write-Host ('RELEASE-OK: ' + $r.tag_name + ' -> ' + $r.html_url)
    } catch {
        $msg = $_.ErrorDetails.Message
        if ($msg -and $msg -match 'already_exists') { Write-Host ('RELEASE-EXISTS: ' + $tag) }
        else { Write-Host ('RELEASE-FAIL: ' + $tag + ' :: ' + $msg); exit 1 }
    }
}

New-Release 'v1.0-no-stock' 'v1.0 · 无炒股版（纯倒卖经营）' @'
对应提交 d5fbf33：不含炒股模拟器的经典版本。

内容
- 16 个教学周 + 第 17/18 周考试周（对齐真实上海大学校历，共 18 周）
- 6 地点 × 12 商品低买高卖、每周限购、地点差价/趋势/季节/事件冲击
- 5 固定节点（开学/期中第 8 周/双十一第 10 周/考试周 17-18）+ 10 随机事件 + 3 条 NPC 小线
- 情报系统（室友每周一条，80% 准确）、打工/学习/社交三线经营
- 3 种结局：站稳脚跟 / 校园合伙人（隐藏）/ 账单之外
- 规则测试 33 项、headless 模拟器 1000 局校准

适合：想体验纯粹"低买高卖 + 学业 + 社交"经营的玩家。
获取：本页下载 Source code (zip)，或 git checkout v1.0-no-stock
'@

New-Release 'v1.01-stock' 'v1.01 · 含炒股模拟器 + 破产结局' @'
对应 main 分支：v1.0 的严格超集。

在 v1.0 基础上新增
- 炒股模拟器（灵感：bilibili 炒外汇模拟器）：虚构标的「浦江文旅（SHU）」，
  ASCII 实时走势图，价格随每次行动 / 周日结算随机走
- 体力系统：每次买卖 -15，休息 +40 / 周结算 +30；体力不足禁止操作
- 新手被收割：持仓时下跌概率 62%（体力 <30 再 +15%），另有黑天鹅暴跌 / 天降横财
- 杠杆融资 1x/2x/5x，负债按周计息 5%；净值 ≤ 0 强制平仓（爆仓），钱包可为负
- 新结局《破产清算》（priority 0）：欠债 ≥ ¥3,000 立即触发，游戏提前终止
- 规则测试 33 → 51 项（杠杆仓位/爆仓/破产 flag/存档往返等）

获取：本页下载 Source code (zip)，或直接克隆 main 分支
'@

Write-Host 'ALL-DONE'
