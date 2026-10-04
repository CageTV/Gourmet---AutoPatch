"""Prints Gourmet's own food records as a compact table (editor id, keywords, effects) from the Spriggit YAML in work/y_*.
Used to derive the conform rules (which effect / magnitude / keyword each kind of food gets). Usage: python tools/dump_gourmet_foods.py [plugin dir]"""
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "work", "y_Gourmet")

NAMES = {}  # "0008A0:Gourmet.esp" -> editor id, from every serialized plugin
for d in os.listdir(os.path.join(HERE, "work")):
    p = os.path.join(HERE, "work", d)
    if not d.startswith(("y_", "p_")) or not os.path.isdir(p):
        continue
    for dp, _, fs in os.walk(p):
        for fn in fs:
            m = re.match(r"(.+) - ([0-9A-F]{6})_(.+)\.yaml$", fn)
            if m:
                NAMES[f"{m.group(2)}:{m.group(3)}"] = m.group(1)
KNOWN = {"0A0E56:Skyrim.esm": "VendorItemFoodRaw", "08CDEA:Skyrim.esm": "VendorItemFood", "0F5CB0:Skyrim.esm": "VendorItemRecipe",
         "002EE1:Update.esm": "SurvHungerVerySmall", "002EE2:Update.esm": "SurvHungerSmall", "002EE3:Update.esm": "SurvHungerMedium",
         "002EE4:Update.esm": "SurvHungerLarge", "002EE5:Update.esm": "SurvRestoreCold", "002EE6:Update.esm": "SurvWarmth"}


def nm(k):
    return KNOWN.get(k) or NAMES.get(k) or k


rows = []
for fn in sorted(os.listdir(os.path.join(SRC, "Ingestibles"))):
    t = open(os.path.join(SRC, "Ingestibles", fn), encoding="utf-8").read()
    eid = re.search(r"^EditorID: (\S+)", t, re.M)
    name = re.search(r"Name:\n\s+TargetLanguage: \w+\n\s+Value: (.+)", t)
    kws = re.search(r"^Keywords:\n((?:- .+\n)+)", t, re.M)
    keywords = [nm(x[2:].strip()) for x in kws.group(1).splitlines()] if kws else []
    effects = []
    for m in re.finditer(r"- BaseEffect: (\S+)\n  Data:(?: \{\}|\n    Magnitude: (\S+)(?:\n    Area: (\S+))?(?:\n    Duration: (\S+))?)", t):
        effects.append(f"{nm(m.group(1))}[{m.group(2) or 0}/{m.group(4) or 0}]")
    val = re.search(r"^Value: (\S+)", t, re.M)
    flags = re.search(r"^Flags:\n((?:- .+\n)+)", t, re.M)
    rows.append((eid.group(1) if eid else fn, name.group(1).strip() if name else "", val.group(1) if val else "?", keywords, effects,
                 [x[2:].strip() for x in flags.group(1).splitlines()] if flags else []))
for r in rows:
    print(f"{r[0]:42} {r[1][:28]:28} v={r[2]:>4} kw={','.join(k.replace('MAG_FoodType','T:') for k in r[3] if k not in ('VendorItemFood',))} fx={' '.join(r[4])}")
print(len(rows), "foods")
