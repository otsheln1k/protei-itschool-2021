#ifndef COMMON_HPP
#define COMMON_HPP

enum ExitCode {
    EC_SUCCESS = 0,
    EC_ERROR = 1,
    EC_BAD_USAGE = 2,
};

extern bool saw_sigint;

void setup_sigint_handler();

#endif
