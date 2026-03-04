#include "wevoa/wstatus.h"

const char* wstatus_str(int code) {
    switch (code) {
        case W_OK:
            return "OK";
        case W_ERR_INVALID_ARG:
            return "INVALID_ARG";
        case W_ERR_NOT_FOUND:
            return "NOT_FOUND";
        case W_ERR_EXISTS:
            return "EXISTS";
        case W_ERR_IO:
            return "IO";
        case W_ERR_NO_SPACE:
            return "NO_SPACE";
        case W_ERR_CORRUPT:
            return "CORRUPT";
        case W_ERR_UNSUPPORTED:
            return "UNSUPPORTED";
        default:
            return "UNKNOWN";
    }
}

