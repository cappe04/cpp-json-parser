#include "json_parser/json.hpp"
#include "json_parser/exception.hpp"

#include <memory>
#include <iostream>

using namespace json;

static const std::string source1 = R"({
    "name": "ShadowBlade",
    "level": 42,
    "health": 98.5,
    "isNPC": false,
    "title": null,
    "guild": "Night Watchers",
    "weapon": {
        "name": "Void Katana",
        "damage": 120
    },
    "skills": ["dash", "invisibility", "backstab"],
    "pet": null
})";

static const std::string source2 = R"({
    "name": "ShadowBlade",
    "level": 42,
    "health": 98.5,
    "isNPC": false
    "title": null,
    "guild": "Night Watchers",
    "weapon": {
        "name": "Void Katana",
        "damage": 120
    },
    "skills": ["dash", "invisibility", "backstab"],
    "pet": null
})";


struct Weapon {
    std::string name;
    float damage;
};

template<>
Weapon View::as<Weapon>() const {
    if (!is_object()) {
        JSON_PARSER_THROW(exception::TypeError, "Cannot parse non-object to struct Weapon.");
    };
    return Weapon {
        get_as<std::string>("name"),
        get_as<float>("damage"),
    };
};


std::unique_ptr<Document> load_character(const std::string& source) {
    Parser parser(source);
    parser.parse();
    if(!parser.get_status()) {
        std::cout << "Error while loading character json: ";
        std::cout << parser.get_error() << std::endl;
        exit(1);
    }
    return parser.get_document_ptr();
}

int main() {

    auto character_doc = load_character(source1);
    View character = character_doc->top_view();
    
    std::cout << "=== Character Loaded ===\n";
    std::cout << "Name: "   << character["name"].as<std::string>() << std::endl;
    std::cout << "Level: "  << character["level"].as<unsigned int>() << std::endl;
    std::cout << "Health: " << character["health"].as<float>() << std::endl;
    std::cout << "Is NPC: " << character["isNPC"].as<bool>() << std::endl;
    std::cout << "Title: "  << character["title"].as_optional<std::string>().value_or("N/A") << std::endl; 
    std::cout << "Guild: "  << character["guild"].as_optional<std::string>().value_or("N/A") << std::endl;
    std::cout << "Pet: "    << character["pet"].as_optional<std::string>().value_or("N/A");
    std::cout << "\n" << std::endl;

    Weapon weapon = character["weapon"].as<Weapon>();
    std::cout << "Weapon: "
              << weapon.name 
              << " (DMG: " << weapon.damage << ")";
    std::cout << "\n" << std::endl;
    
    std::cout << "Skills: " << std::endl;
    for(int i = 0; i<character["skills"].array_size(); ++i) {
        std::cout << i+1 << ". " << character["skills"][i].as<std::string>() << std::endl;
    };

    std::cout << "\n" << std::endl;

    std::cout << "=== Error demo ===" << std::endl;
    auto character2_doc = load_character(source2);

    return 0;
}