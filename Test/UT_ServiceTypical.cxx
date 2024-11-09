
///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE================================================
/**
 * @brief
 *  Verify typical/classic usage/example of IOC's Service APIs.
 *
 *-------------------------------------------------------------------------------------------------
 *
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
 *  US-1: <a>AS a EvtProducer,
 *      I WANT to online a service,
 *      SO THAT EvtConsumer can connect to my service,
 *          AND subscribe events what I published over connected pair Links.
 *      OR <b>AS a EvtConsumer,
 *      I ALSO WANT to online a service,
 *      SO THAT EvtProducer can connect to my service,
 *          AND publish events over connected pair Links.
 *      SAME FOR CmdInitiator and CmdExecutor:
 *          <c>CmdInitiator online a service.
 *          <d>CmdExecutor online a service.
 *      SAME FOR DatSender and DatReceiver:
 *          <e>DatSender online a service.
 *          <f>DatReceiver online a service.
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Acceptance Criteria】
 *
 * [@US-1.a]
 *      AC-1: GIVEN a service is onlined by EvtProducer,
 *          WHEN EvtConsumer connects to the service and establish a pair Link,
 *          THEN EvtConsumer can subscribe events,
 *              AND EvtProducer can post events, EvtConsumer can process them.
 *      AC-2: GIVEN a service is onlined by EvtConsumer,
 *          WHEN MANY EvtProducers connects to the service and EACH establish a pair Link,
 *          THEN EACH EvtConsumer can subscribe different events over each's pair Link,
 *              AND EvtProducer can post events to each EvtConsumer,
 *              AND EACH EvtConsumer will process what it subscribed events only.
 * [@US-1.b]
 *      AC-1: GIVEN a service is onlined by EvtConsumer,
 *          WHEN EvtProducer connects to the service and establish a pair Link,
 *          THEN EvtConsumer can subscribe events over the pair Link,
 *              AND EvtProducer can post events, EvtConsumer can process them.
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

//===TEMPLATE OF UT CASE===
/**
 * @[Name]: ${verifyBehivorX_byDoABC}
 * @[Purpose]: ${according to what in SPEC, and why to verify in this way}
 * @[Steps]: ${how to do}
 *   1) do ..., with ..., as SETUP
 *   2) do ..., with ..., as BEHAVIOR
 *   3) do ..., with ..., as VERIFY
 *   4) do ..., with ..., as CLEANUP
 * @[Expect]: ${how to verify}
 * @[Notes]:
 */
TEST(UT_NameOfCategory, CaseNN_verifyBehivorX_byDoABC) {
    //===SETUP===
    // 1. ...

    //===BEHAVIOR===
    //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)

    //===VERIFY===
    //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)

    //===CLEANUP===
}