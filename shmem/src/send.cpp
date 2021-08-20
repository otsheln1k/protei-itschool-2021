#include <string>
#include <iostream>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "common.hpp"
#include "shmem.hpp"

static bool saw_sigint = false;

static void
handle_sigint(int)
{
    saw_sigint = true;
    std::cerr.put('\n');
}

static constexpr size_t DEFAULT_BUFFER_SIZE = 4;

int
main(int argc, char **argv)
{
    if (2 > argc || argc > 3) {
        std::cerr << "usage: " << argv[0] << " NAME [BUFSIZE]\n";
        return EC_BAD_USAGE;
    }

    size_t bufsize = DEFAULT_BUFFER_SIZE;
    if (argc >= 3) {
        if (std::sscanf(argv[2], "%zu", &bufsize) != 1) {
            std::cerr << argv[0] << ": invalid buffer size\n";
            return EC_BAD_USAGE;
        }
        if (bufsize == 0) {
            std::cerr << argv[0] << ": buffer size must be > 0\n";
            return EC_BAD_USAGE;
        }
    }

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, nullptr) < 0) {
        std::perror("sigaction(SIGINT)");
        return EC_ERROR;
    }

    std::string shmname {"/shmem-"};
    shmname.append(argv[1]);

    int shmfd = shm_open(shmname.data(), O_RDWR | O_CREAT, 0600);
    if (shmfd < 0) {
        std::perror("shm_open");
        return EC_ERROR;
    }

    {
        ShmemBuffer shm {shmfd, bufsize};

        std::string buf;
        bool done = false;
        while (!done) {
            std::getline(std::cin, buf);
            if (saw_sigint
                || std::cin.eof()) {
                break;
            }

            buf.push_back('\n');

            const char *end = buf.data() + buf.size() + 1;
            for (const char *iter = buf.data(); iter != end;) {
                if (!shm.waitWrite()) {
                    std::cerr << argv[0] << ": channel closed on read end\n";
                    done = true;
                    break;
                }

                auto tail = std::min(iter + shm.size(), end);
                std::copy(iter, tail, shm.begin());

                iter = tail;

                shm.sendDoneWriting();
            }
        }

        shm.sendWriteEof();
    }

    return EC_SUCCESS;
}
