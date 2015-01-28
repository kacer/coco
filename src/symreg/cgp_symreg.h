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
#define CGP_FUNC_INPUTS 2


typedef double cgp_value_t;
#define CGP_VALUE_FORMAT "%.10g"
#define CGP_ASCIIART_CONSTANT_FORMAT "%4f"

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


static const char * const CGP_FUNC_NAMES[] = {
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



static const int CGP_FUNC_ARITY[] = {
    2,     // a + b
    2,     // a - b
    2,     // a * b
    2,     // a / b
    1,     // sin(a)
    1,     // cos(a)
    1,     // exp(a)
    1,     // log(a)
    1,     // fabs(a)
};


static const char * const CGP_FUNC_CODE[] = {
    "%s + %s",     // a + b
    "%s - %s",     // a - b
    "%s * %s",     // a * b
    "%s / %s",     // a / b
    "sin(%s)",     // sin(a)
    "cos(%s)",     // cos(a)
    "exp(%s)",     // exp(a)
    "log(%s)",     // log(a)
    "fabs(%s)",    // fabs(a)
};


static const char * const CGP_CODE_PROLOG =
    "#include <math.h>\n\n"
    "typedef double cgp_value_t;\n\n"
;
