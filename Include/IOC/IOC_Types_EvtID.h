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
  IOC_EVT_NAME_TEST_HELLO_FROM_ODD_TO_EVEN = 1 << 1ULL,
  IOC_EVT_NAME_TEST_HELLO_FROM_EVEN_TO_ODD = 1 << 2ULL,
  IOC_EVT_NAME_TEST_SLEEP_9MS = 1 << 3ULL,
  IOC_EVT_NAME_TEST_SLEEP_99MS = 1 << 4ULL,
  // TODO(@W): add more event name here
} IOC_EvtNameTest_T;

#define IOC_EVTID_TEST_KEEPALIVE IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_KEEPALIVE)
#define IOC_EVTID_TEST_HELLO_FROM_ODD_TO_EVEN IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_HELLO_FROM_ODD_TO_EVEN)
#define IOC_EVTID_TEST_HELLO_FROM_EVEN_TO_ODD IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_HELLO_FROM_EVEN_TO_ODD)
#define IOC_EVTID_TEST_SLEEP_9MS IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_SLEEP_9MS)
#define IOC_EVTID_TEST_SLEEP_99MS IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_SLEEP_99MS)
// TODO(@W):

#ifdef __cplusplus
}
#endif
#endif//__IOC_TYPES_EVTID_H__