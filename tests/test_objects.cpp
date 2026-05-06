#include "testing.h"

#include "json_parser/json.hpp"


/**
 * @test Tests deeply nested objects
 */
void test_deep_nested() {
    json::Document doc = json::parse_file(TEST_PATH_TO("deep_nested_object.json"));
    json::View view = doc.top_view();
    auto res1 = view["system"]["logs"]["entries"][0]["context"]["trace"]["steps"][0]["details"]["substeps"][1]["retries"][1]["status"];
    auto res2 = view["system"]["logs"]["entries"][0]["context"]["trace"]["steps"][0]["details"]["substeps"][0]["result"];
    auto res3 = view["system"]["components"][1]["config"]["replicas"][0]["metrics"]["connections"]["history"][1]["value"];
    auto res4 = view["system"]["components"][1]["config"]["replicas"][0]["metrics"]["memory"]["used"];
    auto res5 = view["system"]["components"][0]["config"]["endpoints"][0]["auth"]["tokens"]["access"];
    auto res6 = view["system"]["metadata"]["owner"]["profile"]["contact"]["phones"][1]["number"];

    ASSERT_EQ(res1.as<std::string>(), "success");
    ASSERT_EQ(res2.as<std::string>(), "ok");
    ASSERT_EQ(res3.as<int>(), 110);
    ASSERT_EQ(res4.as<int>(), 2048);
    ASSERT_EQ(res5.as<std::string>(), "abc123");
    ASSERT_EQ(res6.as<std::string>(), "+987654321");
};

/**
 * @test Tests that get-methods returns and throws errors as they should.
 */
void test_get_methods() {
    json::Document doc = json::parse_file(TEST_PATH_TO("deep_nested_object.json"));
    json::View view = doc.top_view()["system"];

    ASSERT_EQ(view["id"].as<std::string>(), "root-001");
    ASSERT_DOESNT_THROW(view["logs"]);
    ASSERT_THROWS(view["flight-logs"]); // Doesn't exist
    ASSERT_DOESNT_THROW(view.try_get("flight-logs"));
    ASSERT_EQ(view.try_get("flight-logs"), std::nullopt);
    ASSERT_EQ(view.try_get("logs").value()["entries"][0]["id"].as<std::string>(), "log-1");
    
    ASSERT_EQ(view.get_as<std::string>("id"), "root-001");
    ASSERT_THROWS(view.get_as<std::string>("flight-logs"));

    json::Document doc2 = json::parse_file(TEST_PATH_TO("empty.json"));
    auto empty_view = doc2.top_view()["object"];
    ASSERT_THROWS(empty_view["asdfcfgbhnm"]);
    ASSERT_DOESNT_THROW(empty_view.try_get("dfghjkjnbvghjnb"));
    ASSERT_TRUE(empty_view.is_object());
    ASSERT_FALSE(empty_view.is_array());
    ASSERT_FALSE(empty_view.is_literal());
}


TEST_CASE(test_objects) {
    test_deep_nested();
    test_get_methods();
}