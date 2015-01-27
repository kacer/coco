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


static inline const char* cgp_func_name(cgp_func_t f) {
    const char *func_names[] = {
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
    return func_names[f];
}
