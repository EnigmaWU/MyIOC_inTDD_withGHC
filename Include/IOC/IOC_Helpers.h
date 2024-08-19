#include <IOC/IOC_Types.h>

#ifndef __IOC_HELPERS_H__
#define __IOC_HELPERS_H__

static inline struct timespec IOC_getCurrentTimeSpec() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now;
}

/**
 * IF: pFromTS <= pToTS:
 *      A) pFromTS.tv_sec == pToTS.tv_sec && pFromTS.tv_nsec <= pToTS.tv_nsec
 *      B) pFromTS.tv_sec < pToTS.tv_sec + 1 && ((pFromTS.tv_nsec  <= pToTS.tv_nsec) || (pFromTS.nsec > pToTS.tv_nsec))
 *      C) pFromTS.tv_sec < pToTS.tv_sec + 2/3/4 && ((pFromTS.tv_nsec  <= pToTS.tv_nsec) || (pFromTS.nsec > pToTS.tv_nsec))
 * ELSE: BUG
 *      because pFromTS and pToTS are both from IOC_getCurrentTimeSpec() which is monotonic.
 */
static inline ULONG_T IOC_deltaTimeSpecInSec(const struct timespec *pFromTS,
                                             const struct timespec *pToTS) {
    ULONG_T diffResult = 0;

    if (pFromTS->tv_sec <= pToTS->tv_sec) {
      ULONG_T diffSec = pToTS->tv_sec - pFromTS->tv_sec;
      diffResult      = diffSec;
    } else {
      assert(true == false);  // BUG
    }

    return diffResult;
}

/**
 * 计算两个时间点之间的差异（以毫秒为单位）。
 *
 * @param pFromTS 指向起始时间点的指针。
 * @param pToTS 指向结束时间点的指针。
 * @return 时间差（毫秒）。
 *
 * 此函数计算两个 timespec 结构体之间的时间差异，并将结果转换为毫秒。
 * 它首先检查起始时间是否早于或等于结束时间，以确保时间差异的计算是有意义的。
 * 如果起始时间晚于结束时间，函数将触发断言，表明这是一个逻辑错误，因为通常预期时间流逝是单向的。
 *
 * 注意：此函数假设两个时间点在相同或相邻的时间段内，不考虑更复杂的场景，如时区变化或闰秒。
 */
static inline ULONG_T IOC_deltaTimeSpecInMS(const struct timespec *pFromTS,
                                            const struct timespec *pToTS) {
    ULONG_T diffResult = 0;

    // 确保起始时间不晚于结束时间
    if (pFromTS->tv_sec <= pToTS->tv_sec) {
      // 计算秒数差异并转换为毫秒
      ULONG_T diffSec = pToTS->tv_sec - pFromTS->tv_sec;
      diffResult      = diffSec * 1000;  // 秒转毫秒

      // 将纳秒差异转换为毫秒并加到总差异中
      diffResult += pToTS->tv_nsec / 1000000;  // 纳秒转毫秒
      diffResult -= pFromTS->tv_nsec / 1000000;
    } else {
      // 如果起始时间晚于结束时间，触发断言指示错误
      assert(true == false);  // BUG: 此处使用 assert(true == false) 来明确标记逻辑错误。
    }

    return diffResult;
}

/**
 * 计算两个时间点之间的差异（以微秒为单位）。
 *
 * @param pFromTS 指向起始时间点的指针。
 * @param pToTS 指向结束时间点的指针。
 * @return 时间差（微秒）。
 *
 * 此函数计算两个 timespec 结构体之间的时间差异，并将结果转换为微秒。
 * 它首先检查起始时间是否早于或等于结束时间，以确保时间差异的计算是有意义的。
 * 如果起始时间晚于结束时间，函数将触发断言，表明这是一个逻辑错误，因为通常预期时间流逝是单向的。
 *
 * 注意：此函数假设两个时间点在相同或相邻的时间段内，不考虑更复杂的场景，如时区变化或闰秒。
 */
static inline ULONG_T IOC_deltaTimeSpecInUS(const struct timespec *pFromTS,
                                            const struct timespec *pToTS) {
    ULONG_T diffResult = 0;

    // 确保起始时间不晚于结束时间
    if (pFromTS->tv_sec <= pToTS->tv_sec) {
      // 计算秒数差异并转换为微秒
      ULONG_T diffSec = pToTS->tv_sec - pFromTS->tv_sec;
      diffResult      = diffSec * 1000000;  // 秒转微秒

      // 将纳秒差异转换为微秒并加到总差异中
      diffResult += pToTS->tv_nsec / 1000;  // 纳秒转微秒
      diffResult -= pFromTS->tv_nsec / 1000;
    } else {
      // 如果起始时间晚于结束时间，触发断言指示错误
      assert(true == false);  // BUG: 此处使用 assert(true == false) 来明确标记逻辑错误。
    }

    return diffResult;
}

/**
 * 计算两个时间点之间的差异（以纳秒为单位）。
 *
 * @param pFromTS 指向起始时间点的指针。
 * @param pToTS 指向结束时间点的指针。
 * @return 时间差（纳秒）。
 *
 * 此函数计算两个 timespec 结构体之间的时间差异，并将结果转换为纳秒。
 * 它首先检查起始时间是否早于或等于结束时间，以确保时间差异的计算是有意义的。
 * 如果起始时间晚于结束时间，函数将触发断言，表明这是一个逻辑错误，因为通常预期时间流逝是单向的。
 *
 * 注意：此函数假设两个时间点在相同或相邻的时间段内，不考虑更复杂的场景，如时区变化或闰秒。
 */
static inline ULONG_T IOC_deltaTimeSpecInNS(const struct timespec *pFromTS,
                                            const struct timespec *pToTS) {
    ULONG_T diffResult = 0;

    // 确保起始时间不晚于结束时间
    if (pFromTS->tv_sec <= pToTS->tv_sec) {
      // 计算秒数差异并转换为纳秒
      ULONG_T diffSec = pToTS->tv_sec - pFromTS->tv_sec;
      diffResult      = diffSec * 1000000000;  // 秒转纳秒

      // 将纳秒差异加到总差异中
      diffResult += pToTS->tv_nsec;
      diffResult -= pFromTS->tv_nsec;
    } else {
      // 如果起始时间晚于结束时间，触发断言指示错误
      assert(true == false);  // BUG: 此处使用 assert(true == false) 来明确标记逻辑错误。
    }

    return diffResult;
}

static inline struct timeval IOC_getCurrentTimeval() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now;
}

/**
 * 计算两个时间点之间的差异（以秒为单位）。
 *
 * @param pFromTV 指向起始时间点的指针。
 * @param pToTV 指向结束时间点的指针。
 * @return 时间差（秒）。
 *
 * 此函数计算两个 timeval 结构体之间的时间差异，并将结果转换为秒。
 * 它首先检查起始时间是否早于或等于结束时间，以确保时间差异的计算是有意义的。
 * 如果起始时间晚于结束时间，函数将触发断言，表明这是一个逻辑错误，因为通常预期时间流逝是单向的。
 *
 * 注意：此函数假设两个时间点在相同或相邻的时间段内，不考虑更复杂的场景，如时区变化或闰秒。
 */
static inline ULONG_T IOC_deltaTimevalInSec(const struct timeval *pFromTV,
                                            const struct timeval *pToTV) {
    ULONG_T diffResult = 0;

    // 确保起始时间不晚于结束时间
    if (pFromTV->tv_sec <= pToTV->tv_sec) {
      // 计算秒数差异
      diffResult = pToTV->tv_sec - pFromTV->tv_sec;
    } else {
      // 如果起始时间晚于结束时间，触发断言指示错误
      assert(true == false);  // BUG: 此处使用 assert(true == false) 来明确标记逻辑错误。
    }

    return diffResult;
}

static inline ULONG_T IOC_deltaTimevalInMS(const struct timeval *pFromTV, const struct timeval *pToTV) {
    ULONG_T diffResult = 0;

    // 确保起始时间不晚于结束时间
    if (pFromTV->tv_sec <= pToTV->tv_sec) {
      // 计算秒数差异并转换为毫秒
      ULONG_T diffSec = pToTV->tv_sec - pFromTV->tv_sec;
      diffResult      = diffSec * 1000;  // 秒转毫秒

      // 将微秒差异转换为毫秒并加到总差异中
      diffResult += pToTV->tv_usec / 1000;  // 微秒转毫秒
      diffResult -= pFromTV->tv_usec / 1000;
    } else {
      // 如果起始时间晚于结束时间，触发断言指示错误
      assert(true == false);  // BUG: 此处使用 assert(true == false) 来明确标记逻辑错误。
    }

    return diffResult;
}

static inline ULONG_T IOC_deltaTimevalInUS(const struct timeval *pFromTV,
                                           const struct timeval *pToTV) {
    ULONG_T diffResult = 0;

    // 确保起始时间不晚于结束时间
    if (pFromTV->tv_sec <= pToTV->tv_sec) {
      // 计算秒数差异并转换为微秒
      ULONG_T diffSec = pToTV->tv_sec - pFromTV->tv_sec;
      diffResult      = diffSec * 1000000;  // 秒转微秒

      // 将微秒差异加到总差异中
      diffResult += pToTV->tv_usec;
      diffResult -= pFromTV->tv_usec;
    } else {
      // 如果起始时间晚于结束时间，触发断言指示错误
      assert(true == false);  // BUG: 此处使用 assert(true == false) 来明确标记逻辑错误。
    }

    return diffResult;
}

#define IOC_calcArrayElmtCnt(array) (sizeof(array) / sizeof(array[0]))

#endif  // __IOC_HELPERS_H__