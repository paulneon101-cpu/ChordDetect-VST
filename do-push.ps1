$ErrorActionPreference = "Stop"
$git      = "C:\Program Files\Git\cmd\git.exe"
$repoDir  = "C:\Users\Johnny Nix\Documents\ChordDetect-VST"
$username = "paulneon101-cpu"
$tokenFile = "C:\Users\Johnny Nix\ghtoken.txt"

Set-Location $repoDir

$token = ([System.IO.File]::ReadAllText($tokenFile)).Trim()

# Stage everything and commit updates
& $git add .
& $git commit -m "Add pitch correction and latency control; red/green skull-mic UI"

# Set remote if not already set
$remotes = & $git remote
if ($remotes -notcontains "origin") {
    & $git remote add origin "https://github.com/$username/ChordDetect-VST.git"
}

# Auth via header (token never in URL or git log)
& $git config --local http.extraheader "AUTHORIZATION: bearer $token"
& $git branch -M main
& $git push -u origin main

# Clean up
& $git config --local --unset http.extraheader
Remove-Item $tokenFile -Force

Write-Host ""
Write-Host "Done! https://github.com/$username/ChordDetect-VST" -ForegroundColor Green
Write-Host "Token file deleted." -ForegroundColor Yellow
