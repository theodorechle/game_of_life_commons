#include "network_input_handler.hpp"

NetworkInputHandler::NetworkInputHandler(int socket, size_t bufferSize) : _socket{socket}, _bufferSize{bufferSize} {
    if (_bufferSize <= 0) throw std::invalid_argument("buffer size should be greater than 0");
}

int NetworkInputHandler::read(size_t length, std::string &out) {
    out = _buffer.substr(_index, length);
    _index += out.size();
    if (length <= _buffer.size() - _index) _index += length;
    else _buffer.erase(0, length + _index);
    length -= out.size();
    if (length == 0) return 0;
    _buffer.clear();
    _index = 0;

    char buffer[_bufferSize] = {0};
    ssize_t bytesRead = 0;
    size_t bytesAdded = 0;

    while (length > 0) {
        bytesRead = recv(_socket, buffer, _bufferSize, 0);

        if (bytesRead == -1) {
#ifdef DEBUG
            std::cerr << "recv returned -1. Is it because of non-blocking? " << (errno == EAGAIN || errno == EWOULDBLOCK ? "yes" : "no") << "\n";
            std::cerr << "string readed before last recv: \"" << out << "\"\n";
#endif
            return 1;
        }
        if (bytesRead == 0) {
#ifdef DEBUG
            std::cerr << "socket closed\n";
            std::cerr << "string readed before last recv: \"" << out << "\"\n";
#endif
            return 2;
        }

        if (static_cast<size_t>(bytesRead) < _bufferSize && static_cast<size_t>(bytesRead) < length) {
#ifdef DEBUG
            std::cerr << "can't read as much bytes as needed\n";
            std::cerr << "string readed before last recv: \"" << out << "\"\n";
            std::cerr << "string readed with last recv: \"";
            std::cerr.write(buffer, bytesRead);
            std::cerr << "\"\n";
#endif
            return 1; // error, can't read as much bytes as needed
        }

        bytesAdded = std::min(static_cast<size_t>(bytesRead), length);

        out.append(buffer, bytesAdded);
        length -= bytesAdded;
    }
    _buffer = std::string(buffer + bytesAdded, bytesRead - bytesAdded);
    return 0;
}

int NetworkInputHandler::readUntilDelimiter(char delimiter, std::string &out, bool includeDelimiter, bool flushDelimiter) {
    bool flush = (!includeDelimiter && flushDelimiter);
    size_t index = _buffer.find(delimiter, _index);
    if (index == std::string::npos) {
        out = _buffer.substr(_index);
        _buffer.clear();
        _index = 0;
#ifdef DEBUG
        std::cerr << "delimiter not found\n";
#endif
    }
    else {
        out = _buffer.substr(_index, index + includeDelimiter);
        _index += index - _index + flush;
#ifdef DEBUG
        std::cerr << "delimiter found\n";
#endif
        return 0;
    }

    char buffer[_bufferSize] = {0};
    ssize_t bytesRead = 0;
    size_t bytesAdded = 0;

    while (true) {
        bytesRead = recv(_socket, buffer, _bufferSize, 0);

        if (bytesRead == -1) {
#ifdef DEBUG
            std::cerr << "recv returned an error. Is it because of non-blocking? " << (errno != EAGAIN && errno != EWOULDBLOCK) << "\n";
            std::cerr << "string readed before last recv: \"" << out << "\"\n";
#endif
            return 1;
        }
        if (bytesRead == 0) {
#ifdef DEBUG
            std::cerr << "socket closed\n";
            std::cerr << "string readed before last recv: \"" << out << "\"\n";
#endif
            return 2;
        }

        char *bufferEnd = buffer + bytesRead;
#ifdef DEBUG
        std::cerr
            << "buffer: "
            << static_cast<void *>(buffer)
            << ", bytesRead: "
            << bytesRead
            << ", bufferEnd: "
            << static_cast<void *>(bufferEnd)
            << ", bufferSize: "
            << (bufferEnd - buffer)
            << "\n";
#endif

        char *pos = std::find(buffer, bufferEnd, delimiter);
#ifdef DEBUG
        std::cerr << "pos: " << static_cast<void *>(pos) << "\n";
#endif

        if (pos != bufferEnd) {
            bytesAdded = pos - buffer + includeDelimiter;
#ifdef DEBUG
            std::cerr << "found, appending to out(size=" << out.size() << ") " << bytesAdded << " bytes:\n";
            std::cerr << std::string(buffer, bytesAdded) << "\n";
#endif
            out.append(buffer, bytesAdded);
            break;
        }
#ifdef DEBUG
        std::cerr << "not found, appending to out(size=" << out.size() << ") " << " bytes:\n";
        std::cerr << std::string(buffer, bytesRead) << "\n";
#endif
        out.append(buffer, bytesRead);
        if (static_cast<size_t>(bytesRead) < _bufferSize) {
#ifdef DEBUG
            std::cerr << "can't read any more bytes\n";
            std::cerr << "string readed before last recv: \"" << out << "\"\n";
            std::cerr << "string readed with last recv: \"";
            std::cerr.write(buffer, bytesRead);
            std::cerr << "\"\n";
#endif
            return 1; // error, can't read any more bytes.
        }
    }
    _buffer = std::string(buffer + bytesAdded + flush, bytesRead - bytesAdded - flush);
    return 0;
}
