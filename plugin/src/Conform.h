/*
 * Gourmet - AutoPatch
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#pragma once

#include "Classifier.h"
#include "Config.h"

// What GourmetAutoPatch does to the food records, and the record of it that the menu shows.
//
// At data load (before the game starts, so nothing is saved) every food in the load order is looked at once:
//   Native       already follows Gourmet's rules (it has one of Gourmet's effects, or is defined by Gourmet itself): untouched
//   Enforced     is a food Gourmet changes but a later mod's override turned back into a vanilla one: Gourmet's definition is put back
//   Auto         a food of another mod, classified from its name, keywords and recipe, and given Gourmet's effect, tier and keyword
//   Rule         classified by a rule of the user's config.json
//   Override     set by hand on the Foods page
//   Excluded     left alone on purpose (excluded plugin, "skip" rule or override, or harmful effects)
//   Unclassified no rule recognised it: left alone, listed so it can be set by hand
//   Left         conforming is switched off
// Then the converted foods are added to Gourmet's vendor leveled lists and, for Survival Mode, to Survival's raw-meat list.
namespace Conform
{
	enum class Source { Native, Enforced, Auto, Rule, Override, Excluded, Unclassified, Left, Count };
	const char* SourceName(Source a_source);

	struct Entry
	{
		RE::AlchemyItem* item = nullptr;
		std::string      name, plugin, editorId, key;  // key = "Plugin.esp|0012AB" (local id), the handle of a user override
		Source           source = Source::Left;
		gclass::Result   cls;                           // the class it is treated as; cls.ok is false when the food is untouched
		std::string      note;                          // why it ended up like this
		std::string      before, after;                 // the effects, as text
		std::vector<std::string> lists;                 // vendor lists it was added to
		bool             inRawList = false;             // added to Survival's raw-meat list

		// the original state, so a change can be undone
		std::vector<RE::Effect*>     origEffects;
		std::vector<RE::BGSKeyword*> addedKeywords;
	};

	struct ListInfo
	{
		std::string         name;
		std::string         kind;           // "Health 25 %", "Ale 50 %" ...
		int                 originalSize = 0;
		std::vector<Entry*> added;
	};

	bool Run();      // at data load; false when Gourmet is not installed
	bool Active();   // Gourmet found and its records resolved

	const std::deque<Entry>&    Entries();
	const std::vector<ListInfo>& Lists();
	int                          Count(Source a_source);
	int                          AddedToLists();

	// The Foods page: set one food by hand ("skip", a class string, or "" to go back to automatic) and apply it now. Vendor lists change at the next start.
	void SetOverride(Entry& a_entry, const std::string& a_class);
	// Redo every food after the settings changed
	void ReapplyAll();
	void WriteReport();
}
