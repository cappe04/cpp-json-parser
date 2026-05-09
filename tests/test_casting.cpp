#include "testing.h"

#include "json_parser/json.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <optional>

static int INTS_CORRECT[] = { 0, 1, -7, 42, 3, -7, 5, 100, 256, -1024, 9999, 2147483647, -2147483648 };
static unsigned int UINTS_CORRECT[] = { 0, 1, 2, 7, 42, 255, 1024, 65535, 2147483648, 4294967295, 3, 999 };
static double DOUBLE_CORRECT[] = { 0.0, 1.5, -3.14, 2.71828, -0.001, 42.0, 1e3, -5.2e-4, 6.022e23, -9.81, 3E8, -7.5E-2, 1.2345E+6, -8.91E12, 5.0e-10, 9.99E0, 123456.789, -0.0000001, 4.56E-15, 7.89e+30 };
static std::string STRINGS_CORRECT[] = { "null", "42", "hello", "true", "false", "3.14", "null", "null", "another string", "0", "null", "end" };
static std::vector<int> VECTOR_CORRECT = { 0, 6, 7, 10 };

void test_builtin_casting() {
    auto doc = json::parse_file(TEST_PATH_TO("casting.json"));
    auto root = doc.top_view();

    // INT
    auto ints = root["int"];
    for(int i = 0; i<ints.array_size(); ++i) {
        int a;
        ASSERT_DOESNT_THROW(a = ints.at_as<int>(i));
        ASSERT_EQ(INTS_CORRECT[i], a);
    };

    auto not_ints = root["!int"];
    for(int i = 0; i<not_ints.array_size(); ++i) {
        ASSERT_THROWS(not_ints.at_as<int>(i));
    }

    // UINT
    auto uints = root["uint"];
    for(int i = 0; i<uints.array_size(); ++i) {
        int a;
        ASSERT_DOESNT_THROW(a = uints.at_as<unsigned int>(i));
        ASSERT_EQ(UINTS_CORRECT[i], a);
    };

    auto not_uints = root["!uint"];
    for(int i = 0; i<not_uints.array_size(); ++i) {
        ASSERT_THROWS(not_uints.at_as<unsigned int>(i));
    }

    // DOUBLE AND FLOATS
    auto float_and_double = root["float_and_double"];
    for(int i = 0; i<uints.array_size(); ++i) {
        double a;
        float b;
        ASSERT_DOESNT_THROW(a = float_and_double.at_as<double>(i));
        ASSERT_DOESNT_THROW(b = float_and_double.at_as<float>(i));
        ASSERT_EQ(DOUBLE_CORRECT[i], a);
        ASSERT_EQ((float)DOUBLE_CORRECT[i], b);
    };

    auto not_float_and_double = root["!float_and_double"];
    for(int i = 0; i<not_uints.array_size(); ++i) {
        ASSERT_THROWS(not_float_and_double.at_as<float>(i));
        ASSERT_THROWS(not_float_and_double.at_as<double>(i));
    }

    // BOOLS
    auto bools = root["bool"];
    for (int i = 0; i<bools.array_size(); ++i) {
        if (i<=3) {
            ASSERT_FALSE(bools.at_as<bool>(i));
        } else {
            ASSERT_TRUE(bools.at_as<bool>(i));
        };
    }

    // STRING (every literal can be cast as a string)
    auto strings = root["string"];
    for (int i = 0; i<strings.array_size(); ++i) {
        std::string_view sv;
        std::string s;
        ASSERT_DOESNT_THROW(sv = strings.at_as<std::string_view>(i));
        ASSERT_DOESNT_THROW(s = strings.at_as<std::string>(i));
        ASSERT_EQ(sv, STRINGS_CORRECT[i]);
        ASSERT_EQ(s, STRINGS_CORRECT[i]);
    }

    auto not_strings = root["!string"];
    for(int i = 0; i<not_strings.array_size(); ++i) {
        ASSERT_THROWS(not_strings.at_as<std::string>(i));
        ASSERT_THROWS(not_strings.at_as<std::string_view>(i));
    };

    // VECTOR
    std::vector<int> vec = root["int_vector"].as_vector<int>();
    for(int i = 0; i<VECTOR_CORRECT.size(); ++i) {
        ASSERT_EQ(vec[i], VECTOR_CORRECT[i]);
    };
};

void test_optional_casting() {
    auto doc = json::parse_file(TEST_PATH_TO("casting.json"));
    auto root = doc.top_view();

    std::vector<bool> null_key = root["null_key"].as_vector<bool>();
    auto nulls = root["null"];
    for(int i = 0; i<nulls.array_size(); ++i) {
        std::optional<std::string> opt;
        ASSERT_DOESNT_THROW(opt = nulls[i].as_optional<std::string>());
        if(null_key[i]) {
            ASSERT_TRUE(opt);
        } else {
            ASSERT_FALSE(opt);
        }
    }
};

struct MyType {
    std::string str;
    float f;
    int i;

    inline static MyType init() {
        return { "Hello, World!", 67.67e2, 3 };
    }
};

template<>
MyType json::View::as<MyType>() const {
    if(!is_object()) {
        throw std::runtime_error("Cannot cast to MyType");
    };

    MyType t;
    t.str = get_as<std::string>("string");
    t.f = get_as<float>("float");
    t.i = get_as<int>("int");
    return t;
}

void test_custom_type_casting() {
    auto doc = json::parse_file(TEST_PATH_TO("casting.json"));
    json::View root = doc.top_view();

    MyType correct = MyType::init();
    MyType t;
    ASSERT_DOESNT_THROW(t = root["MyType"].as<MyType>());
    // Fails if code above throws
    ASSERT_EQ(t.str, correct.str);
    ASSERT_EQ(t.f, correct.f);
    ASSERT_EQ(t.i, correct.i);

    MyType not_t;
    ASSERT_THROWS(not_t = root["NotMyType"].as<MyType>());
}

TEST_CASE(test_casting) {
    TEST(test_builtin_casting);
    TEST(test_optional_casting);
    TEST(test_custom_type_casting);
}