---
applyTo: '**/UT_*.cxx'
---

# Guide for Unit Testing 
- I'm using TDD development methodology which means:
  - always **write unit tests** first before implementing the actual code.
  - when the test fails, it indicates a **missing feature** or **bug** in the code.
    - so you should **recheck the test** to ensure it is correct,
    - then **implement the feature** or **fix the bug** in the code.

- Always remember and follow **UT design** considerations, **US/AC/TC** structure, and **UT template** in:
    - `Test/UT_FreelyDrafts.cxx`