#!/bin/bash
# WUB4 installer: double-click after unzipping. Copies the plugins and clears the macOS download quarantine.
set -e
cd "$(dirname "$0")"
for z in WUB4-VST3-macOS.zip WUB4-AU-macOS.zip; do [ -f "$z" ] && ditto -x -k "$z" . ; done
mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
if [ -d WUB4.vst3 ]; then rm -rf ~/Library/Audio/Plug-Ins/VST3/WUB4.vst3; cp -R WUB4.vst3 ~/Library/Audio/Plug-Ins/VST3/; fi
if [ -d WUB4.component ]; then rm -rf ~/Library/Audio/Plug-Ins/Components/WUB4.component; cp -R WUB4.component ~/Library/Audio/Plug-Ins/Components/; fi
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/WUB4.vst3 2>/dev/null || true
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/WUB4.component 2>/dev/null || true
echo "WUB4 installed. In Ableton: Settings > Plug-Ins > Rescan, then find WUB4 under Plug-Ins."
