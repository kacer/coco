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


#include <stdlib.h>

#include "../cgp/cgp_core.h"


#define PI 3.1415926535897932384626433832795


/**
 * Calculates CGP node output value.
 *
 * Defined in cgp_symreg or cgp_ifilter
 *
 * @param  n CGP node
 * @param  A Input value A
 * @param  B Input value B
 * @param  Y Output value
 * @return Whether CGP function changed and evaluation should be restarted
 */
bool cgp_get_node_output(cgp_node_t *n, cgp_value_t A,
    cgp_value_t B, cgp_value_t *Y)
{
    switch (n->function) {
        case f_add:      *Y = A + B;       break;
        case f_sub:      *Y = A - B;       break;
        case f_mul:      *Y = A * B;       break;
        case f_div:      *Y = A / B;       break;
        case f_sin:      *Y = sin(A);      break;
        case f_cos:      *Y = cos(A);      break;
        case f_exp:      *Y = exp(A);      break;
        case f_log:      *Y = log(A);      break;
        case f_abs:      *Y = fabs(A);     break;
        default:    abort();
    }


    /* If we did something strange, like dividing be zero or logarithm of
       negative number, the node is set to generate constant and the evaluation
       must be restarted.
    */
    if (isinf(*Y)) {
        *Y = 1.5;
        n->is_constant = true;
        n->constant_value = 1.5;
        return true;

    } else if (isnan(*Y)) {
        *Y = PI;
        n->is_constant = true;
        n->constant_value = PI;
        return true;

    } else {
        return false;
    }
}
