#include "network_tests.hpp"

namespace networkTests {
    bool createSocket(int socket[2], bool nonBlocking) {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket) != 0) {
            std::cerr << "can't create socket pair\n";
            return true;
        }

        if (nonBlocking) {
            int flags = fcntl(socket[0], F_GETFL, 0);
            if (flags == -1) {
                perror("fcntl get failed");
                return true;
            }

            // set I/O to non-blocking
            if (fcntl(socket[0], F_SETFL, flags | O_NONBLOCK) == -1) {
                perror("fcntl set failed");
                return true;
            }
        }
        return false;
    }

    test::Result testBufferSizeOfZero() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;
        bool catched = false;

        try {
            NetworkInputHandler(fakeSocket[0], 0);
        }
        catch (const std::invalid_argument &e) {
            std::cerr << e.what() << '\n';
            catched = true;
        }

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        return catched ? test::Result::SUCCESS : test::Result::FAILURE;
    }

    test::Result testReadSmallerThanBufferSize() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 10);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.read(5, output);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output == message) return test::Result::SUCCESS;
        std::cerr << "Expected '" << message << "', received: '" << output << "'\n";
        return test::Result::FAILURE;
    }

    test::Result testReadBiggerThanBufferSize() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 2);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.read(5, output);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output == message) return test::Result::SUCCESS;
        std::cerr << "Expected '" << message << "', received: '" << output << "'\n";
        return test::Result::FAILURE;
    }

    test::Result testReadCloseSocket() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 2);

        close(fakeSocket[1]);

        std::string output;

        int errorCode = inputHandler.read(5, output);

        close(fakeSocket[0]);

        if (errorCode != 2) {
            std::cerr << "read returned code " << errorCode << " instead of " << 2 << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadAskingForMoreThanSentWithMessageHavingSameSizeAsBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 5);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.read(10, output);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadAskingForMoreThanSentWithMessageHavingSizeSmallerThanBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 7);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.read(10, output);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadAskingForMoreThanSentWithMessageHavingSizeBiggerThanBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 2);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.read(10, output);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadTwoMessagesWhoFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 12);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.read(5, output);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != hello) {
            std::cerr << "Expected '" << hello << "', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.read(5, output);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != world) {
            std::cerr << "Expected '" << world << "', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadTwoMessagesWhoDoesntFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 3);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.read(5, output);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != hello) {
            std::cerr << "Expected '" << hello << "', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.read(5, output);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != world) {
            std::cerr << "Expected '" << world << "', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadTwoMessagesWhoEachFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 5);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.read(5, output);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != hello) {
            std::cerr << "Expected '" << hello << "', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.read(5, output);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != world) {
            std::cerr << "Expected '" << world << "', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadRetryIfNoByteReceived() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0]);

        std::string output;

        int errorCode;

        std::thread readThread([&errorCode, &inputHandler, &output] { errorCode = inputHandler.read(5, output, true); });

        const char *message = "Hello";

        std::thread writeThread([fakeSocket, message] {
            // wait a bit before sending the message.
            // it's not perfect, since the readThread could have not started, but I don't know of a better way to do it
            std::this_thread::sleep_until(std::chrono::steady_clock::now() + std::chrono::seconds(2));
            write(fakeSocket[1], message, strlen(message));
        });

        readThread.join();
        writeThread.join();

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output == message) return test::Result::SUCCESS;
        std::cerr << "Expected '" << message << "', received: '" << output << "'\n";
        return test::Result::FAILURE;
    }

    test::Result testReadRetryIfNoByteReceivedAfterByteReceived() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0]);

        std::string output;

        int errorCode;

        std::thread readThread([&errorCode, &inputHandler, &output] { errorCode = inputHandler.read(10, output, true); });

        const char *message = "Hello";

        std::thread writeThread([fakeSocket, message] { write(fakeSocket[1], message, strlen(message)); });

        readThread.join();
        writeThread.join();

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterCloseSocket() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 2);

        close(fakeSocket[1]);

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('o', output);

        close(fakeSocket[0]);

        if (errorCode != 2) {
            std::cerr << "read returned code " << errorCode << " instead of " << 2 << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterSmallerThanBufferSize() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 10);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('o', output);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output == "Hell") return test::Result::SUCCESS;
        std::cerr << "Expected 'Hell', received: '" << output << "'\n";
        return test::Result::FAILURE;
    }

    test::Result testReadUntilDelimiterBiggerThanBufferSize() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 2);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('o', output);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output == "Hell") return test::Result::SUCCESS;
        std::cerr << "Expected 'Hell', received: '" << output << "'\n";
        return test::Result::FAILURE;
    }

    test::Result testReadUntilDelimiterAskingForDelimiterWhoIsNotInMessage() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 5);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('a', output);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeSmallerThanBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 7);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('a', output);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeBiggerThanBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 2);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('a', output);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterTwoMessagesWhoFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 12);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.readUntilDelimiter('o', output);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != "Hell") {
            std::cerr << "Expected 'Hell', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.readUntilDelimiter('d', output);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != "oworl") {
            std::cerr << "Expected 'oworl', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterTwoMessagesWhoDoesntFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 3);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.readUntilDelimiter('o', output);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != "Hell") {
            std::cerr << "Expected 'Hell', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.readUntilDelimiter('d', output);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != "oworl") {
            std::cerr << "Expected 'oworl', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterTwoMessagesWhoEachFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 5);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.readUntilDelimiter('o', output);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != "Hell") {
            std::cerr << "Expected 'Hell', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.readUntilDelimiter('d', output);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != "oworl") {
            std::cerr << "Expected 'oworl', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterFlushingDelimiterSmallerThanBufferSize() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 10);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('o', output, false, true);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output == "Hell") return test::Result::SUCCESS;
        std::cerr << "Expected 'Hell', received: '" << output << "'\n";
        return test::Result::FAILURE;
    }

    test::Result testReadUntilDelimiterFlushingDelimiterBiggerThanBufferSize() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 2);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('o', output, false, true);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output == "Hell") return test::Result::SUCCESS;
        std::cerr << "Expected 'Hell', received: '" << output << "'\n";
        return test::Result::FAILURE;
    }

    test::Result testReadUntilDelimiterFlushingDelimiterAskingForDelimiterWhoIsNotInMessage() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 5);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('a', output, false, true);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterFlushingDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeSmallerThanBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 7);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('a', output, false, true);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterFlushingDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeBiggerThanBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 2);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('a', output, false, true);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterFlushingDelimiterTwoMessagesWhoFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 12);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.readUntilDelimiter('o', output, false, true);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != "Hell") {
            std::cerr << "Expected 'Hell', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.readUntilDelimiter('d', output);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != "worl") {
            std::cerr << "Expected 'worl', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterFlushingDelimiterTwoMessagesWhoDoesntFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 3);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.readUntilDelimiter('o', output, false, true);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != "Hell") {
            std::cerr << "Expected 'Hell', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.readUntilDelimiter('d', output);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != "worl") {
            std::cerr << "Expected 'worl', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterFlushingDelimiterTwoMessagesWhoEachFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 5);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.readUntilDelimiter('o', output, false, true);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != "Hell") {
            std::cerr << "Expected 'Hell', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.readUntilDelimiter('d', output);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != "worl") {
            std::cerr << "Expected 'worl', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterIncludingDelimiterSmallerThanBufferSize() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 10);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('o', output, true);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output == message) return test::Result::SUCCESS;
        std::cerr << "Expected '" << message << "', received: '" << output << "'\n";
        return test::Result::FAILURE;
    }

    test::Result testReadUntilDelimiterIncludingDelimiterBiggerThanBufferSize() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 2);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('o', output, true);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output == message) return test::Result::SUCCESS;
        std::cerr << "Expected '" << message << "', received: '" << output << "'\n";
        return test::Result::FAILURE;
    }

    test::Result testReadUntilDelimiterIncludingDelimiterAskingForDelimiterWhoIsNotInMessage() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 5);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('a', output, true);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterIncludingDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeSmallerThanBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 7);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('a', output, true);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterIncludingDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeBiggerThanBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 2);

        const char *message = "Hello";
        write(fakeSocket[1], message, strlen(message));

        std::string output;

        int errorCode = inputHandler.readUntilDelimiter('a', output, true);

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterIncludingDelimiterTwoMessagesWhoFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 12);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.readUntilDelimiter('o', output, true);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != hello) {
            std::cerr << "Expected '" << hello << "', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.readUntilDelimiter('d', output, true);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != world) {
            std::cerr << "Expected '" << world << "', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterIncludingDelimiterTwoMessagesWhoDoesntFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 3);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.readUntilDelimiter('o', output, true);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != hello) {
            std::cerr << "Expected '" << hello << "', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.readUntilDelimiter('d', output, true);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != world) {
            std::cerr << "Expected '" << world << "', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }

    test::Result testReadUntilDelimiterIncludingDelimiterTwoMessagesWhoEachFitsInTheBuffer() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0], 5);

        const char *hello = "Hello";
        const char *world = "world";
        write(fakeSocket[1], hello, strlen(hello));
        write(fakeSocket[1], world, strlen(world));

        std::string output;
        int errorCode;

        errorCode = inputHandler.readUntilDelimiter('o', output, true);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        if (output != hello) {
            std::cerr << "Expected '" << hello << "', received: '" << output << "'\n";
            close(fakeSocket[0]);
            close(fakeSocket[1]);
            return test::Result::FAILURE;
        }

        errorCode = inputHandler.readUntilDelimiter('d', output, true);
        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output != world) {
            std::cerr << "Expected '" << world << "', received: '" << output << "'\n";
            return test::Result::FAILURE;
        }

        return test::Result::SUCCESS;
    }


    test::Result testReadUntilDelimiterRetryIfNoByteReceived() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0]);

        std::string output;

        int errorCode;

        std::thread readThread([&errorCode, &inputHandler, &output] { errorCode = inputHandler.readUntilDelimiter('\n', output, true, true, true); });

        const char *message = "Hello\n";

        std::thread writeThread([fakeSocket, message] {
            // wait a bit before sending the message.
            // it's not perfect, since the readThread could have not started, but I don't know of a better way to do it
            std::this_thread::sleep_until(std::chrono::steady_clock::now() + std::chrono::seconds(2));
            write(fakeSocket[1], message, strlen(message));
        });

        readThread.join();
        writeThread.join();

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode) {
            std::cerr << "read returned code " << errorCode << "\n";
            std::cerr << "errno: " << errno << "\n";
            return test::Result::FAILURE;
        }

        if (output == message) return test::Result::SUCCESS;
        std::cerr << "Expected '" << message << "', received: '" << output << "'\n";
        return test::Result::FAILURE;
    }

    test::Result testReadUntilDelimiterRetryIfNoByteReceivedAfterByteReceived() {
        int fakeSocket[2];
        if (createSocket(fakeSocket)) return test::Result::ERROR;

        NetworkInputHandler inputHandler = NetworkInputHandler(fakeSocket[0]);

        std::string output;

        int errorCode;

        std::thread readThread([&errorCode, &inputHandler, &output] { errorCode = inputHandler.readUntilDelimiter('\n', output, true); });

        const char *message = "Hello";

        std::thread writeThread([fakeSocket, message] { write(fakeSocket[1], message, strlen(message)); });

        readThread.join();
        writeThread.join();

        close(fakeSocket[0]);
        close(fakeSocket[1]);

        if (errorCode != 1) {
            std::cerr << "read didn't failed and returned message \"" << output << "\"\n";
            return test::Result::FAILURE;
        }
        return test::Result::SUCCESS;
    }


    void testNetwork(test::Tests *tests) {
        tests->beginTestBlock("test network input handler");
        tests->addTest(testBufferSizeOfZero, "buffer size of zero");

        tests->beginTestBlock("test read");
        tests->addTest(testReadSmallerThanBufferSize, "read smaller than buffer size");
        tests->addTest(testReadBiggerThanBufferSize, "read bigger than buffer size");
        tests->addTest(testReadCloseSocket, "read close socket");
        tests->addTest(testReadAskingForMoreThanSentWithMessageHavingSameSizeAsBuffer,
                       "read bigger than buffer size with message having same size as buffer");
        tests->addTest(testReadAskingForMoreThanSentWithMessageHavingSizeSmallerThanBuffer,
                       "read asking for more than sent with message having size smaller than buffer");
        tests->addTest(testReadAskingForMoreThanSentWithMessageHavingSizeBiggerThanBuffer,
                       "read asking for more than sent with message having size bigger than buffer");
        tests->addTest(testReadTwoMessagesWhoFitsInTheBuffer, "read two messages who fits in the buffer");
        tests->addTest(testReadTwoMessagesWhoDoesntFitsInTheBuffer, "read two messages who doesn't fits in the buffer");
        tests->addTest(testReadTwoMessagesWhoEachFitsInTheBuffer, "read two messages who each fits in the buffer");

        tests->beginTestBlock("retry if no byte received");
        tests->addTest(testReadRetryIfNoByteReceived, "read retry if no byte received");
        tests->addTest(testReadRetryIfNoByteReceivedAfterByteReceived, "read retry if no byte received after byte received");
        tests->endTestBlock();
        tests->endTestBlock();

        tests->beginTestBlock("test read until delimiter");
        tests->addTest(testReadUntilDelimiterCloseSocket, "read until delimiter close socket");

        tests->beginTestBlock("not including delimiter");
        tests->addTest(testReadUntilDelimiterSmallerThanBufferSize, "read until delimiter smaller than buffer size");
        tests->addTest(testReadUntilDelimiterBiggerThanBufferSize, "read until delimiter bigger than buffer size");
        tests->addTest(testReadUntilDelimiterAskingForDelimiterWhoIsNotInMessage, "read until delimiter asking for delimiter who is not in message");
        tests->addTest(testReadUntilDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeSmallerThanBuffer,
                       "read until delimiter asking for delimiter who is not in message with message having size smaller than buffer");
        tests->addTest(testReadUntilDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeBiggerThanBuffer,
                       "read until delimiter asking for delimiter who is not in message with message having size bigger than buffer");
        tests->addTest(testReadUntilDelimiterTwoMessagesWhoFitsInTheBuffer, "read until delimiter two messages who fits in the buffer");
        tests->addTest(testReadUntilDelimiterTwoMessagesWhoDoesntFitsInTheBuffer, "read until delimiter two messages who doesn't fits in the buffer");
        tests->addTest(testReadUntilDelimiterTwoMessagesWhoEachFitsInTheBuffer, "read until delimiter two messages who each fits in the buffer");
        tests->endTestBlock();

        tests->beginTestBlock("flushing delimiter");
        tests->addTest(testReadUntilDelimiterFlushingDelimiterSmallerThanBufferSize,
                       "read until delimiter flushing delimiter smaller than buffer size");
        tests->addTest(testReadUntilDelimiterFlushingDelimiterBiggerThanBufferSize,
                       "read until delimiter flushing delimiter bigger than buffer size");
        tests->addTest(testReadUntilDelimiterFlushingDelimiterAskingForDelimiterWhoIsNotInMessage,
                       "read until delimiter flushing delimiter asking for delimiter who is not in message");
        tests->addTest(
            testReadUntilDelimiterFlushingDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeSmallerThanBuffer,
            "read until delimiter flushing delimiter asking for delimiter who is not in message with message having size smaller than buffer");
        tests->addTest(
            testReadUntilDelimiterFlushingDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeBiggerThanBuffer,
            "read until delimiter flushing delimiter asking for delimiter who is not in message with message having size bigger than buffer");
        tests->addTest(testReadUntilDelimiterFlushingDelimiterTwoMessagesWhoFitsInTheBuffer,
                       "read until delimiter flushing delimiter two messages who fits in the buffer");
        tests->addTest(testReadUntilDelimiterFlushingDelimiterTwoMessagesWhoDoesntFitsInTheBuffer,
                       "read until delimiter flushing delimiter two messages who doesn't fits in the buffer");
        tests->addTest(testReadUntilDelimiterFlushingDelimiterTwoMessagesWhoEachFitsInTheBuffer,
                       "read until delimiter flushing delimiter two messages who each fits in the buffer");
        tests->endTestBlock();

        tests->beginTestBlock("including delimiter");
        tests->addTest(testReadUntilDelimiterIncludingDelimiterSmallerThanBufferSize,
                       "read until delimiter including delimiter smaller than buffer size");
        tests->addTest(testReadUntilDelimiterIncludingDelimiterBiggerThanBufferSize,
                       "read until delimiter including delimiter bigger than buffer size");
        tests->addTest(testReadUntilDelimiterIncludingDelimiterAskingForDelimiterWhoIsNotInMessage,
                       "read until delimiter including delimiter asking for delimiter who is not in message");
        tests->addTest(
            testReadUntilDelimiterIncludingDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeSmallerThanBuffer,
            "read until delimiter including delimiter asking for delimiter who is not in message with message having size smaller than buffer");
        tests->addTest(
            testReadUntilDelimiterIncludingDelimiterAskingForDelimiterWhoIsNotInMessageWithMessageHavingSizeBiggerThanBuffer,
            "read until delimiter including delimiter asking for delimiter who is not in message with message having size bigger than buffer");
        tests->addTest(testReadUntilDelimiterIncludingDelimiterTwoMessagesWhoFitsInTheBuffer,
                       "read until delimiter including delimiter delimiter two messages who fits in the buffer");
        tests->addTest(testReadUntilDelimiterIncludingDelimiterTwoMessagesWhoDoesntFitsInTheBuffer,
                       "read until delimiter including delimiter delimiter two messages who doesn't fits in the buffer");
        tests->addTest(testReadUntilDelimiterIncludingDelimiterTwoMessagesWhoEachFitsInTheBuffer,
                       "read until delimiter including delimiter delimiter two messages who each fits in the buffer");
        tests->endTestBlock();

        tests->beginTestBlock("retry if no byte received");
        tests->addTest(testReadUntilDelimiterRetryIfNoByteReceived, "read until delimiter retry if no byte received");
        tests->addTest(testReadUntilDelimiterRetryIfNoByteReceivedAfterByteReceived,
                       "read until delimiter retry if no byte received after byte received");
        tests->endTestBlock();
        tests->endTestBlock();
        tests->endTestBlock();
    }
} // namespace networkTests
