#ifndef NETWORK_INPUT_HANDLER_HPP
#define NETWORK_INPUT_HANDLER_HPP

#include <algorithm>
#include <string>
#include <sys/socket.h>
#include <stdexcept>
#ifdef DEBUG
#include <iostream>
#endif

constexpr size_t BUFFER_LENGTH = 1024;

class NetworkInputHandler {
    int _socket;
    size_t _bufferSize;
    std::string _buffer = "";
    size_t _index = 0;

public:
    NetworkInputHandler(int socket, size_t bufferSize = BUFFER_LENGTH);

    int read(size_t length, std::string &out);

    /**
     * returns a string who contains the network input before and including the delimiter.
     */
    int readUntilDelimiter(char delimiter, std::string &out);
};

#endif // NETWORK_INPUT_HANDLER_HPP
