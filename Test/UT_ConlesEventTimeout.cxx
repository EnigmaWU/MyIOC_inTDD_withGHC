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
 *  US-1: As a EvtProducer, I WANT to post an event with TIMEOUT Option, whenver in ASync or Sync Mode,
 *        SO THAT I will not be blocked forever or wait too long time to do my next job.
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Acceptance Criteria】
 *
 * [@US-1]
 * AC-1: GIVEN EvtProducer posts an event with TIMEOUT Option in ASync Mode,
 *        WHEN the queue is FULL,
 *        THEN the result value should be IOC_RESULT_FULL_QUEUING_EVTDESC.
 * AC-2: GIVEN EvtProducer posts an event with TIMEOUT Option in Sync Mode,
 *        WHEN the queue is NOT EMPTY,
 *        THEN the result value should be IOC_RESULT_NOT_EMPTY_EVTDESC_QUEUE.
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Test Cases】
 *
 * [@AC-n]
 * TC-1: verifyBehivorX_byDoABC
 *
 */

//======END OF UNIT TESTING DESIGN=================================================================
