/*
 * Gourmet - AutoPatch
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#include "PCH.h"
#include "Animations.h"
#include "Classifier.h"
#include "Config.h"
#include "Conform.h"

namespace Animations
{
	namespace
	{
		// EAS's keys and the group each one is checked in (from EAS_KID.ini of Eating Animations and Sounds SE 1.9.4)
		struct Key { const char* name; int group; };
		constexpr Key kKeys[] = {
			{ "Ale", 1 }, { "AltoWine01", 1 }, { "AppleCabbageStew", 1 }, { "AshHopperLeg", 1 }, { "AshHopperMeat", 1 }, { "AshYam", 1 },
			{ "BakedPotatoes", 1 }, { "BeefStew", 1 }, { "BlackBriarMead", 2 }, { "BoarMeat", 2 }, { "BoiledCremeTreat", 2 }, { "BraidedBread", 2 },
			{ "Bread", 2 }, { "BreadHalf", 2 }, { "Butter", 2 }, { "Cabbage", 2 }, { "CabbagePotatoSoup", 2 }, { "CabbageSoup", 3 }, { "Carrot", 3 },
			{ "CharredSkeeverMeat", 3 }, { "ChickenBreast", 3 }, { "ClamChowder", 3 }, { "ClamMeat", 3 }, { "CookedBeef", 3 }, { "CookedBoarMeat", 3 },
			{ "DogMeat", 3 }, { "Dumpling", 3 }, { "EidarCheeseWedge", 4 }, { "EidarCheeseWheel", 4 }, { "ElsweyrFondue", 4 }, { "Flin", 4 },
			{ "GarlicBread", 4 }, { "GoatCheeseWedge", 4 }, { "GoatCheeseWheel", 4 }, { "Gourd", 4 }, { "GreenApple", 4 }, { "GrilledChickenBreast", 5 },
			{ "GrilledLeeks", 5 }, { "Honey", 5 }, { "HoneyNutTreat", 5 }, { "HonningbrewMead", 5 }, { "HorkerAndAshYamStew", 5 }, { "HorkerLoaf", 5 },
			{ "HorkerMeat", 5 }, { "HorkerStew", 5 }, { "HorseHaunch", 5 }, { "HorseMeat", 6 }, { "JazbayCrostata", 6 }, { "JugOfMilk", 6 },
			{ "JuniperBerryCrostata", 6 }, { "Leek", 6 }, { "LegOfGoat", 6 }, { "LegofGoatRoast", 6 }, { "LongTaffyTreat", 6 }, { "MammothCheeseBowl", 6 },
			{ "MammothSnout", 6 }, { "MammothSteak", 7 }, { "Matze", 7 }, { "MudcrabLegs", 7 }, { "PheasantBreast", 7 }, { "PheasantRoast", 7 }, { "Pie", 7 },
			{ "Potato", 7 }, { "PotatoBread", 7 }, { "PotatoSoup", 7 }, { "RabbitHaunch", 7 }, { "RawBeef", 8 }, { "RawRabbitLeg", 8 }, { "RedApple", 8 },
			{ "SackOfFlour", 8 }, { "SalmonMeat", 8 }, { "SalmonSteak", 8 }, { "SearedSlaughterfish", 8 }, { "Shein", 8 }, { "SlicedEidarCheese", 8 },
			{ "SlicedGoatCheese", 9 }, { "SnowberryCrostata", 9 }, { "SpicedWine", 9 }, { "SteamedMudcrabLegs", 9 }, { "Sujamma", 9 }, { "SweetRoll", 9 },
			{ "Tomato", 10 }, { "TomatoSoup", 10 }, { "VegetableSoup", 10 }, { "Venison", 10 }, { "VenisonChop", 10 }, { "VenisonStew", 10 }, { "Wine01", 10 },
		};

		std::unordered_map<std::string, RE::BGSKeyword*> easKeywords;  // editor id -> keyword, for the EASKID_* / EASkey_* keywords
		RE::BGSKeyword* marker = nullptr;                               // aa_EAAS
		RE::BGSKeyword* raw = nullptr;                                  // VendorItemFoodRaw
		int             added = 0;
		bool            done = false;

		bool Animated(const RE::AlchemyItem* a_item)
		{
			if (auto it = easKeywords.find("EASKID_All"); it != easKeywords.end() && a_item->HasKeyword(it->second)) {
				return true;
			}
			for (auto* fx : a_item->effects) {
				if (fx && fx->baseEffect && marker && fx->baseEffect->HasKeyword(marker)) {
					return true;
				}
			}
			return false;
		}

		// The EAS key of the vanilla food closest to this one, or nullptr
		const char* Pick(const std::vector<std::string>& w, gclass::Type a_type, bool a_raw, bool a_drink)
		{
			auto has = [&](std::initializer_list<const char*> a_any) {
				return std::any_of(w.begin(), w.end(), [&](const std::string& x) { return std::any_of(a_any.begin(), a_any.end(), [&](const char* y) { return x == y; }); });
			};
			using T = gclass::Type;
			if (has({ "milk" })) {
				return "JugOfMilk";
			}
			if (a_drink || a_type == T::Ale || a_type == T::Wine) {
				if (has({ "honningbrew" })) return "HonningbrewMead";
				if (has({ "briar" })) return "BlackBriarMead";
				if (has({ "sujamma" })) return "Sujamma";
				if (has({ "flin" })) return "Flin";
				if (has({ "shein" })) return "Shein";
				if (has({ "matze" })) return "Matze";
				if (has({ "spiced" }) && has({ "wine" })) return "SpicedWine";
				if (has({ "wine", "brandy", "port", "sherry" })) return "Wine01";
				if (has({ "rum" })) return "AltoWine01";
				if (has({ "ale", "mead", "beer", "stout", "lager" })) return "Ale";
				return a_type == T::Wine ? "Wine01" : "Ale";
			}
			switch (a_type) {
			case T::Stew:
			case T::Chowder:
			case T::Soup:
				if (has({ "chowder" })) return "ClamChowder";
				if (has({ "horker" })) return has({ "yam" }) ? "HorkerAndAshYamStew" : "HorkerStew";
				if (has({ "venison" })) return "VenisonStew";
				if (has({ "apple" }) && has({ "cabbage" })) return "AppleCabbageStew";
				if (has({ "tomato" })) return "TomatoSoup";
				if (has({ "potato" })) return has({ "cabbage" }) ? "CabbagePotatoSoup" : "PotatoSoup";
				if (has({ "cabbage" })) return "CabbageSoup";
				return a_type == T::Soup ? "VegetableSoup" : "BeefStew";
			case T::Pie:
				if (has({ "jazbay" })) return "JazbayCrostata";
				if (has({ "snowberry" })) return "SnowberryCrostata";
				if (has({ "crostata" })) return "JuniperBerryCrostata";
				return "Pie";
			case T::Dessert:
				if (has({ "sweetroll" }) || (has({ "sweet" }) && has({ "roll" }))) return "SweetRoll";
				if (has({ "taffy" })) return "LongTaffyTreat";
				if (has({ "honey" }) && has({ "nut" })) return "HoneyNutTreat";
				if (has({ "creme", "cream" })) return "BoiledCremeTreat";
				if (has({ "dumpling", "dumplings" })) return "Dumpling";
				if (has({ "crostata" })) return "JuniperBerryCrostata";
				if (has({ "honey" })) return "Honey";
				return "SweetRoll";
			case T::Bread:
				if (has({ "garlic" })) return "GarlicBread";
				if (has({ "braided" })) return "BraidedBread";
				if (has({ "potato" })) return "PotatoBread";
				if (has({ "flour" })) return "SackOfFlour";
				if (has({ "half", "slice", "sliced" })) return "BreadHalf";
				return "Bread";
			case T::Cheese:
				if (has({ "bowl" })) return "MammothCheeseBowl";
				if (has({ "fondue" })) return "ElsweyrFondue";
				if (has({ "eidar" })) return has({ "wheel" }) ? "EidarCheeseWheel" : has({ "slice", "sliced" }) ? "SlicedEidarCheese" : "EidarCheeseWedge";
				return has({ "wheel" }) ? "GoatCheeseWheel" : has({ "slice", "sliced" }) ? "SlicedGoatCheese" : "GoatCheeseWedge";
			case T::Fruit:
				return has({ "green" }) && has({ "apple" }) ? "GreenApple" : "RedApple";
			case T::Vegetable:
				if (has({ "carrot", "carrots" })) return "Carrot";
				if (has({ "cabbage" })) return "Cabbage";
				if (has({ "potato", "potatoes" })) return has({ "baked" }) ? "BakedPotatoes" : "Potato";
				if (has({ "leek", "leeks" })) return has({ "grilled" }) ? "GrilledLeeks" : "Leek";
				if (has({ "tomato", "tomatoes" })) return "Tomato";
				if (has({ "gourd" })) return "Gourd";
				if (has({ "yam" })) return "AshYam";
				return "Carrot";
			case T::Fish:
				if (has({ "salmon" })) return a_raw ? "SalmonMeat" : "SalmonSteak";
				return a_raw ? "SalmonMeat" : "SearedSlaughterfish";
			case T::Crab:
				if (has({ "clam", "clams" })) return "ClamMeat";
				return a_raw ? "MudcrabLegs" : "SteamedMudcrabLegs";
			case T::Meat:
				if (has({ "horker" })) return a_raw ? "HorkerMeat" : "HorkerLoaf";
				if (has({ "horse" })) return a_raw ? "HorseMeat" : "HorseHaunch";
				if (has({ "goat" })) return a_raw ? "LegOfGoat" : "LegofGoatRoast";
				if (has({ "mammoth" })) return a_raw ? "MammothSnout" : "MammothSteak";
				if (has({ "venison", "deer", "elk" })) return a_raw ? "Venison" : "VenisonChop";
				if (has({ "rabbit", "hare" })) return a_raw ? "RawRabbitLeg" : "RabbitHaunch";
				if (has({ "chicken" })) return a_raw ? "ChickenBreast" : "GrilledChickenBreast";
				if (has({ "pheasant", "hawk", "tern", "bird", "fowl", "duck", "goose", "quail" })) return a_raw ? "PheasantBreast" : "PheasantRoast";
				if (has({ "boar", "pig", "pork", "bacon", "ham" })) return a_raw ? "BoarMeat" : "CookedBoarMeat";
				if (has({ "skeever" })) return a_raw ? "RawBeef" : "CharredSkeeverMeat";
				if (has({ "hopper" })) return a_raw ? "AshHopperMeat" : "CookedBeef";
				if (has({ "dog", "wolf", "fox" }) && a_raw) return "DogMeat";
				return a_raw ? "RawBeef" : "CookedBeef";
			default:
				if (has({ "honey" })) return "Honey";
				if (has({ "butter" })) return "Butter";
				return nullptr;
			}
		}

		bool Give(RE::AlchemyItem* a_item, const char* a_key)
		{
			const auto key = std::find_if(std::begin(kKeys), std::end(kKeys), [&](const Key& k) { return std::strcmp(k.name, a_key) == 0; });
			if (key == std::end(kKeys)) {
				return false;
			}
			const auto all = easKeywords.find("EASKID_All");
			const auto grp = easKeywords.find(std::format("EASKID_Group{:02}", key->group));
			const auto one = easKeywords.find(std::string("EASkey_") + a_key);
			if (all == easKeywords.end() || grp == easKeywords.end() || one == easKeywords.end()) {
				return false;
			}
			for (auto* kw : { all->second, grp->second, one->second }) {
				if (!a_item->HasKeyword(kw)) {
					a_item->AddKeyword(kw);
				}
			}
			return true;
		}
	}

	bool EasInstalled()
	{
		auto* dh = RE::TESDataHandler::GetSingleton();
		return dh && dh->LookupModByName("TaberuAnimation.esp");
	}

	int Added() { return added; }

	void Apply()
	{
		if (done || !Config::Get().animations || !EasInstalled()) {
			return;
		}
		done = true;
		auto* dh = RE::TESDataHandler::GetSingleton();
		marker = dh->LookupForm<RE::BGSKeyword>(0x19BB69, "TaberuAnimation.esp");
		raw = dh->LookupForm<RE::BGSKeyword>(0x0A0E56, "Skyrim.esm");
		for (auto* kw : dh->GetFormArray<RE::BGSKeyword>()) {
			const char* id = kw ? kw->GetFormEditorID() : nullptr;
			if (id && (std::strncmp(id, "EASKID_", 7) == 0 || std::strncmp(id, "EASkey_", 7) == 0)) {
				easKeywords.emplace(id, kw);
			}
		}
		if (!easKeywords.contains("EASKID_All")) {
			SKSE::log::warn("Eating animations: Eating Animations and Sounds is loaded but its keywords were not found; nothing done");
			return;
		}

		int already = 0, none = 0;
		for (const auto& e : Conform::Entries()) {
			auto* item = e.item;
			if (!item || Conform::OwnedOrHarmful(item)) {
				continue;
			}
			if (Animated(item)) {
				++already;
				continue;
			}
			gclass::Input in;
			in.name = e.name;
			in.editorId = e.editorId;
			in.rawKeyword = raw && item->HasKeyword(raw);
			in.value = item->GetGoldValue();
			const auto c = gclass::Classify(in);
			auto       type = Conform::GourmetType(item);
			if (type == gclass::Type::None && c.ok) {
				type = c.type;
			}
			const bool  isRaw = in.rawKeyword || (c.ok && c.raw);
			const char* key = (c.ok || type != gclass::Type::None) ? Pick(gclass::Words(e.name), type, isRaw, c.ok && c.drink) : nullptr;
			if (key && Give(item, key)) {
				++added;
				SKSE::log::info("Eating animation: {} ({}) -> {}", e.name, e.plugin, key);
			} else {
				++none;
			}
		}
		SKSE::log::info("Eating animations: {} foods already animated by Eating Animations and Sounds or a patch, {} given the animation of the closest vanilla food, "
		                "{} left without one (not recognised)",
			already, added, none);
	}
}
