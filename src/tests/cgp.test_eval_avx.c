/**
 * Tests CGP evaluation = calculation of the outputs.
 * Compile with -DTEST_EVAL_AVX
 */

#include <stdlib.h>
#include <stdio.h>
#include <immintrin.h>

#include "../cpu.h"
#include "../cgp.h"
#include "../cgp_avx.h"



#define in1 {0, 1, 2, 3, 4, 5, 6, 7, 8}
#define in4 in1, in1, in1, in1
#define in16 in4, in4, in4, in4
#define in32 in16, in16


int main(int argc, char const *argv[])
{
    // pre-flight check
    if (!can_use_intel_core_4th_gen_features()) {
        fprintf(stderr, "%s", "AVX2 not supported.\n");
        exit(1);
    }


    cgp_value_t inputs[32][CGP_INPUTS] = {in32};
    cgp_value_t outputs[32][CGP_OUTPUTS] = {};

    cgp_init(0, NULL);

    cgp_genome_t genome = (cgp_genome_t) cgp_alloc_genome();
    struct ga_chr chr = {
        .genome = genome
    };

    // define nodes
    for (int x = 0; x < CGP_COLS; x++) {
        for (int y = 0; y < CGP_ROWS; y++) {
            int i = cgp_node_index(x, y);
            cgp_node_t *n = &(genome->nodes[i]);
            n->inputs[0] = i + CGP_INPUTS - y - y - 1;
            n->inputs[1] = i + CGP_INPUTS - CGP_ROWS;
            n->function = (cgp_func_t) (i % CGP_FUNC_COUNT);
        }
    }

    // define outputs
    genome->outputs[0] = 13;

    cgp_dump_chr_asciiart(&chr, stdout);
    putchar('\n');
    cgp_get_output_avx(&chr, inputs, outputs);

    free(genome);
    cgp_deinit();
}
