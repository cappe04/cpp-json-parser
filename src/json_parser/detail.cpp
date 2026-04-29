// Copyright (c) 2026 Casper Bené
// SPDX-License-Identifier: BSD-2-Clause

#include "json_parser/detail/detail.hpp"

#include <sstream>

using namespace json::detail;

JsonNode::JsonNode() : value(JsonValue::Null), composite{nullptr, 0} {};
JsonNode::JsonNode(JsonValue value)
: value(value), composite{nullptr, 0}
{};
JsonNode::JsonNode(JsonValue value, std::string_view sv) 
: value(value), literal{sv}
{};

void JsonNode::free() noexcept {
    if (value == JsonValue::Array || value == JsonValue::Object) {
        for(int i = 0; i<composite.size; ++i) {
            composite.child[i].free();
        }
        delete[] composite.child;
    }
};

void JsonNode::ss_append(std::stringstream& ss, int indent) const noexcept {
    switch (value) {
    case JsonValue::Null:
    case JsonValue::True:
    case JsonValue::False:
    case JsonValue::Number:
        ss << literal.sv;
        break;
    case JsonValue::String:
        ss << '\"' << literal.sv << '\"';
        break;
    case JsonValue::Array: {
        if(composite.child == nullptr) {
            ss << "[]";
            break;
        };
        ss << '[' << std::endl;
        for(int i = 0; i<composite.size; ++i) {
            for(int i = 0; i<indent+1; ++i) ss << '\t';
            composite.child[i].ss_append(ss, indent+1);
            ss << ',' << std::endl;
        }
        for(int i = 0; i<indent; ++i) ss << '\t';
        ss << ']';
        break;
    }
    case JsonValue::Object: {
        if(composite.child == nullptr) {
            ss << "{}";
            break;
        };
        ss << '{' << std::endl;
        for(int i = 0; i<composite.size; i += 2) {
            for(int j = 0; j<indent+1; ++j) ss << '\t';
            composite.child[i].ss_append(ss, indent+1);
            ss << ": ";

            composite.child[i+1].ss_append(ss, indent+1);
            ss << ',' << std::endl;
        }
        for(int k = 0; k<indent; ++k) ss << '\t';
        ss << '}';
        break;
    }
    default:
        ss << "Unknown printing error" << std::endl;
        break;
    }
}

std::string JsonNode::to_string() const noexcept {
    std::stringstream ss;
    ss_append(ss, 0);
    return ss.str();
};