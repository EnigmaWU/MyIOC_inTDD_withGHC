#include "_IOC.h"

static IOC_Result_T __IOC_getCapability_inConetMode(
    /*ARG_INOUT*/ IOC_CapabilityDescription_pT pCapDesc) {
    pCapDesc->ConetMode.MaxSrvNum = _MAX_IOC_SRV_OBJ_NUM;
    pCapDesc->ConetMode.MaxCliNum = _MAX_IOC_CLI_OBJ_NUM_PER_SRV;
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T IOC_getCapability(
    /*ARG_INOUT*/ IOC_CapabilityDescription_pT pCapDesc) {
    IOC_Result_T Result = IOC_RESULT_BUG;

    switch (pCapDesc->CapID) {
        case IOC_CAPID_CONLES_MODE_EVENT: {
            Result = _IOC_getCapability_inConlesMode(pCapDesc);
        } break;

        case IOC_CAPID_CONET_MODE: {
            Result = __IOC_getCapability_inConetMode(pCapDesc);
        } break;

        default:
            _IOC_LogError("Not-Support CapID(%d)", pCapDesc->CapID);
            Result = IOC_RESULT_NOT_SUPPORT;
    }

    return Result;
}
