#pragma once

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

namespace test {
    inline const std::string open_file(const std::string& path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    };

    static unsigned int tests_run = 0;
    static unsigned int tests_failed = 0;

    inline void fail(std::string& message) {
        ++tests_run;
        ++tests_failed;
        std::cout << "[FAIL] " << __FILE__ << ":" << __LINE__
                      << "-> " << message << std::endl;
    };
}

#define TEST_PATH_TO(path) "../tests/json/" path

#define TEST_CASE(name) \
    void name(); \
    int main() { \
        name(); \
        std::cout << "Tests run: " << test::tests_run \
                  << ", Failed: " << test::tests_failed << std::endl; \
        return test::tests_failed == 0 ? 0: 1; \
    } \
    void name()

#define TEST(name) \
    std::cout << "Testing " #name << std::endl; \
    name();

#define ASSERT_TRUE(expr) \
    do { \
        ++test::tests_run; \
        if(!(expr)) { \
            ++test::tests_failed; \
            std::cout << "[FAIL] " << __FILE__ << ":" << __LINE__ \
                      << "-> " #expr << std::endl; \
        } \
    } while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))

#define ASSERT_THROWS(expr) \
    do { \
        ++test::tests_run; \
        try { \
            expr; \
            ++test::tests_failed; \
            std::cout << "[FAIL] " << __FILE__ << ":" << __LINE__ \
                      << "-> " #expr " did not throw." << std::endl; \
        } catch (const std::runtime_error& e) {} \
    } while(0)

#define ASSERT_DOESNT_THROW(expr) \
    do { \
        ++test::tests_run; \
        try { \
            expr; \
        } catch (const std::runtime_error& e) { \
            ++test::tests_failed; \
            std::cout << "[FAIL] " << __FILE__ << ":" << __LINE__ \
                      << "-> " #expr " did throw." << std::endl; \
        } \
    } while(0)

