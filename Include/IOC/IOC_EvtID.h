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

static inline const char *IOC_getEvtClassStr(IOC_EvtID_T EvtID) {
    IOC_EvtClassID_T EvtClassID = IOC_getEvtClassID(EvtID);

    if (EvtClassID == IOC_EVT_CLASS_TEST) {
        return "TEST";
    } else {
        return "UNKNOWN";
    }
}

typedef enum {
    IOC_EVT_NAME_TEST_KEEPALIVE = 1 << 0ULL,
    IOC_EVT_NAME_TEST_KEEPALIVE_RELAY = 1 << 1ULL,

    IOC_EVT_NAME_TEST_HELLO_FROM_ODD_TO_EVEN = 1 << 2ULL,
    IOC_EVT_NAME_TEST_HELLO_FROM_EVEN_TO_ODD = 1 << 3ULL,

    IOC_EVT_NAME_TEST_SLEEP_9MS = 1 << 4ULL,
    IOC_EVT_NAME_TEST_SLEEP_99MS = 1 << 5ULL,
    IOC_EVT_NAME_TEST_SLEEP_999MS = 1 << 6ULL,

    IOC_EVT_NAME_TEST_SLEEP_9US = 1 << 7ULL,
    IOC_EVT_NAME_TEST_SLEEP_99US = 1 << 8ULL,
    IOC_EVT_NAME_TEST_SLEEP_999US = 1 << 9ULL,

    IOC_EVT_NAME_TEST_MOVE_STARTED = 1 << 10ULL,
    IOC_EVT_NAME_TEST_MOVE_KEEPING = 1 << 11ULL,
    IOC_EVT_NAME_TEST_MOVE_STOPPED = 1 << 12ULL,

    IOC_EVT_NAME_TEST_PUSH_STARTED = 1 << 13ULL,
    IOC_EVT_NAME_TEST_PUSH_KEEPING = 1 << 14ULL,
    IOC_EVT_NAME_TEST_PUSH_STOPPED = 1 << 15ULL,

    IOC_EVT_NAME_TEST_PULL_STARTED = 1 << 16ULL,
    IOC_EVT_NAME_TEST_PULL_KEEPING = 1 << 17ULL,
    IOC_EVT_NAME_TEST_PULL_STOPPED = 1 << 18ULL,

    // TODO(@W): add more event name here
} IOC_EvtNameTest_T;

static inline const char *IOC_getTestClassEvtNameStr(IOC_EvtNameID_T EvtNameID) {
    switch (EvtNameID) {
        case IOC_EVT_NAME_TEST_KEEPALIVE:
            return "KEEPALIVE";
        case IOC_EVT_NAME_TEST_KEEPALIVE_RELAY:
            return "KEEPALIVE_RELAY";

        case IOC_EVT_NAME_TEST_HELLO_FROM_ODD_TO_EVEN:
            return "HELLO_FROM_ODD_TO_EVEN";
        case IOC_EVT_NAME_TEST_HELLO_FROM_EVEN_TO_ODD:
            return "HELLO_FROM_EVEN_TO_ODD";

        case IOC_EVT_NAME_TEST_SLEEP_9MS:
            return "SLEEP_9MS";
        case IOC_EVT_NAME_TEST_SLEEP_99MS:
            return "SLEEP_99MS";
        case IOC_EVT_NAME_TEST_SLEEP_999MS:
            return "SLEEP_999MS";

        case IOC_EVT_NAME_TEST_SLEEP_9US:
            return "SLEEP_9US";
        case IOC_EVT_NAME_TEST_SLEEP_99US:
            return "SLEEP_99US";
        case IOC_EVT_NAME_TEST_SLEEP_999US:
            return "SLEEP_999US";

        case IOC_EVT_NAME_TEST_MOVE_STARTED:
            return "MOVE_STARTED";
        case IOC_EVT_NAME_TEST_MOVE_KEEPING:
            return "MOVE_KEEPING";
        case IOC_EVT_NAME_TEST_MOVE_STOPPED:
            return "MOVE_STOPPED";

        case IOC_EVT_NAME_TEST_PUSH_STARTED:
            return "PUSH_STARTED";
        case IOC_EVT_NAME_TEST_PUSH_KEEPING:
            return "PUSH_KEEPING";
        case IOC_EVT_NAME_TEST_PUSH_STOPPED:
            return "PUSH_STOPPED";

        case IOC_EVT_NAME_TEST_PULL_STARTED:
            return "PULL_STARTED";
        case IOC_EVT_NAME_TEST_PULL_KEEPING:
            return "PULL_KEEPING";
        case IOC_EVT_NAME_TEST_PULL_STOPPED:
            return "PULL_STOPPED";

        default:
            return "UNKNOWN";
    }
}

static inline const char *IOC_getEvtNameStr(IOC_EvtID_T EvtID) {
    IOC_EvtNameID_T EvtNameID = IOC_getEvtNameID(EvtID);
    IOC_EvtClassID_T EvtClassID = IOC_getEvtClassID(EvtID);

    if (EvtClassID == IOC_EVT_CLASS_TEST) {
        return IOC_getTestClassEvtNameStr(EvtNameID);
    } else {
        return "UNKNOWN";
    }
}

#define IOC_EVTID_TEST_KEEPALIVE IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_KEEPALIVE)
#define IOC_EVTID_TEST_KEEPALIVE_RELAY IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_KEEPALIVE_RELAY)

#define IOC_EVTID_TEST_HELLO_FROM_ODD_TO_EVEN \
    IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_HELLO_FROM_ODD_TO_EVEN)
#define IOC_EVTID_TEST_HELLO_FROM_EVEN_TO_ODD \
    IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_HELLO_FROM_EVEN_TO_ODD)

#define IOC_EVTID_TEST_SLEEP_9MS IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_SLEEP_9MS)
#define IOC_EVTID_TEST_SLEEP_99MS IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_SLEEP_99MS)
#define IOC_EVTID_TEST_SLEEP_999MS IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_SLEEP_999MS)

#define IOC_EVTID_TEST_SLEEP_9US IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_SLEEP_9US)
#define IOC_EVTID_TEST_SLEEP_99US IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_SLEEP_99US)
#define IOC_EVTID_TEST_SLEEP_999US IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_SLEEP_999US)

#define IOC_EVTID_TEST_MOVE_STARTED IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_MOVE_STARTED)
#define IOC_EVTID_TEST_MOVE_KEEPING IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_MOVE_KEEPING)
#define IOC_EVTID_TEST_MOVE_STOPPED IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_MOVE_STOPPED)

#define IOC_EVTID_TEST_PUSH_STARTED IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_PUSH_STARTED)
#define IOC_EVTID_TEST_PUSH_KEEPING IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_PUSH_KEEPING)
#define IOC_EVTID_TEST_PUSH_STOPPED IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_PUSH_STOPPED)

#define IOC_EVTID_TEST_PULL_STARTED IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_PULL_STARTED)
#define IOC_EVTID_TEST_PULL_KEEPING IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_PULL_KEEPING)
#define IOC_EVTID_TEST_PULL_STOPPED IOC_defineEvtID(IOC_EVT_CLASS_TEST, IOC_EVT_NAME_TEST_PULL_STOPPED)

// TODO(@W):

#ifdef __cplusplus
}
#endif
#endif  //__IOC_TYPES_EVTID_H__