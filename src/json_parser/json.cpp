// Copyright (c) 2026 Casper Bené
// SPDX-License-Identifier: BSD-2-Clause

#include "json_parser/json.hpp"

#include <cctype>
#include <charconv>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <stdexcept>

#define JSON_KEY_ERROR "json_parser::KeyError"
#define JSON_INDEX_OUT_OF_RANGE "json_parser::IndexOutOfRange"
#define JSON_PARSE_ERROR "json_parser::ParseError"
#define JSON_WRONG_TYPE "json_parser::WrongType"
#define JSON_THROW(err, txt) throw std::runtime_error(err ": " txt)

using namespace json;
using namespace json::detail;


///////////////////////////////////
////////////// View ///////////////
///////////////////////////////////

View::View(const detail::JsonNode* const node) : node(node) {};

const View json::View::operator[](std::string_view key) const {
    if(!is_object()) {
        JSON_THROW(JSON_WRONG_TYPE, "View is not object type.");
    }
    for(uint32_t i = 0; i < node->composite.size; i += 2) {
        if(node->composite.child[i].literal.sv == key) {
            return View(&node->composite.child[i+1]);
        }
    }

    JSON_THROW(JSON_KEY_ERROR, "Key not in View.");
}

const View View::operator[](uint32_t index) const {
    if(is_array() && index >= node->composite.size) {
        JSON_THROW(JSON_INDEX_OUT_OF_RANGE, "Index out of range.");
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

///// View::as templates //////

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
        JSON_THROW(JSON_WRONG_TYPE, "Could not convert View to " #t); \
    }; \
    std::optional<t> value = as_number<t>(node->literal.sv); \
    if(value) { \
        return value.value(); \
    } \
    JSON_THROW(JSON_WRONG_TYPE, "Could not convert View to " #t)


template<>      int              View::as<int>()             const { VIEW_AS_NUMBER(int); };
template<>      float            View::as<float>()           const { VIEW_AS_NUMBER(float); };
template<>      unsigned int     View::as<unsigned int>()    const { VIEW_AS_NUMBER(unsigned int); };
template<>      double           View::as<double>()          const { VIEW_AS_NUMBER(double); };

// Not numbers
template<>
std::string View::as<std::string>() const {
    if(!is_literal()) {
        JSON_THROW(JSON_WRONG_TYPE, "View must be literal type to convert to std::string.");
    };
    return std::string(node->literal.sv);
};

template<>
std::string_view View::as<std::string_view>() const {
    if(!is_literal()) {
        JSON_THROW(JSON_WRONG_TYPE, "View must be literal type to convert to std::string_view.");
    };
    return node->literal.sv;
};

template<>
bool View::as<bool>() const {
    return node->value != JsonValue::False;
};



namespace {

    enum class Status { Success, Failure, Error };

    /////////////////////////////////////
    ////////////// Lexer ////////////////
    /////////////////////////////////////


    struct LexerResult {
        std::string_view sv;
        Status status;
        
        static constexpr std::string_view empty_sv = "";
        inline static LexerResult fail() { return LexerResult { empty_sv, Status::Failure }; };
        inline static LexerResult success(std::string_view sv) { return LexerResult { sv, Status::Success }; };
    };

    /**
     * @brief Private lexer class
     */
    class Lexer {
    public:
        std::shared_ptr<const std::string> source;
        uint32_t end;
        uint32_t row;
        uint32_t column;
        uint32_t start;

        Lexer(std::shared_ptr<const std::string> source)
        : source(source), end(0), start(0), row(1), 
          column(1), row_attempt(0), col_attempt(0),
          source_view(*source)
        {};

        char peek() const { return source->at(end); }
        char consume() { ++column; return source->at(end++); }
        bool ok() const { return end < source->length(); }
        void begin_attempt() { start = end; row_attempt = row; col_attempt = column; };
        void reset_attempt() { end = start; row = row_attempt; column = col_attempt; };
        bool expect(char c) {
            if(peek() == c) {
                consume();
                return true;
            }
            return false;
        };

        inline bool is_digit() const { return std::isdigit(static_cast<unsigned char>(peek())); }
        inline bool is_alnum() const { return std::isalnum(static_cast<unsigned char>(peek())); }

        void whitespace() {
            if(!ok()) return;

            if(peek() == '\n') {
                ++row;
                column = 0;
            };

            switch (peek()) {
            case ' ':
            case '\n':
            case '\t':
            case '\r':
                consume();
                whitespace();
            default:
                return;
            }
        };

        LexerResult get_string() {
            if(!expect('\"')) return LexerResult::fail();

            begin_attempt();
            while(ok() && !expect('\"')) { 
                if(peek() == '\n') {
                    reset_attempt();
                    return LexerResult::fail();
                }
                consume();
            }

            return LexerResult::success(source_view.substr(start, end-start-1));
        };

        LexerResult get_keyword(const std::string& keyword) {
            begin_attempt();

            for(auto it = keyword.cbegin(); it != keyword.cend(); ++it) {
                if(!ok() || consume() != *it) {
                    reset_attempt();
                    return LexerResult::fail();
                }
            };

            if(ok() && is_alnum()) {
                reset_attempt();
                return LexerResult::fail();
            }

            return LexerResult::success(source_view.substr(start, end - start));
        };

        LexerResult get_number() {
            begin_attempt();

            expect('-');

            if(expect('0')) {
                goto decimal_or_exponent;
            }

            if(!(ok() && is_digit())) {
                reset_attempt();
                return LexerResult::fail();
            }

            while(ok() && is_digit()) consume();

            decimal_or_exponent:

            if(ok() && peek() == '.') {
                consume();
                if(!ok() || !is_digit()) {
                    reset_attempt();
                    return LexerResult::fail();
                }
                while(ok() && is_digit()) consume();
            }
            
            if(ok() && (peek() == 'e' || peek() == 'E')) {
                consume();
                ok() && (expect('-') || expect('+'));
                if(!ok() || !is_digit()) {
                    reset_attempt();
                    return LexerResult::fail();
                }
                while(ok() && is_digit()) consume();
            }

            if(start - end == 0) {
                reset_attempt();
                return LexerResult::fail();
            }

            return LexerResult::success(source_view.substr(start, end - start));
        };

        LexerResult get_char(char c) {
            if (ok() && peek() == c) {
                consume();
                return LexerResult::success(source_view.substr(end, 1));
            }
            return LexerResult::fail();
        }

    private:
        std::string_view source_view;
        uint32_t row_attempt;
        uint32_t col_attempt;
    };


    /////////////////////////////////////
    ////////// Parser Helpers ///////////
    /////////////////////////////////////

    struct ParserResult {
        Status status;
        union {
            JsonNode node;
            char __dummy;
        };

        ParserResult() : status(Status::Failure), __dummy('\0') {};
        ParserResult(Status status) : status(status), __dummy('\0') {};
        ParserResult(Status status, JsonNode node) : status(status), node(node) {};

        static ParserResult fail() { return ParserResult(Status::Failure); };
        static ParserResult error() { return ParserResult(Status::Error); };
        static ParserResult success(JsonNode node) { return ParserResult(Status::Success, node); };
    };


    class NodeBuider {
    public:
        NodeBuider(JsonValue value) 
        : node(JsonNode(value))
        {};

        JsonNode build() {
            node.composite.child = new JsonNode[children.size()];
            node.composite.size = children.size();
            std::copy(children.begin(), children.end(), node.composite.child);
            return node;
        };

        void add_child(JsonNode child_node) {
            children.push_back(child_node);
        }

        void bail() {
            for(auto it = children.begin(); it != children.end(); ++it) {
                it->free();
            };
        };
    private:
        std::vector<JsonNode> children;
        JsonNode node;
    };

}


////////////////////////////////////
///////// Private Parser ///////////
////////////////////////////////////

struct Parser::Impl {
    std::unique_ptr<JsonNode> root;
    Lexer lexer;
    std::stringstream err;
    bool status;

    void error_expected(std::string expected) {
        err << "Expected a \"" << expected
            << "\" at row " << lexer.row 
            << " and column "<< lexer.column 
            << ", got " << (*lexer.source)[lexer.end] 
            << std::endl;
    }

    ParserResult parse_value() {

        lexer.whitespace();

        LexerResult lr;

        #define TRY_PARSE_SIMPLE(x, t) \
            lr = lexer.x; \
            if(lr.status == Status::Success) { \
                JsonNode node = JsonNode(t, lr.sv); \
                lexer.whitespace(); \
                return ParserResult::success(node);\
            }

        TRY_PARSE_SIMPLE(get_keyword("null"), JsonValue::Null);
        TRY_PARSE_SIMPLE(get_keyword("false"), JsonValue::False);
        TRY_PARSE_SIMPLE(get_keyword("true"), JsonValue::True);
        TRY_PARSE_SIMPLE(get_string(), JsonValue::String);
        TRY_PARSE_SIMPLE(get_number(), JsonValue::Number);

        ParserResult pr;

        #define TRY_PARSE_COMPOSITE(x) \
            pr = x; \
            if(pr.status == Status::Success) { \
                lexer.whitespace(); \
                return ParserResult::success(pr.node); \
            } \
            else if(pr.status == Status::Error) { \
                return ParserResult::error(); \
            }
        
        TRY_PARSE_COMPOSITE(parse_array());
        TRY_PARSE_COMPOSITE(parse_object());

        // Syntax error
        err << "Syntax Error at row " << lexer.row 
            << " and column " << lexer.column 
            << ", \"" << (*lexer.source)[lexer.end] << "\"" 
            << std::endl;
        return ParserResult::error();
    };

    ParserResult parse_array() {
        if(lexer.get_char('[').status != Status::Success) {
            return ParserResult::fail();
        }
        
        lexer.whitespace();

        NodeBuider builder(JsonValue::Array);

        if(lexer.get_char(']').status == Status::Success) {
            return ParserResult::success(builder.build());
        };

        while (true) {
            ParserResult pr = parse_value();
            if(pr.status == Status::Success) {
                builder.add_child(pr.node);
            } else {
                builder.bail();
                return ParserResult::error();
            }

            if(lexer.get_char(']').status == Status::Success) {
                return ParserResult::success(builder.build());
            }

            if(lexer.get_char(',').status != Status::Success) {
                error_expected(",");
                builder.bail();
                return ParserResult::error();
            }
        };
    };

    ParserResult parse_object() {
        if(lexer.get_char('{').status != Status::Success) return ParserResult::fail();
        
        lexer.whitespace();

        NodeBuider builder(JsonValue::Object);

        if(lexer.get_char('}').status == Status::Success) {
            return ParserResult::success(builder.build());
        };

        while(true) {

            lexer.whitespace();

            LexerResult lr = lexer.get_string();
            if(lr.status == Status::Success) {
                builder.add_child(JsonNode(JsonValue::String, lr.sv));
            } else {
                error_expected("string");
                builder.bail();
                return ParserResult::error();
            }

            lexer.whitespace();

            if(lexer.get_char(':').status != Status::Success) {
                error_expected(":");
                builder.bail();
                return ParserResult::error();
            }

            ParserResult pr = parse_value();
            if(pr.status == Status::Success) {
                builder.add_child(pr.node);
            } else {
                builder.bail();
                return ParserResult::error();
            }

            if(lexer.get_char('}').status == Status::Success) {
                return ParserResult::success(builder.build());
            }

            if(lexer.get_char(',').status != Status::Success) {
                error_expected(",");
                builder.bail();
                return ParserResult::error();
            }
        }
    };
};

/////////////////////////////////////
/////////// Public Parser ///////////
/////////////////////////////////////

Parser::Parser(std::shared_ptr<const std::string> source)
: impl(std::make_unique<Impl>(Impl { 
    .lexer = Lexer(source), 
    .status = false 
  }))
{}

Parser::Parser(const std::string &source)
: impl(std::make_unique<Impl>(Impl { 
    .lexer = Lexer(std::make_shared<const std::string>(source)), 
    .status = false 
  }))
{}

Parser::~Parser() {
    if(impl->root) {
        impl->root->free();
    }
}

void Parser::parse() {
    ParserResult pr = impl->parse_array();
    if(pr.status == Status::Success) {
        impl->status = true;
        impl->root = std::make_unique<JsonNode>(pr.node);
        return;
    } else if(pr.status == Status::Error) {
        return;
    }

    pr = impl->parse_object();
    if(pr.status == Status::Success) {
        impl->status = true;
        impl->root = std::make_unique<JsonNode>(pr.node);
        return;
    } else if(pr.status == Status::Error) {
        return;
    }
}

bool Parser::get_status() const noexcept {
    return impl->status;
}

std::string Parser::get_error() const noexcept {
    return impl->err.str();
}

Document Parser::get_document() {
    if(!impl->status) JSON_THROW(JSON_PARSE_ERROR, "Unable to get JSON document as an error occured during parsing or as it is un-parsed.");
    return Document(std::move(impl->root), impl->lexer.source);
}


///////////////////////////////
////////// Document ///////////
///////////////////////////////


Document::Document(std::unique_ptr<detail::JsonNode> root,
                   std::shared_ptr<const std::string> source)
: root(std::move(root)), source(source)
{}

Document::~Document() {
    if(root) {
        root->free();
    };
}

std::string Document::to_string() const noexcept {
    return root->to_string();
}

View Document::top_view() const noexcept {
    return View(root.get());
}


//////////////////////////////
////////// functions /////////
//////////////////////////////

Document json::parse_string(const std::string& source) {
    Parser parser(source);
    parser.parse();
    if(!parser.get_status()) {
        JSON_THROW(JSON_PARSE_ERROR, "Unable to parse file.");
    }
    return parser.get_document();
};

Document json::parse_string(std::shared_ptr<const std::string> source) {
    Parser parser(source);
    parser.parse();
    if(!parser.get_status()) {
        JSON_THROW(JSON_PARSE_ERROR, "Unable to parse file.");
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
