#include <IOC/IOC.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===>BEGIN: IOC Event in Conles Mode
static IOC_SubEvtArgs_T _mConlesModeSubEvtArgs;

static IOC_Result_T __IOC_subEVT_inConlesMode(
    /*ARG_IN*/ const IOC_SubEvtArgs_pT pSubEvtArgs) {
  memcpy(&_mConlesModeSubEvtArgs, pSubEvtArgs, sizeof(IOC_SubEvtArgs_T));
  return IOC_RESULT_SUCCESS;
}

static IOC_Result_T __IOC_unsubEVT_inConlesMode(
    /*ARG_IN*/ const IOC_UnsubEvtArgs_pT pUnsubEvtArgs) {
  if (_mConlesModeSubEvtArgs.CbProcEvt_F && (pUnsubEvtArgs->CbProcEvt_F == _mConlesModeSubEvtArgs.CbProcEvt_F)) {
    memset(&_mConlesModeSubEvtArgs, 0, sizeof(IOC_SubEvtArgs_T));
    return IOC_RESULT_SUCCESS;
  }
  return IOC_RESULT_NO_EVTCOSMER;
}

static IOC_Result_T __IOC_postEVT_inConlesMode(
    /*ARG_IN*/ const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/ const IOC_Options_pT pOptions) {
  if (_mConlesModeSubEvtArgs.CbProcEvt_F) {
    return _mConlesModeSubEvtArgs.CbProcEvt_F(pEvtDesc, _mConlesModeSubEvtArgs.pCbPrivData);
  } else {
    return IOC_RESULT_NO_EVTCOSMER;
  }
}
//===>END: IOC Event in Conles Mode
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IOC_Result_T IOC_postEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_EvtDesc_pT pEvtDesc,
    /*ARG_IN_OPTIONAL*/IOC_Options_pT pOptions)
{
  if (LinkID == IOC_CONLES_MODE_AUTO_LINK_ID) {
    return __IOC_postEVT_inConlesMode(pEvtDesc, pOptions);
  } else {
    return IOC_RESULT_NOT_IMPLEMENTED;
  }
}

IOC_Result_T IOC_subEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_SubEvtArgs_pT pSubEvtArgs)
{
  if (LinkID == IOC_CONLES_MODE_AUTO_LINK_ID) {
    return __IOC_subEVT_inConlesMode(pSubEvtArgs);
  } else {
    return IOC_RESULT_NOT_IMPLEMENTED;
  }
}

IOC_Result_T IOC_unsubEVT(
    /*ARG_IN*/IOC_LinkID_T LinkID, 
    /*ARG_IN*/const IOC_UnsubEvtArgs_pT pUnsubEvtArgs)
{
  if (LinkID == IOC_CONLES_MODE_AUTO_LINK_ID) {
    return __IOC_unsubEVT_inConlesMode(pUnsubEvtArgs);
  } else {
    return IOC_RESULT_NOT_IMPLEMENTED;
  }
}
