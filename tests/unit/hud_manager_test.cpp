#include "catch2/catch_amalgamated.hpp"
#include "components/identity.hpp"

TEST_CASE("Unit: Name component stores ship name", "[unit][name]") {
	Name shipName{"Fighter"};
	CHECK(shipName.value == "Fighter");
	shipName.value = "Mothership";
	CHECK(shipName.value == "Mothership");
}
