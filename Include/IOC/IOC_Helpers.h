#include <IOC/IOC_Types.h>
#include <time.h>

#ifndef __IOC_HELPERS_H__
#define __IOC_HELPERS_H__

static inline struct timespec IOC_getCurrentTimeSpec() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now;
}

static inline ULONG_T IOC_diffTimeSpecInSec(const struct timespec *pFromTS, const struct timespec *pToTS) {
    if (pToTS->tv_nsec < pFromTS->tv_nsec) {
        // 如果pToTS的纳秒小于pFromTS的纳秒，需要从秒数中借位
        return (pToTS->tv_sec - pFromTS->tv_sec - 1) + ((pToTS->tv_nsec + 1000000000) - pFromTS->tv_nsec) / 1000000000;
    } else {
        // 正常情况下的计算
        return (pToTS->tv_sec - pFromTS->tv_sec) + (pToTS->tv_nsec - pFromTS->tv_nsec) / 1000000000;
    }
}

#endif  // __IOC_HELPERS_H__