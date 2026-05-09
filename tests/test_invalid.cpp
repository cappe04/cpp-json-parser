#include "testing.h"

#include "json_parser/json.hpp"

using namespace json;

/**
 * @test Tests that json syntax errors a caught and documented.
 */
void test_syntax_errors() {
    for (int i = 1; i<14; ++i) {
        std::string path = "../tests/json/invalid/" + std::to_string(i) + ".json"; // riktigt korkat att lägga de små felen i egna filer!
        const std::string source = test::open_file(path);
        Parser parser(source);
        parser.parse();
        if(parser.get_status()){
            auto doc = parser.get_document();
            auto message = doc.to_string();
            test::fail(message);
        } else {
            ASSERT_NE(parser.get_error(), ""); // Parsing errors must have an explanation!
        }
    }
}

TEST_CASE(test_invalid) {
    TEST(test_syntax_errors);
}