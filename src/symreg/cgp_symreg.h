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


#define CGP_INPUTS 1
#define CGP_OUTPUTS 1


typedef double cgp_value_t;


/* node functions list */


/* remember, number of inputs is declared in cgp_config.h */
#define CGP_FUNC_COUNT 9
typedef enum
{
    f_add,        // a + b
    f_sub,        // a - b
    f_mul,        // a * b
    f_div,        // a / b
    f_sin,        // sin(a)
    f_cos,        // cos(a)
    f_exp,        // exp(a)
    f_log,        // log(a)
    f_abs,        // fabs(a)
} cgp_func_t;


static inline const char* cgp_func_name(cgp_func_t f) {
    const char *func_names[] = {
        " +  ",     // a + b
        " -  ",     // a - b
        " *  ",     // a * b
        " /  ",     // a / b
        " sin",     // sin(a)
        " cos",     // cos(a)
        " exp",     // exp(a)
        " log",     // log(a)
        " abs",     // fabs(a)
    };
    return func_names[f];
}

