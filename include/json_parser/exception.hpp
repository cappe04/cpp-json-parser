// Copyright (c) 2026 Casper Bené
// SPDX-License-Identifier: BSD-2-Clause

/**
 * @file exception.hpp
 * @brief Library exceptions
 * @details Macro `JSON_PARSER_NO_EXCEPTION` can be defined to do `std::abort` instead
 * of throwing.
 */

#pragma once

#include <stdexcept>
#include <string>

#ifndef JSON_PARSER_NO_EXCEPTION

/**
 * @def JSON_PARSER_THROW(err, msg)
 * @brief Macro that throws
 */
#define JSON_PARSER_THROW(err, msg) throw err(msg)

/**
 * @namespace json::exception
 * @brief Library exceptions
 */
namespace json::exception {

    /**
     * @class JsonError
     * @brief Base class for all library exceptions
     */
    class JsonError : public std::runtime_error {
    public:
        explicit JsonError(const std::string& msg)
        : std::runtime_error(msg) {}; 
    };

    /**
     * @class ParseError
     * @brief Parsing errors ins't handled properly.
     */
    class ParseError : public JsonError {
    public:
        explicit ParseError(const std::string& msg)
        : JsonError("json::ParseError: " + msg) {}; 
    };

    /**
     * @class KeyError
     * @brief Trying to access key that doesn't exist
     */
    class KeyError : public JsonError {
    public:
        explicit KeyError(const std::string& msg)
        : JsonError("json::KeyError: " + msg) {}; 
    };

    /**
     * @class IndexOutOfRange
     * @brief Trying to element that is out of range.
     */
    class IndexOutOfRange : public JsonError {
    public:
        explicit IndexOutOfRange(const std::string& msg)
        : JsonError("json::IndexOutOfRange: " + msg) {}; 
    };

    /**
     * @class TypeError
     * @brief Trying to make an impossible cast or use in a way that isn't allowed.
     */
    class TypeError : public JsonError {
    public:
        explicit TypeError(const std::string& msg)
        : JsonError("json::TypeError: " + msg) {}; 
    };
};


#else
    #include <cstdlib>
    #define JSON_PARSER_THROW(err, msg) std::abort()
#endif