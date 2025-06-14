///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF OVERVIEW OF THIS UNIT TESTING FILE===============================================
/**
 * @brief
 *  ${What VERIFICATIONS to do in this UTF for which PART of current module.}
 *
 *-------------------------------------------------------------------------------------------------
 *++Some context or background related to this UTF for better understanding and GenAisTNT.
 *  ${Relationship with module or other submodules}
 *  ${What's the key point or concept of this submodule?}
 *  ${What's the functionality/interface/data structure/operation/... of this submodule?}
 */
//======>END OF OVERVIEW OF THIS UNIT TESTING FILE=================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF UNIT TESTING DESIGN==============================================================
/**
 * At least one User Story(a.k.a US),
 *    and at least one Acceptance Criteria(a.k.a AC) for each US,
 *      and at least one Test Case(a.k.a TC) for each AC.
 *
 * US takes VALUE from USR perspective.
 * AC clear CONDITIONS may relate to the USR.
 * TC details each condition's STEPS to verify.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF USER STORY=======================================================================
/**************************************************************************************************
 * @brief 【User Story】
 *
 *  US-1: AS: ...,
 *    I WANT: ...,
 *   SO THAT: ...
 *
 *  US-2: AS: ...,
 *    I WANT: ...,
 *   SO THAT: ...
 *
 *  US-n: AS: ...,
 *    I WANT: ...,
 *   SO THAT: ...
 *
 *************************************************************************************************/
//======>END OF USER STORY=========================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//=======>BEGIN OF ACCEPTANCE CRITERIA=============================================================
/**************************************************************************************************
 * @brief 【Acceptance Criteria】
 *
 * [@US-1]
 *  AC-1: GIVEN: ...,
 *         WHEN: ...,
 *         THEN: ...
 *
 *  AC-2: GIVEN: ...,
 *         WHEN: ...,
 *         THEN: ...
 *
 *  AC-n: GIVEN: ...,
 *         WHEN: ...,
 *         THEN: ...
 *---------------------------------------------------------------------------------------------------
 *  [@US-2]
 *  AC-1: GIVEN: ...,
 *         WHEN: ...,
 *         THEN: ...
 *
 *  AC-2: GIVEN: ...,
 *         WHEN: ...,
 *         THEN: ...
 *
 *  AC-n: GIVEN: ...,
 *         WHEN: ...,
 *         THEN: ...
 */
//=======>END OF ACCEPTANCE CRITERIA================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======>BEGIN OF TEST CASES=======================================================================
/**************************************************************************************************
 * @brief 【Test Cases】
 *
 * [@AC-1,US-1]
 *  TC-1:
 *      @[Name]: verifyBehaviorX_byDoA_expectSomething
 *      @[Purpose]: ${what purpose or why to verify in this way}
 *      @[Brief]: ${what to do in this case}
 *  TC-2:
 *      @[Name]: verifyBehaviorY_byDoB_expectSomething
 *      @[Purpose]: ${what purpose or why to verify in this way}
 *      @[Brief]: ${what to do in this case}
 *  TC-n:
 *      @[Name]: verifyBehaviorZ_byDoC_expectSomething
 *      @[Purpose]: ${what purpose or why to verify in this way}
 *      @[Brief]: ${what to do in this case}
 *---------------------------------------------------------------------------------------------------
 * [@AC-2,US-1]
 *  TC-1:
 *      @[Name]: verifyBehaviorX_byDoA_expectSomething
 *      @[Purpose]: ${what purpose or why to verify in this way}
 *      @[Brief]: ${what to do in this case}
 *  TC-2:
 *      @[Name]: verifyBehaviorY_byDoB_expectSomething
 *      @[Purpose]: ${what purpose or why to verify in this way}
 *      @[Brief]: ${what to do in this case}
 *  TC-n:
 *      @[Name]: verifyBehaviorZ_byDoC_expectSomething
 *      @[Purpose]: ${what purpose or why to verify in this way}
 *      @[Brief]: ${what to do in this case}
 *
 *---------------------------------------------------------------------------------------------------
 * More than one US, AC, TC can be added here.
 *
 *************************************************************************************************/
//======>END OF TEST CASES=========================================================================
//======>END OF UNIT TESTING DESIGN================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
//======BEGIN OF UNIT TESTING IMPLEMENTATION=======================================================
#include "_UT_IOC_Common.h"

//===TEMPLATE OF UT CASE===
/**
 * @[Name]: ${verifyBehaviorX_byDoA_expectSomething}
 * @[Steps]: ${how to do}
 *   1) do ..., with ..., as SETUP
 *   2) do ..., with ..., as BEHAVIOR
 *   3) do ..., with ..., as VERIFY
 *   4) do ..., with ..., as CLEANUP
 * @[Expect]: ${how to verify}
 * @[Notes]:
 */
TEST(UT_NameOfCategory, verifyBehaviorX_byDoA_expectSomething) {
    //===SETUP===
    // 1. ...

    //===BEHAVIOR===
    //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)
    printf("BEHAVIOR: verifyBehaviorX_byDoA_expectSomething\n");

    //===VERIFY===
    //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)

    //===CLEANUP===
}

TEST(UT_NameOfCategory, verifyBehaviorY_byDoB_expectSomething) {
    //===SETUP===
    // 1. ...

    //===BEHAVIOR===
    //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)
    printf("BEHAVIOR: verifyBehaviorY_byDoB_expectSomething\n");

    //===VERIFY===
    //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)

    //===CLEANUP===
}

TEST(UT_NameOfCategory, verifyBehaviorZ_byDoC_expectSomething) {
    //===SETUP===
    // 1. ...

    //===BEHAVIOR===
    //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)
    printf("BEHAVIOR: verifyBehaviorZ_byDoC_expectSomething\n");

    //===VERIFY===
    //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)

    //===CLEANUP===
}

//---------------------------------------------------------------------------------------------------------------------
class UT_NameofCategoryFixture : public ::testing::Test {
   protected:
    static void SetUpTestSuite() {
        // 1. ...
        printf("UT_NameofCategoryFixture->SETUP: SetUpTestSuite\n");
    }
    static void TearDownTestSuite() {
        // 1. ...
        printf("UT_NameofCategoryFixture->CLEANUP: TearDownTestSuite\n");
    }

    void SetUp() override {
        // 1. ...
        printf("UT_NameofCategoryFixture->SETUP: SetUp\n");
    }

    void TearDown() override {
        // 1. ...
        printf("UT_NameofCategoryFixture->CLEANUP: TearDown\n");
    }
};

TEST_F(UT_NameofCategoryFixture, verifyBehaviorX_byDoA_expectSomething) {
    //===SETUP===

    //===BEHAVIOR===
    //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)
    printf("NameofCategoryFixture->BEHAVIOR: verifyBehaviorX_byDoA_expectSomething\n");

    //===VERIFY===
    //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)

    //===CLEANUP===
}

TEST_F(UT_NameofCategoryFixture, verifyBehaviorY_byDoB_expectSomething) {
    //===SETUP===

    //===BEHAVIOR===
    //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)
    printf("NameofCategoryFixture->BEHAVIOR: verifyBehaviorY_byDoB_expectSomething\n");

    //===VERIFY===
    //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)

    //===CLEANUP===
}

TEST_F(UT_NameofCategoryFixture, verifyBehaviorZ_byDoC_expectSomething) {
    //===SETUP===

    //===BEHAVIOR===
    //@VerifyPoint xN(each case MAY have many 'ASSERT_XYZ' check points)
    printf("NameofCategoryFixture->BEHAVIOR: verifyBehaviorZ_byDoC_expectSomething\n");

    //===VERIFY===
    //@KeyVerifyPoint<=3(each case SHOULD has less than 3 key 'ASSERT_XYZ' verify points)

    //===CLEANUP===
}

//======END OF UNIT TESTING IMPLEMENTATION=========================================================
///////////////////////////////////////////////////////////////////////////////////////////////////

// TODO(@W): Freely start a new UT just from here, if you have a idea in mind,
//  and then refine it later, JUST keep smooth mind and low resistance.
