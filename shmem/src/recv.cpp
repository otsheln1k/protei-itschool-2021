#include <cstring>

#include <string>
#include <iostream>

#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "shmem.hpp"

static bool saw_sigint = false;

static void
handle_sigint(int)
{
    saw_sigint = true;
    std::cerr.put('\n');
}

int
main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " NAME\n";
        return 2;
    }

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, nullptr) < 0) {
        std::perror("sigaction(SIGINT)");
        return 1;
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

    if (shm_unlink(shmname.data()) < 0) {
        std::perror("shm_unlink");
        return 1;
    }

    return 0;
}
