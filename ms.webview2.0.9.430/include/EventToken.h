// This header is missing in mingw

#include "rpc.h"
#include "rpcndr.h"

#ifndef __eventtoken_h__
#define __eventtoken_h__

typedef struct EventRegistrationToken {
  __int64 value;
} EventRegistrationToken;

#endif
