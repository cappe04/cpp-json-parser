#include "testing.h"

#include "json_parser/json.hpp"

using namespace json;

/**
 * @test Tests deeply nested arrays
 */
void test_deep_nested() {
    Document doc = parse_file(TEST_PATH_TO("deep_nested_array.json"));
    View view = doc.top_view();
    int x = view[0][0][0][0][0][0][0][0][0][0][0][0][0][0][0].as<int>(); // 15 st
    ASSERT_EQ(x, 8);
    int y = view[0][0][0][0][0][0][1].as<int>();
    ASSERT_EQ(y, 6);
}

/**
 * @test Tests that at-methods returns and throws errors as they should.
 */
void test_at_methods() {
    Document doc = parse_file(TEST_PATH_TO("deep_nested_array.json"));
    View view = doc.top_view();

    ASSERT_DOESNT_THROW(view[2]);
    ASSERT_THROWS(view[4]);
    ASSERT_THROWS(view[5]);
    ASSERT_DOESNT_THROW(view.try_at(67));

    ASSERT_EQ(view[3].as<std::string>(), "Hello!");
    ASSERT_EQ(view.at_as<std::string>(3), "Hello!");
    ASSERT_EQ(view.try_at(3).value().as<std::string>(), "Hello!");

    ASSERT_THROWS(view.at_as<std::string>(67));

    auto doc2 = parse_file(TEST_PATH_TO("empty.json"));
    auto empty_array = doc2.top_view()["array"];
    ASSERT_THROWS(empty_array[0]);
    ASSERT_THROWS(empty_array.at_as<int>(0));
    ASSERT_DOESNT_THROW(empty_array.try_at(0));
};

TEST_CASE(test_arrays) {
    TEST(test_deep_nested);
    TEST(test_at_methods);
}