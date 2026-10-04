#include "Classifier.h"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <unordered_set>

namespace gclass
{
	namespace
	{
		using WordSet = std::unordered_set<std::string>;

		const WordSet kSkip = { "potion", "poison", "skooma", "eversnow", "sleeping", "balmora", "tonic", "elixir", "coffee", "tea", "cocoa", "water", "juice",
			                    "homecooked", "marriage", "bonemeal", "bone", "moon", "sugar", "powder", "salt", "spice", "spices", "herb", "herbs", "seed", "seeds" };
		const WordSet kWineDrink = { "wine", "bloodwine", "brandy", "liqueur", "cognac", "port", "sherry", "champagne", "vermouth", "sake", "sujamma", "flin", "wines" };
		const WordSet kAleDrink = { "ale", "mead", "beer", "lager", "stout", "rum", "whiskey", "whisky", "grog", "brew", "bock", "moonshine", "vodka", "gin", "shein",
			                        "mazte", "rye", "absinthe", "schnapps", "ales", "meads", "briar", "honningbrew" };
		const WordSet kFineDrink = { "reserve", "special", "aged", "vintage", "premium", "fine", "private", "imperial", "limited", "gold" };

		const WordSet kCooked = { "cooked", "roast", "roasted", "steak", "grilled", "charred", "baked", "fried", "boiled", "steamed", "smoked", "seared", "braised",
			                      "broiled", "bbq", "barbecue", "barbecued", "chop", "haunch", "loaf", "kebab", "skewer", "sausage", "bacon", "ham", "jerky", "patty",
			                      "cutlet", "drumstick", "mashed", "toasted", "stuffed", "spiced", "pan", "sauteed", "poached", "dried", "salted", "cured" };
		const WordSet kRaw = { "raw", "uncooked" };
		const WordSet kStew = { "stew", "chowder", "casserole", "bisque", "curry", "goulash", "ragout", "hotpot", "tagine", "jambalaya", "pottage", "stroganoff",
			                    "feast", "gumbo", "fricassee", "cassoulet" };
		const WordSet kSoup = { "soup", "broth", "bouillon", "consomme", "porridge", "gruel", "oatmeal", "congee" };
		const WordSet kPie = { "pie", "pasty", "crostata", "cobbler", "strudel", "quiche", "pot" };
		const WordSet kDessert = { "sweetroll", "cake", "cookie", "muffin", "scone", "pudding", "treat", "candy", "taffy", "brownie", "donut", "doughnut", "fudge",
			                       "pastry", "dumpling", "custard", "trifle", "sorbet", "marzipan", "nougat", "lollipop", "tart", "turnover", "danish", "pancake",
			                       "waffle", "cupcake", "chocolate", "caramel", "toffee", "macaron", "tiramisu", "flan", "mousse" };
		const WordSet kSweetIngredient = { "honey", "jam", "syrup", "marmalade", "jelly", "molasses", "nectar" };
		const WordSet kBread = { "bread", "loaf", "bun", "baguette", "flatbread", "tortilla", "pita", "cracker", "hardtack", "pretzel", "noodle", "pasta", "biscuit",
			                     "dough", "flour", "roll", "bagel", "croissant", "wrap", "sandwich" };
		const WordSet kVegetable = { "potato", "carrot", "cabbage", "leek", "tomato", "gourd", "pumpkin", "squash", "onion", "turnip", "beet", "lettuce", "corn", "bean",
			                         "pea", "yam", "radish", "cucumber", "salad", "vegetable", "greens", "asparagus", "broccoli", "cauliflower", "celery", "spinach",
			                         "kale", "parsnip", "rutabaga", "eggplant", "zucchini", "pepper", "olive", "artichoke", "fennel", "chard" };
		const WordSet kFruit = { "apple", "berry", "berries", "grape", "pear", "peach", "plum", "cherry", "melon", "watermelon", "orange", "lemon", "lime", "banana",
			                     "fig", "date", "cranberry", "strawberry", "blueberry", "raspberry", "pomegranate", "coconut", "mango", "pineapple", "apricot",
			                     "fruit", "snowberry", "jazbay", "nectarine", "tangerine", "persimmon", "currant", "gooseberry" };
		const WordSet kCheese = { "cheese", "fondue", "curd", "kefir", "yogurt", "yoghurt" };
		const WordSet kDairy = { "butter", "milk", "cream", "ghee", "whey" };
		const WordSet kMushroom = { "mushroom", "mushrooms", "toadstool", "morel", "chanterelle", "truffle" };

		// animals and cuts
		const WordSet kMeat = { "meat", "beef", "pork", "boar", "venison", "mutton", "lamb", "chicken", "pheasant", "rabbit", "hare", "goat", "horse", "dog", "wolf",
			                    "fox", "bear", "sabre", "sabrecat", "cat", "mammoth", "horker", "skeever", "deer", "elk", "human", "flesh", "hopper", "duck", "goose",
			                    "turkey", "bird", "hawk", "tern", "blackbird", "cow", "ox", "yak", "snake", "rat", "dragon", "troll", "leg", "snout", "tenderloin",
			                    "ribs", "rib", "breast", "thigh", "wing", "brisket", "sirloin", "loin", "shank", "tongue", "liver", "heart", "gizzard", "hock", "ham",
			                    "bacon", "sausage", "jerky", "kebab", "mincemeat", "caveworm", "bloodsucker", "leech", "wisp", "ichor", "hound", "nix", "scrib",
			                    "chaurus", "hunter", "pigeon", "quail", "grouse", "partridge", "ostrich", "camel", "pony", "donkey", "sheep", "cattle", "bull", "lizard",
			                    "frog", "toad", "worm", "grub", "larva", "insect", "bug", "beetle", "spider", "cricket", "grasshopper", "ant", "locust", "moth" };
		const WordSet kFish = { "fish", "salmon", "trout", "bass", "carp", "char", "pike", "perch", "cod", "eel", "slaughterfish", "catfish", "lamprey", "tuna",
			                    "sturgeon", "herring", "sardine", "anchovy", "mackerel", "pufferfish", "fillet", "filet", "whitefish", "snapper", "halibut", "sole",
			                    "minnow", "angler", "pearlfish", "lungfish", "koi", "roe", "caviar", "darter", "goldfish", "bream", "tilapia" };
		const WordSet kCrab = { "crab", "mudcrab", "clam", "lobster", "shrimp", "prawn", "oyster", "mussel", "scallop", "crawdad", "crayfish", "shellfish", "squid",
			                    "octopus", "snail", "crabmeat", "barnacle", "cockle", "whelk", "langoustine", "krill" };
		const WordSet kPart = { "meat", "leg", "snout", "breast", "thigh", "wing", "tenderloin", "ribs", "rib", "flesh", "fillet", "filet", "tongue", "liver", "heart",
			                    "gizzard", "shank", "brisket", "loin", "sirloin", "hock", "mincemeat" };
		const WordSet kSpecialFood = { "meal" };

		bool IsWordIn(const WordSet& a_set, const std::string& a_word)
		{
			if (a_set.count(a_word)) {
				return true;
			}
			if (a_word.size() > 3 && a_word.back() == 's' && a_word[a_word.size() - 2] != 's' && a_set.count(a_word.substr(0, a_word.size() - 1))) {
				return true;
			}
			if (a_word.size() > 4 && a_word.compare(a_word.size() - 2, 2, "es") == 0 && a_set.count(a_word.substr(0, a_word.size() - 2))) {
				return true;
			}
			return false;
		}

		// the first matching word of a set, or nullptr
		const std::string* Find(const std::vector<std::string>& a_words, const WordSet& a_set)
		{
			for (const auto& w : a_words) {
				if (IsWordIn(a_set, w)) {
					return &w;
				}
			}
			return nullptr;
		}

		bool Contains(const std::vector<std::string>& a_words, std::initializer_list<const char*> a_any)
		{
			for (const auto& w : a_words) {
				for (const char* t : a_any) {
					if (w == t) {
						return true;
					}
				}
			}
			return false;
		}

		std::string Lower(std::string_view a_text)
		{
			std::string s(a_text);
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return s;
		}

		int RecipeTier(const Input& a_in, int a_default)
		{
			if (a_in.recipeIngredients >= 4) {
				return 50;
			}
			if (a_in.recipeIngredients >= 1) {
				return 25;
			}
			return a_default;
		}
	}

	std::vector<std::string> Words(std::string_view a_text)
	{
		std::vector<std::string> out;
		std::string              cur;
		auto                     flush = [&]() {
            if (!cur.empty()) {
                out.push_back(Lower(cur));
                cur.clear();
            }
		};
		for (std::size_t i = 0; i < a_text.size(); ++i) {
			const unsigned char c = static_cast<unsigned char>(a_text[i]);
			if (!std::isalnum(c)) {
				flush();  // spaces, hyphens, apostrophes, underscores...
				continue;
			}
			// camelCase: a capital after a lower-case letter starts a new word ("FoxStew" -> fox, stew)
			if (!cur.empty() && std::isupper(c) && std::islower(static_cast<unsigned char>(cur.back()))) {
				flush();
			}
			cur.push_back(static_cast<char>(c));
		}
		flush();
		return out;
	}

	const char* StatName(Stat a_stat)
	{
		switch (a_stat) {
		case Stat::Health: return "Health";
		case Stat::Stamina: return "Stamina";
		case Stat::Magicka: return "Magicka";
		default: return "-";
		}
	}

	const char* TypeName(Type a_type)
	{
		switch (a_type) {
		case Type::Meat: return "Meat";
		case Type::Fish: return "Fish";
		case Type::Crab: return "Shellfish";
		case Type::Stew: return "Stew";
		case Type::Soup: return "Soup";
		case Type::Chowder: return "Chowder";
		case Type::Pie: return "Pie";
		case Type::Vegetable: return "Vegetable";
		case Type::Fruit: return "Fruit";
		case Type::Bread: return "Bread";
		case Type::Cheese: return "Cheese";
		case Type::Dessert: return "Dessert";
		case Type::Ale: return "Ale";
		case Type::Wine: return "Wine";
		default: return "-";
		}
	}

	Result Classify(const Input& a_in)
	{
		Result r;
		auto   words = Words(a_in.name);
		if (words.empty()) {
			words = Words(a_in.editorId);
		}
		const std::string phrase = Lower(a_in.name);
		auto              no = [&](const char* a_why) {
            r.ok = false;
            r.why = a_why;
            return r;
		};
		if (words.empty()) {
			return no("no name");
		}
		if (const auto* w = Find(words, kSkip)) {
			if (!(*w == "tea" && Find(words, kDessert)) && !(*w == "salt" && words.size() > 1 && Find(words, kCooked))) {
				return no(("not a regular food (" + *w + ")").c_str());
			}
		}

		// ---- drinks ------------------------------------------------------------------------------------------------------------
		const auto* wine = Find(words, kWineDrink);
		const auto* ale = Find(words, kAleDrink);
		if ((wine || ale) && !Find(words, kStew) && !Find(words, kDessert) && !Find(words, kPie)) {
			r.ok = true;
			r.drink = true;
			r.duration = 600;
			const bool isWine = wine && !(ale && !wine);
			r.stat = isWine ? Stat::Magicka : Stat::Stamina;
			r.type = isWine ? Type::Wine : Type::Ale;
			r.tier = (Find(words, kFineDrink) || a_in.value >= 18) ? 50 : 25;
			r.why = std::string(isWine ? "wine-like drink (" : "ale-like drink (") + *(isWine ? wine : ale) + ")";
			return r;
		}

		const auto* cooked = Find(words, kCooked);
		const auto* rawWord = Find(words, kRaw);
		const auto* stewWord = Find(words, kStew);
		const auto* soupWord = Find(words, kSoup);
		const auto* pieWord = Find(words, kPie);
		const auto* dessertWord = Find(words, kDessert);
		const bool  sweetRoll = phrase.find("sweet roll") != std::string::npos;
		const auto* breadWord = Find(words, kBread);
		const auto* vegWord = Find(words, kVegetable);
		const auto* fruitWord = Find(words, kFruit);
		const auto* cheeseWord = Find(words, kCheese);
		const auto* dairyWord = Find(words, kDairy);
		const auto* sweetWord = Find(words, kSweetIngredient);
		const auto* mushroomWord = Find(words, kMushroom);
		const auto* crabWord = Find(words, kCrab);
		const auto* fishWord = Find(words, kFish);
		const auto* meatWord = Find(words, kMeat);
		// fish added by other mods often have names no list knows ("Arctic Grayling"), but their editor ids say so ("...FishFoodArcticGrayling")
		const auto  idWords = Words(a_in.editorId);
		if (!fishWord && !crabWord) {
			fishWord = Find(idWords, WordSet{ "fish", "fishes" });
			crabWord = Find(idWords, kCrab);
		}
		const bool animal = crabWord || fishWord || meatWord;
		const bool  hotWord = Contains(words, { "hot" }) || a_in.warmth;
		r.hot = hotWord;
		// "pot pie" is a pie, but a bare "pot" is not
		if (pieWord && *pieWord == "pot" && !Contains(words, { "pie", "pies" })) {
			pieWord = nullptr;
		}

		auto done = [&](Stat a_stat, Type a_type, int a_tier, bool a_raw, std::string a_why) {
			r.ok = true;
			r.stat = a_stat;
			r.type = a_type;
			r.tier = a_tier;
			r.raw = a_raw;
			r.why = std::move(a_why);
			return r;
		};
		auto animalType = [&]() { return crabWord ? Type::Crab : (fishWord ? Type::Fish : Type::Meat); };
		auto animalWord = [&]() -> std::string { return crabWord ? *crabWord : (fishWord ? *fishWord : (meatWord ? *meatWord : std::string())); };

		// ---- cooked dishes -----------------------------------------------------------------------------------------------------
		if (stewWord) {
			const bool chowder = *stewWord == "chowder" || *stewWord == "bisque" || crabWord;
			return done(Stat::Health, chowder ? Type::Chowder : Type::Stew, 50, false, "stew-like dish (" + *stewWord + ")");
		}
		if (soupWord) {
			if (animal) {
				return done(Stat::Health, crabWord ? Type::Chowder : Type::Stew, 50, false, "meat soup is a stew (" + animalWord() + " " + *soupWord + ")");
			}
			int tier = (phrase.find(" and ") != std::string::npos || phrase.find(" & ") != std::string::npos) ? 50 : RecipeTier(a_in, 25);
			return done(Stat::Stamina, Type::Soup, tier, false, "soup (" + *soupWord + ")");
		}
		if (pieWord) {
			if (animal || Contains(words, { "meat", "mincemeat", "pasty" })) {
				return done(Stat::Health, Type::Pie, 50, false, "meat pie (" + *pieWord + ")");
			}
			const bool big = *pieWord != "tart";
			return done(Stat::Magicka, Type::Dessert, big ? 50 : 25, false, "fruit pie or crostata (" + *pieWord + ")");
		}
		if (dessertWord && animal && Contains(words, { "dumpling", "dumplings", "cake", "cakes" })) {
			// Gourmet removes meat dumplings (they become cooked meat); crab cakes and the like are fish dishes
			const bool cake = Contains(words, { "cake", "cakes" });
			return done(Stat::Health, animalType(), cake ? 50 : 25, false, "savoury " + *dessertWord + " is a meat dish (" + animalWord() + ")");
		}
		if (dessertWord || sweetRoll) {
			const std::string w = sweetRoll ? "sweet roll" : *dessertWord;
			return done(Stat::Magicka, Type::Dessert, (w == "pudding" || w == "cake") ? 50 : 25, false, "dessert (" + w + ")");
		}
		if (cheeseWord) {
			int tier = 10;
			if (Contains(words, { "fondue" })) {
				tier = 50;
			} else if (Contains(words, { "wheel", "bowl" })) {
				tier = 25;
			} else if (!Contains(words, { "wedge", "slice", "sliver" }) && a_in.value > 3) {
				tier = 25;
			}
			return done(Stat::Magicka, Type::Cheese, tier, false, "cheese (" + *cheeseWord + ")");
		}
		if (dairyWord) {
			return done(Stat::Magicka, Type::None, 10, false, "dairy (" + *dairyWord + ")");
		}
		if (sweetWord) {
			return done(Stat::Magicka, Type::None, 10, false, "sweet ingredient (" + *sweetWord + ")");
		}

		// ---- meat, fish and shellfish: raw or cooked ---------------------------------------------------------------------------
		if (animal && !(breadWord && !Find(words, kPart) && *breadWord != "loaf" && *breadWord != "roll")) {
			const bool partWord = Find(words, kPart) != nullptr;
			bool       raw = a_in.rawKeyword ? !cooked : (rawWord != nullptr || (!cooked && !a_in.warmth));
			// a bare "Venison" or "Bear Meat" is raw; "Horker Loaf", "Rabbit Haunch", "Salmon Steak" and the like are cooked
			if (cooked) {
				raw = false;
			}
			if (a_in.recipeIngredients >= 1 && !rawWord && !a_in.rawKeyword && !partWord) {
				raw = false;  // made at a cooking pot, so it is a dish
			}
			const std::string why = std::string(raw ? "raw " : "cooked ") + (crabWord ? "shellfish" : fishWord ? "fish" : "meat") + " (" + animalWord() +
			                        (cooked ? " " + *cooked : "") + ")";
			return done(Stat::Health, animalType(), raw ? 10 : 25, raw, why);
		}
		if (mushroomWord) {
			if (cooked) {
				return done(Stat::Health, Type::None, 25, false, "cooked mushrooms (" + *cooked + ")");
			}
			return no("raw mushrooms are an ingredient, not a food");
		}
		if (breadWord) {
			int def = (*breadWord == "flour" || (words.size() == 1 && (*breadWord == "bread" || *breadWord == "dough"))) ? 10 : 25;
			if (a_in.recipeIngredients >= 1 && *breadWord != "flour") {
				def = RecipeTier(a_in, 25);
			}
			if (def == 25 && words.size() > 1 && a_in.recipeIngredients < 0 && (*breadWord == "bread" || *breadWord == "loaf") && a_in.value > 10) {
				def = 50;
			}
			return done(Stat::Stamina, Type::Bread, def, false, "bread (" + *breadWord + ")");
		}
		if (vegWord) {
			const bool prepared = cooked || *vegWord == "salad";
			return done(Stat::Stamina, Type::Vegetable, prepared ? 25 : 10, false, std::string(prepared ? "prepared" : "raw") + " vegetable (" + *vegWord + ")");
		}
		if (fruitWord) {
			return done(Stat::Magicka, Type::Fruit, 10, false, "fruit (" + *fruitWord + ")");
		}
		if (cooked && *cooked != "spiced" && *cooked != "salted" && *cooked != "dried" && *cooked != "cured" && *cooked != "pan") {
			return done(Stat::Health, Type::None, 25, false, "cooked dish of something unknown (" + *cooked + ")");
		}
		return no("no food words recognised");
	}
}
