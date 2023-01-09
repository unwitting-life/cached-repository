@echo off
git pull
git fetch --all
git reset --hard origin/master
copy config.cpp.in config.cpp /y 2>nul
