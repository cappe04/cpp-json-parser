#include "json_parser/json.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

const std::string open_file(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Not actual test btw
int main() {

    { 
        json::Document doc = json::parse_file("tests/json/1.json");

        std::cout << " === Whole document 1: ===" << std::endl;
        std::cout << doc.to_string() << std::endl;

        std::cout << "=== As Types 1: ===" << std::endl;
        json::View view = doc.top_view();
        std::cout << view[0].as<std::string>() << std::endl;
        std::cout << view[1].as<int>() << std::endl;
        std::cout << view[2].as_optional<std::string>().value_or("null") << std::endl;
        std::cout << view[3].as<bool>() << std::endl;
        std::cout << view[4].as<bool>() << std::endl;
        std::cout << view[5].as<float>() << std::endl;
        std::cout << view[5].as<double>() << std::endl;
        std::cout << view[5].as<bool>() << std::endl;
    }
    
    {
        // json::Document doc = json::parse_file("tests/json/2.json");
        const std::string source = open_file("tests/json/2.json");
        json::Parser parser(source);
        parser.parse();
        if(!parser.get_status()) {
            std::cout << parser.get_error() << std::endl;
        }
        json::Document doc = parser.get_document();

        std::cout << " === Whole document 2: ===" << std::endl;
        std::cout << doc.to_string() << std::endl;

        std::cout << "=== As Types 2: ===" << std::endl;
        json::View view = doc.top_view();
        std::cout << view["array"].to_string() << std::endl;
        std::cout << view["null"].as_optional<std::string>().value_or("null") << std::endl;
        std::cout << view["int"].as<int>() << std::endl;
        std::cout << view["float"].as<float>() << std::endl;
        std::cout << view["string"].as<std::string>() << std::endl;
        std::cout << view["object"].to_string() << std::endl;
        std::cout << view["object"]["67"].as<bool>() << std::endl;
        std::cout << view["object"]["68"].as<bool>() << std::endl;
        std::cout << view["object"]["69"].as<bool>() << std::endl;

        std::cout << "name: " << view["name"].as<std::string>() << std::endl;
        std::cout << "face: " << view["🤯"].as<std::string>() << std::endl;
        if(view.try_get("feet").has_value()) {
            std::cout << view.try_get("feet").value().as<std::string>() << std::endl;
        } else {
            std::cout << "feet: " << "feet: no feet" << std::endl;
        }
    }

    {
        std::cout << " === Code Example 1: ===" << std::endl;

        const std::string code = open_file("tests/json/3.json");
        json::Parser parser(code);
        parser.parse();
        if(!parser.get_status()) { // In case of incorrectly formated input
            std::cout << parser.get_error() << std::endl;
        } else {
            json::Document doc = parser.get_document();
            json::View view = doc.top_view();
            std::cout << view["foo"]["bar"][0].as<int>() << std::endl; // output: 1
        }
    }

    return 0;
}