#include "IOC_EvtID.h"
#include "IOC_MsgDesc.h"
#include "IOC_Types.h"

#ifndef __IOC_TYPES_EVTDESC_H__
#define __IOC_TYPES_EVTDESC_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // MsgCommon
    IOC_MsgDesc_T MsgDesc;

    // EvtSpecific
    IOC_EvtID_T EvtID;
    ULONG_T EvtValue;

    // TOOD(@W): +More..., such as EvtPayload
} IOC_EvtDesc_T, *IOC_EvtDesc_pT;

static inline ULONG_T IOC_EvtDesc_getSeqID(IOC_EvtDesc_pT pEvtDesc) { return pEvtDesc->MsgDesc.SeqID; }
static inline IOC_EvtID_T IOC_EvtDesc_getEvtID(IOC_EvtDesc_pT pEvtDesc) { return pEvtDesc->EvtID; }
static inline ULONG_T IOC_EvtDesc_getEvtValue(IOC_EvtDesc_pT pEvtDesc) { return pEvtDesc->EvtValue; }

static inline const char *IOC_EvtDesc_getEvtClassStr(IOC_EvtDesc_pT pEvtDesc) {
    IOC_EvtID_T EvtID = IOC_EvtDesc_getEvtID(pEvtDesc);
    return IOC_getEvtClassStr(EvtID);
}

static inline const char *IOC_EvtDesc_getEvtNameStr(IOC_EvtDesc_pT pEvtDesc) {
    IOC_EvtID_T EvtID = IOC_EvtDesc_getEvtID(pEvtDesc);
    return IOC_getEvtNameStr(EvtID);
}

static inline const char *IOC_EvtDesc_getEvtFullNameStr(IOC_EvtDesc_pT pEvtDesc, char *EvtFullNameBuf,
                                                        size_t EvtFullNameBufSize) {
    static char _mEvtFullNameBuf[32];  // Use static buffer if EvtFullNameBuf is NULL,
                                       // for easy use but not thread-safe.
    //---------------------------------------------------------------------------------------------
    if (!EvtFullNameBuf) {
        EvtFullNameBuf = &_mEvtFullNameBuf[0];
        EvtFullNameBufSize = sizeof(_mEvtFullNameBuf);
    }

    snprintf(EvtFullNameBuf, EvtFullNameBufSize, "%s:%s", IOC_EvtDesc_getEvtClassStr(pEvtDesc),
             IOC_EvtDesc_getEvtNameStr(pEvtDesc));
    return EvtFullNameBuf;
}

#define IOC_EVTDESC_PRINTABLE_BUF_SIZE 64
static inline const char *IOC_EvtDesc_printDetail(IOC_EvtDesc_pT pEvtDesc, char *EvtDescBuf, size_t EvtDescBufSize) {
    // Use static buffer if EvtDescBuf is NULL,
    // for easy use but not thread-safe.
    static char _mEvtDescBuf[IOC_EVTDESC_PRINTABLE_BUF_SIZE];

    //---------------------------------------------------------------------------------------------
    char EvtFullNameBuf[32] = {0};

    if (!EvtDescBuf) {
        EvtDescBuf = &_mEvtDescBuf[0];
        EvtDescBufSize = sizeof(_mEvtDescBuf);
    }

    snprintf(EvtDescBuf, EvtDescBufSize, "SeqID=%lu, ID=%zu(%s), Value=%lu", IOC_EvtDesc_getSeqID(pEvtDesc),
             IOC_EvtDesc_getEvtID(pEvtDesc),
             IOC_EvtDesc_getEvtFullNameStr(pEvtDesc, EvtFullNameBuf, sizeof(EvtFullNameBuf)),
             IOC_EvtDesc_getEvtValue(pEvtDesc));
    return EvtDescBuf;
}

#ifdef __cplusplus
}
#endif
#endif  // __IOC_TYPES_EVTDESC_H__