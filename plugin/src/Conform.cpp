/*
 * Gourmet - AutoPatch
 * Copyright (C) 2026 CageTV
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your option) any later version. It is distributed WITHOUT ANY
 * WARRANTY; see LICENSE.txt for the full text.
 */
#include "PCH.h"
#include "Conform.h"
#include "GourmetData.h"

#include <nlohmann/json.hpp>

namespace Conform
{
	namespace
	{
		struct Anchors
		{
			RE::EffectSetting* regen[3]{};  // Health, Stamina, Magicka regeneration
			RE::EffectSetting* corrector{};
			RE::EffectSetting *fortStamina{}, *dmgMagicka{}, *fortMagicka{}, *dmgStamina{};
			RE::EffectSetting* hunger[4]{};  // Survival: very small, small, medium, large
			RE::EffectSetting *warmth{}, *cold{};
			RE::BGSKeyword*    type[15]{};  // indexed by gclass::Type
			RE::BGSKeyword *   stewHot{}, *chowderHot{}, *pieHot{};
			RE::BGSKeyword *   vendorFood{}, *vendorRaw{};
			RE::BGSListForm*   rawMeat{};
			bool               ok = false;
		} A;

		std::deque<Entry>     entries;
		std::vector<ListInfo> lists;
		int                   counts[static_cast<int>(Source::Count)]{};
		int                   addedToLists = 0;
		bool                  active = false;
		std::unordered_map<RE::FormID, int> recipeIngredients;

		RE::TESDataHandler* DH() { return RE::TESDataHandler::GetSingleton(); }

		template <class T>
		T* Look(unsigned a_id, const char* a_file)
		{
			auto* dh = DH();
			return dh ? dh->LookupForm<T>(a_id, a_file) : nullptr;
		}

		bool EqualNoCase(std::string_view a, std::string_view b)
		{
			return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) { return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y)); });
		}

		std::string Hex6(unsigned a_id)
		{
			char buf[16];
			std::snprintf(buf, sizeof(buf), "%06X", a_id & 0xFFFFFF);
			return buf;
		}

		bool ResolveAnchors()
		{
			bool ok = true;
			auto need = [&](auto* p, const char* what) {
				if (!p) {
					SKSE::log::error("Gourmet record not found: {}", what);
					ok = false;
				}
				return p;
			};
			A.regen[0] = need(Look<RE::EffectSetting>(gdata::kFoodFortifyHealthRegenBasic, "Gourmet.esp"), "health regeneration effect");
			A.regen[1] = need(Look<RE::EffectSetting>(gdata::kFoodFortifyStaminaRegenBasic, "Gourmet.esp"), "stamina regeneration effect");
			A.regen[2] = need(Look<RE::EffectSetting>(gdata::kFoodFortifyMagickaRegenBasic, "Gourmet.esp"), "magicka regeneration effect");
			A.corrector = Look<RE::EffectSetting>(gdata::kFoodCorrectorEffect, "Gourmet.esp");
			A.fortStamina = Look<RE::EffectSetting>(gdata::kAlcoholFortifyStamina, "Gourmet.esp");
			A.dmgMagicka = Look<RE::EffectSetting>(gdata::kAlcoholDamageMagicka, "Gourmet.esp");
			A.fortMagicka = Look<RE::EffectSetting>(gdata::kAlcoholFortifyMagicka, "Gourmet.esp");
			A.dmgStamina = Look<RE::EffectSetting>(gdata::kAlcoholDamageStamina, "Gourmet.esp");
			for (int i = 0; i < 4; ++i) {
				A.hunger[i] = Look<RE::EffectSetting>(0x2EE1 + i, "Update.esm");  // Survival_FoodRestoreHunger VerySmall..Large
			}
			A.cold = Look<RE::EffectSetting>(0x2EE5, "Update.esm");
			A.warmth = Look<RE::EffectSetting>(0x2EE6, "Update.esm");
			const unsigned typeIds[15] = { 0, gdata::kFoodTypeMeat, gdata::kFoodTypeFish, gdata::kFoodTypeCrab, gdata::kFoodTypeStew, gdata::kFoodTypeSoup,
				                           gdata::kFoodTypeChowder, gdata::kFoodTypePie, gdata::kFoodTypeVegetable, gdata::kFoodTypeFruit, gdata::kFoodTypeBread,
				                           gdata::kFoodTypeCheese, gdata::kFoodTypeDessert, gdata::kFoodTypeAle, gdata::kFoodTypeWine };
			for (int i = 1; i < 15; ++i) {
				A.type[i] = Look<RE::BGSKeyword>(typeIds[i], "Gourmet.esp");
			}
			A.stewHot = Look<RE::BGSKeyword>(gdata::kFoodTypeStewHot, "Gourmet.esp");
			A.chowderHot = Look<RE::BGSKeyword>(gdata::kFoodTypeChowderHot, "Gourmet.esp");
			A.pieHot = Look<RE::BGSKeyword>(gdata::kFoodTypePieHot, "Gourmet.esp");
			A.vendorFood = Look<RE::BGSKeyword>(0x08CDEA, "Skyrim.esm");
			A.vendorRaw = Look<RE::BGSKeyword>(0x0A0E56, "Skyrim.esm");
			A.rawMeat = Look<RE::BGSListForm>(0x8B0, "ccqdrsse001-survivalmode.esl");
			if (!A.corrector || !A.fortStamina || !A.dmgMagicka || !A.fortMagicka || !A.dmgStamina) {
				SKSE::log::warn("some of Gourmet's effects were not found: drinks and the corrector effect may be skipped");
			}
			if (!A.hunger[0] || !A.hunger[1] || !A.hunger[2] || !A.hunger[3]) {
				SKSE::log::info("Survival Mode hunger effects not found (Update.esm): no hunger effects will be added");
			}
			return ok;
		}

		bool IsGourmetEffect(const RE::EffectSetting* a_mgef)
		{
			const auto* f = a_mgef ? a_mgef->GetFile(0) : nullptr;
			return f && EqualNoCase(f->GetFilename(), "Gourmet.esp");
		}

		bool IsGourmetSourceFile(std::string_view a_name)
		{
			for (const char* s : gdata::kSources) {
				if (EqualNoCase(a_name, s)) {
					return true;
				}
			}
			return false;
		}

		bool IsHunger(const RE::EffectSetting* m)
		{
			for (auto* h : A.hunger) {
				if (h && h == m) {
					return true;
				}
			}
			return false;
		}

		// the "restore health / stamina / magicka" kind of effect a food normally has, which Gourmet's regeneration effect replaces
		bool IsNutrition(const RE::Effect* a_fx)
		{
			const auto* m = a_fx ? a_fx->baseEffect : nullptr;
			if (!m || m->IsHostile() || m->IsDetrimental()) {
				return false;
			}
			using AT = RE::EffectArchetypes::ArchetypeID;
			const auto arch = m->GetArchetype();
			if (arch != AT::kValueModifier && arch != AT::kPeakValueModifier && arch != AT::kDualValueModifier) {
				return false;
			}
			switch (m->data.primaryAV) {
			case RE::ActorValue::kHealth:
			case RE::ActorValue::kMagicka:
			case RE::ActorValue::kStamina:
			case RE::ActorValue::kHealRate:
			case RE::ActorValue::kMagickaRate:
			case RE::ActorValue::kStaminaRate:
			case RE::ActorValue::kHealRateMult:
			case RE::ActorValue::kMagickaRateMult:
			case RE::ActorValue::kStaminaRateMult:
				return true;
			default:
				return false;
			}
		}

		std::string Describe(const RE::BSTArray<RE::Effect*>& a_effects)
		{
			std::string out;
			for (auto* fx : a_effects) {
				if (!fx || !fx->baseEffect) {
					continue;
				}
				if (!out.empty()) {
					out += "; ";
				}
				const char* n = fx->baseEffect->GetFullName();
				out += (n && n[0]) ? n : "(effect)";
				if (fx->effectItem.magnitude != 0.0f) {
					char b[32];
					std::snprintf(b, sizeof(b), " %g", fx->effectItem.magnitude);
					out += b;
				}
				if (fx->effectItem.duration != 0) {
					out += " (" + std::to_string(fx->effectItem.duration) + "s)";
				}
			}
			return out.empty() ? "(none)" : out;
		}

		// ---- plans ------------------------------------------------------------------------------------------------------------
		struct EffPlan
		{
			RE::EffectSetting* mgef;
			float              magnitude;
			std::uint32_t      area;
			std::uint32_t      duration;
		};
		struct Plan
		{
			std::vector<EffPlan>         effects;
			std::vector<RE::BGSKeyword*> keywords;
		};

		int TypeBit(gclass::Type a_type) { return static_cast<int>(a_type); }

		Plan BuildPlan(const gclass::Result& a_c, bool a_survivalEffects, bool a_corrector)
		{
			Plan p;
			if (a_c.drink) {
				const bool ale = a_c.stat == gclass::Stat::Stamina;
				auto*      fortify = ale ? A.fortStamina : A.fortMagicka;
				auto*      damage = ale ? A.dmgMagicka : A.dmgStamina;
				if (fortify) {
					p.effects.push_back({ fortify, static_cast<float>(a_c.tier), 0, 600 });
				}
				if (damage) {
					p.effects.push_back({ damage, static_cast<float>(a_c.tier), 0, 600 });
				}
			} else {
				const int stat = a_c.stat == gclass::Stat::Health ? 0 : a_c.stat == gclass::Stat::Stamina ? 1 : 2;
				p.effects.push_back({ A.regen[stat], static_cast<float>(a_c.tier), 0, 1200 });
				if (a_survivalEffects) {
					const bool animal = a_c.type == gclass::Type::Meat || a_c.type == gclass::Type::Fish || a_c.type == gclass::Type::Crab;
					const int  h = a_c.tier >= 50 ? 3 : a_c.tier >= 25 ? 2 : (a_c.raw && animal) ? 1 : 0;
					if (A.hunger[h]) {
						p.effects.push_back({ A.hunger[h], 0.0f, 0, 0 });
					}
				}
				if (a_corrector && a_c.tier == 25 && !a_c.raw && A.corrector) {
					p.effects.push_back({ A.corrector, 0.0f, 0, 0 });
				}
			}
			if (a_c.type != gclass::Type::None && A.type[TypeBit(a_c.type)]) {
				p.keywords.push_back(A.type[TypeBit(a_c.type)]);
			}
			if (a_c.hot) {
				if (a_c.type == gclass::Type::Stew && A.stewHot) {
					p.keywords.push_back(A.stewHot);
				} else if (a_c.type == gclass::Type::Chowder && A.chowderHot) {
					p.keywords.push_back(A.chowderHot);
				} else if (a_c.type == gclass::Type::Pie && A.pieHot) {
					p.keywords.push_back(A.pieHot);
				}
			}
			if (a_c.raw && A.vendorRaw) {
				p.keywords.push_back(A.vendorRaw);
			}
			if (!a_c.drink && A.vendorFood) {
				p.keywords.push_back(A.vendorFood);
			}
			return p;
		}

		void Revert(Entry& e)
		{
			auto* it = e.item;
			if (!it) {
				return;
			}
			it->effects.clear();
			for (auto* fx : e.origEffects) {
				it->effects.push_back(fx);
			}
			for (auto* kw : e.addedKeywords) {
				it->RemoveKeyword(kw);
			}
			e.addedKeywords.clear();
			e.cls = {};
		}

		// replaces the food's nutrition and hunger effects with the plan's effects (every other effect stays), and adds the plan's keywords
		void ApplyPlan(Entry& e, const Plan& a_plan, bool a_replaceHunger)
		{
			auto* it = e.item;
			it->effects.clear();
			for (auto* fx : e.origEffects) {
				if (IsNutrition(fx) || (a_replaceHunger && fx && IsHunger(fx->baseEffect))) {
					continue;
				}
				it->effects.push_back(fx);
			}
			for (const auto& pe : a_plan.effects) {
				if (!pe.mgef) {
					continue;
				}
				auto* fx = new RE::Effect();
				fx->baseEffect = pe.mgef;
				fx->effectItem.magnitude = pe.magnitude;
				fx->effectItem.area = pe.area;
				fx->effectItem.duration = pe.duration;
				fx->cost = pe.mgef->data.baseCost;
				it->effects.push_back(fx);
			}
			for (auto* kw : a_plan.keywords) {
				if (kw && !it->HasKeyword(kw)) {
					it->AddKeyword(kw);
					e.addedKeywords.push_back(kw);
				}
			}
		}

		// ---- the foods Gourmet itself defines ----------------------------------------------------------------------------------
		std::map<std::pair<std::string, unsigned>, const gdata::RefFood*> refTable;  // (plugin, local id) -> the entry of the source loaded last

		void BuildRefTable()
		{
			auto*                       dh = DH();
			std::map<std::string, int>  order;  // source -> load order position
			for (const char* s : gdata::kSources) {
				if (const auto* f = dh->LookupModByName(s)) {
					order[s] = f->GetCombinedIndex();
				}
			}
			for (const auto& rf : gdata::kRefFoods) {
				const char* src = gdata::kSources[rf.source];
				if (!order.count(src)) {
					continue;  // that official file is not loaded
				}
				auto key = std::make_pair(std::string(rf.food.plugin), rf.food.id);
				auto it = refTable.find(key);
				if (it == refTable.end() || order[gdata::kSources[it->second->source]] < order[src]) {
					refTable[key] = &rf;
				}
			}
		}

		bool PlanFromRef(const gdata::RefFood& rf, Plan& a_out)
		{
			auto* dh = DH();
			for (int i = 0; i < rf.fxCount; ++i) {
				const auto& re = gdata::kRefEffects[rf.fxBegin + i];
				auto*       m = dh->LookupForm<RE::EffectSetting>(re.effect.id, re.effect.plugin);
				if (m) {
					a_out.effects.push_back({ m, re.magnitude, re.area, re.duration });
				}
			}
			for (int i = 0; i < rf.kwCount; ++i) {
				const auto& rk = gdata::kRefKeywords[rf.kwBegin + i];
				if (auto* k = dh->LookupForm<RE::BGSKeyword>(rk.id, rk.plugin)) {
					a_out.keywords.push_back(k);
				}
			}
			return !a_out.effects.empty();
		}

		// ---- deciding what a food is ---------------------------------------------------------------------------------------------
		bool HasGourmetEffect(const RE::AlchemyItem* it)
		{
			for (auto* fx : it->effects) {
				if (fx && IsGourmetEffect(fx->baseEffect)) {
					return true;
				}
			}
			return false;
		}

		bool Harmful(const RE::AlchemyItem* it)
		{
			for (auto* fx : it->effects) {
				if (fx && fx->baseEffect && (fx->baseEffect->IsHostile() || fx->baseEffect->IsDetrimental())) {
					return true;
				}
			}
			return false;
		}

		bool MatchRule(const Config::Rule& r, const Entry& e)
		{
			if (!r.plugin.empty() && !EqualNoCase(r.plugin, e.plugin)) {
				return false;
			}
			try {
				std::regex re(r.match, std::regex::icase | std::regex::ECMAScript);
				return std::regex_search(e.name, re) || (!e.editorId.empty() && std::regex_search(e.editorId, re));
			} catch (const std::exception&) {
				return false;
			}
		}

		gclass::Input MakeInput(const Entry& e)
		{
			gclass::Input in;
			in.name = e.name;
			in.editorId = e.editorId;
			in.rawKeyword = A.vendorRaw && e.item->HasKeyword(A.vendorRaw);
			for (auto* fx : e.origEffects) {
				if (fx && fx->baseEffect && (fx->baseEffect == A.warmth || fx->baseEffect == A.cold)) {
					in.warmth = true;
				}
			}
			if (const auto r = recipeIngredients.find(e.item->GetFormID()); r != recipeIngredients.end()) {
				in.recipeIngredients = r->second;
			}
			in.value = e.item->GetGoldValue();
			return in;
		}

		// (re)decides and applies one food; returns nothing, fills the entry
		void Process(Entry& e)
		{
			const auto& cfg = Config::Get();
			Revert(e);
			e.lists.clear();
			e.cls = {};
			e.note.clear();
			Plan plan;
			bool apply = false;
			bool replaceHunger = true;

			auto fromClass = [&](const gclass::Result& c, Source src, const std::string& note) {
				e.cls = c;
				e.source = src;
				e.note = note;
				plan = BuildPlan(c, cfg.survival, cfg.foodCorrector);
				apply = !plan.effects.empty();
				replaceHunger = cfg.survival;
			};

			const auto file0 = e.item->GetFile(0);
			const std::string fileName = file0 ? std::string(file0->GetFilename()) : std::string();
			if (const auto ov = cfg.overrides.find(e.key); ov != cfg.overrides.end()) {
				if (ov->second == "skip") {
					e.source = Source::Excluded;
					e.note = "left alone by your choice";
				} else {
					gclass::Result c;
					if (Config::ParseClass(ov->second, c)) {
						fromClass(c, Source::Override, "set by hand: " + ov->second);
					} else {
						e.source = Source::Unclassified;
						e.note = "the override \"" + ov->second + "\" is not a valid class";
					}
				}
			} else if (std::any_of(cfg.excludePlugins.begin(), cfg.excludePlugins.end(), [&](const std::string& p) { return EqualNoCase(p, fileName); })) {
				e.source = Source::Excluded;
				e.note = "its plugin is in excludePlugins";
			} else if (IsGourmetSourceFile(fileName) || HasGourmetEffect(e.item)) {
				e.source = Source::Native;
				e.note = "already follows Gourmet's rules";
			} else if (const auto rf = refTable.find({ fileName, e.item->GetLocalFormID() }); cfg.enforce && rf != refTable.end()) {
				e.source = Source::Enforced;
				e.note = std::string("a food Gourmet defines (") + rf->second->editorId + "); a later mod turned it back into a vanilla one";
				if (PlanFromRef(*rf->second, plan)) {
					apply = true;
					replaceHunger = true;
					// the class, for the menu and the vendor lists: read it from the plan
					for (const auto& pe : plan.effects) {
						for (int s = 0; s < 3; ++s) {
							if (pe.mgef == A.regen[s]) {
								e.cls.ok = true;
								e.cls.stat = s == 0 ? gclass::Stat::Health : s == 1 ? gclass::Stat::Stamina : gclass::Stat::Magicka;
								e.cls.tier = static_cast<int>(pe.magnitude);
							}
						}
					}
					e.cls.raw = A.vendorRaw && std::any_of(plan.keywords.begin(), plan.keywords.end(), [](auto* k) { return k == A.vendorRaw; });
					e.cls.why = "Gourmet's own definition";
				} else {
					e.source = Source::Left;
					e.note = "Gourmet's definition could not be resolved";
				}
			} else if (Harmful(e.item)) {
				e.source = Source::Excluded;
				e.note = "has harmful effects (a poison or similar)";
			} else {
				bool done = false;
				for (const auto& r : cfg.rules) {
					if (MatchRule(r, e)) {
						done = true;
						gclass::Result c;
						if (r.cls == "skip") {
							e.source = Source::Excluded;
							e.note = "left alone by rule \"" + r.match + "\"";
						} else if (Config::ParseClass(r.cls, c)) {
							fromClass(c, Source::Rule, "rule \"" + r.match + "\" -> " + r.cls);
						} else {
							done = false;
							continue;
						}
						break;
					}
				}
				if (!done) {
					if (!cfg.conform) {
						e.source = Source::Left;
						e.note = "conforming other mods' foods is switched off";
					} else {
						auto c = gclass::Classify(MakeInput(e));
						if (c.ok) {
							fromClass(c, Source::Auto, c.why);
						} else {
							e.source = Source::Unclassified;
							e.note = c.why;
						}
					}
				}
			}
			if (apply) {
				ApplyPlan(e, plan, replaceHunger);
			}
			e.after = Describe(e.item->effects);
		}

		// ---- vendor lists ----------------------------------------------------------------------------------------------------------
		bool TypeCompatible(unsigned a_mask, int a_bit)
		{
			if (a_mask & (1u << a_bit)) {
				return true;
			}
			const bool fishLike = a_bit == 2 || a_bit == 3;  // fish and shellfish go together
			return fishLike && (a_mask & ((1u << 2) | (1u << 3)));
		}

		bool Matches(const gdata::VendorList& l, const Entry& e)
		{
			const auto& c = e.cls;
			if (!c.ok) {
				return false;
			}
			const bool drink = c.drink;
			if ((l.kind == 'd') != drink) {
				return false;
			}
			char stat;
			if (drink) {
				stat = c.stat == gclass::Stat::Stamina ? 'A' : 'W';
			} else {
				stat = c.stat == gclass::Stat::Health ? 'H' : c.stat == gclass::Stat::Stamina ? 'S' : 'M';
			}
			const bool hot = c.hot && (c.type == gclass::Type::Stew || c.type == gclass::Type::Chowder || c.type == gclass::Type::Pie);
			return l.stat == stat && l.tier == c.tier && l.raw == c.raw && l.hot == hot && TypeCompatible(l.typeMask, TypeBit(c.type));
		}

		void AddEntriesToList(RE::TESLevItem* a_list, const std::vector<RE::TESForm*>& a_forms)
		{
			const std::size_t oldCount = a_list->numEntries;
			const std::size_t room = 255 - oldCount;
			const std::size_t add = std::min(room, a_forms.size());
			if (add == 0) {
				return;
			}
			const std::size_t total = oldCount + add;
			// the game's SimpleArray keeps its element count in a size_t just before the first element, and other plugins read it (entries.size(), range-for)
			auto* block = static_cast<std::size_t*>(RE::malloc(sizeof(std::size_t) + sizeof(RE::LEVELED_OBJECT) * total));
			if (!block) {
				return;
			}
			*block    = total;
			auto* arr = reinterpret_cast<RE::LEVELED_OBJECT*>(block + 1);
			auto* old = a_list->numEntries ? a_list->entries.data() : nullptr;
			if (old) {
				std::memcpy(arr, old, sizeof(RE::LEVELED_OBJECT) * oldCount);
			}
			for (std::size_t i = 0; i < add; ++i) {
				auto& o = arr[oldCount + i];
				o.form = a_forms[i];
				o.count = 1;
				o.level = 1;
				o.pad0C = 0;
				o.itemExtra = nullptr;
			}
			// the old block is left alone (it may be shared with the master's data); the list now points at the new, headed array
			*reinterpret_cast<RE::LEVELED_OBJECT**>(&a_list->entries) = arr;
			a_list->numEntries = static_cast<std::uint8_t>(total);
		}

		std::string KindText(const gdata::VendorList& l)
		{
			std::string s;
			if (l.kind == 'd') {
				s = l.stat == 'A' ? "Ale-like drink" : "Wine-like drink";
			} else {
				s = l.stat == 'H' ? "Health" : l.stat == 'S' ? "Stamina" : "Magicka";
				s += l.raw ? " raw" : "";
				s += l.hot ? " hot" : "";
			}
			return s + " " + std::to_string(l.tier) + " %";
		}

		// foods that must never turn up in a merchant's stock: containers, spoilage stages of other mods (Last Seed), leftovers.
		// A food the user placed by hand on the Foods page is never held back.
		bool NotForSale(const Entry& a_e)
		{
			if (a_e.source == Source::Override) {
				return false;
			}
			auto lower = [](std::string s) {
				std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				return s;
			};
			const auto n = lower(a_e.name);
			const auto id = lower(a_e.editorId);
			if (n.rfind("empty ", 0) == 0 || n.rfind("leftovers", 0) == 0 || id.rfind("leftovers", 0) == 0) {
				return true;
			}
			for (const char* w : { "moldy", "spoiled", "ruined", "rotten", "rotting" }) {
				if (n.find(w) != std::string::npos) {
					return true;
				}
			}
			return false;
		}

		void Distribute()
		{
			const auto& cfg = Config::Get();
			auto*       dh = DH();
			if (!cfg.distribute || cfg.maxAddedPerList <= 0) {
				return;
			}
			for (const auto& l : gdata::kLists) {
				auto* list = dh->LookupForm<RE::TESLevItem>(l.id, "Gourmet.esp");
				if (!list) {
					continue;
				}
				std::vector<Entry*> cand;
				for (auto& e : entries) {
					if ((e.source == Source::Auto || e.source == Source::Rule || e.source == Source::Override) && !NotForSale(e) && Matches(l, e)) {
						cand.push_back(&e);
					}
				}
				if (cand.empty()) {
					continue;
				}
				// no more than maxAddedPerList, spread over the mods that bring foods (a mod with 40 foods must not crowd out the others)
				std::map<std::string, std::vector<Entry*>> byPlugin;
				for (auto* c : cand) {
					byPlugin[c->plugin].push_back(c);
				}
				std::vector<Entry*> picked;
				for (std::size_t round = 0; picked.size() < static_cast<std::size_t>(cfg.maxAddedPerList); ++round) {
					bool any = false;
					for (auto& [plugin, v] : byPlugin) {
						if (round < v.size() && picked.size() < static_cast<std::size_t>(cfg.maxAddedPerList)) {
							picked.push_back(v[round]);
							any = true;
						}
					}
					if (!any) {
						break;
					}
				}
				// skip the foods the list already holds
				std::vector<RE::TESForm*> forms;
				std::vector<Entry*>       used;
				for (auto* p : picked) {
					bool present = false;
					for (std::size_t i = 0; i < list->numEntries; ++i) {
						if (list->entries.data()[i].form == p->item) {
							present = true;
						}
					}
					if (!present) {
						forms.push_back(p->item);
						used.push_back(p);
					}
				}
				if (forms.empty()) {
					continue;
				}
				ListInfo info;
				info.name = l.name;
				info.kind = KindText(l);
				info.originalSize = list->numEntries;
				AddEntriesToList(list, forms);
				for (auto* u : used) {
					u->lists.push_back(l.name);
					info.added.push_back(u);
				}
				addedToLists += static_cast<int>(used.size());
				lists.push_back(std::move(info));
			}
		}

		void FillRawMeatList()
		{
			if (!A.rawMeat || !Config::Get().survival) {
				return;
			}
			for (auto& e : entries) {
				if (!(e.source == Source::Auto || e.source == Source::Rule || e.source == Source::Override) || !e.cls.ok || !e.cls.raw) {
					continue;
				}
				if (e.cls.stat != gclass::Stat::Health || !(e.cls.type == gclass::Type::Meat || e.cls.type == gclass::Type::Fish || e.cls.type == gclass::Type::Crab)) {
					continue;
				}
				// the list's own array, not the script-added one: nothing is saved, so it is rebuilt the same way every start
				const bool present = std::find(A.rawMeat->forms.begin(), A.rawMeat->forms.end(), e.item) != A.rawMeat->forms.end();
				if (!present) {
					A.rawMeat->forms.push_back(e.item);
				}
				e.inRawList = true;
			}
		}

		void Recount()
		{
			std::fill(std::begin(counts), std::end(counts), 0);
			for (const auto& e : entries) {
				++counts[static_cast<int>(e.source)];
			}
		}
	}

	const char* SourceName(Source s)
	{
		switch (s) {
		case Source::Native: return "Gourmet";
		case Source::Enforced: return "Restored";
		case Source::Auto: return "Converted";
		case Source::Rule: return "By rule";
		case Source::Override: return "By hand";
		case Source::Excluded: return "Left alone";
		case Source::Unclassified: return "Unrecognised";
		default: return "Not touched";
		}
	}

	bool Active() { return active; }
	const std::deque<Entry>& Entries() { return entries; }
	const std::vector<ListInfo>& Lists() { return lists; }
	int Count(Source s) { return counts[static_cast<int>(s)]; }
	int AddedToLists() { return addedToLists; }

	bool Run()
	{
		auto* dh = DH();
		if (!dh || !dh->LookupModByName("Gourmet.esp")) {
			SKSE::log::info("Gourmet.esp is not loaded: nothing to do");
			return false;
		}
		if (!ResolveAnchors()) {
			SKSE::log::error("Gourmet's records could not be resolved (is this Gourmet 1.2.0?): nothing is changed");
			return false;
		}
		A.ok = true;
		BuildRefTable();
		for (auto* cobj : dh->GetFormArray<RE::BGSConstructibleObject>()) {
			if (!cobj || !cobj->createdItem) {
				continue;
			}
			if (auto* ai = cobj->createdItem->As<RE::AlchemyItem>()) {
				auto& n = recipeIngredients[ai->GetFormID()];
				n = std::max(n, static_cast<int>(cobj->requiredItems.numContainerObjects));
			}
		}
		for (auto* item : dh->GetFormArray<RE::AlchemyItem>()) {
			if (!item || item->IsDeleted() || !item->IsFood() || item->IsPoison()) {
				continue;
			}
			auto& e = entries.emplace_back();
			e.item = item;
			const char* n = item->GetName();
			e.name = (n && n[0]) ? n : "";
			const char* ed = item->GetFormEditorID();
			e.editorId = ed ? ed : "";
			if (e.name.empty()) {
				e.name = e.editorId.empty() ? "(unnamed)" : e.editorId;
			}
			const auto* f = item->GetFile(0);
			e.plugin = f ? std::string(f->GetFilename()) : "?";
			e.key = e.plugin + "|" + Hex6(item->GetLocalFormID());
			for (auto* fx : item->effects) {
				e.origEffects.push_back(fx);
			}
			e.before = Describe(item->effects);
		}
		for (auto& e : entries) {
			Process(e);
		}
		Distribute();
		FillRawMeatList();
		Recount();
		active = true;
		SKSE::log::info("{} foods: {} follow Gourmet already, {} restored, {} converted, {} by rule, {} by hand, {} left alone, {} unrecognised; {} added to {} vendor lists",
		                entries.size(), Count(Source::Native), Count(Source::Enforced), Count(Source::Auto), Count(Source::Rule), Count(Source::Override),
		                Count(Source::Excluded), Count(Source::Unclassified), addedToLists, lists.size());
		WriteReport();
		return true;
	}

	void SetOverride(Entry& e, const std::string& a_class)
	{
		auto& cfg = Config::Get();
		if (a_class.empty()) {
			cfg.overrides.erase(e.key);
		} else {
			cfg.overrides[e.key] = a_class;
		}
		Config::Save();
		Process(e);
		Recount();
	}

	void ReapplyAll()
	{
		for (auto& e : entries) {
			Process(e);
		}
		Recount();
	}

	void WriteReport()
	{
		nlohmann::json j = nlohmann::json::array();
		for (const auto& e : entries) {
			nlohmann::json o;
			o["key"] = e.key;
			o["name"] = e.name;
			o["editorId"] = e.editorId;
			o["source"] = SourceName(e.source);
			o["class"] = Config::FormatClass(e.cls);
			o["why"] = e.note;
			o["before"] = e.before;
			o["after"] = e.after;
			o["vendorLists"] = e.lists;
			j.push_back(std::move(o));
		}
		std::error_code ec;
		std::filesystem::create_directories(Config::Folder(), ec);
		std::ofstream out(Config::Folder() / "report.json", std::ios::trunc);
		if (out) {
			out << j.dump(1) << "\n";
		}
	}
}
