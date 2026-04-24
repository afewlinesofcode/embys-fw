#pragma once

#define SET_BIT_V(REG, BIT) ((REG) = (REG) | (BIT))
#define CLEAR_BIT_V(REG, BIT) ((REG) = (REG) & ~(BIT))
#define INC_V(NUM) NUM = NUM + 1
#define DEC_V(NUM) NUM = NUM - 1
#define TRY(EXPR)                                                              \
  {                                                                            \
    int _try_result = (EXPR);                                                  \
    if (_try_result < 0)                                                       \
      return _try_result;                                                      \
  }
