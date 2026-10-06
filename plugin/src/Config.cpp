/*
 * Gourmet - AutoPatch
 * Copyright (c) 2026 CageTV
 *
 * Released under the MIT License; see LICENSE.txt.
 */
#include "PCH.h"
#include "Config.h"

#include <nlohmann/json.hpp>

namespace Config
{
	namespace
	{
		Settings settings;

		std::filesystem::path File() { return Folder() / "config.json"; }

		gclass::Type TypeFromText(const std::string& a_text, bool& a_ok)
		{
			static const std::pair<const char*, gclass::Type> names[] = {
				{ "meat", gclass::Type::Meat }, { "fish", gclass::Type::Fish }, { "crab", gclass::Type::Crab }, { "shellfish", gclass::Type::Crab },
				{ "stew", gclass::Type::Stew }, { "soup", gclass::Type::Soup }, { "chowder", gclass::Type::Chowder }, { "pie", gclass::Type::Pie },
				{ "vegetable", gclass::Type::Vegetable }, { "fruit", gclass::Type::Fruit }, { "bread", gclass::Type::Bread },
				{ "cheese", gclass::Type::Cheese }, { "dessert", gclass::Type::Dessert }, { "ale", gclass::Type::Ale }, { "wine", gclass::Type::Wine },
				{ "", gclass::Type::None }, { "none", gclass::Type::None }
			};
			std::string lower = a_text;
			std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			for (const auto& [n, t] : names) {
				if (lower == n) {
					a_ok = true;
					return t;
				}
			}
			a_ok = false;
			return gclass::Type::None;
		}
	}

	std::filesystem::path Folder() { return std::filesystem::path("Data") / "SKSE" / "Plugins" / "GourmetAutoPatch"; }

	Settings& Get() { return settings; }

	bool ParseClass(const std::string& a_text, gclass::Result& a_out)
	{
		if (a_text.size() < 3) {
			return false;
		}
		const char s = static_cast<char>(std::toupper(static_cast<unsigned char>(a_text[0])));
		if (s != 'H' && s != 'S' && s != 'M' && s != 'A' && s != 'W') {
			return false;
		}
		std::size_t i = 1;
		int         tier = 0;
		while (i < a_text.size() && std::isdigit(static_cast<unsigned char>(a_text[i]))) {
			tier = tier * 10 + (a_text[i] - '0');
			++i;
		}
		if (tier <= 0 || tier > 200) {
			return false;
		}
		bool raw = false;
		if (i < a_text.size() && a_text[i] == 'r') {
			raw = true;
			++i;
		}
		std::string rest = i < a_text.size() && a_text[i] == ':' ? a_text.substr(i + 1) : std::string();
		bool        hot = false;
		if (const auto p = rest.find(":hot"); p != std::string::npos) {
			hot = true;
			rest = rest.substr(0, p);
		} else if (rest == "hot") {
			hot = true;
			rest.clear();
		}
		bool ok = false;
		auto type = TypeFromText(rest, ok);
		if (!ok) {
			return false;
		}
		a_out = {};
		a_out.ok = true;
		a_out.tier = tier;
		a_out.raw = raw;
		a_out.hot = hot;
		a_out.type = type;
		a_out.drink = s == 'A' || s == 'W';
		a_out.stat = s == 'H' ? gclass::Stat::Health : (s == 'S' || s == 'A') ? gclass::Stat::Stamina : gclass::Stat::Magicka;
		if (a_out.drink && type == gclass::Type::None) {
			a_out.type = s == 'A' ? gclass::Type::Ale : gclass::Type::Wine;
		}
		a_out.duration = a_out.drink ? 600 : 1200;
		a_out.why = "set by hand or by a rule";
		return true;
	}

	std::string FormatClass(const gclass::Result& a_r)
	{
		if (!a_r.ok) {
			return "-";
		}
		char s = a_r.stat == gclass::Stat::Health ? 'H' : a_r.stat == gclass::Stat::Stamina ? 'S' : 'M';
		if (a_r.drink) {
			s = a_r.stat == gclass::Stat::Stamina ? 'A' : 'W';
		}
		std::string out(1, s);
		out += std::to_string(a_r.tier);
		if (a_r.raw) {
			out += 'r';
		}
		out += ':';
		if (a_r.type != gclass::Type::None && !(a_r.drink && (a_r.type == gclass::Type::Ale || a_r.type == gclass::Type::Wine))) {
			std::string t = gclass::TypeName(a_r.type);
			std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			out += t == "shellfish" ? "crab" : t;
		}
		if (a_r.hot) {
			out += ":hot";
		}
		return out;
	}

	void Load()
	{
		settings = {};
		std::error_code ec;
		std::filesystem::create_directories(Folder(), ec);
		std::ifstream in(File());
		if (!in) {
			Save();  // first run: write the defaults, so there is a file to edit
			return;
		}
		try {
			const auto j = nlohmann::json::parse(in, nullptr, true, true);
			auto       b = [&](const char* k, bool& v) {
                if (j.contains(k) && j[k].is_boolean()) {
                    v = j[k].get<bool>();
                }
			};
			b("enabled", settings.enabled);
			b("conform", settings.conform);
			b("enforce", settings.enforce);
			b("distribute", settings.distribute);
			b("survival", settings.survival);
			b("foodCorrector", settings.foodCorrector);
			if (j.contains("maxAddedPerList") && j["maxAddedPerList"].is_number_integer()) {
				settings.maxAddedPerList = std::clamp(j["maxAddedPerList"].get<int>(), 0, 50);
			}
			if (j.contains("excludePlugins") && j["excludePlugins"].is_array()) {
				for (const auto& e : j["excludePlugins"]) {
					if (e.is_string()) {
						settings.excludePlugins.push_back(e.get<std::string>());
					}
				}
			}
			if (j.contains("rules") && j["rules"].is_array()) {
				for (const auto& e : j["rules"]) {
					if (e.is_object() && e.contains("match") && e.contains("class")) {
						settings.rules.push_back({ e["match"].get<std::string>(), e.value("plugin", std::string()), e["class"].get<std::string>() });
					}
				}
			}
			if (j.contains("overrides") && j["overrides"].is_object()) {
				for (auto it = j["overrides"].begin(); it != j["overrides"].end(); ++it) {
					if (it.value().is_string()) {
						settings.overrides[it.key()] = it.value().get<std::string>();
					}
				}
			}
		} catch (const std::exception& e) {
			SKSE::log::error("config.json could not be read ({}); using the defaults", e.what());
		}
	}

	bool Save()
	{
		nlohmann::json j;
		j["_help"] = "Gourmet AutoPatch settings. enabled/conform/enforce/distribute/survival: switch parts off (a restart applies them). "
		             "excludePlugins: plugin file names whose foods are never touched. rules: [{\"match\": \"regex on the name\", \"plugin\": \"optional.esp\", "
		             "\"class\": \"H25:Meat\" or \"skip\"}]. class = stat H(ealth)/S(tamina)/M(agicka) or A(le)/W(ine), tier 10/25/50, r = raw, then :type, :hot. "
		             "overrides are written by the in-game Foods page.";
		j["enabled"] = settings.enabled;
		j["conform"] = settings.conform;
		j["enforce"] = settings.enforce;
		j["distribute"] = settings.distribute;
		j["survival"] = settings.survival;
		j["foodCorrector"] = settings.foodCorrector;
		j["maxAddedPerList"] = settings.maxAddedPerList;
		j["excludePlugins"] = settings.excludePlugins;
		j["rules"] = nlohmann::json::array();
		for (const auto& r : settings.rules) {
			nlohmann::json e = { { "match", r.match }, { "class", r.cls } };
			if (!r.plugin.empty()) {
				e["plugin"] = r.plugin;
			}
			j["rules"].push_back(e);
		}
		j["overrides"] = settings.overrides;
		std::error_code ec;
		std::filesystem::create_directories(Folder(), ec);
		std::ofstream out(File(), std::ios::trunc);
		if (!out) {
			return false;
		}
		out << j.dump(2) << "\n";
		return true;
	}
}
