#include "_IOC.h"

//=================================================================================================

IOC_Result_T IOC_onlineService(
    /*ARG_OUT */ IOC_SrvID_pT pSrvID,
    /*ARG_IN*/ const IOC_SrvArgs_pT pSrvArgs) {
    _IOC_LogNotTested();
    return IOC_RESULT_NOT_IMPLEMENTED;
}
IOC_Result_T IOC_offlineService(
    /*ARG_IN*/ IOC_SrvID_T SrvID) {
    _IOC_LogNotTested();
    return IOC_RESULT_NOT_IMPLEMENTED;
}

IOC_Result_T IOC_acceptLink(
    /*ARG_IN*/ IOC_SrvID_T SrvID,
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
    _IOC_LogNotTested();
    return IOC_RESULT_NOT_IMPLEMENTED;
}

IOC_Result_T IOC_connectService(
    /*ARG_OUT*/ IOC_LinkID_pT pLinkID,
    /*ARG_IN*/ const IOC_ConnArgs_pT pConnArgs,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOption) {
    _IOC_LogNotTested();
    return IOC_RESULT_NOT_IMPLEMENTED;
}

IOC_Result_T IOC_closeLink(
    /*ARG_IN*/ IOC_LinkID_T LinkID) {
    _IOC_LogNotTested();
    return IOC_RESULT_NOT_IMPLEMENTED;
}