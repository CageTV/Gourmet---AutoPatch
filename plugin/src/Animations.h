/*
 * Gourmet - AutoPatch
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#pragma once

// Eating animations (Eating Animations and Sounds, "EAS") for the foods no EAS patch covers.
// EAS animates a food when the player eats it and the food has three keywords: EASKID_All, the group EASKID_GroupNN and EASkey_<food>, the key
// of one of the vanilla foods it has an animation (and a prop) for. Its patches for Gourmet, CACO and other mods add an animation effect instead
// (marked with the keyword aa_EAAS), through SkyPatcher. This runs once a game is loaded, after every patch has done its work, and gives each food
// that has neither the keywords of the closest vanilla food: by name where it can (horker loaf, salmon steak, tomato soup, Eidar cheese...),
// otherwise by kind (raw or cooked meat, stew, soup, bread, cheese, dessert, ale, wine). Nothing is done when EAS is not installed.
namespace Animations
{
	void Apply();  // after a save is loaded or a new game started; works once per game session
	int  Added();  // foods given an animation this session, for the Overview page
	bool EasInstalled();
}
