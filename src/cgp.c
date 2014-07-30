/*
 * Colearning in Coevolutionary Algorithms
 * Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>
 *
 * Master Thesis
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
#include <stdarg.h>

#include "cgp.h"
#include "random.h"


uint_array allowed_vals[CGP_COLS];


void cgp_randomize_gene(cgp_chr chr, uint gene)
{
    if (gene >= CGP_CHR_LENGTH)
        return;

    if (gene < CGP_CHR_OUTPUTS_INDEX) {
        // mutating node input or function
        uint node_index = gene / 3;
        uint gene_index = gene % 3;
        uint col = cgp_node_col(node_index);

#ifdef TEST_RANDOMIZE
        printf("gene %u, node %u, gene %u, col %u\n", gene, node_index, gene_index, col);
#endif
        if (gene_index == CGP_FUNC_INPUTS) {
            // mutating function
#ifdef TEST_RANDOMIZE
            printf("func 0 - %u\n", CGP_FUNC_COUNT - 1);
#endif
            chr->nodes[node_index].function = rand_range(0, CGP_FUNC_COUNT - 1);
        } else {
            // mutating input
#ifdef TEST_RANDOMIZE
            printf("input choice from %u\n", allowed_vals[col].size);
#endif
            chr->nodes[node_index].inputs[gene_index] = rand_uachoice(&allowed_vals[col]);
        }

    } else {
        // mutating primary output connection
#ifdef TEST_RANDOMIZE
        printf("out 0 - %u\n", CGP_INPUTS + CGP_NODES - 1);
#endif
        uint index = gene - CGP_CHR_OUTPUTS_INDEX;
        chr->outputs[index] = rand_range(0, CGP_INPUTS + CGP_NODES - 1);
    }
}


void cgp_init()
{
    // calculate allowed values of node inputs in each column
    for (uint x = 0; x < CGP_COLS; x++) {

        // range of outputs which can be connected to node in i-th column
        // maximum is actually by 1 larger than maximum allowed value
        // (so there is `val < maximum` in the for loop below instead of `<=`)
        int minimum = CGP_ROWS * (x - CGP_LBACK) + CGP_INPUTS;
        if (minimum < CGP_INPUTS) minimum = CGP_INPUTS;
        int maximum = CGP_ROWS * x + CGP_INPUTS;

        uint size = CGP_INPUTS + maximum - minimum;
        allowed_vals[x].size = size;
        allowed_vals[x].values = (uint*) malloc(sizeof(uint) * size);

        int key = 0;
        // primary input indexes
        for (int val = 0; val < CGP_INPUTS; val++, key++) {
            allowed_vals[x].values[key] = val;
        }

        // nodes to the left output indexes
        for (int val = minimum; val < maximum; val++, key++) {
            allowed_vals[x].values[key] = val;
        }
    }


#ifdef TEST_INIT
    for (int x = 0; x < CGP_COLS; x++) {
        printf ("x = %d: ", x);
        for (int y = 0; y < allowed_vals[x].size; y++) {
            if (y > 0) printf(", ");
            printf ("%d", allowed_vals[x].values[y]);
        }
        printf("\n");
    }
#endif
}


void cgp_deinit()
{
    for (uint x = 0; x < CGP_COLS; x++) {
        free(allowed_vals[x].values);
    }
}



cgp_chr cgp_create_chr()
{
    cgp_chr new_chr = (cgp_chr) malloc(sizeof(_cgp_chr));
    new_chr->has_fitness = false;

    for (int i = 0; i < CGP_CHR_LENGTH; i++) {
        cgp_randomize_gene(new_chr, i);
    }

    return new_chr;
}


void cgp_destroy_chr(cgp_chr chr)
{
    free(chr);
}
