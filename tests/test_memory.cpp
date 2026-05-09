#include "testing.h"

#include "json_parser/json.hpp"

#include <unordered_set>
#include <cstdlib>
#include <cstdint>

struct {
    std::unordered_set<void*> allocated;
    bool do_tracking;
    bool do_malloc_count;
    uint32_t malloc_count;

    inline void start_count() { do_malloc_count = true; };
    inline void stop_count() { do_malloc_count = false; };
    inline void reset_count() { malloc_count = 0; };
    inline void clear() { allocated.clear(); }
    inline bool is_empty() { return allocated.empty(); };
    inline size_t size() { return allocated.size(); };

} allocations = { std::unordered_set<void*>(), false, false, 0 };

void* operator new[](size_t size) {
    void* p = std::malloc(size);
    if (!p) {
        throw std::bad_alloc();
    }

    if(allocations.do_tracking && allocations.allocated.find(p) == allocations.allocated.end()) {
        allocations.allocated.insert(p);
        if(allocations.do_malloc_count) {
            ++allocations.malloc_count;
        }
    };

    return p;
};

void operator delete[](void* p, size_t size) noexcept {
    if(allocations.do_tracking) {
        allocations.allocated.erase(p);
    }

    std::free(p);
};

void operator delete[](void* p) noexcept {
    if(allocations.do_tracking) {
        allocations.allocated.erase(p);
    }

    std::free(p);
};

#define ASSERT_ALLOC(allocations, body) \
    allocations.clear(); \
    { \
        body; \
    }; \
    ASSERT_TRUE(allocations.is_empty())


void test_valid_parsing() {
    allocations.do_tracking = true;

    ASSERT_ALLOC(allocations,
        auto doc = json::parse_file(TEST_PATH_TO("deep_nested_object.json"));
        ASSERT_FALSE(allocations.is_empty());
    );

    const std::string source = test::open_file(TEST_PATH_TO("deep_nested_object.json"));
    std::shared_ptr<const std::string> source_ptr = std::make_shared<const std::string>(source);

    ASSERT_ALLOC(allocations,
        auto doc = json::parse_string(source);
        ASSERT_FALSE(allocations.is_empty());
    );

    ASSERT_ALLOC(allocations,
        auto doc = json::parse_string(source_ptr);
        ASSERT_FALSE(allocations.is_empty());
    );
};

void test_invalid_parsing() {
    allocations.do_tracking = true;

    const std::string source = test::open_file(TEST_PATH_TO("long_invalid.json"));
    std::shared_ptr<const std::string> source_ptr = std::make_shared<const std::string>(source);
    
    ASSERT_ALLOC(allocations,
        ASSERT_THROWS(auto doc = json::parse_string(source));
    );

    ASSERT_ALLOC(allocations,
        ASSERT_THROWS(auto doc = json::parse_string(source_ptr));
    );

    ASSERT_ALLOC(allocations,
        ASSERT_THROWS(auto doc = json::parse_file(TEST_PATH_TO("long_invalid.json")));
    );

    allocations.clear();
    {
        json::Parser parser(source);
        allocations.clear();
        allocations.start_count();
        parser.parse();
        ASSERT_TRUE(allocations.is_empty()); // Everything should be freed if error
        allocations.stop_count();
        std::cout << "test_invalid_parsing: Parser allocated " << allocations.malloc_count << std::endl;
        ASSERT_FALSE(parser.get_status()); // Not ok to use
        ASSERT_NE(parser.get_error(), ""); // Gives error message
    };
    ASSERT_TRUE(allocations.is_empty());


};

TEST_CASE(test_memory) {
    TEST(test_valid_parsing);
    TEST(test_invalid_parsing);
};
