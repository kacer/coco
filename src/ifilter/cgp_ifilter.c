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


#define SWAP(A, B) (((A & 0x0F) << 4) | ((B & 0x0F)))
#define ADD_SAT(A, B) ((A > 0xFF - B) ? 0xFF : A + B)
#define MAX(A, B) ((A > B) ? A : B)
#define MIN(A, B) ((A < B) ? A : B)


/**
 * Calculates CGP node output value.
 *
 * Defined in cgp_symreg or cgp_ifilter
 *
 * @param  n CGP node
 * @param  A Input value A
 * @param  B Input value B
 * @param  Y Output value
 * @return Whether CGP function has changed and evaluation should be restarted
 */
bool cgp_get_node_output(cgp_node_t *n, cgp_value_t A,
    cgp_value_t B, cgp_value_t *Y)
{
    switch (n->function) {
        case c255:          *Y = 255;               break;
        case identity:      *Y = A;                 break;
        case inversion:     *Y = 255 - A;           break;
        case b_or:          *Y = A | B;             break;
        case b_not1or2:     *Y = ~A | B;            break;
        case b_and:         *Y = A & B;             break;
        case b_nand:        *Y = ~(A & B);          break;
        case b_xor:         *Y = A ^ B;             break;
        case rshift1:       *Y = A >> 1;            break;
        case rshift2:       *Y = A >> 2;            break;
        case swap:          *Y = SWAP(A, B);        break;
        case add:           *Y = A + B;             break;
        case add_sat:       *Y = ADD_SAT(A, B);     break;
        case avg:           *Y = (A + B) >> 1;      break;
        case max:           *Y = MAX(A, B);         break;
        case min:           *Y = MIN(A, B);         break;
        default:            abort();
    }

    return false;
}
