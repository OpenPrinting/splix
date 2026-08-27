#include <gtest/gtest.h>

#include "cache.h"
#include "page.h"

#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

namespace {
class CacheShutdown final {
public:
    ~CacheShutdown() { EXPECT_TRUE(uninitializeCache()); }
};

TEST(CacheConcurrency, ProducesAndConsumesEveryPageInOrder) {
    constexpr uint32_t kPageCount = 64;
    ASSERT_TRUE(initializeCache());
    CacheShutdown shutdown;
    setCachePolicy(EveryPagesIncreasing);

    std::jthread producer([] {
        for (uint32_t pageNumber = 1; pageNumber <= kPageCount; ++pageNumber) {
            auto page = std::make_unique<Page>();
            page->setPageNr(pageNumber);
            registerPage(std::move(page));
        }
        setNumberOfPages(kPageCount);
    });

    std::vector<uint32_t> consumed;
    consumed.reserve(kPageCount);
    for (uint32_t expected = 1; expected <= kPageCount; ++expected) {
        auto result = getNextPage();
        ASSERT_TRUE(result.has_value());
        ASSERT_NE(result.value(), nullptr);
        consumed.push_back(result.value()->pageNr());
    }
    producer.join();

    auto end = getNextPage();
    ASSERT_TRUE(end.has_value());
    EXPECT_EQ(end.value(), nullptr);
    ASSERT_EQ(consumed.size(), kPageCount);
    for (uint32_t index = 0; index < kPageCount; ++index) {
        EXPECT_EQ(consumed[index], index + 1);
    }
}

TEST(CacheConcurrency, PreservesDuplexPagePolicyOrder) {
    constexpr uint32_t kPageCount = 8;
    ASSERT_TRUE(initializeCache());
    CacheShutdown shutdown;

    for (uint32_t pageNumber = 1; pageNumber <= kPageCount; ++pageNumber) {
        auto page = std::make_unique<Page>();
        page->setPageNr(pageNumber);
        registerPage(std::move(page));
    }
    setNumberOfPages(kPageCount);
    setCachePolicy(EvenDecreasing);

    const std::vector<uint32_t> expectedOrder{8, 6, 4, 2, 1, 3, 5, 7};
    for (const uint32_t expected : expectedOrder) {
        auto result = getNextPage();
        ASSERT_TRUE(result.has_value());
        ASSERT_NE(result.value(), nullptr);
        EXPECT_EQ(result.value()->pageNr(), expected);
    }
    auto end = getNextPage();
    ASSERT_TRUE(end.has_value());
    EXPECT_EQ(end.value(), nullptr);
}
}  // namespace
