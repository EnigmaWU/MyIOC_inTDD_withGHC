#include "IOC_CmdID.h"
#include "IOC_MsgDesc.h"
#include "IOC_Types.h"

#ifndef __IOC_TYPES_CMDDESC_H__
#define __IOC_TYPES_CMDDESC_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Command execution status enumeration
 */
typedef enum {
    IOC_CMD_STATUS_PENDING = 0,     // Command is waiting to be processed
    IOC_CMD_STATUS_PROCESSING = 1,  // Command is being processed
    IOC_CMD_STATUS_SUCCESS = 2,     // Command executed successfully
    IOC_CMD_STATUS_FAILED = 3,      // Command execution failed
    IOC_CMD_STATUS_TIMEOUT = 4,     // Command execution timeout
    // TODO: IOC_CMD_STATUS_CANCELED = 5,    // Command was canceled
} IOC_CmdStatus_E;

/**
 * @brief Command payload structure for carrying command parameters and results
 *     IF EmbDataSize > 0, then EmbData[] is used to store the command data.
 *     IF EmbDataSize == 0, then pData is used to store the command data in heap memory.
 *     EmbData[] is used for small data payloads to avoid heap allocation.
 *     pData is used for larger data payloads that exceed the size of EmbData[].
 */
typedef struct {
    void *pData;          // Pointer to command data
    ULONG_T PtrDataSize;  // Size of command data in bytes

    ULONG_T EmdDataSize;  // Actual size of the data in bytes
    ULONG_T EmdData[8];
} IOC_CmdPayload_T, *IOC_CmdPayload_pT;

/**
 * @brief Command description structure
 *        Contains all information about a command including its ID, parameters, and results
 */
typedef struct {
    // MsgCommon - inherited from IOC_MsgDesc_T
    IOC_MsgDesc_T MsgDesc;

    // CmdSpecific
    IOC_CmdID_T CmdID;       // Command identifier
    IOC_CmdStatus_E Status;  // Current execution status
    IOC_Result_T Result;     // Execution result code

    // Command payload for parameters and results
    IOC_CmdPayload_T InPayload;   // Input parameters payload
    IOC_CmdPayload_T OutPayload;  // Output results payload

    // Execution context
    ULONG_T TimeoutMs;   // Command timeout in milliseconds (0 = no timeout)
    void *pExecContext;  // Execution context data (optional)

    // TODO(@W): +More..., such as priority, retry count, etc.
} IOC_CmdDesc_T, *IOC_CmdDesc_pT;

// Inline getter functions for command description
static inline ULONG_T IOC_CmdDesc_getSeqID(IOC_CmdDesc_pT pCmdDesc) { return pCmdDesc->MsgDesc.SeqID; }

static inline IOC_CmdID_T IOC_CmdDesc_getCmdID(IOC_CmdDesc_pT pCmdDesc) { return pCmdDesc->CmdID; }

static inline IOC_CmdStatus_E IOC_CmdDesc_getStatus(IOC_CmdDesc_pT pCmdDesc) { return pCmdDesc->Status; }

static inline IOC_Result_T IOC_CmdDesc_getResult(IOC_CmdDesc_pT pCmdDesc) { return pCmdDesc->Result; }

static inline const char *IOC_CmdDesc_getCmdClassStr(IOC_CmdDesc_pT pCmdDesc) {
    IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
    return IOC_getCmdClassStr(CmdID);
}

static inline const char *IOC_CmdDesc_getCmdNameStr(IOC_CmdDesc_pT pCmdDesc) {
    IOC_CmdID_T CmdID = IOC_CmdDesc_getCmdID(pCmdDesc);
    return IOC_getCmdNameStr(CmdID);
}

static inline const char *IOC_CmdDesc_getCmdFullNameStr(IOC_CmdDesc_pT pCmdDesc, char *CmdFullNameBuf,
                                                        size_t CmdFullNameBufSize) {
    static char _mCmdFullNameBuf[64];  // Use static buffer if CmdFullNameBuf is NULL,
                                       // for easy use but not thread-safe.
    //---------------------------------------------------------------------------------------------
    if (!CmdFullNameBuf) {
        CmdFullNameBuf = &_mCmdFullNameBuf[0];
        CmdFullNameBufSize = sizeof(_mCmdFullNameBuf);
    }

    snprintf(CmdFullNameBuf, CmdFullNameBufSize, "%s:%s", IOC_CmdDesc_getCmdClassStr(pCmdDesc),
             IOC_CmdDesc_getCmdNameStr(pCmdDesc));
    return CmdFullNameBuf;
}

static inline const char *IOC_CmdDesc_getStatusStr(IOC_CmdDesc_pT pCmdDesc) {
    switch (pCmdDesc->Status) {
        case IOC_CMD_STATUS_PENDING:
            return "PENDING";
        case IOC_CMD_STATUS_PROCESSING:
            return "PROCESSING";
        case IOC_CMD_STATUS_SUCCESS:
            return "SUCCESS";
        case IOC_CMD_STATUS_FAILED:
            return "FAILED";
        case IOC_CMD_STATUS_TIMEOUT:
            return "TIMEOUT";
        default:
            return "UNKNOWN";
    }
}

// Inline setter functions for command description
static inline void IOC_CmdDesc_setStatus(IOC_CmdDesc_pT pCmdDesc, IOC_CmdStatus_E Status) { pCmdDesc->Status = Status; }

static inline void IOC_CmdDesc_setResult(IOC_CmdDesc_pT pCmdDesc, IOC_Result_T Result) { pCmdDesc->Result = Result; }

static inline void IOC_CmdDesc_setTimeout(IOC_CmdDesc_pT pCmdDesc, ULONG_T TimeoutMs) {
    pCmdDesc->TimeoutMs = TimeoutMs;
}

// Helper functions for command payload management
static inline IOC_Result_T IOC_CmdDesc_setInPayload(IOC_CmdDesc_pT pCmdDesc, void *pData, ULONG_T DataSize) {
    if (!pCmdDesc) return IOC_RESULT_INVALID_PARAM;

    if (DataSize <= sizeof(pCmdDesc->InPayload.EmdData)) {
        // Use embedded data if size is small enough
        pCmdDesc->InPayload.EmdDataSize = DataSize;
        memcpy(pCmdDesc->InPayload.EmdData, pData, DataSize);
        pCmdDesc->InPayload.pData = NULL;     // Clear pointer data
        pCmdDesc->InPayload.PtrDataSize = 0;  // No pointer data size
    } else {
        // Use pointer data for larger payloads
        pCmdDesc->InPayload.pData =
            malloc(DataSize + 1);  // Allocate memory for pointer data, +1 for null terminator if string
        if (!pCmdDesc->InPayload.pData) {
            return IOC_RESULT_POSIX_ENOMEM;  // Memory allocation failed
        }
        memset(pCmdDesc->InPayload.pData, 0, DataSize + 1);  // Clear memory
        memcpy(pCmdDesc->InPayload.pData, pData, DataSize);
        pCmdDesc->InPayload.EmdDataSize = 0;         // Clear embedded data size
        pCmdDesc->InPayload.PtrDataSize = DataSize;  // Set actual size for pointer data
        pCmdDesc->InPayload.EmdDataSize = 0;         // Clear embedded data size
    }
    return IOC_RESULT_SUCCESS;
}

static inline IOC_Result_T IOC_CmdDesc_setOutPayload(IOC_CmdDesc_pT pCmdDesc, void *pData, ULONG_T DataSize) {
    if (!pCmdDesc) return IOC_RESULT_INVALID_PARAM;

    if (DataSize <= sizeof(pCmdDesc->OutPayload.EmdData)) {
        // Use embedded data if size is small enough
        pCmdDesc->OutPayload.EmdDataSize = DataSize;
        memcpy(pCmdDesc->OutPayload.EmdData, pData, DataSize);
        pCmdDesc->OutPayload.pData = NULL;     // Clear pointer data
        pCmdDesc->OutPayload.PtrDataSize = 0;  // No pointer data size
    } else {
        // Use pointer data for larger payloads
        pCmdDesc->OutPayload.pData =
            malloc(DataSize + 1);  // Allocate memory for pointer data, +1 for null terminator if string
        if (!pCmdDesc->OutPayload.pData) {
            return IOC_RESULT_POSIX_ENOMEM;  // Memory allocation failed
        }
        memset(pCmdDesc->OutPayload.pData, 0, DataSize + 1);  // Clear memory
        memcpy(pCmdDesc->OutPayload.pData, pData, DataSize);
        pCmdDesc->OutPayload.EmdDataSize = 0;         // Clear embedded data size
        pCmdDesc->OutPayload.PtrDataSize = DataSize;  // Set actual size for pointer data
    }

    return IOC_RESULT_SUCCESS;
}

static inline void *IOC_CmdDesc_getInData(IOC_CmdDesc_pT pCmdDesc) {
    if (pCmdDesc) {
        if (pCmdDesc->InPayload.EmdDataSize > 0) {
            return pCmdDesc->InPayload.EmdData;  // Use embedded data if available
        } else {
            return pCmdDesc->InPayload.pData;  // Use pointer data otherwise
        }
    }
    return NULL;  // Return NULL if pCmdDesc is NULL
}

static inline ULONG_T IOC_CmdDesc_getInDataSize(IOC_CmdDesc_pT pCmdDesc) {
    if (pCmdDesc) {
        return pCmdDesc->InPayload.PtrDataSize > 0 ? pCmdDesc->InPayload.PtrDataSize
                                                   : pCmdDesc->InPayload.EmdDataSize;  // Return size of data
    }
    return 0;  // Return 0 if pCmdDesc is NULL
}

static inline void *IOC_CmdDesc_getOutData(IOC_CmdDesc_pT pCmdDesc) {
    if (pCmdDesc) {
        if (pCmdDesc->OutPayload.EmdDataSize > 0) {
            return pCmdDesc->OutPayload.EmdData;  // Use embedded data if available
        } else {
            return pCmdDesc->OutPayload.pData;  // Use pointer data otherwise
        }
    }
    return NULL;  // Return NULL if pCmdDesc is NULL
}

static inline ULONG_T IOC_CmdDesc_getOutDataSize(IOC_CmdDesc_pT pCmdDesc) {
    if (pCmdDesc) {
        return pCmdDesc->OutPayload.PtrDataSize > 0 ? pCmdDesc->OutPayload.PtrDataSize
                                                    : pCmdDesc->OutPayload.EmdDataSize;  // Return size of data
    }
    return 0;  // Return 0 if pCmdDesc is NULL
}

#define IOC_CMDDESC_PRINTABLE_BUF_SIZE 128

// Helper function to create a printable string representation of command description
static inline const char *IOC_CmdDesc_toPrintableStr(IOC_CmdDesc_pT pCmdDesc, char *PrintableBuf,
                                                     size_t PrintableBufSize) {
    static char _mPrintableBuf[IOC_CMDDESC_PRINTABLE_BUF_SIZE];  // Use static buffer if PrintableBuf is NULL
    //---------------------------------------------------------------------------------------------
    if (!PrintableBuf) {
        PrintableBuf = &_mPrintableBuf[0];
        PrintableBufSize = sizeof(_mPrintableBuf);
    }

    snprintf(PrintableBuf, PrintableBufSize, "CmdDesc[SeqID=%lu, Cmd=%s, Status=%s, Result=%d, TimeoutMs=%lu]",
             IOC_CmdDesc_getSeqID(pCmdDesc), IOC_CmdDesc_getCmdFullNameStr(pCmdDesc, NULL, 0),
             IOC_CmdDesc_getStatusStr(pCmdDesc), IOC_CmdDesc_getResult(pCmdDesc), pCmdDesc->TimeoutMs);
    return PrintableBuf;
}

#ifdef __cplusplus
}
#endif
#endif  // __IOC_TYPES_CMDDESC_H__
