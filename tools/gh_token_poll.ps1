# tools/gh_token_poll.ps1 - poll GitHub device flow for the OAuth token
# Run from the project root: powershell -NoProfile -ExecutionPolicy Bypass -File tools\gh_token_poll.ps1
$ErrorActionPreference = 'Stop'
$raw = (Get-Content 'tools\device_code.json' -Raw)
$raw = $raw.Trim().Trim('"')
$m = [regex]::Match($raw, 'device_code=([a-f0-9]+)')
if (-not $m.Success) { Write-Host 'NO-DEVICE-CODE'; exit 1 }
$dc = $m.Groups[1].Value

$token = $null
for ($i = 0; $i -lt 24 -and -not $token; $i++) {
    Start-Sleep -Seconds 5
    try {
        $r = Invoke-RestMethod -Method Post -Uri 'https://github.com/login/oauth/access_token' `
            -ContentType 'application/x-www-form-urlencoded' `
            -Headers @{ Accept = 'application/json' } `
            -Body ('client_id=178c6fc778ccc68e1d6a&device_code=' + $dc + '&grant_type=urn:ietf:params:oauth:grant-type:device_code') `
            -TimeoutSec 20
    } catch { continue }
    if ($r.access_token) { $token = $r.access_token; break }
    if ($r.error -and $r.error -ne 'authorization_pending') { Write-Host ('ERROR: ' + $r.error); break }
    Write-Host ('pending ' + $i)
}

if ($token) {
    Set-Content -LiteralPath 'tools\.gh_token' -Value $token -NoNewline -Encoding ascii
    Write-Host 'TOKEN-OK (saved to tools/.gh_token, not displayed)'
} else {
    Write-Host 'TOKEN-FAIL'
    exit 1
}
