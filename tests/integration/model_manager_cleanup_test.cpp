#include "catch2/catch_amalgamated.hpp"
#include "classes/model_manager.hpp"
#include "classes/renderer.hpp"
#include <type_traits>

TEST_CASE("ModelManager and Renderer are non-copyable to prevent shallow-copy double-frees", "[integration][resource_cleanup]")
{
    STATIC_CHECK(!std::is_copy_constructible_v<ModelManager>);
    STATIC_CHECK(!std::is_copy_assignable_v<ModelManager>);
    STATIC_CHECK(!std::is_copy_constructible_v<Renderer>);
    STATIC_CHECK(!std::is_copy_assignable_v<Renderer>);
}

TEST_CASE("ModelManager handles repeated unloadAll cleanly and idempotently", "[integration][resource_cleanup]")
{
    ModelManager manager;
    CHECK_NOTHROW(manager.unloadAll());
    CHECK_NOTHROW(manager.unloadAll());
}
