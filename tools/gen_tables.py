"""Writes plugin/src/GourmetData.h from Gourmet's own plugins (Spriggit YAML in work/y_*):

  1. anchors   - local FormIDs in Gourmet.esp of the effects and keywords the add-on applies (found by EditorID, so a typo fails loudly)
  2. kRefFoods - every food Gourmet and its official patches define or change (effects + keywords), the table that "enforce" restores when a
                 later mod's override has turned a Gourmet food back into a vanilla one
  3. kLists    - Gourmet's vendor leveled lists that hold one kind of food, with that kind, so converted foods can be added to the matching lists

Usage: python tools/gen_tables.py
"""
import collections
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = os.path.join(HERE, "work")
OUT = os.path.join(HERE, "plugin", "src", "GourmetData.h")
# sources in load order of the official files; the one loaded last wins at run time (see Conform.cpp)
SOURCES = [("Gourmet.esp", "y_Gourmet"), ("GourmetUSSEP.esp", "y_GourmetUSSEP"), ("GourmetFishing.esp", "y_GourmetFishing"), ("GourmetSurvival.esp", "y_GourmetSurvival")]
RX = re.compile(r"(.+) - ([0-9A-F]{6})_(.+)\.yaml$")


def records(d, sub):
    p = os.path.join(W, d, sub)
    if not os.path.isdir(p):
        return
    for fn in sorted(os.listdir(p)):
        m = RX.match(fn)
        if m:
            yield m.group(1), m.group(2), m.group(3), open(os.path.join(p, fn), encoding="utf-8").read()


def split_key(k):
    a, b = k.strip().split(":", 1)
    return b, int(a, 16)


# ---- anchors -------------------------------------------------------------------------------------------------------------
ids = {}
for sub in ("MagicEffects", "Keywords"):
    for eid, fid, plug, _ in records("y_Gourmet", sub):
        if plug == "Gourmet.esp":
            ids[eid] = int(fid, 16)
NEED_MGEF = ["MAG_FoodFortifyHealthRegenBasic", "MAG_FoodFortifyStaminaRegenBasic", "MAG_FoodFortifyMagickaRegenBasic", "MAG_FoodCorrectorEffect",
             "MAG_AlcoholFortifyStamina", "MAG_AlcoholDamageMagicka", "MAG_AlcoholFortifyMagicka", "MAG_AlcoholDamageStamina"]
NEED_KW = ["MAG_FoodTypeMeat", "MAG_FoodTypeFish", "MAG_FoodTypeCrab", "MAG_FoodTypeStew", "MAG_FoodTypeStewHot", "MAG_FoodTypeSoup", "MAG_FoodTypeChowder",
           "MAG_FoodTypeChowderHot", "MAG_FoodTypePie", "MAG_FoodTypePieHot", "MAG_FoodTypeVegetable", "MAG_FoodTypeFruit", "MAG_FoodTypeBread",
           "MAG_FoodTypeCheese", "MAG_FoodTypeDessert", "MAG_FoodTypeAle", "MAG_FoodTypeWine", "MAG_FoodTypeDrugs"]
missing = [n for n in NEED_MGEF + NEED_KW if n not in ids]
if missing:
    sys.exit(f"not found in Gourmet.esp: {missing}")

# ---- reference foods --------------------------------------------------------------------------------------------------------
refs = {}  # (plugin, id) -> (source index, [kw], [effects])
for si, (src, d) in enumerate(SOURCES):
    for eid, fid, plug, t in records(d, "Ingestibles"):
        kws = []
        m = re.search(r"^Keywords:\n((?:- .+\n)+)", t, re.M)
        if m:
            kws = [split_key(x[2:]) for x in m.group(1).splitlines()]
        fx = []
        for e in re.finditer(r"- BaseEffect: (\S+)\n  Data:(?: \{\}|((?:\n    \w+: \S+)+))", t):
            vals = dict(re.findall(r"(\w+): (\S+)", e.group(2) or ""))
            fx.append((split_key(e.group(1)), float(vals.get("Magnitude", 0)), int(vals.get("Area", 0)), int(vals.get("Duration", 0))))
        refs[(plug, int(fid, 16))] = (si, eid, kws, fx)

# ---- distribution lists ---------------------------------------------------------------------------------------------------
STATK = {"MAG_FoodFortifyHealthRegenBasic": "H", "MAG_FoodFortifyStaminaRegenBasic": "S", "MAG_FoodFortifyMagickaRegenBasic": "M"}
name_of = {}
for d in ("y_Gourmet", "y_GourmetUSSEP", "y_GourmetFishing", "y_GourmetSurvival"):
    for sub in ("MagicEffects", "Keywords"):
        for eid, fid, plug, _ in records(d, sub):
            name_of[(plug, int(fid, 16))] = eid
for k, v in {("Skyrim.esm", 0x0A0E56): "VendorItemFoodRaw", ("Skyrim.esm", 0x08CDEA): "VendorItemFood"}.items():
    name_of[k] = v


def klass(key):
    if key not in refs:
        return None
    si, eid, kws, fx = refs[key]
    stat, tier, drink, drug = None, 0, 0, False
    for (plug, i), mag, _a, _d in fx:
        n = name_of.get((plug, i), "")
        if n in STATK:
            stat, tier = STATK[n], int(mag)
        elif n in ("MAG_AlcoholFortifyStamina", "MAG_AlcoholDamageStamina"):
            drink = 1 if n == "MAG_AlcoholFortifyStamina" else 2
            tier = int(mag)
        elif n in ("MAG_AlcoholFortifyMagicka", "MAG_AlcoholDamageMagicka"):
            drink = 2 if n == "MAG_AlcoholFortifyMagicka" else 1
            tier = int(mag)
        elif n.startswith("MAG_Drugs"):
            drug = True
    kn = {name_of.get(k, "") for k in kws}
    raw = "VendorItemFoodRaw" in kn
    typ = next((x[len("MAG_FoodType"):] for x in sorted(kn) if x.startswith("MAG_FoodType") and not x.endswith("Hot")), "")
    hot = any(x.endswith("Hot") for x in kn)
    if drug:
        return None
    if drink:
        return ("drink", drink, tier or 25, False, typ, False)
    if not stat:
        return None
    return ("food", stat, tier, raw, typ, hot)


lists = {}
for d in ("y_Gourmet", "y_GourmetUSSEP", "y_GourmetFishing", "y_GourmetSurvival"):
    for eid, fid, plug, t in records(d, "LeveledItems"):
        if plug == "Gourmet.esp":
            lists[(plug, int(fid, 16))] = (eid, [split_key(r) for r in re.findall(r"Reference: (\S+)", t)])
SKIP = re.compile(r"Recipe|Cookbook|BlackBriar|Honningbrew|Barrel|Death|Caravan|Fence|Limited|Drugs|ECSS|Rare|Brewer|Inn(City|Regional)|General(City|Regional)|Lite$|Grocer$|Butcher$", re.I)
chosen = []
for key, (eid, entries) in lists.items():
    if SKIP.search(eid) or any(e in lists for e in entries):
        continue
    cls = [klass(e) for e in entries]
    known = [c for c in cls if c]
    if len(entries) < 1 or len(known) < max(1, len(entries) * 0.8):
        continue
    top, n = collections.Counter(c[:4] + (c[5],) for c in known).most_common(1)[0]
    if n < len(known) * 0.85 or len(known) < 2:
        continue
    types = sorted({c[4] for c in known})
    chosen.append((eid, key[1], top, types, len(entries)))
chosen.sort()

# ---- write -----------------------------------------------------------------------------------------------------------------
TYPES = ["", "Meat", "Fish", "Crab", "Stew", "Soup", "Chowder", "Pie", "Vegetable", "Fruit", "Bread", "Cheese", "Dessert", "Ale", "Wine"]


def q(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


with open(OUT, "w", encoding="utf-8", newline="\n") as f:
    f.write("// GENERATED by tools/gen_tables.py from Gourmet's own plugins. Do not edit by hand.\n#pragma once\n\nnamespace gdata\n{\n")
    f.write("\t// Local FormIDs in Gourmet.esp (effects and keywords the add-on applies)\n")
    for n in NEED_MGEF + NEED_KW:
        f.write(f"\tinline constexpr unsigned k{n.replace('MAG_', '')} = 0x{ids[n]:06X};\n")
    f.write("\n\tstruct FormKey { const char* plugin; unsigned id; };\n")
    f.write("\tstruct RefEffect { FormKey effect; float magnitude; unsigned area; unsigned duration; };\n")
    f.write("\tstruct RefFood { FormKey food; int source; int kwBegin, kwCount; int fxBegin, fxCount; const char* editorId; };\n")
    f.write("\tinline constexpr const char* kSources[] = { " + ", ".join(q(s) for s, _ in SOURCES) + " };\n\n")
    kwa, fxa, foods = [], [], []
    for (plug, i), (si, eid, kws, fx) in sorted(refs.items(), key=lambda kv: (kv[1][0], kv[0][1])):
        foods.append((plug, i, si, len(kwa), len(kws), len(fxa), len(fx), eid))
        kwa += kws
        fxa += fx
    f.write("\tinline constexpr FormKey kRefKeywords[] = {\n")
    for plug, i in kwa:
        f.write(f"\t\t{{ {q(plug)}, 0x{i:06X} }},\n")
    f.write("\t};\n\tinline constexpr RefEffect kRefEffects[] = {\n")
    for (plug, i), mag, area, dur in fxa:
        f.write(f"\t\t{{ {{ {q(plug)}, 0x{i:06X} }}, {mag}f, {area}, {dur} }},\n")
    f.write("\t};\n\tinline constexpr RefFood kRefFoods[] = {\n")
    for plug, i, si, kb, kc, fb, fc, eid in foods:
        f.write(f"\t\t{{ {{ {q(plug)}, 0x{i:06X} }}, {si}, {kb}, {kc}, {fb}, {fc}, {q(eid)} }},\n")
    f.write("\t};\n\n")
    f.write("\t// Gourmet's vendor leveled lists that hold one kind of food. kind: 'f' food, 'd' drink. stat: H/S/M (food), A/W (drink: ale / wine).\n")
    f.write("\tstruct VendorList { unsigned id; char kind; char stat; int tier; bool raw; bool hot; unsigned typeMask; int size; const char* name; };\n")
    f.write("\tinline constexpr VendorList kLists[] = {\n")
    for eid, i, top, types, size in chosen:
        mask = 0
        for t in types:
            mask |= 1 << (TYPES.index(t) if t in TYPES else 0)
        if top[0] == "drink":
            kind, stat, tier, raw, hot = "d", "A" if top[1] == 1 else "W", top[2], False, False
        else:
            kind, stat, tier, raw, hot = "f", top[1], top[2], top[3], top[4]
        f.write(f"\t\t{{ 0x{i:06X}, '{kind}', '{stat}', {tier}, {str(raw).lower()}, {str(hot).lower()}, 0x{mask:X}u, {size}, {q(eid)} }},\n")
    f.write("\t};\n")
    f.write("\t// bit n of VendorList::typeMask = the food-type keyword kTypeNames[n] occurs among the list's foods (bit 0: foods without a type keyword)\n")
    f.write("\tinline constexpr const char* kTypeNames[] = { " + ", ".join(q(t) for t in TYPES) + " };\n}\n")
print(f"wrote {OUT}: {len(foods)} reference foods, {len(kwa)} keyword refs, {len(fxa)} effect refs, {len(chosen)} vendor lists")
for eid, i, top, types, size in chosen:
    print(f"  {eid:44} n={size:2} {top} types={types}")
