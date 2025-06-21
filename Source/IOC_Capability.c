#include "_IOC.h"

static IOC_Result_T __IOC_getCapability_inConetModeData(
    /*ARG_INOUT*/ IOC_CapabilityDescription_pT pCapDesc) {
    pCapDesc->ConetModeData.Common.MaxSrvNum = _MAX_IOC_SRV_OBJ_NUM;
    pCapDesc->ConetModeData.Common.MaxCliNum = _MAX_IOC_CLI_OBJ_NUM_PER_SRV;
    pCapDesc->ConetModeData.MaxDataQueueSize =
        (128 << 10);  // TODO(@W): Define proper constant by Tiny/Typical/Titan Version
                      // such as: CONFIG_IOC_TINY_VERSION, CONFIG_IOC_TYPICAL_VERSION, CONFIG_IOC_TITAN_VERSION
    return IOC_RESULT_SUCCESS;
}

static IOC_Result_T __IOC_getCapability_inConetModeEvent(
    /*ARG_INOUT*/ IOC_CapabilityDescription_pT pCapDesc) {
    pCapDesc->ConetModeEvent.Common.MaxSrvNum = _MAX_IOC_SRV_OBJ_NUM;
    pCapDesc->ConetModeEvent.Common.MaxCliNum = _MAX_IOC_CLI_OBJ_NUM_PER_SRV;
    return IOC_RESULT_SUCCESS;
}

static IOC_Result_T __IOC_getCapability_inConetModeCommand(
    /*ARG_INOUT*/ IOC_CapabilityDescription_pT pCapDesc) {
    pCapDesc->ConetModeCommand.Common.MaxSrvNum = _MAX_IOC_SRV_OBJ_NUM;
    pCapDesc->ConetModeCommand.Common.MaxCliNum = _MAX_IOC_CLI_OBJ_NUM_PER_SRV;
    return IOC_RESULT_SUCCESS;
}

IOC_Result_T IOC_getCapability(
    /*ARG_INOUT*/ IOC_CapabilityDescription_pT pCapDesc) {
    IOC_Result_T Result = IOC_RESULT_BUG;

    switch (pCapDesc->CapID) {
        case IOC_CAPID_CONLES_MODE_EVENT: {
            Result = _IOC_getCapability_inConlesMode(pCapDesc);
        } break;

        case IOC_CAPID_CONET_MODE_DATA: {
            Result = __IOC_getCapability_inConetModeData(pCapDesc);
        } break;

        case IOC_CAPID_CONET_MODE_EVENT: {
            Result = __IOC_getCapability_inConetModeEvent(pCapDesc);
        } break;

        case IOC_CAPID_CONET_MODE_COMMAND: {
            Result = __IOC_getCapability_inConetModeCommand(pCapDesc);
        } break;

        default:
            _IOC_LogError("Not-Support CapID(%d)", pCapDesc->CapID);
            Result = IOC_RESULT_NOT_SUPPORT;
    }

    return Result;
}
