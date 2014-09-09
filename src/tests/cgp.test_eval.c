/**
 * Tests CGP evaluation = calculation of the outputs.
 * Compile with -DTEST_EVAL
 */

#include <stdlib.h>
#include <stdio.h>

#include "../cgp.h"


int main(int argc, char const *argv[])
{
    cgp_value_t inputs[CGP_INPUTS] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    cgp_value_t outputs[CGP_OUTPUTS] = {};

    cgp_init();

    cgp_genome_t genome = (cgp_genome_t) malloc(sizeof(struct cgp_genome));
    struct ga_chr chr = {
        .genome = genome
    };

    // define nodes
    for (int i = 0; i < CGP_NODES; i++) {
        cgp_node_t *n = &(genome->nodes[i]);

        n->inputs[0] = i + CGP_INPUTS - CGP_ROWS - 3;
        n->inputs[1] = i + CGP_INPUTS - CGP_ROWS - 1;
        n->function = (cgp_func_t) (i % CGP_FUNC_COUNT);
    }

    // define outputs
    genome->outputs[0] = 13;

    cgp_dump_chr_asciiart(&chr, stdout);
    putchar('\n');
    cgp_get_output(&chr, inputs, outputs);

    free(genome);
    cgp_deinit();
}
