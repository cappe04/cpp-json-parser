// Copyright (c) 2026 Casper Bené
// SPDX-License-Identifier: BSD-2-Clause

#include "json_parser/json.hpp"
#include "json_parser/detail/detail.hpp"
#include "json_parser/exception.hpp"

using namespace json;
using namespace json::detail;
using namespace json::exception;

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
            detail::JsonNode node;
            char __dummy;
        };

        ParserResult() : status(Status::Failure), __dummy('\0') {};
        ParserResult(Status status) : status(status), __dummy('\0') {};
        ParserResult(Status status, detail::JsonNode node) : status(status), node(node) {};

        static ParserResult fail() { return ParserResult(Status::Failure); };
        static ParserResult error() { return ParserResult(Status::Error); };
        static ParserResult success(detail::JsonNode node) { return ParserResult(Status::Success, node); };
    };


    class NodeBuider {
    public:
        NodeBuider(JsonValue value) 
        : node(detail::JsonNode(value))
        {};

        detail::JsonNode build() {
            node.composite.child = new detail::JsonNode[children.size()];
            node.composite.size = children.size();
            std::copy(children.begin(), children.end(), node.composite.child);
            return node;
        };

        void add_child(detail::JsonNode child_node) {
            children.push_back(child_node);
        }

        void bail() {
            for(auto it = children.begin(); it != children.end(); ++it) {
                it->free();
            };
        };
    private:
        std::vector<detail::JsonNode> children;
        detail::JsonNode node;
    };

}


////////////////////////////////////
///////// Private Parser ///////////
////////////////////////////////////

struct Parser::Impl {
    std::unique_ptr<detail::JsonNode> root;
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
                detail::JsonNode node = detail::JsonNode(t, lr.sv); \
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
                builder.add_child(detail::JsonNode(JsonValue::String, lr.sv));
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

void Parser::parse() noexcept {
    ParserResult pr = impl->parse_array();
    if(pr.status == Status::Success) {
        impl->status = true;
        impl->root = std::make_unique<detail::JsonNode>(pr.node);
        return;
    } else if(pr.status == Status::Error) {
        return;
    }

    pr = impl->parse_object();
    if(pr.status == Status::Success) {
        impl->status = true;
        impl->root = std::make_unique<detail::JsonNode>(pr.node);
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
    if(!impl->status) JSON_PARSER_THROW(ParseError, "Unable to get JSON document as an error occured during parsing or as it is un-parsed.");
    impl->status = false;
    return Document(std::move(impl->root), impl->lexer.source);
}

std::unique_ptr<Document> Parser::get_document_ptr() {
    if(!impl->status) JSON_PARSER_THROW(ParseError, "Unable to get JSON document as an error occured during parsing or as it is un-parsed.");
    impl->status = false;
    return std::unique_ptr<Document>(new Document(std::move(impl->root), impl->lexer.source));
}
