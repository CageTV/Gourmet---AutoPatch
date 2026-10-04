#include "PCH.h"
#include "Menu.h"
#include "Conform.h"
#include "Config.h"
#include "GourmetData.h"

#include "SKSEMenuFramework.h"

namespace Menu
{
	namespace
	{
		using namespace ImGuiMCP;

		constexpr ImU32 Col(int r, int g, int b, int a = 255) { return (static_cast<ImU32>(a) << 24) | (static_cast<ImU32>(b) << 16) | (static_cast<ImU32>(g) << 8) | static_cast<ImU32>(r); }
		ImVec4          Vec(ImU32 c) { return ImVec4((c & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, ((c >> 16) & 0xFF) / 255.0f, ((c >> 24) & 0xFF) / 255.0f); }

		struct SourceStyle
		{
			Conform::Source source;
			ImU32           color;
		};
		constexpr SourceStyle kStyle[] = {
			{ Conform::Source::Native, Col(94, 140, 98) },       // follows Gourmet already
			{ Conform::Source::Enforced, Col(70, 150, 190) },    // Gourmet's definition put back
			{ Conform::Source::Auto, Col(214, 156, 62) },        // converted
			{ Conform::Source::Rule, Col(170, 120, 200) },       // by rule
			{ Conform::Source::Override, Col(204, 110, 150) },   // by hand
			{ Conform::Source::Excluded, Col(120, 120, 120) },   // left alone
			{ Conform::Source::Unclassified, Col(196, 84, 70) }, // not recognised
			{ Conform::Source::Left, Col(70, 70, 70) },
		};

		ImU32 SourceColor(Conform::Source s)
		{
			for (const auto& st : kStyle) {
				if (st.source == s) {
					return st.color;
				}
			}
			return Col(100, 100, 100);
		}

		ImU32 StatColor(const gclass::Result& c)
		{
			if (!c.ok) {
				return Col(80, 80, 80);
			}
			if (c.drink) {
				return c.stat == gclass::Stat::Stamina ? Col(176, 128, 40) : Col(140, 70, 140);
			}
			switch (c.stat) {
			case gclass::Stat::Health: return Col(180, 64, 60);
			case gclass::Stat::Stamina: return Col(76, 150, 84);
			default: return Col(70, 110, 190);
			}
		}

		// "H 25" in a coloured pill
		void Chip(const gclass::Result& c)
		{
			const std::string text = !c.ok ? "-" : std::string(c.drink ? (c.stat == gclass::Stat::Stamina ? "Ale" : "Wine") : (c.stat == gclass::Stat::Health ? "HP" : c.stat == gclass::Stat::Stamina ? "SP" : "MP")) + " " + std::to_string(c.tier) + (c.raw ? " raw" : "");
			const auto        pos = GetCursorScreenPos();
			const auto        size = CalcTextSize(text.c_str());
			auto*             dl = GetWindowDrawList();
			ImDrawListManager::AddRectFilled(dl, ImVec2(pos.x - 2.0f, pos.y - 1.0f), ImVec2(pos.x + size.x + 6.0f, pos.y + size.y + 1.0f), StatColor(c), 4.0f, 0);
			SetCursorPosX(GetCursorPosX() + 2.0f);
			TextColored(ImVec4(1, 1, 1, 1), "%s", text.c_str());
		}

		void Pill(const char* text, ImU32 color)
		{
			const auto pos = GetCursorScreenPos();
			const auto size = CalcTextSize(text);
			auto*      dl = GetWindowDrawList();
			ImDrawListManager::AddRectFilled(dl, ImVec2(pos.x - 2.0f, pos.y - 1.0f), ImVec2(pos.x + size.x + 6.0f, pos.y + size.y + 1.0f), color, 4.0f, 0);
			SetCursorPosX(GetCursorPosX() + 2.0f);
			TextColored(ImVec4(1, 1, 1, 1), "%s", text);
		}

		std::string Lower(std::string s)
		{
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return s;
		}

		// ---- Overview -----------------------------------------------------------------------------------------------------------
		bool needsRestart = false;
		bool changedNow = false;

		void Setting(const char* label, bool& value, const char* tip, bool a_restart = true)
		{
			if (Checkbox(label, &value)) {
				Config::Save();
				changedNow = true;
				needsRestart = needsRestart || a_restart;
			}
			if (IsItemHovered()) {
				BeginTooltip();
				PushTextWrapPos(420.0f);
				TextUnformatted(tip);
				PopTextWrapPos();
				EndTooltip();
			}
		}

		constexpr const char* kLogoPath = "Data\\Interface\\gourmet\\gourmet_logo.png";
		constexpr float       kLogoAspect = 480.0f / 1024.0f;

		void Logo()
		{
			static ImTextureID tex = SKSEMenuFramework::LoadTexture(kLogoPath);
			if (!tex) {
				return;
			}
			const float avail = GetContentRegionAvail().x;
			const float w = std::min(avail, 460.0f);
			SetCursorPosX(GetCursorPosX() + (avail - w) * 0.5f);
			Image(tex, ImVec2(w, w * kLogoAspect));
			Spacing();
		}

		void __stdcall RenderOverview()
		{
			Logo();
			SeparatorText("Gourmet AutoPatch");
			if (!Conform::Active()) {
				TextColored(ImVec4(1.0f, 0.55f, 0.45f, 1.0f), "Gourmet - A Cooking Overhaul is not loaded, or its records were not found.");
				TextWrapped("Nothing is changed. This add-on needs Gourmet.esp (version 1.2.0). See GourmetAutoPatch.log in your SKSE logs folder.");
				return;
			}
			TextWrapped("At game start every food in your load order is looked at once. Foods of other mods are made to follow Gourmet's rules (one regeneration buff, "
			            "10 / 25 / 50 %), so they need no patch plugin. Nothing is written to your save and nothing is permanent: switch it off and the foods are as the "
			            "mods made them.");
			Spacing();

			const int total = static_cast<int>(Conform::Entries().size());
			Text("%d foods in your load order", total);
			// one bar, one coloured segment per source
			const auto  avail = GetContentRegionAvail().x;
			const float width = std::min(avail - 8.0f, 640.0f);
			const auto  start = GetCursorScreenPos();
			auto*       dl = GetWindowDrawList();
			float       x = start.x;
			ImDrawListManager::AddRectFilled(dl, ImVec2(start.x - 1, start.y - 1), ImVec2(start.x + width + 1, start.y + 21), Col(30, 30, 30), 3.0f, 0);
			for (const auto& st : kStyle) {
				const int n = Conform::Count(st.source);
				if (n == 0 || total == 0) {
					continue;
				}
				const float w = width * static_cast<float>(n) / static_cast<float>(total);
				ImDrawListManager::AddRectFilled(dl, ImVec2(x, start.y), ImVec2(x + w, start.y + 20), st.color, 0.0f, 0);
				x += w;
			}
			Dummy(ImVec2(width, 24.0f));
			static const char* what[] = {
				"already follows Gourmet (Gourmet's own foods, and mods with a Gourmet patch)",
				"Gourmet's own foods that a later mod had turned back into vanilla ones: Gourmet's definition put back",
				"foods of other mods, converted automatically",
				"converted by a rule in config.json",
				"set by hand on the Foods page",
				"left alone on purpose (poisons, excluded plugins, skipped by you)",
				"not recognised as a food Gourmet has a rule for (potions, coffee, special items...): listed on the Foods page so you can set them",
				"not touched (conforming is switched off)",
			};
			int i = 0;
			for (const auto& st : kStyle) {
				const int n = Conform::Count(st.source);
				if (n > 0) {
					const auto p = GetCursorScreenPos();
					ImDrawListManager::AddRectFilled(dl, ImVec2(p.x, p.y + 3), ImVec2(p.x + 12, p.y + 15), st.color, 2.0f, 0);
					SetCursorPosX(GetCursorPosX() + 20.0f);
					Text("%4d  %s - %s", n, Conform::SourceName(st.source), what[i]);
				}
				++i;
			}
			Spacing();
			Text("%d converted foods were added to %d of Gourmet's vendor lists (Vendors page).", Conform::AddedToLists(), static_cast<int>(Conform::Lists().size()));

			Spacing();
			SeparatorText("Settings");
			auto& cfg = Config::Get();
			Setting("Convert foods of other mods", cfg.conform, "Work out what a food is (meat, stew, soup, bread, dessert, drink...) and give it Gourmet's regeneration buff and keyword.");
			Setting("Restore Gourmet's own foods", cfg.enforce, "Some mods load after Gourmet and turn its foods (raw beef and so on) back into vanilla ones. This puts Gourmet's effects back.");
			Setting("Add converted foods to vendor lists", cfg.distribute, "Converted foods are added to the matching Gourmet vendor lists (butchers, grocers, inns, drinks). Applied at the next start.");
			Setting("Survival Mode effects", cfg.survival, "Give converted foods Survival Mode's hunger effect, and list raw meat in Survival's raw-meat list (food poisoning).");
			Setting("Gourmet's Food Corrector on 25 % foods", cfg.foodCorrector, "Gourmet's own cooked 25 % foods carry its Food Corrector effect; converted ones get it too.");
			int cap = cfg.maxAddedPerList;
			SetNextItemWidth(200.0f);
			if (SliderInt("Most foods added to one vendor list", &cap, 0, 20)) {
				cfg.maxAddedPerList = cap;
				Config::Save();
				needsRestart = true;
			}
			if (IsItemHovered()) {
				SetTooltip("Gourmet's lists hold about 10 foods each. This keeps converted foods from crowding them out. Applied at the next start.");
			}
			if (changedNow) {
				if (Button("Apply to foods now")) {
					Conform::ReapplyAll();
					Conform::WriteReport();
					changedNow = false;
				}
				SameLine();
				TextDisabled("(the vendor lists change at the next start)");
			}
			if (needsRestart) {
				TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Some changes apply when you restart the game.");
			}
			Spacing();
			TextDisabled("Full list of what was done: Data/SKSE/Plugins/GourmetAutoPatch/report.json. Your own rules: config.json in the same folder.");
		}

		// ---- Foods ---------------------------------------------------------------------------------------------------------------
		int         filter = 0;  // 0 all, 1..8 = a source, 9 = "changed" (restored, converted, rule, hand)
		char        search[64] = "";
		std::string selectedKey;

		struct Editor
		{
			std::string key;
			int         kind = 0;   // 0 health, 1 stamina, 2 magicka, 3 ale, 4 wine
			int         tier = 1;   // 0 = 10, 1 = 25, 2 = 50
			bool        raw = false;
			bool        hot = false;
			int         type = 0;
		} ed;

		const char* const kKinds[] = { "Health (meat, fish, stew)", "Stamina (vegetable, soup, bread)", "Magicka (fruit, cheese, dessert)", "Ale-like drink", "Wine-like drink" };
		const char* const kTiers[] = { "10 % (raw / ingredient)", "25 % (cooked)", "50 % (stew, pie, big soup)" };
		const char* const kTypes[] = { "(no type)", "Meat", "Fish", "Shellfish", "Stew", "Soup", "Chowder", "Pie", "Vegetable", "Fruit", "Bread", "Cheese", "Dessert" };
		const gclass::Type kTypeValues[] = { gclass::Type::None, gclass::Type::Meat, gclass::Type::Fish, gclass::Type::Crab, gclass::Type::Stew, gclass::Type::Soup,
			                                 gclass::Type::Chowder, gclass::Type::Pie, gclass::Type::Vegetable, gclass::Type::Fruit, gclass::Type::Bread,
			                                 gclass::Type::Cheese, gclass::Type::Dessert };

		void LoadEditor(const Conform::Entry& e)
		{
			ed.key = e.key;
			const auto& c = e.cls;
			ed.kind = !c.ok ? 0 : c.drink ? (c.stat == gclass::Stat::Stamina ? 3 : 4) : (c.stat == gclass::Stat::Health ? 0 : c.stat == gclass::Stat::Stamina ? 1 : 2);
			ed.tier = c.tier >= 50 ? 2 : c.tier >= 25 ? 1 : (c.ok ? 0 : 1);
			ed.raw = c.raw;
			ed.hot = c.hot;
			ed.type = 0;
			for (int i = 0; i < 13; ++i) {
				if (kTypeValues[i] == c.type) {
					ed.type = i;
				}
			}
		}

		std::string EditorClass()
		{
			gclass::Result c;
			c.ok = true;
			c.drink = ed.kind >= 3;
			c.stat = ed.kind == 0 ? gclass::Stat::Health : (ed.kind == 1 || ed.kind == 3) ? gclass::Stat::Stamina : gclass::Stat::Magicka;
			c.tier = ed.tier == 0 ? 10 : ed.tier == 1 ? 25 : 50;
			c.raw = ed.raw && !c.drink;
			c.hot = ed.hot && !c.drink;
			c.type = c.drink ? (ed.kind == 3 ? gclass::Type::Ale : gclass::Type::Wine) : kTypeValues[ed.type];
			return Config::FormatClass(c);
		}

		bool Visible(const Conform::Entry& e)
		{
			using S = Conform::Source;
			switch (filter) {
			case 1: if (e.source != S::Native) return false; break;
			case 2: if (e.source != S::Enforced) return false; break;
			case 3: if (e.source != S::Auto) return false; break;
			case 4: if (e.source != S::Rule) return false; break;
			case 5: if (e.source != S::Override) return false; break;
			case 6: if (e.source != S::Excluded) return false; break;
			case 7: if (e.source != S::Unclassified) return false; break;
			case 8: if (e.source != S::Enforced && e.source != S::Auto && e.source != S::Rule && e.source != S::Override) return false; break;
			default: break;
			}
			if (search[0]) {
				const auto q = Lower(search);
				if (Lower(e.name).find(q) == std::string::npos && Lower(e.plugin).find(q) == std::string::npos && Lower(e.editorId).find(q) == std::string::npos) {
					return false;
				}
			}
			return true;
		}

		void __stdcall RenderFoods()
		{
			if (!Conform::Active()) {
				TextDisabled("Gourmet is not loaded: nothing to show.");
				return;
			}
			static const char* kFilters[] = { "All foods", "Follow Gourmet already", "Restored (Gourmet's own)", "Converted automatically", "Converted by rule", "Set by hand",
				                              "Left alone", "Not recognised", "Everything this add-on changed" };
			SetNextItemWidth(260.0f);
			Combo("Show", &filter, kFilters, 9);
			SameLine();
			SetNextItemWidth(200.0f);
			InputText("Search", search, sizeof(search));

			const float tableHeight = std::max(180.0f, GetContentRegionAvail().y - 270.0f);
			if (BeginTable("##foods", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp,
			               ImVec2(0.0f, tableHeight))) {
				TableSetupScrollFreeze(0, 1);
				TableSetupColumn("Food", ImGuiTableColumnFlags_WidthStretch, 3.0f);
				TableSetupColumn("Mod", ImGuiTableColumnFlags_WidthStretch, 2.4f);
				TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed, 74.0f);
				TableSetupColumn("What was done", ImGuiTableColumnFlags_WidthFixed, 96.0f);
				TableSetupColumn("Why", ImGuiTableColumnFlags_WidthStretch, 3.2f);
				TableHeadersRow();
				int shown = 0;
				for (auto& e : const_cast<std::deque<Conform::Entry>&>(Conform::Entries())) {
					if (!Visible(e)) {
						continue;
					}
					++shown;
					PushID(e.key.c_str());
					TableNextRow();
					TableSetColumnIndex(0);
					const bool selected = e.key == selectedKey;
					if (Selectable(e.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
						selectedKey = e.key;
						LoadEditor(e);
					}
					TableSetColumnIndex(1);
					TextUnformatted(e.plugin.c_str());
					TableSetColumnIndex(2);
					Chip(e.cls);
					TableSetColumnIndex(3);
					Pill(Conform::SourceName(e.source), SourceColor(e.source));
					TableSetColumnIndex(4);
					TextUnformatted(e.note.c_str());
					PopID();
				}
				EndTable();
				if (shown == 0) {
					TextDisabled("No food matches.");
				}
			}

			// details and the by-hand editor of the selected food
			Conform::Entry* sel = nullptr;
			for (auto& e : const_cast<std::deque<Conform::Entry>&>(Conform::Entries())) {
				if (e.key == selectedKey) {
					sel = &e;
					break;
				}
			}
			if (!sel) {
				TextDisabled("Click a food to see what happened to it and to change it.");
				return;
			}
			if (ed.key != sel->key) {
				LoadEditor(*sel);
			}
			Separator();
			TextColored(Vec(Col(255, 214, 140)), "%s", sel->name.c_str());
			SameLine();
			TextDisabled("%s", sel->key.c_str());
			Text("Before: %s", sel->before.c_str());
			Text("Now:    %s", sel->after.c_str());
			if (!sel->lists.empty()) {
				std::string l;
				for (const auto& n : sel->lists) {
					l += (l.empty() ? "" : ", ") + n;
				}
				TextWrapped("Sold by: %s", l.c_str());
			}
			if (sel->inRawList) {
				TextDisabled("Listed as raw meat for Survival Mode's food poisoning.");
			}
			Spacing();
			SetNextItemWidth(250.0f);
			Combo("##kind", &ed.kind, kKinds, 5);
			SameLine();
			SetNextItemWidth(220.0f);
			Combo("##tier", &ed.tier, kTiers, 3);
			if (ed.kind < 3) {
				SetNextItemWidth(160.0f);
				Combo("##type", &ed.type, kTypes, 13);
				SameLine();
				Checkbox("Raw", &ed.raw);
				SameLine();
				Checkbox("Hot (Survival)", &ed.hot);
			}
			if (Button("Treat it like this")) {
				Conform::SetOverride(*sel, EditorClass());
			}
			SameLine();
			if (Button("Leave it alone")) {
				Conform::SetOverride(*sel, "skip");
			}
			SameLine();
			if (Button("Back to automatic")) {
				Conform::SetOverride(*sel, "");
				LoadEditor(*sel);
			}
			TextDisabled("Applied to the food right away; vendor lists follow at the next start. Saved in config.json.");
		}

		// ---- Vendors ---------------------------------------------------------------------------------------------------------------
		void __stdcall RenderVendors()
		{
			if (!Conform::Active()) {
				TextDisabled("Gourmet is not loaded: nothing to show.");
				return;
			}
			TextWrapped("Gourmet decides what merchants sell with leveled lists, one per kind of food. Converted foods are added to the list of their kind, a few per list, "
			            "so a food from another mod shows up in butchers' and grocers' stock like Gourmet's own.");
			Spacing();
			if (Conform::Lists().empty()) {
				TextDisabled("No converted food was added to any list (nothing matched, or adding is switched off).");
				return;
			}
			if (BeginTable("##lists", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
				TableSetupColumn("Gourmet list", ImGuiTableColumnFlags_WidthStretch, 3.0f);
				TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthStretch, 1.6f);
				TableSetupColumn("Gourmet's foods", ImGuiTableColumnFlags_WidthFixed, 110.0f);
				TableSetupColumn("Added", ImGuiTableColumnFlags_WidthStretch, 4.0f);
				TableHeadersRow();
				for (const auto& l : Conform::Lists()) {
					TableNextRow();
					TableSetColumnIndex(0);
					TextUnformatted(l.name.c_str());
					TableSetColumnIndex(1);
					TextUnformatted(l.kind.c_str());
					TableSetColumnIndex(2);
					Text("%d", l.originalSize);
					TableSetColumnIndex(3);
					std::string names;
					for (const auto* e : l.added) {
						names += (names.empty() ? "" : ", ") + e->name;
					}
					TextWrapped("%d: %s", static_cast<int>(l.added.size()), names.c_str());
				}
				EndTable();
			}
		}

		std::atomic<bool> registered{ false };
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			SKSE::log::warn("SKSE Menu Framework not found: GourmetAutoPatch still works, it just has no pages (see report.json)");
			return;
		}
		SKSEMenuFramework::SetSection("Gourmet");
		SKSEMenuFramework::AddSectionItem("Overview", RenderOverview);
		SKSEMenuFramework::AddSectionItem("Foods", RenderFoods);
		SKSEMenuFramework::AddSectionItem("Vendors", RenderVendors);
		registered = true;
		SKSE::log::info("Registered with SKSE Menu Framework {}", SKSEMenuFramework::GetMenuFrameworkVersion());
	}
}
