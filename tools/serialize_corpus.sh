#!/bin/bash
# Serializes third-party plugins that add or change foods, for the classifier test corpus (work/c_<name>).
S="/e/Tabula Rasa/mods/Skyrim-Claude Code Modder's Toolkit/tools/spriggit-cli.sh"
M="/e/Tabula Rasa/mods"
cd "$(dirname "$0")/.."
mkdir -p work/cstage
while IFS='|' read -r rel; do
  [ -z "$rel" ] && continue
  f="$M/$rel"; n=$(basename "$f"); b="${n%.*}"; out="work/c_${b// /_}"
  [ -d "$out" ] && continue
  cp "$f" "work/cstage/$n"
  if bash "$S" serialize --InputPath "work/cstage/$n" --OutputPath "$out" --GameRelease SkyrimSE --PackageName Spriggit.Yaml --PackageVersion 0.41.0 >/dev/null 2>&1; then echo "ok $n"; else echo "FAIL $n"; fi
done <<'LIST'
Meats Meals and More - Food Additions/Meats Meals & More.esp
Meats Meals and More - Survival Addon/Meats Meals & More - Survival Addon.esp
Farm Animals SSE/Farm Animals_HF.esp
Milandriel - A small piece of Valenwood in Skyrim/NewBosmerHouse.esp
Skyrim Extended Cut - Saints and Seducers/Skyrim Extended Cut - Saints and Seducers.esp
TMD Winery/TMDWinery.esp
Coffee and Water in Inns and Other Places SE/Coffee.esp
Stress and Fear - A Dynamic Sanity System/Stress and Fear.esp
Simple Hunting Overhaul/Simple Hunting Overhaul.esp
Echoes of Oblivion/Echoes of Oblivion.esp
Lively Farms/Lively Farms.esp
Wyrmstooth/Wyrmstooth.esp
Campfire 2026 - Patches/Simple Food and Hunting Overhaul - Campfire.esp
VIGILANT - English Translation (Plus Voiced Addon)/Vigilant.esm
LIST
echo done
