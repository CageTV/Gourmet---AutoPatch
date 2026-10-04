"""Builds work/corpus.tsv for tools/classify_test.cpp from the Spriggit YAML in work/y_* (Gourmet's own plugins: ground truth is known) and work/c_*
(third-party plugins: no ground truth, the test just prints what the classifier makes of them).

Columns: set, formkey, editorId, name, rawKeyword, warmth, recipeIngredients, value, truth
truth = <H|S|M|A|W><tier>[r]:<type>[:hot] from the food's own Gourmet effects and keywords, or "-" when there is none.
"""
import os
import re

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = os.path.join(HERE, "work")
RX = re.compile(r"(.+) - ([0-9A-F]{6})_(.+)\.yaml$")
STATK = {"MAG_FoodFortifyHealthRegenBasic": "H", "MAG_FoodFortifyStaminaRegenBasic": "S", "MAG_FoodFortifyMagickaRegenBasic": "M"}
RAWKW, WARM = "0A0E56:Skyrim.esm", {"002EE5:Update.esm", "002EE6:Update.esm"}

names = {}
for d in os.listdir(W):
    if d.startswith("y_") or d.startswith("c_") or d.startswith("p_"):
        for sub in ("MagicEffects", "Keywords"):
            p = os.path.join(W, d, sub)
            if os.path.isdir(p):
                for fn in os.listdir(p):
                    m = RX.match(fn)
                    if m:
                        names[f"{m.group(2)}:{m.group(3)}"] = m.group(1)


def read(p):
    return open(p, encoding="utf-8").read()


# recipes of every plugin: created object -> distinct ingredient entries (a food's recipe can live in another plugin than the food)
recipes = {}
for d in sorted(os.listdir(W)):
    if d.startswith(("y_", "c_", "p_")):
        cp = os.path.join(W, d, "ConstructibleObjects")
        if os.path.isdir(cp):
            for fn in os.listdir(cp):
                t = read(os.path.join(cp, fn))
                co = re.search(r"^CreatedObject: (\S+)", t, re.M)
                if co:
                    recipes[co.group(1)] = len(re.findall(r"^- Item:", t, re.M))

rows = []
for d in sorted(os.listdir(W)):
    if not (d.startswith("y_") or d.startswith("c_")):
        continue
    base = os.path.join(W, d)
    ip = os.path.join(base, "Ingestibles")
    if not os.path.isdir(ip):
        continue
    for fn in sorted(os.listdir(ip)):
        m = RX.match(fn)
        if not m:
            continue
        t = read(os.path.join(ip, fn))
        if "FoodItem" not in t:
            continue
        key = f"{m.group(2)}:{m.group(3)}"
        eid = m.group(1)
        nm = re.search(r"Name:\n\s+TargetLanguage: \w+\n\s+Value: (.+)", t)
        name = nm.group(1).strip().strip("'\"") if nm else ""
        kws = re.search(r"^Keywords:\n((?:- .+\n)+)", t, re.M)
        kw = [x[2:].strip() for x in kws.group(1).splitlines()] if kws else []
        effs = re.findall(r"- BaseEffect: (\S+)\n  Data:(?: \{\}|\n    Magnitude: (\S+))", t)
        val = re.search(r"^Value: (\S+)", t, re.M)
        truth = "-"
        stat, tier, drink = None, 0, None
        for e, mag in effs:
            n = names.get(e, "")
            if n in STATK:
                stat, tier = STATK[n], int(float(mag or 0))
            elif n in ("MAG_AlcoholFortifyStamina", "MAG_AlcoholDamageStamina"):
                drink, tier = ("A" if n == "MAG_AlcoholFortifyStamina" else "W"), int(float(mag or 0))
            elif n in ("MAG_AlcoholFortifyMagicka", "MAG_AlcoholDamageMagicka"):
                drink, tier = ("W" if n == "MAG_AlcoholFortifyMagicka" else "A"), int(float(mag or 0))
        kn = [names.get(k, k) for k in kw]
        typ = next((x[len("MAG_FoodType"):] for x in sorted(kn) if x.startswith("MAG_FoodType") and not x.endswith("Hot")), "")
        hot = any(x.startswith("MAG_FoodType") and x.endswith("Hot") for x in kn)
        raw = RAWKW in kw
        if drink:
            truth = f"{drink}{tier or 25}:{typ}"
        elif stat and "Marriage" not in t:
            truth = f"{stat}{tier}{'r' if raw else ''}:{typ}{':hot' if hot else ''}"
        warm = any(e in WARM for e, _ in effs)
        co = recipes.get(key.split(":")[0] + ":" + key.split(":")[1], recipes.get(key, -1))
        rows.append((d[2:], key, eid, name, int(raw), int(warm), co, val.group(1) if val else 0, truth))

with open(os.path.join(W, "corpus.tsv"), "w", encoding="utf-8", newline="\n") as f:
    for r in rows:
        f.write("\t".join(str(x) for x in r) + "\n")
print(len(rows), "foods,", sum(1 for r in rows if r[8] != "-"), "with ground truth")
