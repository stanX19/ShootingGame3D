#include "catch2/catch_amalgamated.hpp"
#include "components/unit.hpp"

TEST_CASE("Unit: Name component stores ship name", "[unit][name]") {
	Name shipName{"Fighter"};
	CHECK(shipName.val == "Fighter");
	shipName.val = "Mothership";
	CHECK(shipName.val == "Mothership");
}
