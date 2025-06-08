#include <stdint.h>

#ifndef __IOC_TYPES_CMDID_H__
#define __IOC_TYPES_CMDID_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t IOC_CmdID_T;
typedef uint64_t IOC_CmdNameID_T;
typedef uint64_t IOC_CmdClassID_T;

/**
 * @brief CmdID is IOC_CmdID_T which = CmdClass | CmdName
 */
#define IOC_defineCmdID(CmdClass, CmdName) ((IOC_CmdID_T)(((IOC_CmdID_T)CmdClass) | (((IOC_CmdID_T)CmdName) << 16)))
#define IOC_getCmdClassID(CmdID) ((IOC_CmdClassID_T)(CmdID & 0xFFFFULL))
#define IOC_getCmdNameID(CmdID) ((IOC_CmdNameID_T)(CmdID >> 16))

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//=====BEGIN: Command Class
//Enums========================================================================================
enum {
    IOC_CMD_CLASS_TEST = 1 << 0ULL,

    /**
     * @brief The system command class.
     *      This class is used to define IOC's internal system management commands.
     *      Use these commands to control and query IOC's internal state and behavior.
     */
    IOC_CMD_CLASS_SYSTEM = 1 << 1ULL,

    /**
     * @brief The diagnostic command class.
     *      This class is used to define IOC's internal diagnostic commands.
     *      Use these commands to verify IOC's behavior and diagnose any issues.
     */
    IOC_CMD_CLASS_DIAG = 1 << 2ULL,

    // TODO(@W): add more command class here
};

static inline const char *IOC_getCmdClassStr(IOC_CmdID_T CmdID) {
    IOC_CmdClassID_T CmdClassID = IOC_getCmdClassID(CmdID);

    if (CmdClassID == IOC_CMD_CLASS_TEST) {
        return "TEST";
    } else if (CmdClassID == IOC_CMD_CLASS_SYSTEM) {
        return "SYSTEM";
    } else if (CmdClassID == IOC_CMD_CLASS_DIAG) {
        return "DIAG";
    } else {
        return "UNKNOWN";
    }
}
//=====END: Command Class
//Enums==========================================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//=====BEGIN: Command Name Enums=======================================================================================
typedef enum {
    // Test Commands
    IOC_CMD_NAME_TEST_PING = 1 << 0ULL,
    IOC_CMD_NAME_TEST_ECHO = 1 << 1ULL,
    IOC_CMD_NAME_TEST_DELAY = 1 << 2ULL,
    IOC_CMD_NAME_TEST_CALC = 1 << 3ULL,

    // TODO(@W): add more command names here
} IOC_TestCmdNameID_E;

typedef enum {
    // System Commands
    IOC_CMD_NAME_SYSTEM_GET_VERSION = 1 << 0ULL,
    IOC_CMD_NAME_SYSTEM_GET_STATUS = 1 << 1ULL,
    IOC_CMD_NAME_SYSTEM_SHUTDOWN = 1 << 2ULL,
    IOC_CMD_NAME_SYSTEM_RESET = 1 << 3ULL,

    // TODO(@W): add more command names here
} IOC_SystemCmdNameID_E;

typedef enum {
    // Diagnostic Commands
    IOC_CMD_NAME_DIAG_GET_STATS = 1 << 0ULL,
    IOC_CMD_NAME_DIAG_DUMP_STATE = 1 << 1ULL,
    IOC_CMD_NAME_DIAG_TRACE_ENABLE = 1 << 2ULL,
    IOC_CMD_NAME_DIAG_TRACE_DISABLE = 1 << 3ULL,

    // TODO(@W): add more command names here
} IOC_DiagCmdNameID_E;

// TODO(@W): add more command name enums here

static inline const char *IOC_getCmdNameStr(IOC_CmdID_T CmdID) {
    IOC_CmdClassID_T CmdClassID = IOC_getCmdClassID(CmdID);
    IOC_CmdNameID_T CmdNameID = IOC_getCmdNameID(CmdID);

    if (CmdClassID == IOC_CMD_CLASS_TEST) {
        switch (CmdNameID) {
            case IOC_CMD_NAME_TEST_PING:
                return "PING";
            case IOC_CMD_NAME_TEST_ECHO:
                return "ECHO";
            case IOC_CMD_NAME_TEST_DELAY:
                return "DELAY";
            case IOC_CMD_NAME_TEST_CALC:
                return "CALC";
            default:
                return "UNKNOWN_TEST";
        }
    } else if (CmdClassID == IOC_CMD_CLASS_SYSTEM) {
        switch (CmdNameID) {
            case IOC_CMD_NAME_SYSTEM_GET_VERSION:
                return "GET_VERSION";
            case IOC_CMD_NAME_SYSTEM_GET_STATUS:
                return "GET_STATUS";
            case IOC_CMD_NAME_SYSTEM_SHUTDOWN:
                return "SHUTDOWN";
            case IOC_CMD_NAME_SYSTEM_RESET:
                return "RESET";
            default:
                return "UNKNOWN_SYSTEM";
        }
    } else if (CmdClassID == IOC_CMD_CLASS_DIAG) {
        switch (CmdNameID) {
            case IOC_CMD_NAME_DIAG_GET_STATS:
                return "GET_STATS";
            case IOC_CMD_NAME_DIAG_DUMP_STATE:
                return "DUMP_STATE";
            case IOC_CMD_NAME_DIAG_TRACE_ENABLE:
                return "TRACE_ENABLE";
            case IOC_CMD_NAME_DIAG_TRACE_DISABLE:
                return "TRACE_DISABLE";
            default:
                return "UNKNOWN_DIAG";
        }
    } else {
        return "UNKNOWN";
    }
}
//=====END: Command Name Enums=========================================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//====== Pre-defined Command IDs ======================================================================================

#define IOC_CMDID_TEST_PING IOC_defineCmdID(IOC_CMD_CLASS_TEST, IOC_CMD_NAME_TEST_PING)
#define IOC_CMDID_TEST_ECHO IOC_defineCmdID(IOC_CMD_CLASS_TEST, IOC_CMD_NAME_TEST_ECHO)
#define IOC_CMDID_TEST_DELAY IOC_defineCmdID(IOC_CMD_CLASS_TEST, IOC_CMD_NAME_TEST_DELAY)
#define IOC_CMDID_TEST_CALC IOC_defineCmdID(IOC_CMD_CLASS_TEST, IOC_CMD_NAME_TEST_CALC)

#define IOC_CMDID_SYSTEM_GET_VERSION IOC_defineCmdID(IOC_CMD_CLASS_SYSTEM, IOC_CMD_NAME_SYSTEM_GET_VERSION)
#define IOC_CMDID_SYSTEM_GET_STATUS IOC_defineCmdID(IOC_CMD_CLASS_SYSTEM, IOC_CMD_NAME_SYSTEM_GET_STATUS)
#define IOC_CMDID_SYSTEM_SHUTDOWN IOC_defineCmdID(IOC_CMD_CLASS_SYSTEM, IOC_CMD_NAME_SYSTEM_SHUTDOWN)
#define IOC_CMDID_SYSTEM_RESET IOC_defineCmdID(IOC_CMD_CLASS_SYSTEM, IOC_CMD_NAME_SYSTEM_RESET)

#define IOC_CMDID_DIAG_GET_STATS IOC_defineCmdID(IOC_CMD_CLASS_DIAG, IOC_CMD_NAME_DIAG_GET_STATS)
#define IOC_CMDID_DIAG_DUMP_STATE IOC_defineCmdID(IOC_CMD_CLASS_DIAG, IOC_CMD_NAME_DIAG_DUMP_STATE)
#define IOC_CMDID_DIAG_TRACE_ENABLE IOC_defineCmdID(IOC_CMD_CLASS_DIAG, IOC_CMD_NAME_DIAG_TRACE_ENABLE)
#define IOC_CMDID_DIAG_TRACE_DISABLE IOC_defineCmdID(IOC_CMD_CLASS_DIAG, IOC_CMD_NAME_DIAG_TRACE_DISABLE)

// TODO(@W): add more pre-defined command IDs here

//====== Pre-defined Command IDs ======================================================================================

#ifdef __cplusplus
}
#endif
#endif  // __IOC_TYPES_CMDID_H__
