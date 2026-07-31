#include "catch2/catch_amalgamated.hpp"
#include "utils/basic_utils.hpp"

TEST_CASE("merge_vectors preserves input order", "[unit][utils]")
{
    const std::vector<int> first{1, 2};
    const std::vector<int> second{3, 4};
    const std::vector<int> expected{1, 2, 3, 4};

    CHECK(merge_vectors(first, second) == expected);
}

TEST_CASE("merge_vectors supports multiple and empty inputs", "[unit][utils]")
{
    const std::vector<int> first{};
    const std::vector<int> second{5};
    const std::vector<int> third{6, 7};
    const std::vector<int> expected{5, 6, 7};

    CHECK(merge_vectors(first, second, third) == expected);
}
