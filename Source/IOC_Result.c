#include <IOC/IOC.h>

// Note: Several IOC_Result_T values are aliases (same numeric value).
// Since we cannot distinguish aliases at runtime by value, we return the
// base name for each unique numeric value.
const char* IOC_getResultStr(IOC_Result_T Result) {
    switch (Result) {
        // Generic results
        case IOC_RESULT_SUCCESS: return "IOC_RESULT_SUCCESS";
        case IOC_RESULT_FAILURE:
            return "IOC_RESULT_FAILURE";

        // POSIX-derived results
        case IOC_RESULT_POSIX_ENOMEM: return "IOC_RESULT_POSIX_ENOMEM";
    // EINVAL handled below with INVALID_PARAM alias
        case IOC_RESULT_POSIX_EAGAIN: return "IOC_RESULT_POSIX_EAGAIN";
        // handled above aliasing FAILURE

        // IOC feature/support
        case IOC_RESULT_NOT_IMPLEMENTED: return "IOC_RESULT_NOT_IMPLEMENTED";
        case IOC_RESULT_NOT_SUPPORT:     return "IOC_RESULT_NOT_SUPPORT";

        // Capacity/limits
        case IOC_RESULT_TOO_MANY: return "IOC_RESULT_TOO_MANY";

        // Conflicts
        case IOC_RESULT_CONFLICT: return "IOC_RESULT_CONFLICT";

        // Not exist / lookup failures
        case IOC_RESULT_NOT_EXIST: return "IOC_RESULT_NOT_EXIST";

        // Flow/control and state
        case IOC_RESULT_TIMEOUT:      return "IOC_RESULT_TIMEOUT";
        case IOC_RESULT_BUSY:         return "IOC_RESULT_BUSY";
        case IOC_RESULT_LINK_BROKEN:  return "IOC_RESULT_LINK_BROKEN";

        // Command subsystem
        case IOC_RESULT_CMD_EXEC_FAILED:  return "IOC_RESULT_CMD_EXEC_FAILED";
        case IOC_RESULT_NO_CMD_EXECUTOR:  return "IOC_RESULT_NO_CMD_EXECUTOR";

        // Data subsystem
        case IOC_RESULT_BUFFER_FULL:      return "IOC_RESULT_BUFFER_FULL";
        case IOC_RESULT_BUFFER_TOO_SMALL: return "IOC_RESULT_BUFFER_TOO_SMALL";
        case IOC_RESULT_DATA_TOO_LARGE:   return "IOC_RESULT_DATA_TOO_LARGE";
        case IOC_RESULT_NO_DATA:          return "IOC_RESULT_NO_DATA";
        case IOC_RESULT_NOT_EXIST_STREAM: return "IOC_RESULT_NOT_EXIST_STREAM";
        case IOC_RESULT_ACK_CMD_FAILED:   return "IOC_RESULT_ACK_CMD_FAILED";

        // Event queueing
        case IOC_RESULT_EVTDESC_QUEUE_EMPTY:           return "IOC_RESULT_EVTDESC_QUEUE_EMPTY";
        case IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE: return "IOC_RESULT_TOO_LONG_EMPTYING_EVTDESC_QUEUE";
        case IOC_RESULT_INVALID_AUTO_LINK_ID:          return "IOC_RESULT_INVALID_AUTO_LINK_ID";

        // Event consumer availability
        case IOC_RESULT_NO_EVENT_CONSUMER: return "IOC_RESULT_NO_EVENT_CONSUMER";

        // Parameter validation (EINVAL base and its alias)
        case IOC_RESULT_INVALID_PARAM:
            return "IOC_RESULT_INVALID_PARAM";

        // Internal consistency
        case IOC_RESULT_BUG: return "IOC_RESULT_BUG";

        default:
            return "IOC_RESULT_UNKNOWN";
    }
}