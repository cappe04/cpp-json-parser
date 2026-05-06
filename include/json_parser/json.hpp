// Copyright (c) 2026 Casper Bené
// SPDX-License-Identifier: BSD-2-Clause

/**
 * @file json.hpp
 * @brief All library functionality.
 */

#include "detail/detail.hpp"

#include <memory>
#include <string>
#include <sstream>
#include <cstdint>
#include <string_view>

/**
 * @namespace json
 * @brief All library functionality.
 * 
 * @details This namespace includes all if the libraries functionality.
 * 
 * Usage Example 1:
 * @code
 * json::Document doc = json::open_document("./students.json");
 * json::View grades = doc.top_level()["grades"];
 * std::cout << grades["Casper"].as<int>() << std::endl; // output: 100 
 * @endcode
 * 
 */
namespace json {

    /**
     * @struct View
     * @brief A view into a Document.

     * @warning If the document that a View instansce is pointing to is
     destroyed, using the View will result in "use after free".

     * @note As the constructor is private, use Document::top_view() to get
     access to the top level View, then use either View::operator[],
     View::try_get(), or View::try_at() to access subsequent views.

     * @see Document
     */
    struct View {

    public:
        
        /**
         * @brief Access fields of JSON object
         * @warning throws if key isn't in the object. To avoid this
         * use View::try_get()
         * @returns a View to the value paired with `key`.
         */
        const View operator[](std::string_view key) const;
         
         /**
         * @brief Access items of JSON array
         * @warning throws if index is out of range for the array type. To 
         * avoid this use View::try_at()
         * @returns a View to the value at `index`.
          */
        const View operator[](uint32_t index) const;

        /**
         * @brief Wrapper around View::operator[] that wont throw.
         * @return View if successfull, else `std::nullopt`
         */
        const std::optional<View> try_get(std::string_view key) const noexcept;
        
        /**
         * @brief Wrapper around View::operator[] that wont throw.
         * @return View if successfull, else `std::nullopt`
         */
        const std::optional<View> try_at(uint32_t index) const noexcept;

        /**
         * @brief Check if View is a literal type
         * @returns `ture` if View is literal type, else `false`.
         */
        bool is_literal() const noexcept;
        
        /**
         * @brief Check if View is a array type
         * @returns `ture` if View is array type, else `false`.
         */
        bool is_array() const noexcept;
        
        /**
         * @brief Check if View is a object type
         * @returns `ture` if View is object type, else `false`.
         */
        bool is_object() const noexcept;

        /**
         * @brief String representation of the view.
         * @note This will output the same as Document::to_string() if
         * the View is the top level View.
         */
        std::string to_string() const noexcept;

        /**
         * @brief Parse View to typename T
         * @warning This only works if this View instance is a literal type.
         * Will throw an error otherwise (which?).
         * @tparam T The type to parse View into.
         *  The build supported types are:
         * - int
         * - unsigned int
         * - float
         * - double
         * - std::string
         * - bool
         * 
         * To add support for a custom type, define:
         * @code
         * template<>
         * MyType View::as<MyType>() { ... }
         * @endcode
         */
        template<typename T>
        T as() const;

        /**
         * @brief Parse Views that may be null.
         * @tparam T Type to be parsed as. See View::as() for supported types.
         * @returns Parsed value as type `T` or `std::nullopt` if View is of type null.
         */
        template<typename T>
        std::optional<T> as_optional() const {
            if (node->value == detail::JsonValue::Null) {
                return std::nullopt;
            };
            return as<T>();
        };

        /**
         * @brief Wrapper around View::operator[] combined with View::as().
         * @tparam T Type to be parsed as. See View::as() for supported types.
         * @note Will throw the same errors as View::operator[] and View::as().
         */
        template<typename T>
        T get_as(std::string_view key) const { return (*this)[key].as<T>(); };
        
        /**
         * @brief Wrapper around View::operator[] combined with View::as().
         * @tparam T Type to be parsed as. See View::as() for supported types.
         * @note Will throw the same errors as View::operator[] and View::as().
         */
        template<typename T>
        T at_as(uint32_t index) const { return (*this)[index].as<T>(); };

    private:
        /**
         * @brief Private class constructor
         * @details This class can only be initilzed with
         * a pointer to the internal JSON tree structure.
         */
        View(const detail::JsonNode* const node);

        const detail::JsonNode* const node;

        friend class Document;
    };


    /**
     * @class Document
     * @brief Parsed JSON document. 
     * 
     * @details This class is the sole owner of the parsed JSON document. The data
     * can and should only be accessed through View instances.
     * 
     * @note This class can only be created from Parser::get_document()
     * 
     * @see Parser
     * @see View
     */
    class Document {
    public:
        /**
         * @brief Class destructor.
         * @details Frees the memory for the JSON document. Any attempt
         * to use a view pointing towards this document after this destructor is
         * called will result in "use after frees". 
         */
        ~Document();

        /**
         * @brief String representation of the JSON document.
         */
        std::string to_string() const noexcept;

        /**
         * @brief Gets the top level View.
         * @details Gets a View the top level statment of the JSON Document.
         * This will either be an object type or array type
         */
        View top_view() const noexcept;

        /**
         * @brief Copy constructor is deleted.
         * @details This class has unique ownership of its resource and 
         * cannot be copied.
         */
        Document(const Document&) = delete;

        /**
         * @brief Copy assignment is deleted.
         * @details Prevents accidental copying of JSON document.
         */
        Document& operator=(const Document&) = delete;
    private:
        /**
         * @brief Private constructor
         * @details This class can only be initilized with
         * a std::unique_ptr to the parsed JSON. 
         * To initilze a new document, use Parser::get_document().
         */
        Document(std::unique_ptr<detail::JsonNode> root,
                 std::shared_ptr<const std::string> source);

        std::unique_ptr<detail::JsonNode> root;
        std::shared_ptr<const std::string> source;

        friend class Parser;
    };
    

    /**
     * @class Parser
     * @brief Parses JSON
     * 
     * @details A class to parse JSON. In case of invalid JSON code
     * an error message will be provided with Parser::get_erros().
     * The validity of the JSON can be check with Parser::get_status();
     * 
     * The JSON will be parsed and saved into a Document instance which can
     * be exracted with Parser::get_document(). If this method isn't called
     * the Parser instance will own the document and free it when destroyed.
     * 
     * Example:
     * @code
     * // source = some std::shared_ptr to a const string
     * json::Parser parser(source);
     * parser.parse();
     * if(!parser.get_status()) {
     *      std::cout << parser.get_error() << std::endl;
     *      // Handle error
     * } else {
     *      std::cout << parser.to_string() << std::endl; // prints json file.
     * }
     * @endcode
     * 
     * @see Document
     */
    class Parser {
    public:
        /**
         * @brief Class constructor
         * @param source A std::shared_ptr to the JSON file as a string 
         */
        Parser(std::shared_ptr<const std::string> source);

        /**
         * @brief Class constructor
         * * @param source THe JSON file as a string
         */
        Parser(const std::string& source);
        /**
         * @brief Class destructor
         * @details Will free parsed JSON document if Parser::get_document()
         * havn't been called.
         */
        ~Parser();

        /**
         * @brief Parse JSON.
         * @details Will output eventuall parsing errors to internal buffer
         * which can be recieved with Parser::get_error().
         */
        void parse();

        /**
         * @brief Get status of parsing attempt
         * @return `true` if parsing was successfull and it's safe to
         * call Parser::get_doucent, `false` otherwise.
         */
        bool get_status() const noexcept;

        /**
         * @brief Gets the internal error buffer as a string.
         * @note Will give an empty string if no error has ouccred during
         * parsing.
         */
        std::string get_error() const noexcept;

        /**
         * @brief Gets the parsed document
         * @returns a Document that is the sole owner of the parsed JSON.
         * @warning This should only be called if Parser::get_status() is true.
         */
        Document get_document();

    private:
        struct Impl; // pImpl design pattern
        std::unique_ptr<Impl> impl;
        
    };

    
    /**
     * @brief Wrapper around Parser
     * @param source std::string or std::shared_ptr to the source code of the JSON. 
     * @return Parsed JSON document.
     * 
     * @warning Will raise exception if unable to parse JSON document. To avoid
     * this use the Parser class.
     * 
     * @see Parser
     */
    Document parse_string(const std::string& source);

    /**
     * @copydoc parse_string(const std::string&)
     */
    Document parse_string(std::shared_ptr<const std::string> source);
    
    /**
     * @brief Wrapper around Parser
     * @param path Path to file 
     * @return Parsed JSON document.
     * 
     * @warning Will raise exception if unable to parse JSON document. To avoid
     * this use the Parser class.
     * 
     * @warning Will raise exception if unable to open the file correctly.
     * 
     * @see Parser
     */
    Document parse_file(std::string_view path);

};