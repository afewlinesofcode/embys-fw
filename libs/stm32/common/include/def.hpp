#pragma once

#define SET_BIT_V(REG, BIT) ((REG) = (REG) | (BIT))
#define CLEAR_BIT_V(REG, BIT) ((REG) = (REG) & ~(BIT))
#define INC_V(NUM) NUM = NUM + 1
#define DEC_V(NUM) NUM = NUM - 1
