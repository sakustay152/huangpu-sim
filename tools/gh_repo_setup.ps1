# tools/gh_repo_setup.ps1 - create the repo and print owner info (token never echoed)
$ErrorActionPreference = 'Stop'
$token = (Get-Content 'tools\.gh_token' -Raw).Trim()
if (-not $token) { Write-Host 'NO-TOKEN'; exit 1 }
$headers = @{
    Authorization = 'token ' + $token
    Accept        = 'application/vnd.github+json'
    'User-Agent'  = 'huangpu-sim-setup'
}

# 1. who am I
$user = Invoke-RestMethod -Uri 'https://api.github.com/user' -Headers $headers -TimeoutSec 30
Write-Host ('LOGIN: ' + $user.login)
Write-Host ('USER_ID: ' + $user.id)

# 2. create repo (public, empty, no auto-init: we push our own history)
$body = @{
    name        = 'huangpu-sim'
    description = 'The Rise and Fall on the Huangpu River - Campus Life Chronicles: OOP course project (C++17 business simulation, inspired by Beijing Fushengji)'
    private     = $false
    auto_init   = $false
} | ConvertTo-Json
try {
    $repo = Invoke-RestMethod -Method Post -Uri 'https://api.github.com/user/repos' -Headers $headers -Body $body -ContentType 'application/json' -TimeoutSec 30
    Write-Host ('REPO_CREATED: ' + $repo.full_name)
    Write-Host ('REPO_URL: ' + $repo.html_url)
} catch {
    $msg = $_.ErrorDetails.Message
    if ($msg -and $msg -match 'already exists') {
        Write-Host 'REPO_EXISTS: ' + $user.login + '/huangpu-sim'
    } else {
        Write-Host ('REPO_FAIL: ' + $msg)
        exit 1
    }
}
