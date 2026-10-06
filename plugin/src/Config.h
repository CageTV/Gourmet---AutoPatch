/*
 * Gourmet - AutoPatch
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#pragma once

#include "Classifier.h"

// Settings and the user's own rules, in Data/SKSE/Plugins/GourmetAutoPatch/config.json (created on first run).
namespace Config
{
	struct Rule
	{
		std::string match;   // case-insensitive regular expression on the food's name (or editor id)
		std::string plugin;  // optional: only foods from this plugin
		std::string cls;     // a class string like "H50:Stew", or "skip"
	};

	struct Settings
	{
		bool enabled = true;
		bool conform = true;       // classify foods of other mods and give them Gourmet's effects
		bool enforce = true;       // put Gourmet's own definition back on the foods a later mod has turned into vanilla ones
		bool distribute = true;    // add converted foods to Gourmet's vendor leveled lists
		bool survival = true;      // Survival Mode hunger effects on converted foods and Survival's raw-meat list
		bool foodCorrector = true; // Gourmet's "Food Corrector" effect on 25 % cooked foods, like Gourmet's own
		int  maxAddedPerList = 6;  // most foods added to any one vendor list, so Gourmet's own foods stay in the majority
		std::vector<std::string> excludePlugins;
		std::vector<Rule>        rules;
		std::map<std::string, std::string> overrides;  // "Plugin.esp|0012AB" -> class string or "skip"
	};

	Settings& Get();
	void      Load();
	bool      Save();

	// "H25:Meat", "S50:Soup", "M10r:Fruit", "H50:Stew:hot", "A25:Ale" (alcohol: A = ale-like, W = wine-like; "r" = raw)
	bool        ParseClass(const std::string& a_text, gclass::Result& a_out);
	std::string FormatClass(const gclass::Result& a_result);

	std::filesystem::path Folder();
}
