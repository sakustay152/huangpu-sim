@echo off
REM read-only inspection of teammate commits
cd /d "%~dp0.."
tools\mingit\cmd\git.exe log origin/feature/C-player-part-modified -8 --format="%%h | A: %%an ^<%%ae^> | C: %%cn ^<%%ce^> | %%s"
echo ===TAGS===
for %%t in (c-v0.1.0-trade c-v0.2.0-jobs c-v0.3.0-study c-v0.3.1-eventbus c-v0.4.0-goal c-v1.0.0) do (
    tools\mingit\cmd\git.exe log %%t -1 --format="[%%t] A: %%an ^<%%ae^> | %%s"
)
