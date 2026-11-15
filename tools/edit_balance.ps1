# tools/edit_balance.ps1 - insert affinityDecay + weather keys into balance.json
$ErrorActionPreference = 'Stop'
$f = 'data/balance.json'
$t = [IO.File]::ReadAllText($f, [Text.UTF8Encoding]::new($false))

$q = [char]34
$nl = [char]10
$old1 = '  ' + $q + 'socialHours' + $q + ': 2,'
$new1 = $old1 + $nl + '  ' + $q + 'affinityDecay' + $q + ': 2,'
if (-not $t.Contains($old1)) { Write-Host 'ANCHOR1-NOT-FOUND'; exit 1 }
$t = $t.Replace($old1, $new1)

$old2 = '  ' + $q + 'insomniaPenaltyHours' + $q + ': 2,'
$new2 = $old2 + $nl + $nl + '  ' + $q + 'rainChance' + $q + ': 0.30,' + $nl +
        '  ' + $q + 'rainMoveStaminaExtra' + $q + ': 3,' + $nl +
        '  ' + $q + 'umbrellaRainFactor' + $q + ': 1.5,'
if (-not $t.Contains($old2)) { Write-Host 'ANCHOR2-NOT-FOUND'; exit 1 }
$t = $t.Replace($old2, $new2)

[IO.File]::WriteAllText($f, $t, [Text.UTF8Encoding]::new($false))
Write-Host 'BALANCE-OK'
