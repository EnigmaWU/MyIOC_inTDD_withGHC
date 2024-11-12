#include <IOC/IOC.h>

const char* IOC_getResultStr(IOC_Result_T Result) {
    switch (Result) {
        case IOC_RESULT_SUCCESS:
            return "IOC_RESULT_SUCCESS";
        case IOC_RESULT_FAILURE:
            return "IOC_RESULT_FAILURE";
        case IOC_RESULT_POSIX_ENOMEM:
            return "IOC_RESULT_POSIX_ENOMEM";
        case IOC_RESULT_NOT_IMPLEMENTED:
            return "IOC_RESULT_NOT_IMPLEMENTED";
        case IOC_RESULT_NOT_SUPPORT:
            return "IOC_RESULT_NOT_SUPPORT";
        case IOC_RESULT_NO_EVENT_CONSUMER:
            return "IOC_RESULT_NO_EVENT_CONSUMER";
        case IOC_RESULT_TOO_MANY:
            return "IOC_RESULT_TOO_MANY";
        case IOC_RESULT_CONFLICT_EVENT_CONSUMER:
            return "IOC_RESULT_CONFLICT_EVENT_CONSUMER";
        case IOC_RESULT_INVALID_PARAM:
            return "IOC_RESULT_INVALID_PARAM";
        case IOC_RESULT_BUG:
            return "IOC_RESULT_BUG";
        default:
            return "IOC_RESULT_UNKNOWN";
    }
}