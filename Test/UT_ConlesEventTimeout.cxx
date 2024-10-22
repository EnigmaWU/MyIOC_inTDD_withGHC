///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE================================================
/**
 * @brief
 *  Use this UTF to verify the behavior of ConlesEventTimeout.
 *    According to:
 *      README_UserCase::UC-x(FIXME: missing?)
 *      README_Specification::V2-z.8
 *
 *-------------------------------------------------------------------------------------------------
 * IF EvtProducer call IOC_postEVT_inConlesMode with OPTION args defined with:
 *    IOC_Option_defineASyncTimeout or IOC_Option_defineSyncTimeout
 * THEN EvtProducer MAY get result value:
 *    IOC_RESULT_FULL_QUEUING_EVTDESC when ASync,
 *      which means too long time to wait for Link's queue has space.
 *    IOC_RESULT_NOT_EMPTY_EVTDESC_QUEUE when Sync,
 *      which means too long time to wait for Link's queue become empty.
 *
 *-------------------------------------------------------------------------------------------------
 * @attention Timeout's behavior is ALMOST SAME as NonBlockMode's behavior,
 *  which means: when meet WAIT condition, TimeoutMode will wait for a while(>=1us),
 *              while NonBlockMode will return immediately(0us).
 */
//======END OF OVERVIEW OF THIS UNIT TESTING FILE==================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING DESIGN===============================================================
/**
 * At least one User Story(a.k.a US),
 *    and at least one Acceptance Criteria(a.k.a AC) for each US,
 *      and at least one Test Case(a.k.a TC) for each AC.
 *
 * US takes VALUE from USR perspective.
 * AC clear CONDITIONS may relate to the USR.
 * TC details each condition's STEPS to verify.
 */
//-------------------------------------------------------------------------------------------------
/**
 * @brief 【User Story】
 *
 *  US-1: AS a EvtProducer,
 *          I WANT to post an event with TIMEOUT Option, whenver in ASync or Sync Mode,
 *            SO THAT I will not be blocked forever or wait too long time to do my next job if IOC is BUSY.
 *  US-2  and I WANT the timeout value range from 0us to SOME_MAX_ENOUGH_TIMEOUT_VALUE(such as 1-day)
 *            SO THAT I can easyly control the timeout value such as 0us is NonBlockMode, and 1-day is almost forever.
 *  US-3  and I WANT to get the OUT_OF_TIMEOUT_RANGE error when the timeout value is out of range,
 *            SO THAT I can know the timeout value is invalid to cacth my programming error.
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Acceptance Criteria】
 *
 * [@US-1]
 * AC-1: GIVEN EvtProducer posts an event with TIMEOUT Option in ASync Mode,
 *        WHEN the queue is FULL,
 *          THEN the result value MUST be IOC_RESULT_FULL_QUEUING_EVTDESC.
 *            AND the wait cost time SHOULD be equal to the TIMEOUT value.
 *        WHEN the queue is NOT FULL,
 *          THEN the result value MUST be IOC_RESULT_SUCCESS.
 *            AND the wait cost time SHOULD be LT the TIMEOUT value or almost 0.
 * AC-2: GIVEN EvtProducer posts an event with TIMEOUT Option in Sync Mode,
 *        WHEN the queue is NOT EMPTY,
 *          THEN the result value MUST be IOC_RESULT_NOT_EMPTY_EVTDESC_QUEUE.
 *            AND the wait cost time SHOULD be equal to the TIMEOUT value.
 *        WHEN the queue is EMPTY,
 *          THEN the result value MUST be IOC_RESULT_SUCCESS.
 *            AND the wait cost time SHOULD be LT the TIMEOUT value or almost 0.
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Test Cases】
 *
 * [@AC-1]
 * TC-1:
 *  @[Name]: verifyASyncDifferentTimeoutValue_byQueueFromEmptyToFullToEmpty_inAtLeastTenTimes
 *  @[Purpose]: According to AC-1, we need to VERIFY the behavior of IOC_postEVT_inConlesMode when the queue is full and
 * then empty, with different timeout values.
 *
 */

//======END OF UNIT TESTING DESIGN=================================================================

//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

/**
 * @[Name]: verifyASyncDifferentTimeoutValue_byQueueFromEmptyToFullToEmpty_inAtLeastTenTimes
 * @[Steps]:
 *   1) Get DepthEvtDescQueue by IOC_getCapabilty as SETUP
 *   2) EvtConsumer call IOC_subEVT_inConlesMode with CbProcEvt_F of:
 *      a) block on the first event, until wake up by EvtProducer.
 *      b) don't block following events.
 *        as SETUP.
 *   3) EvtProducer call IOC_postEVT_inConlesMode with different timeout values by IOC_Option_defineASyncTimeout
 *     a) random select (0us,1us,10us,100us,1ms,10ms,100ms,1s) as timeout value as BEHAVIOR
 *     b) check from 1 to (DepthEvtDescQueue-1)'s result is IOC_RESULT_SUCCESS as VERIFY
 *     c) check the DepthEvtDescQueue's result is IOC_RESULT_FULL_QUEUING_EVTDESC as VERIFY
 *        repeat all above timeout value as BEHAVIOR
 *     d) wake up EvtConsumer
 *     e) check from (DepthEvtDescQueue-1) to 1's result is IOC_RESULT_SUCCESS as VERIFY
 *   4) EvtConsumer call IOC_unsubEVT_inConlesMode as CLEANUP
 *   5) Repeat 2) to 4) at _MAX_REPEAT_TIMES
 * @[Expect]: ${how to verify}
 * @[Notes]:
 */
TEST(UT_ConlesEventTimeout, verifyASyncDifferentTimeoutValue_byQueueFromEmptyToFullToEmpty_inAtLeastTenTimes) {}

//======END OF UNIT TESTING IMPLEMENTATION=========================================================
///////////////////////////////////////////////////////////////////////////////////////////////////
