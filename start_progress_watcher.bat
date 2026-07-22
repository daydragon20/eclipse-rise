@echo off
rem ECLIPSE - start de dashboard-watcher: ververst progress_auto.js elke 10 minuten
rem zodat PROGRESS.html bij elke (auto-)herlaad verse commits/tests/screenshots toont.
title ECLIPSE progress watcher
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\update_progress.ps1" -Loop -IntervalSec 600
