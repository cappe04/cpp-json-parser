// Copyright (c) 2026 Casper Bené
// SPDX-License-Identifier: BSD-2-Clause

#include "json_parser/json.hpp"
#include "json_parser/exception.hpp"
#include "json_parser/detail/detail.hpp"

#include <charconv>

using namespace json;
using namespace json::detail;
using namespace json::exception;

View::View(const detail::JsonNode* const node) : node(node) {};

const View json::View::operator[](std::string_view key) const {
    if(!is_object()) {
        JSON_PARSER_THROW(TypeError, "View is not object type.");
    }
    for(uint32_t i = 0; i < node->composite.size; i += 2) {
        if(node->composite.child[i].literal.sv == key) {
            return View(&node->composite.child[i+1]);
        }
    }

    JSON_PARSER_THROW(KeyError, "Key not in View.");
}

const View View::operator[](uint32_t index) const {
    if(is_array() && index >= node->composite.size) {
        JSON_PARSER_THROW(IndexOutOfRange, "Index out of range.");
    }
    return View(&node->composite.child[index]);
}

const std::optional<View> View::try_get(std::string_view key) const noexcept {
    if(!is_object()) {
        return std::nullopt;
    }
    for(uint32_t i = 0; i < node->composite.size; i += 2) {
        if(node->composite.child[i].literal.sv == key) {
            return View(&node->composite.child[i+1]);
        }
    }

    return std::nullopt; // not in View
}

const std::optional<View> View::try_at(uint32_t index) const noexcept {
    if(is_array() && index >= node->composite.size) {
        return std::nullopt;
    }
    return View(&node->composite.child[index]);
}

bool View::is_literal() const noexcept {
    return node->value > JsonValue::Array;
}

bool View::is_object() const noexcept {
    return node->value == JsonValue::Object;
}

bool View::is_array() const noexcept {
    return node->value == JsonValue::Array;
}

std::string View::to_string() const noexcept {
    return node->to_string();
}

uint32_t json::View::array_size() const noexcept {
    if(!is_array()) {
        return 0;
    };
    
    return node->composite.size;
};

std::vector<std::string> json::View::object_keys() const noexcept {
    std::vector<std::string> vec;
    if(!is_object()) {
        return vec;
    };

    for(int i = 0; i<node->composite.size; i += 2) {
        std::string s(node->composite.child[i].literal.sv);
        vec.push_back(s);
    };

    return vec;
} 


// View::as template

// Numbers
template<typename T>
static std::optional<T> as_number(std::string_view sv) {
    T value;
    auto res = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if(res.ec == std::errc()) {
        return value;
    }
    return std::nullopt;
}

#define VIEW_AS_NUMBER(t) \
    if(node->value != JsonValue::Number) { \
        JSON_PARSER_THROW(TypeError, "Could not convert View to " #t); \
    }; \
    std::optional<t> value = as_number<t>(node->literal.sv); \
    if(value) { \
        return value.value(); \
    } \
    JSON_PARSER_THROW(TypeError, "Could not convert View to " #t)


template<> int              View::as<int>()             const { VIEW_AS_NUMBER(int); };
template<> float            View::as<float>()           const { VIEW_AS_NUMBER(float); };
template<> unsigned int     View::as<unsigned int>()    const { VIEW_AS_NUMBER(unsigned int); };
template<> double           View::as<double>()          const { VIEW_AS_NUMBER(double); };

// Not numbers
template<>
std::string View::as<std::string>() const {
    if(!is_literal()) {
        JSON_PARSER_THROW(TypeError, "View must be literal type to convert to std::string.");
    };
    return std::string(node->literal.sv);
};

template<>
std::string_view View::as<std::string_view>() const {
    if(!is_literal()) {
        JSON_PARSER_THROW(TypeError, "View must be literal type to convert to std::string_view.");
    };
    return node->literal.sv;
};

template<>
bool View::as<bool>() const {
    return node->value != JsonValue::False;
};