#include <cstring>

#include <string>
#include <iostream>

#include <sys/stat.h>
#include <fcntl.h>

#include "shmem.hpp"

int
main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " NAME\n";
        return 2;
    }

    std::string shmname {"/shmem-"};
    shmname.append(argv[1]);

    int shmfd = shm_open(shmname.data(), O_RDWR, 0600);
    if (shmfd < 0) {
        std::perror("shm_open");
        return 1;
    }

    {
        ShmemBuffer shm {shmfd};

        while (shm.waitRead()) {
            std::string s {reinterpret_cast<const char *>(shm.data())};

            std::cout << s;
            std::cout.flush();

            shm.sendDoneReading();
        }
    }

    if (shm_unlink(shmname.data()) < 0) {
        std::perror("shm_unlink");
        return 1;
    }

    return 0;
}
