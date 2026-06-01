# Run this script once to initialise git, create the GitHub repo, and push.
# Prerequisites:
#   1. Install Git:     https://git-scm.com/download/win
#   2. Install gh CLI:  https://cli.github.com/
#   3. Authenticate:    gh auth login
#
# Then, from this directory, run:   .\push-to-github.ps1

$ErrorActionPreference = "Stop"

$repoName = "ChordDetect-VST"

Write-Host "==> Initialising git repo..." -ForegroundColor Cyan
git init
git add .
git commit -m "Initial commit: ChordDetect VST3 chord detection plugin"

Write-Host "==> Creating public GitHub repo '$repoName'..." -ForegroundColor Cyan
gh repo create $repoName --public --description "Real-time chord detection VST3 plugin built with JUCE" --source . --remote origin --push

Write-Host ""
Write-Host "Done! Repo live at: https://github.com/$(gh api user --jq .login)/$repoName" -ForegroundColor Green
