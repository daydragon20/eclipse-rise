# ECLIPSE — "save & push everything" safety net.
# Run this any time (e.g. before switching machines, or if Claude hits a
# session limit) to commit all current work and push it to GitHub so the
# other computer can pull it. Portable: uses its own folder as the repo root.

$ErrorActionPreference = 'Stop'
$repo = $PSScriptRoot
Set-Location $repo

Write-Host "== ECLIPSE push-all ==  ($repo)" -ForegroundColor Cyan

# 1. Stage + commit everything, if there is anything to commit.
git add -A
$changes = git status --porcelain
if ([string]::IsNullOrWhiteSpace($changes)) {
    Write-Host "Niets nieuw om te committen." -ForegroundColor Green
} else {
    $stamp = Get-Date -Format 'yyyy-MM-dd HH:mm'
    git commit -m "[WIP] Auto-save $stamp (handoff snapshot)"
    Write-Host "Gecommit als handoff-snapshot." -ForegroundColor Green
}

# 2. Push, if a remote is configured.
$remotes = @(git remote)
if ($remotes -notcontains 'origin') {
    Write-Host ""
    Write-Host "Nog geen GitHub-remote ingesteld. Eenmalig (na 'gh auth login'):" -ForegroundColor Yellow
    Write-Host "  gh repo create eclipse-rise --private --source `"$repo`" --remote origin --push" -ForegroundColor Yellow
    exit 0
}

git push origin HEAD
Write-Host "Gepusht naar origin. De andere computer kan nu 'git pull' doen." -ForegroundColor Green
