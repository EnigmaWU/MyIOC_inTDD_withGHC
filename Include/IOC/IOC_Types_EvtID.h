#include <stdint.h>

// #include "IOC_Types.h"

#ifndef __IOC_TYPES_EVTID_H__
#define __IOC_TYPES_EVTID_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t IOC_EvtID_T;
typedef uint64_t IOC_EvtNameID_T;
typedef uint64_t IOC_EvtClassID_T;

/**
 * @brief EvtID is IOC_EvtID_T which = EvtClass | EvtName
 */
#define IOC_defineEvtID(EvtClass, EvtName) ((IOC_EvtID_T)((EvtClass) | ((EvtName) << 16)))
#define IOC_getEvtClassID(EvtID) ((IOC_EvtClassID_T)(EvtID & 0xFFFFULL))
// TODO: IOC_getEvtClassStr(EvtID)
#define IOC_getEvtNameID(EvtID) ((IOC_EvtNameID_T)(EvtID >> 16))
// TODO: IOC_getEvtNameStr(EvtID)

enum {
  IOC_EVT_CLASS_TEST = 1 << 0ULL,
  // TODO(@W): add more event class here
};

static inline const char *IOC_getEvtClassByID(IOC_EvtID_T EvtID) {
  IOC_EvtClassID_T EvtClassID = IOC_getEvtClassID(EvtID);

  if (EvtClassID == IOC_EVT_CLASS_TEST) {
    return "TEST";
  } else {
    return "UNKNOWN";
  }
}

typedef enum {
  IOC_EVT_NAME_TEST_KEEPALIVE              = 1 << 0ULL,
  IOC_EVT_NAME_TEST_HELLO_FROM_ODD_TO_EVEN = 1 << 1ULL,
  IOC_EVT_NAME_TEST_HELLO_FROM_EVEN_TO_ODD = 1 << 2ULL,
  IOC_EVT_NAME_TEST_SLEEP_9MS              = 1 << 3ULL,
  IOC_EVT_NAME_TEST_SLEEP_99MS             = 1 << 4ULL,
  IOC_EVT_NAME_TEST_KEEPALIVE_RELAY        = 1 << 5ULL,
  // TODO(@W): add more event name here
} IOC_EvtNameTest_T;

static inline const char *IOC_getEvtNameOfClassTest(IOC_EvtNameID_T EvtNameID) {
  switch (EvtNameID) {
    case IOC_EVT_NAME_TEST_KEEPALIVE:
      return "KEEPALIVE";
    case IOC_EVT_NAME_TEST_HELLO_FROM_ODD_TO_EVEN:
      return "HELLO_FROM_ODD_TO_EVEN";
    case IOC_EVT_NAME_TEST_HELLO_FROM_EVEN_TO_ODD:
      return "HELLO_FROM_EVEN_TO_ODD";
    case IOC_EVT_NAME_TEST_SLEEP_9MS:
      return "SLEEP_9MS";
    case IOC_EVT_NAME_TEST_SLEEP_99MS:
      return "SLEEP_99MS";
    case IOC_EVT_NAME_TEST_KEEPALIVE_RELAY:
      return "KEEPALIVE_RELAY";
    default:
      return "UNKNOWN";
  }
}

static inline const char *IOC_getEvtNameByID(IOC_EvtID_T EvtID) {
  IOC_EvtNameID_T EvtNameID   = IOC_getEvtNameID(EvtID);
  IOC_EvtClassID_T EvtClassID = IOC_getEvtClassID(EvtID);

  if (EvtClassID == IOC_EVT_CLASS_TEST) {
    return IOC_getEvtNameOfClassTest(EvtNameID);
  } else {
    return "UNKNOWN";
  }
}

#define IOC_EVTID_TEST_KEEPALIVE IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_KEEPALIVE)
#define IOC_EVTID_TEST_HELLO_FROM_ODD_TO_EVEN IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_HELLO_FROM_ODD_TO_EVEN)
#define IOC_EVTID_TEST_HELLO_FROM_EVEN_TO_ODD IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_HELLO_FROM_EVEN_TO_ODD)
#define IOC_EVTID_TEST_SLEEP_9MS IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_SLEEP_9MS)
#define IOC_EVTID_TEST_SLEEP_99MS IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_SLEEP_99MS)
#define IOC_EVTID_TEST_KEEPALIVE_RELAY IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_KEEPALIVE_RELAY)
// TODO(@W):

#ifdef __cplusplus
}
#endif
#endif//__IOC_TYPES_EVTID_H__