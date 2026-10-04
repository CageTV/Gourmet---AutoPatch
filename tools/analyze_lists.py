"""For each Gourmet leveled list (vendor / barrel / death lists), flattens its sublists and prints a histogram of the Gourmet food classes it holds.
Class = (stat, tier, raw?/drink/drug) from the food's own Gourmet effects. Used to decide which lists a converted food of a given class is added to.
Usage: python tools/analyze_lists.py [list-name-substring]"""
import collections
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = os.path.join(HERE, "work")
flt = sys.argv[1].lower() if len(sys.argv) > 1 else ""

STAT = {"MAG_FoodFortifyHealthRegenBasic": "H", "MAG_FoodFortifyStaminaRegenBasic": "S", "MAG_FoodFortifyMagickaRegenBasic": "M",
        "MAG_FoodFortifyHealthRegenMarriage": "X"}
foods = {}
names = {}
for d in os.listdir(W):
    if not d.startswith("y_"):
        continue
    for sub in ("Ingestibles",):
        p = os.path.join(W, d, sub)
        if not os.path.isdir(p):
            continue
        for fn in os.listdir(p):
            m = re.match(r"(.+) - ([0-9A-F]{6})_(.+)\.yaml$", fn)
            t = open(os.path.join(p, fn), encoding="utf-8").read()
            key = f"{m.group(2)}:{m.group(3)}"
            names[key] = m.group(1)
            fx = re.findall(r"- BaseEffect: (\S+)\n  Data:(?: \{\}|\n    Magnitude: (\S+))", t)
            # effect names via the MGEF file names
            foods[key] = (m.group(1), t, fx)
mgef = {}
for d in os.listdir(W):
    if d.startswith("y_") and os.path.isdir(os.path.join(W, d, "MagicEffects")):
        for fn in os.listdir(os.path.join(W, d, "MagicEffects")):
            m = re.match(r"(.+) - ([0-9A-F]{6})_(.+)\.yaml$", fn)
            mgef[f"{m.group(2)}:{m.group(3)}"] = m.group(1)


def klass(key):
    if key not in foods:
        return None
    name, t, fx = foods[key]
    stat, tier = None, 0
    drink = drug = False
    for eff, mag in fx:
        n = mgef.get(eff, eff)
        if n in STAT:
            stat = STAT[n]
            tier = int(float(mag or 0))
        elif n.startswith("MAG_Alcohol"):
            drink = True
        elif n.startswith("MAG_Drugs"):
            drug = True
    raw = "VendorItemFoodRaw" in t or "0A0E56:Skyrim.esm" in t
    if drug:
        return "drug"
    if drink:
        return "drink"
    if stat is None:
        return "other"
    return f"{stat}{tier}" + ("raw" if raw else "")


lists = {}
for d in os.listdir(W):
    if (d.startswith("y_") or d.startswith("p_")) and os.path.isdir(os.path.join(W, d, "LeveledItems")):
        for fn in os.listdir(os.path.join(W, d, "LeveledItems")):
            m = re.match(r"(.+) - ([0-9A-F]{6})_(.+)\.yaml$", fn)
            t = open(os.path.join(W, d, "LeveledItems", fn), encoding="utf-8").read()
            refs = re.findall(r"Reference: (\S+)", t)
            lists[f"{m.group(2)}:{m.group(3)}"] = (m.group(1), refs)


def flatten(key, seen):
    if key in seen:
        return []
    seen.add(key)
    out = []
    for r in lists[key][1] if key in lists else []:
        if r in lists:
            out += flatten(r, seen)
        else:
            out.append(r)
    return out


for key, (name, refs) in sorted(lists.items(), key=lambda kv: kv[1][0]):
    if flt and flt not in name.lower():
        continue
    leaves = flatten(key, set())
    h = collections.Counter(klass(r) or "?" for r in leaves)
    if not any(k not in ("?", None) for k in h):
        continue
    print(f"{name:42} n={len(leaves):3} {dict(h.most_common())}")
