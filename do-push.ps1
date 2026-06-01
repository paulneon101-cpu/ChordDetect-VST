$git      = "C:\Program Files\Git\cmd\git.exe"
$repoDir  = "C:\Users\Johnny Nix\Documents\ChordDetect-VST"
$tokenFile = "C:\Users\Johnny Nix\ghtoken.txt"

Set-Location $repoDir

if (-not (Test-Path $tokenFile)) { Write-Host "ERROR: token file not found." -ForegroundColor Red; exit 1 }
$token    = ([System.IO.File]::ReadAllText($tokenFile)).Trim()
if ($token.Length -lt 10) { Write-Host "ERROR: token file is empty." -ForegroundColor Red; exit 1 }

# Detect repo URL from existing remote, or set it
$remoteUrl = (& $git remote get-url origin 2>$null)
if (-not $remoteUrl) {
    Write-Host "ERROR: No remote 'origin' set. Run: git remote add origin https://github.com/USER/ChordDetect-VST.git" -ForegroundColor Red
    exit 1
}
Write-Host "Pushing to: $remoteUrl"

$username = ($remoteUrl -replace "https://github.com/","" -replace "/.*","")
$b64 = [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes("$username`:$token"))

& $git config --local http.extraheader "AUTHORIZATION: Basic $b64"

$result = & $git push -u origin main 2>&1
$success = $LASTEXITCODE -eq 0

& $git config --local --unset http.extraheader

if ($success) {
    Remove-Item $tokenFile -Force
    Write-Host ""
    Write-Host "Pushed! $remoteUrl" -ForegroundColor Green
    Write-Host "Token file deleted." -ForegroundColor Yellow
} else {
    Write-Host ""
    Write-Host "Push failed. Token file kept for retry." -ForegroundColor Red
    Write-Host $result
}
