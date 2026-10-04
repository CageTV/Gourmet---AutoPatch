// Checks gclass::Classify against work/corpus.tsv (tools/make_corpus.py). Foods with a ground truth (Gourmet's own) are compared field by field;
// the others are just printed. Build and run: tools/classify_test.cmd
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../plugin/src/Classifier.h"

static std::string Truth(const gclass::Result& r)
{
	if (!r.ok) {
		return "-";
	}
	const char* s = r.drink ? (r.stat == gclass::Stat::Stamina ? "A" : "W") : (r.stat == gclass::Stat::Health ? "H" : r.stat == gclass::Stat::Stamina ? "S" : "M");
	std::string t = std::string(s) + std::to_string(r.tier) + (r.raw ? "r" : "") + ":";
	if (r.type != gclass::Type::None) {
		t += (r.type == gclass::Type::Crab ? "Crab" : gclass::TypeName(r.type));
	}
	if (r.hot && (r.type == gclass::Type::Stew || r.type == gclass::Type::Chowder || r.type == gclass::Type::Pie)) {
		t += ":hot";
	}
	return t;
}

int main(int argc, char** argv)
{
	std::ifstream in(argc > 1 ? argv[1] : "work/corpus.tsv");
	std::string   line;
	int           withTruth = 0, statOk = 0, tierOk = 0, rawOk = 0, typeOk = 0, allOk = 0, missed = 0;
	std::vector<std::string> bad, unclassified;
	const bool verbose = argc > 2;
	while (std::getline(in, line)) {
		std::vector<std::string> f;
		std::stringstream        ss(line);
		std::string              cell;
		while (std::getline(ss, cell, '\t')) {
			f.push_back(cell);
		}
		if (f.size() < 9) {
			continue;
		}
		gclass::Input in2;
		in2.editorId = f[2];
		in2.name = f[3];
		in2.rawKeyword = f[4] == "1";
		in2.warmth = f[5] == "1";
		in2.recipeIngredients = std::stoi(f[6]);
		in2.value = f[7] == "?" ? 0 : std::stoi(f[7]);
		const auto  r = gclass::Classify(in2);
		const auto  got = Truth(r);
		const auto& want = f[8];
		if (want == "-") {
			if (verbose || true) {
				std::printf("%-22s %-34s %-26s -> %-18s %s\n", f[0].c_str(), f[2].substr(0, 34).c_str(), f[3].substr(0, 26).c_str(), got.c_str(), r.why.c_str());
			}
			continue;
		}
		++withTruth;
		const auto split = [](const std::string& s) {
			const auto p = s.find(':');
			return std::make_pair(s.substr(0, p), p == std::string::npos ? std::string() : s.substr(p + 1));
		};
		const auto [gs, gt] = split(got);
		const auto [ws, wt] = split(want);
		auto       head = [](const std::string& s) { return s.substr(0, 1); };
		auto       tier = [](const std::string& s) { return std::string(s.begin() + 1, s.end() - (s.back() == 'r' ? 1 : 0)); };
		const bool sOk = got != "-" && head(gs) == head(ws);
		const bool tOk = got != "-" && tier(gs) == tier(ws);
		const bool rOk = got != "-" && (gs.back() == 'r') == (ws.back() == 'r');
		const bool yOk = got != "-" && gt == wt;
		statOk += sOk;
		tierOk += tOk;
		rawOk += rOk;
		typeOk += yOk;
		if (got == "-") {
			++missed;
		}
		if (got == want) {
			++allOk;
		} else {
			bad.push_back(f[3] + "  [" + f[2] + "]  want " + want + "  got " + got + "  (" + r.why + ")");
		}
	}
	std::printf("\n=== ground truth: %d foods; stat %d, tier %d, raw %d, type %d, everything %d, unclassified %d\n", withTruth, statOk, tierOk, rawOk, typeOk, allOk, missed);
	for (const auto& b : bad) {
		std::printf("MISMATCH %s\n", b.c_str());
	}
	return 0;
}
