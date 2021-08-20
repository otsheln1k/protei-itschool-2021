#include <cstring>

#include <string>
#include <iostream>

#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "common.hpp"
#include "shmem.hpp"

static void
run_recv(int shmfd)
{
    ShmemBuffer shm {shmfd};

    for (;;) {
        if (saw_sigint) {
            shm.sendReadEof();
            break;
        }

        try {
            if (!shm.waitRead()) {
                break;
            }
        } catch (const std::system_error &e) {
            if (e.code().value() == EINTR) {
                continue;
            } else {
                throw;
            }
        }

        const char *payload = reinterpret_cast<const char *>(shm.data());

        const void *dataend = std::memchr(shm.data(), 0, shm.size());
        const char *end = reinterpret_cast<const char *>(
            (dataend == nullptr) ? shm.data() + shm.size() : dataend);

        std::string s {payload, end};

        std::cout << s;
        std::cout.flush();

        shm.sendDoneReading();
    }
}

int
main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " NAME\n";
        return EC_BAD_USAGE;
    }

    setup_sigint_handler();

    std::string shmname {"/shmem-"};
    shmname.append(argv[1]);

    int shmfd = shm_open(shmname.data(), O_RDWR, 0600);
    if (shmfd < 0) {
        std::perror("shm_open");
        return EC_ERROR;
    }

    run_recv(shmfd);

    if (shm_unlink(shmname.data()) < 0) {
        std::perror("shm_unlink");
        return EC_ERROR;
    }

    return EC_SUCCESS;
}
