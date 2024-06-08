// This is a common header file for all UTs of IOC from API caller's perspective,
//   which means the UTs focus on IOC's behavior from user viewpoint, but its internal implementation.
// We design UT from following aspects/category:
//   FreelyDrafts, Typical, Demo, Boundary, State, Performance, Concurrency, Robust, Fault, Misuse, Others.
//      align to IMPROVE VALUE、AVOID LOST、BALANCE SKILL vs COST.

//[FreelyDrafts]: Any natural or intuitive idea, first write down here freely and causally as quickly as possible,
//  then refine it, rethink it, refactor it to a category from one a main aspect or category.
//[Typical]: a typical case, such as IOC's basic typical usage or call flow examples.
//[Capabilty]: a capability case, such as max EvtConsumer may call subEVT success in ConlesMode.
//[Demo]: a demo case, used to demo a complete feature of a product model or series.
//[Boundary]: a boundary case, used to verify API's argument boundary or use scenario boundary.
//[State]: a state case, used to verify FSM of IOC's Objects, such as FSM_ofConlesEVT.
//[Performance]: such as how many times of API can be called in 1 second, or each API's time consumption.
//[Concurrency]: such as many threads call IOC's API at the same time and always related to:
//       ASync/Sync, MayBlock/NonBlock/Timeout, Burst/RaceCondition/Priority/Parallel/Serial/DeadLock/Starvation/...
//[Robust]: such as repeatly reach IOC's max capacity, let its buffer full then empty.
//[Fault]: such as one process crash or kill by OS, then it auto restarted.
//[Misuse]: such as call API in wrong order, or call API with wrong arguments.
//[Compatibility]: such as call API in different version of IOC, or call API in different OS.
//[Others]: any other cases, not have clear category, but still has value to verify.
//===>RefMore: TEMPLATE OF UT CASE in UT_FreelyDrafts.cxx

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <thread>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define GTEST_HAS_PTHREAD 1
#include <gtest/gtest.h>

#include <IOC/IOC.h>

static inline uint32_t IOC_deltaTimevalInMS(const struct timeval *pFromTV, const struct timeval *pToTV) {
  return (pToTV->tv_sec - pFromTV->tv_sec) * 1000 + pToTV->tv_usec / 1000 - pFromTV->tv_usec / 1000;
}
