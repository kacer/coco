/*
 * Colearning in Coevolutionary Algorithms
 * Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>
 *
 * Master's Thesis
 * 2014/2015
 *
 * Supervisor: Ing. Michaela Šikulová <isikulova@fit.vutbr.cz>
 *
 * Faculty of Information Technologies
 * Brno University of Technology
 * http://www.fit.vutbr.cz/
 *
 * Started on 28/07/2014.
 *      _       _
 *   __(.)=   =(.)__
 *   \___)     (___/
 */


#pragma once


#define CGP_INPUTS 9
#define CGP_OUTPUTS 1
#define CGP_FUNC_INPUTS 2


typedef unsigned char cgp_value_t;


/* remember, number of inputs is declared in cgp_config.h */
#define CGP_FUNC_COUNT 16
typedef enum
{
    c255 = 0,   // 255
    identity,   // a
    inversion,  // 255 - a
    b_or,       // a or b
    b_not1or2,  // (not a) or b
    b_and,      // a and b
    b_nand,     // not (a and b)
    b_xor,      // a xor b
    rshift1,    // a >> 1
    rshift2,    // a >> 2
    swap,       // a <-> b
    add,        // a + b
    add_sat,    // a +S b
    avg,        // (a + b) >> 1
    max,        // max(a, b)
    min,        // min(a, b)
} cgp_func_t;


static const char *CGP_FUNC_NAMES[] = {
    " FF ",     // 255
    "  a ",     // a
    "FF-a",     // 255 - a
    " or ",     // a or b
    "~1|2",     // (not a) or b
    " and",     // a and b
    "nand",     // not (a and b)
    " xor",     // a xor b
    "a>>1",     // a >> 1
    "a>>2",     // a >> 2
    "swap",     // a <-> b
    " +  ",     // a + b
    " +S ",     // a +S b
    " avg",     // (a + b) >> 1
    " max",     // max(a, b)
    " min",     // min(a, b)
};


static const int CGP_FUNC_ARITY[] = {
    0,     // 255
    1,     // a
    1,     // 255 - a
    2,     // a or b
    2,     // (not a) or b
    2,     // a and b
    2,     // not (a and b)
    2,     // a xor b
    1,     // a >> 1
    1,     // a >> 2
    2,     // a <-> b
    2,     // a + b
    2,     // a +S b
    2,     // (a + b) >> 1
    2,     // max(a, b)
    2,     // min(a, b)
};


static const char *CGP_FUNC_CODE[] = {
    "255",              // 255
    "%s",               // a
    "255 - %s",         // 255 - a
    "%s | %s",          // a or b
    "(~%s) | %s",       // (not a) or b
    "%s & %s",          // a and b
    "~(%s & %s)",       // not (a and b)
    "%s ^ %s",          // a xor b
    "%s >> 1",          // a >> 1
    "%s >> 2",          // a >> 2
    "SWAP(%s, %s)",     // a <-> b
    "%s + %s",          // a + b
    "ADD_SAT(%s, %s)",  // a +S b
    "(%s + %s) >> 1",   // (a + b) >> 1
    "MAX(%s, %s)",      // max(a, b)
    "MIN(%s, %s)",      // min(a, b)
};


static const char *CGP_CODE_PROLOG =
    "typedef unsigned char cgp_value_t;\n\n"
    "#define SWAP(A, B) (((A & 0x0F) << 4) | ((B & 0x0F)))\n"
    "#define ADD_SAT(A, B) ((A > 0xFF - B) ? 0xFF : A + B)\n"
    "#define MAX(A, B) ((A > B) ? A : B)\n"
    "#define MIN(A, B) ((A < B) ? A : B)\n\n"
;
