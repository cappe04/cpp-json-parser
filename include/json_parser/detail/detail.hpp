/**
 * @file detail/detail.hpp
 * @brief Private library functionality.
 */

#include <stdint.h>
#include <string_view>
#include <optional>

/**
 * @namespace json::detail
 * @brief Private library functionality.
*/
namespace json::detail {

    /**
     * @enum JsonValue
     * @brief Enum for json values: https://www.json.org/json-en.html
     */
    enum class JsonValue { String, Number, Object, Array, True, False, Null };

    /**
     * @struct JsonNode
     * @brief Nodes for tree structure
     */
    struct JsonNode {
        JsonValue value;
        union {
            struct {
                JsonNode* child;
                uint32_t size;
            } composite; // Object or Array
            struct {
                std::string_view sv;
            } literal;
        };

        /**
         * @brief Constructor
         * @details string_view in the union needs this
         */
        JsonNode();
        JsonNode(JsonValue value);
        JsonNode(JsonValue value, std::string_view sv);

        /**
         * @brief Free node and subsequent nodes.
         */
        void free() noexcept;

        /**
         * @brief Append string representation of node and subsequent nodes to std::stringstream.
         */
        void ss_append(std::stringstream& ss, int indent) const noexcept;

        /**
         * @brief String representation of node and subsequent nodes.
         */
        std::string to_string() const noexcept;
    };
};