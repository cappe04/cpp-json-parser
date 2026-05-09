// Copyright (c) 2026 Casper Bené
// SPDX-License-Identifier: BSD-2-Clause

#include "json_parser/json.hpp"
#include "json_parser/exception.hpp"
#include "json_parser/detail/detail.hpp"

#include <fstream>

using namespace json;
using namespace json::detail;
using namespace json::exception;

Document json::parse_string(const std::string& source) {
    Parser parser(source);
    parser.parse();
    if(!parser.get_status()) {
        JSON_PARSER_THROW(ParseError, "Unable to parse file.");
    }
    return parser.get_document();
};

Document json::parse_string(std::shared_ptr<const std::string> source) {
    Parser parser(source);
    parser.parse();
    if(!parser.get_status()) {
        JSON_PARSER_THROW(ParseError, "Unable to parse file.");
    }
    return parser.get_document();
};

Document json::parse_file(std::string_view path)
{
    std::string string_path(path);
    std::ifstream file(string_path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    return parse_string(source);
}