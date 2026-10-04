/*
 * Gourmet - AutoPatch
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>

// Decides which kind of food a food record is, in Gourmet's own terms, from nothing but words and a few flags. Plain C++ (no game types) so the
// same code runs in the DLL and in tools/classify_test, which checks it against the foods Gourmet itself defines.
//
// Gourmet's scheme (read from Gourmet.esp): a food gives one regeneration buff for 20 minutes - Health (meat, fish, stews, chowders, meat pies),
// Stamina (vegetables, soups, bread) or Magicka (fruit, cheese, dairy, honey, desserts) - at 10 % (raw / ingredient), 25 % (cooked) or 50 % (stews,
// pies, big soups). Drinks: ale-like ones fortify Stamina and damage Magicka, wine-like ones the reverse, for 10 minutes at 25 or 50.
namespace gclass
{
	enum class Stat { None, Health, Stamina, Magicka };

	// The Gourmet food-type keyword (MAG_FoodType*) the food gets; None = the food gets no type keyword (butter, milk, honey, cooked mushrooms...).
	enum class Type { None, Meat, Fish, Crab, Stew, Soup, Chowder, Pie, Vegetable, Fruit, Bread, Cheese, Dessert, Ale, Wine };

	struct Input
	{
		std::string name;       // display name
		std::string editorId;   // may be empty (editor ids are only known when a mod like po3's Tweaks keeps them)
		bool        rawKeyword = false;   // has VendorItemFoodRaw
		bool        warmth = false;       // has a Survival "Fortify Warmth" or "Restore Cold" effect (a hot food)
		int         recipeIngredients = -1;  // distinct ingredients of the recipe that makes it, -1 when no recipe is known
		int         value = 0;            // gold value
	};

	struct Result
	{
		bool        ok = false;          // false: no rule matched, the food is left alone
		std::string why;                 // the words that decided it, or why nothing matched
		Stat        stat = Stat::None;
		Type        type = Type::None;
		int         tier = 0;            // 10 / 25 / 50
		bool        raw = false;
		bool        hot = false;
		bool        drink = false;       // alcohol: stat is Stamina for ale-like, Magicka for wine-like
		int         duration = 1200;     // seconds
	};

	Result Classify(const Input& a_in);

	const char* StatName(Stat a_stat);
	const char* TypeName(Type a_type);
	// "tokens" of a name: lower-case words, camelCase and underscores split (exposed for the tests)
	std::vector<std::string> Words(std::string_view a_text);
}
