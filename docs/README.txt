Gourmet - AutoPatch 1.0.0
An SKSE add-on for Gourmet - A Cooking Overhaul (Nexus 96876) by Simon Magus, Jelidity and wolll.

WHAT IT DOES
------------
Gourmet makes every food give one long regeneration buff (Health for meat, fish and stews; Stamina for vegetables, soups and bread; Magicka for
fruit, cheese and desserts) at 10 / 25 / 50 %. Food added by other mods stays vanilla until someone makes a patch plugin for it.

This add-on does that work at game start, for every mod, with no patch plugin:

  CONVERT   every food of another mod is classified from its name, keywords and recipe (raw or cooked meat, stew, soup, bread, vegetable, fruit,
            cheese, dessert, ale- or wine-like drink) and gets Gourmet's effect, tier and type keyword, exactly the way Gourmet's own foods have them.
            Gourmet's icons (Inventory Injector, Crafting Categories) and its perks then work on those foods too.
  RESTORE   when a mod that loads after Gourmet turns one of Gourmet's foods (raw beef, say) back into a vanilla one, Gourmet's own definition is put
            back. (In a typical load order this happens to dozens of foods.)
  SELL      converted foods are added to the matching Gourmet vendor lists (butchers, grocers, inns, brewers), a few per list, so a food from another
            mod turns up in stock like Gourmet's own. Empty containers ("Empty Wine Bottle"), spoilage stages ("Moldy ...", "Spoiled ...", "Ruined ...")
            and leftovers are never put on sale; a food you place by hand on the Foods page always is.
  SURVIVAL  converted foods get Survival Mode's hunger effect like Gourmet's, and raw meat goes on Survival's raw-meat list (food poisoning).

Nothing is saved to your game and no plugin is added: the changes are made in memory while the game loads. Remove the add-on and everything is as
the mods made it. Foods that already follow Gourmet (its own, and any with a Gourmet patch) are never touched.

Recipes are left alone: another mod's recipes work as that mod made them (they are not locked behind Gourmet's cookbooks).

SEE WHAT IT DID
---------------
With SKSE Menu Framework 3, the "Gourmet" section has three pages:
  Overview  how many foods, a colour bar of what happened to them, and the switches
  Foods     every food, filterable and searchable: what it is treated as (HP / SP / MP and tier), what was done and why, its effects before and
            after, and where it is sold. Click a food to set it by hand ("treat it like this", "leave it alone") - applied at once, saved.
  Vendors   which Gourmet lists got which foods
Without SKSE Menu Framework the add-on still works; Data/SKSE/Plugins/GourmetAutoPatch/report.json lists everything.

YOUR OWN RULES
--------------
Data/SKSE/Plugins/GourmetAutoPatch/config.json (created on first start; with Mod Organizer 2 it lands in your Overwrite folder):
  "conform", "enforce", "distribute", "survival", "foodCorrector"   switch parts off
  "maxAddedPerList"                                                  most foods added to one vendor list (default 6)
  "excludePlugins": ["Some Mod.esp"]                                 never touch that plugin's foods
  "rules": [ { "match": "regex on the food's name", "plugin": "optional.esp", "class": "S25:Soup" } ]
  "overrides"                                                        written by the Foods page
A class is: H, S or M (Health, Stamina, Magicka) or A / W (ale-like / wine-like drink), then the tier (10, 25, 50), "r" if raw, then ":" and a
type (meat, fish, crab, stew, soup, chowder, pie, vegetable, fruit, bread, cheese, dessert), and ":hot" for a Survival hot food. Examples:
H25:meat, H10r:fish, S50:soup, M50:dessert, H50:stew:hot, A25. "skip" leaves a food alone.

REQUIRES
--------
- Gourmet - A Cooking Overhaul 1.2.0 (Nexus 96876)
- SKSE64 and the Address Library for SKSE Plugins for your game version
- SKSE Menu Framework 3 (Nexus 120352), optional: only for the pages

WHICH BUILD (the installer asks)
--------------------------------
Two builds of the same plugin are included, because the SKSE library they are made with differs by game version:
  Skyrim 1.6.1170 and newer (SE / AE)     the "new" build. Works on 1.6.1170 and on every later version, including 1.7.x.
  Skyrim VR, or 1.6.1130 and older        the "older" build.
The installer picks the one that fits your game version and always lets you change it. On 1.6.1170 both work. Install only one: both files are
named GourmetAutoPatch.dll.

KNOWN LIMITS
------------
- A food is recognised by its words. Odd names (a potion-like "Greef", "Shivering Surprise") are listed as "Not recognised" and left alone; set them
  on the Foods page or with a rule. Poisons and foods with harmful effects are never converted.
- Drinks and foods that were changed by a Gourmet patch for that mod are left to the patch.
- Foods that other SKSE plugins read at start (Last Seed, Keyword Item Distributor) see the converted foods, because this plugin runs first.

CREDITS
-------
Gourmet is by Simon Magus, Jelidity and wolll: this add-on only reads its records (effects, keywords, leveled lists) and ships none of its files.
Add-on by CageTV. MIT License (LICENSE.txt). Source: https://github.com/CageTV/Gourmet---AutoPatch
