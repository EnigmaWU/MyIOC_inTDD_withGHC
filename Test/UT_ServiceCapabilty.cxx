
///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE================================================
/**
 * @brief
 *  Use this unit testing file to verify the capability of IOC's Service, such as:
 *      - MAX NUMBER of services may be online at the same time.
 *      - MAX NUMBER of clients may be connected to a service at the same time.
 *
 *-------------------------------------------------------------------------------------------------
 * @Usage
 *  IOC_getCapability(CapDesc.CapID=IOC_CAPID_CONET_MODE) to get the capability of ConetMode.
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
 *  US-1: AS A USR of IOC's service, such as EvtProducer,
 *          I WANT to know how many MAX services may be onlined at the same time,
 *          SO THAT I can design my system to meet the requirement,
 *            OR I can catch the out-of-capability exception and handle it properly.
 *  US-2: AS A USR of IOC's client, such as EvtConsumer,
 *          I WANT to know how many MAX clients may be connected to a service at the same time,
 *          SO THAT I can design my system to meet the requirement,
 *
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Acceptance Criteria】
 *
 * [@US-1]
 *  AC-1: GIVEN USR know the MAX NUMBER of services may be onlined at the same time BY IOC_getCapability,
 *    WHEN USR online services less than the MAX NUMBER,
 *      THEN USR will get IOC_RESULT_SUCCESS result.
 *    WHEN USR online services more than the MAX NUMBER,
 *      THEN USR will get IOC_RESULT_TOO_MANY_SERVICES result.
 *    WHEN USR offline a service and online a new service,
 *      THEN USR will get IOC_RESULT_SUCCESS result.
 *    AND upper steps is REPEATABLE.
 *
 */

//-------------------------------------------------------------------------------------------------
/**
 * @brief 【Test Cases】
 *
 * [@AC-1,US-1]
 *  TC-1:
 *      @[Name]: verifyOnlineMoreThanCapabilityServices_shouldGetTooManyServices_andRepeatable
 *      @[Purpose]: verify US-1,AC-1
 *      @[Brief]: Repeat NxTimes of online from 0 to MAX_NUMBER+1 services, then offline one and retry online again.
 *
 */

//======END OF UNIT TESTING DESIGN=================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

/**
 * @[Name]: <US-1,AC-1,TC-1>verifyOnlineMoreThanCapabilityServices_shouldGetTooManyServices_andRepeatable
 * @[Steps]:
 *   1) Get the MAX_SRV_NUM by IOC_getCapability(CAPID=CONET_MODE_EVENT) as SETUP.
 *   2) TODO...
 * @[Expect]:
 * @[Notes]:
 */
TEST(UT_ServiceCapability, verifyOnlineMoreThanCapabilityServices_shouldGetTooManyServices_andRepeatable) {
    //===SETUP===
    // 1. ...

    //===BEHAVIOR===
    //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)

    //===VERIFY===
    //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)

    //===CLEANUP===
}

//======END OF UNIT TESTING IMPLEMENTATION=========================================================
///////////////////////////////////////////////////////////////////////////////////////////////////
