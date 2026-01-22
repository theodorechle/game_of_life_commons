#ifndef NETWORK_INPUT_HANDLER_HPP
#define NETWORK_INPUT_HANDLER_HPP

#include <algorithm>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#ifdef DEBUG
#include <iostream>
#endif

class NetworkInputHandler {
    int _socket;
    size_t _bufferSize;
    std::string _buffer = "";
    size_t _index = 0;

public:
    NetworkInputHandler(int socket, size_t bufferSize = 1024);

    int read(size_t length, std::string &out);

    /**
     * flushDelimiter is only checked if includeDelimiter is false
     */
    int readUntilDelimiter(char delimiter, std::string &out, bool includeDelimiter = false, bool flushDelimiter = false);
};

#endif // NETWORK_INPUT_HANDLER_HPP
