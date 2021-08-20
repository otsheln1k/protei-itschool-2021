#include <iostream>
#include <system_error>

#include <errno.h>
#include <signal.h>

bool saw_sigint = false;

static void
handle_sigint(int)
{
    saw_sigint = true;
    std::cerr.put('\n');
}

void
setup_sigint_handler()
{
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, nullptr) < 0) {
        throw std::system_error(errno, std::system_category());
    }
}
