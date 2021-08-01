#include <string>
#include <iostream>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "shmem.hpp"

static constexpr size_t DEFAULT_BUFFER_SIZE = 4;

int
main(int argc, char **argv)
{
    if (2 > argc || argc > 3) {
        std::cerr << "usage: " << argv[0] << " NAME [BUFSIZE]\n";
        return 2;
    }

    size_t bufsize = DEFAULT_BUFFER_SIZE;
    if (argc >= 3) {
        if (std::sscanf(argv[2], "%zu", &bufsize) != 1) {
            std::cerr << argv[0] << ": invalid buffer size\n";
            return 2;
        }
        if (bufsize == 0) {
            std::cerr << argv[0] << ": buffer size must be > 0\n";
            return 2;
        }
    }

    std::string shmname {"/shmem-"};
    shmname.append(argv[1]);

    int shmfd = shm_open(shmname.data(), O_RDWR | O_CREAT, 0600);
    if (shmfd < 0) {
        std::perror("shm_open");
        return 1;
    }

    {
        ShmemBuffer shm {shmfd, bufsize};

        std::string buf;
        for (;;) {
            std::getline(std::cin, buf);
            if (std::cin.eof()) {
                break;
            }

            buf.push_back('\n');

            const char *end = buf.data() + buf.size() + 1;
            for (const char *iter = buf.data(); iter != end;) {
                shm.waitWrite();

                auto tail = std::min(iter + shm.size(), end);
                std::copy(iter, tail, shm.begin());

                iter = tail;

                shm.sendDoneWriting();
            }
        }

        shm.sendWriteEof();
    }

    return 0;
}
