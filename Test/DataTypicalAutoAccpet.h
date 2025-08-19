///////////////////////////////////////////////////////////////////////////////////////////////////
// Shared helpers for DAT Typical Auto-Accept tests (US-1/US-2)
//
// UT Design Notes:
// - This header centralizes small TDD helpers extracted from the original
//   UT_DataTypicalAutoAccept.cxx to avoid duplication after splitting tests.
// - Keep helpers minimal and portable across US-1 (service=receiver) and
//   US-2 (service=sender) scenarios. Avoid heavy dependencies here.
// - Coalescing-aware comparisons and byte-accurate checks live in the test
//   files; here we only provide callback skeletons or tiny utilities.
///////////////////////////////////////////////////////////////////////////////////////////////////
// Shared helpers for Data Typical Auto-Accept tests (US-1/US-2)
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <mutex>

#include "_UT_IOC_Common.h"

// Generic private data for DAT receive callback (used by service or client)
typedef struct TDD_DatRecvPriv {
    std::atomic<int> ReceivedDataCnt{0};
    std::atomic<ULONG_T> TotalReceivedSize{0};
    std::atomic<bool> CallbackExecuted{false};
    std::atomic<bool> ConnectionAccepted{false};
    char ReceivedContent[204800];  // 200KB buffer for typical/large payload checks
    int Index{0};                  // Optional identifier for logs
    std::mutex LinksMu;
    IOC_LinkID_T UniqueLinks[16] = {0};
    std::atomic<int> UniqueLinkCnt{0};
} TDD_DatRecvPriv_T;

// Static inline callback used by receivers (service in US-1, client in US-2)
static inline IOC_Result_T TDD_CbRecvDat(IOC_LinkID_T LinkID, IOC_DatDesc_pT pDatDesc, void *pCbPriv) {
    TDD_DatRecvPriv_T *pPriv = (TDD_DatRecvPriv_T *)pCbPriv;
    if (!pPriv || !pDatDesc) return IOC_RESULT_INVALID_PARAM;

    pPriv->ConnectionAccepted = true;  // arrival implies accepted

    void *pData = nullptr;
    ULONG_T DataSize = 0;
    IOC_Result_T r = IOC_getDatPayload(pDatDesc, &pData, &DataSize);
    if (r != IOC_RESULT_SUCCESS) {
        std::printf("TDD_CbRecvDat: get payload failed, r=%d\n", r);
        return r;
    }

    int curCnt = pPriv->ReceivedDataCnt.fetch_add(1) + 1;
    pPriv->CallbackExecuted = true;

    // Track unique LinkIDs
    {
        std::lock_guard<std::mutex> g(pPriv->LinksMu);
        bool found = false;
        int cnt = pPriv->UniqueLinkCnt.load();
        for (int i = 0; i < cnt; ++i)
            if (pPriv->UniqueLinks[i] == LinkID) {
                found = true;
                break;
            }
        if (!found && cnt < (int)(sizeof(pPriv->UniqueLinks) / sizeof(pPriv->UniqueLinks[0]))) {
            pPriv->UniqueLinks[cnt] = LinkID;
            pPriv->UniqueLinkCnt.store(cnt + 1);
        }
    }

    ULONG_T curTotal = pPriv->TotalReceivedSize.load();
    if (curTotal + DataSize <= sizeof(pPriv->ReceivedContent)) {
        std::memcpy(pPriv->ReceivedContent + curTotal, pData, DataSize);
    }
    pPriv->TotalReceivedSize.fetch_add(DataSize);

    std::printf("TDD_CbRecvDat: idx=%d, LinkID=%llu, got %lu bytes, count=%d, total=%lu\n", pPriv->Index, LinkID,
                DataSize, curCnt, pPriv->TotalReceivedSize.load());
    return IOC_RESULT_SUCCESS;
}

// Helper to send one DAT buffer on a link
static inline IOC_Result_T TDD_sendOneDAT(IOC_LinkID_T LinkID, const void *data, size_t size) {
    IOC_DatDesc_T d = {0};
    IOC_initDatDesc(&d);
    d.Payload.pData = (void *)data;
    d.Payload.PtrDataSize = size;
    d.Payload.PtrDataLen = size;
    return IOC_sendDAT(LinkID, &d, NULL);
}
