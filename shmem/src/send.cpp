#include <string>
#include <iostream>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "common.hpp"
#include "shmem.hpp"

static constexpr size_t DEFAULT_BUFFER_SIZE = 4;

struct ProgramOptions {
    const char *progname;
    const char *name;
    size_t bufsize;
};

static bool
parse_cmdline(int argc, char **argv, ProgramOptions &opts)
{
    if (2 > argc || argc > 3) {
        return false;
    }

    opts.progname = argv[0];
    opts.name = argv[1];

    opts.bufsize = DEFAULT_BUFFER_SIZE;
    if (argc >= 3) {
        if (std::sscanf(argv[2], "%zu", &opts.bufsize) != 1
            || opts.bufsize == 0) {
            return false;
        }
    }

    return true;
}

static void
run_send(int shmfd, const ProgramOptions &opts)
{
    ShmemBuffer shm {shmfd, opts.bufsize};

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
                std::cerr << opts.progname << ": channel closed on read end\n";
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

int
main(int argc, char **argv)
{
    ProgramOptions opts;
    if (!parse_cmdline(argc, argv, opts)) {
        std::cerr
            << "usage: " << argv[0] << " NAME [BUFSIZE]\n"
            << "  NAME is a \"channel\" identifier. It is used to\n"
            << "    construct a shm object name.\n"
            << "  BUFSIZE (positive integer) is the buffer size to\n"
            << "    use for messages. The actual shm area size will\n"
            << "    be bigger by 2*sizeof(shm_t).\n";
        return EC_BAD_USAGE;
    }

    setup_sigint_handler();

    std::string shmname {"/shmem-"};
    shmname.append(opts.name);

    int shmfd = shm_open(shmname.data(), O_RDWR | O_CREAT, 0600);
    if (shmfd < 0) {
        std::perror("shm_open");
        return EC_ERROR;
    }

    run_send(shmfd, opts);

    return EC_SUCCESS;
}
