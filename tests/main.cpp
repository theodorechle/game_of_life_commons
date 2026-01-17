#include "../cpp_tests/src/tests.hpp"
#include "network_tests/network_tests.hpp"

int main() {
    test::Tests tests = test::Tests();
    networkTests::testNetwork(&tests);
    tests.runTests();
    tests.displaySummary();
    return !tests.allTestsPassed();
}
