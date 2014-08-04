/**
 * Tests CGP evaluation = calculation of the outputs.
 * Compile with -DTEST_EVAL
 */

#include <stdio.h>

#include "../cgp.h"


int main(int argc, char const *argv[])
{
    cgp_value_t inputs[CGP_INPUTS] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    cgp_value_t outputs[CGP_OUTPUTS] = {};

    cgp_init();
    cgp_chr chr = cgp_create_chr();

    // define nodes
    for (uint i = 0; i < CGP_NODES; i++) {
        _cgp_node *n = &(chr->nodes[i]);

        n->inputs[0] = i + CGP_INPUTS - CGP_ROWS - 3;
        n->inputs[1] = i + CGP_INPUTS - CGP_ROWS - 1;
        n->function = i % CGP_FUNC_COUNT;
    }

    // define outputs
    chr->outputs[0] = 13;

    cgp_dump_chr_asciiart(chr, stdout);
    putchar('\n');
    cgp_get_output(chr, inputs, outputs);

    cgp_destroy_chr(chr);
    cgp_deinit();
}
