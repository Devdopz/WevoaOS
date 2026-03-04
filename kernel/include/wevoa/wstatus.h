#ifndef WEVOA_WSTATUS_H
#define WEVOA_WSTATUS_H

enum wevoa_status {
    W_OK = 0,
    W_ERR_INVALID_ARG = -1,
    W_ERR_NOT_FOUND = -2,
    W_ERR_EXISTS = -3,
    W_ERR_IO = -4,
    W_ERR_NO_SPACE = -5,
    W_ERR_CORRUPT = -6,
    W_ERR_UNSUPPORTED = -7,
};

const char* wstatus_str(int code);

#endif

