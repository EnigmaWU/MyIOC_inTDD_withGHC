#include "IOC_Types.h"

#ifndef __IOC_TYPES_MSGDESC_H__
#define __IOC_TYPES_MSGDESC_H__
#ifdef __cplusplus
extern "C" {
#endif

#if 0
typedef enum {
  IOC_MSGFLAG_MAYDROP = 1 << 0,  // set this flag to allow drop this message if no enough resource.
} IOC_MsgFlags_T;
#endif

// MSG is Common Head of Evt and Cmd
typedef struct {
  // TODO: ULONG_T MagicID;  // MagicID is used to identify the message type.
  ULONG_T SeqID;
  struct timespec TimeStamp;  // when call IOC_postEVT set value from IOC_getCurrentTimeSpec

  // IOC_MsgFlags_T Flags;
} IOC_MsgDesc_T, *IOC_MsgDesc_pT;

#ifdef __cplusplus
}
#endif
#endif  // __IOC_TYPES_MSGDESC_H__