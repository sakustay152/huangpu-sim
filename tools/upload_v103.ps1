# tools/upload_v103.ps1 - extract release id and upload the v1.03 zip asset
$ErrorActionPreference = 'Stop'
$raw = [IO.File]::ReadAllText('tools/rel_resp.json', [Text.UTF8Encoding]::new($false))
$m = [regex]::Match($raw, '"id"\s*:\s*(\d+)')
if (-not $m.Success) { Write-Host 'NO-RELEASE-ID'; exit 1 }
$id = $m.Groups[1].Value
Write-Host ('RELEASE-ID: ' + $id)

$token = (Get-Content 'tools/.gh_token' -Raw).Trim()
$headers = @{
    Authorization = 'token ' + $token
    Accept        = 'application/vnd.github+json'
    'User-Agent'  = 'huangpu-sim-upload'
}

$zip = 'huangpu-sim-v1.03-win64.zip'
$up = 'https://uploads.github.com/repos/sakustay152/huangpu-sim/releases/' + $id + '/assets?name=' + $zip
$a = Invoke-RestMethod -Method Post -Uri $up -Headers $headers -ContentType 'application/zip' -InFile $zip -TimeoutSec 300
Write-Host ('UPLOADED: ' + $a.name + '  ' + [math]::Round($a.size/1KB) + ' KB  state=' + $a.state)
Write-Host ('URL: ' + $a.browser_download_url)
