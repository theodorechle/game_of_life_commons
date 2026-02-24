#ifndef NETWORK_TESTS_HPP
#define NETWORK_TESTS_HPP

#include "../../cpp_tests/src/tests.hpp"
#include "../../src/network_input_handler/network_input_handler.hpp"
#include <fcntl.h>
#include <thread>

namespace networkTests {
    /*
        returns true in case of error
    */
    bool createSocket(int socket[2], bool nonBlocking = true);

    void testNetwork(test::Tests *tests);
} // namespace networkTests

#endif // NETWORK_TESTS_HPP
