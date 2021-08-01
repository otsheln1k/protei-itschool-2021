#include <string>
#include <iostream>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "shmem.hpp"

static constexpr size_t SHMEM_BUFFER_SIZE = 4;

int
main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " NAME\n";
        return 2;
    }

    std::string shmname {"/shmem-"};
    shmname.append(argv[1]);

    int shmfd = shm_open(shmname.data(), O_RDWR | O_CREAT, 0600);
    if (shmfd < 0) {
        std::perror("shm_open");
        return 1;
    }

    {
        ShmemBuffer shm {shmfd, SHMEM_BUFFER_SIZE};

        std::string buf;
        for (;;) {
            std::getline(std::cin, buf);
            if (std::cin.eof()) {
                break;
            }

            buf.push_back('\n');

            for (auto iter = buf.begin(); iter != buf.end();) {
                shm.waitWrite();

                auto tail = std::min(iter + shm.size() - 1, buf.end());
                auto endpart = std::copy(iter, tail, shm.begin());
                *endpart = 0;

                iter = tail;

                shm.sendDoneWriting();
            }
        }

        shm.sendWriteEof();
    }

    return 0;
}
