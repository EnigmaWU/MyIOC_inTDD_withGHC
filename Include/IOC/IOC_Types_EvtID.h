#include <stdint.h>

#ifndef __IOC_TYPES_EVTID_H__
#define __IOC_TYPES_EVTID_H__
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EvtID is IOC_EvtID_T which = EvtClass | EvtName
 */
#define IOC_defineEvtID(EvtClass, EvtName) ((IOC_EvtID_T)((EvtClass) | ((EvtName) << 16)))

typedef enum {
  IOC_EVT_CLASS_TEST = 1 << 0ULL,
  // TODO(@W): add more event class here
} IOC_EvtClass_T;

typedef enum {
  IOC_EVT_NAME_TEST_KEEPALIVE = 1 << 0ULL,
  // TODO(@W): add more event name here
} IOC_EvtNameTest_T;

#define IOC_EVTID_TEST_KEEPALIVE IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_KEEPALIVE)
// TODO(@W):

#ifdef __cplusplus
}
#endif
#endif//__IOC_TYPES_EVTID_H__