$ErrorActionPreference = 'Stop'
$notes = [IO.File]::ReadAllText('tools/release-notes/v111_release.json', [Text.UTF8Encoding]::new($false))
$token = (Get-Content 'tools/.gh_token' -Raw).Trim()
$headers = @{
    Authorization = 'token ' + $token
    Accept        = 'application/vnd.github+json'
    'User-Agent'  = 'huangpu-sim-upload'
}
$body = [Text.Encoding]::UTF8.GetBytes($notes)
$r = Invoke-RestMethod -Method Post -Uri 'https://api.github.com/repos/sakustay152/huangpu-sim/releases' -Headers $headers -ContentType 'application/json' -Body $body -TimeoutSec 60
Write-Host ('RELEASE-OK: ' + $r.tag_name + '  id=' + $r.id)
$zip = 'huangpu-sim-v1.1.1-win64.zip'
$up = 'https://uploads.github.com/repos/sakustay152/huangpu-sim/releases/' + $r.id + '/assets?name=' + $zip
$a = Invoke-RestMethod -Method Post -Uri $up -Headers $headers -ContentType 'application/zip' -InFile $zip -TimeoutSec 300
Write-Host ('UPLOADED: ' + $a.name + '  ' + [math]::Round($a.size/1KB) + ' KB  state=' + $a.state)
Write-Host ('URL: ' + $a.browser_download_url)
