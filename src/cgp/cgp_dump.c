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
#include <stdarg.h>
#include <string.h>

#include "cgp_dump.h"


#define FITNESS_PRECISE_FMT "%a"


/**
 * Dumps chromosome to given file pointer
 * @param fp
 */
void cgp_dump_chr(ga_chr_t chr, FILE *fp, cgp_dump_format fmt)
{
    if (fmt == readable) {
        cgp_dump_chr_readable(chr, fp);

    } else if (fmt == asciiart) {
        cgp_dump_chr_asciiart(chr, fp, false);

    } else if (fmt == asciiart_active) {
        cgp_dump_chr_asciiart(chr, fp, true);

    } else if (fmt == code) {
        cgp_dump_chr_code(chr, fp);

    } else {
        cgp_dump_chr_compat(chr, fp);
    }

}


/**
 * Dumps primary outputs to given file in "(X, Y, Z)"" format
 * @param chr
 * @param fp
 */
void cgp_dump_chr_outputs(ga_chr_t chr, FILE *fp)
{
    cgp_genome_t genome = (cgp_genome_t) chr->genome;

    fprintf(fp, "(");
    for (int i = 0; i < CGP_OUTPUTS; i++) {
        if (i > 0) fprintf(fp, ", ");
        fprintf(fp, "%u", genome->outputs[i]);
    }
    fprintf(fp, ")\n");
}


/**
 * Dumps basic info in human-friendly format
 * @param chr
 * @param fp
 */
void cgp_dump_chr_header(ga_chr_t chr, FILE *fp)
{
    fprintf(fp,
        "Inputs: %u\n"
        "Outputs: %u\n"
        "Size: %u x %u\n"
        "Blocks: %u-ary, %u output(s), %u functions\n",
        CGP_INPUTS, CGP_OUTPUTS, CGP_COLS, CGP_ROWS, CGP_FUNC_INPUTS,
        1, CGP_FUNC_COUNT);

    if (chr->has_fitness) {
        fprintf(fp, "Fitness: %lf\n", chr->fitness);
    } else {
        fprintf(fp, "Fitness: <none>\n");
    }
}


/**
 * Dumps chromosome to given file in CGP-viewer compatible format
 * @param chr
 * @param fp
 */
void cgp_dump_chr_compat(ga_chr_t chr, FILE *fp)
{
    cgp_genome_t genome = (cgp_genome_t) chr->genome;

    fprintf(fp, "{%u, %u, %u, %u, %u, %u, %u}",
        CGP_INPUTS, CGP_OUTPUTS, CGP_COLS, CGP_ROWS, CGP_FUNC_INPUTS,
        1, CGP_FUNC_COUNT);

    for (int i = 0; i < CGP_NODES; i++) {
        cgp_node_t *n = &(genome->nodes[i]);
        fprintf(fp, "([%u] %u, %u, %u)",
            CGP_INPUTS + i, n->inputs[0], n->inputs[1], n->function);
    }

    cgp_dump_chr_outputs(chr, fp);
}


/**
 * Dumps chromosome to given file in more human-friendly fashion
 * @param chr
 * @param fp
 */
void cgp_dump_chr_readable(ga_chr_t chr, FILE *fp)
{
    cgp_genome_t genome = (cgp_genome_t) chr->genome;

    cgp_dump_chr_header(chr, fp);

    for (int y = 0; y < CGP_ROWS; y++) {
        for (int x = 0; x < CGP_COLS; x++) {
            int i = cgp_node_index(x, y);
            cgp_node_t *n = &(genome->nodes[i]);

            fprintf(fp, "([%2u] %2u, %2u, %2u)  ",
                CGP_INPUTS + i, n->inputs[0], n->inputs[1], n->function);
        }
        if (CGP_OUTPUTS <= CGP_ROWS && y < CGP_OUTPUTS) {
            fprintf(fp, "  (%2u)", genome->outputs[y]);
        }
        fprintf(fp, "\n");
    }

    if (CGP_OUTPUTS > CGP_ROWS) {
        fprintf(fp, "Primary outputs: ");
        cgp_dump_chr_outputs(chr, fp);
    }
}


/**
 * Dumps chromosome to given file as C code
 * @param chr
 * @param fp
 */
void cgp_dump_chr_code(ga_chr_t chr, FILE *fp)
{
    cgp_genome_t genome = (cgp_genome_t) chr->genome;

    fprintf(fp, "%s", CGP_CODE_PROLOG);
    fprintf(fp, "cgp_value_t cgp(cgp_value_t inputs[%d])\n{\n", CGP_INPUTS);

    for (int i = 0; i < CGP_NODES; i++) {
        cgp_node_t *n = &(genome->nodes[i]);
        if (n->is_active) {
            int arity = CGP_FUNC_ARITY[n->function];
            char input1[20], input2[20];

            if (n->is_constant) {
                fprintf(fp, "\tcgp_value_t n%02d = " CGP_VALUE_FORMAT ";\n", i, n->constant_value);
                continue;
            }

            if (n->inputs[0] < CGP_INPUTS) {
                snprintf(input1, 20, "inputs[%d]", n->inputs[0]);
            } else {
                snprintf(input1, 20, "n%02d", n->inputs[0] - CGP_INPUTS);
            }

             if (n->inputs[1] < CGP_INPUTS) {
                snprintf(input2, 20, "inputs[%d]", n->inputs[1]);
            } else {
                snprintf(input2, 20, "n%02d", n->inputs[1] - CGP_INPUTS);
            }

            fprintf(fp, "\tcgp_value_t n%02d = ", i);
            if (arity == 0) {
                fprintf(fp, "%s", CGP_FUNC_CODE[n->function]);

            } else if (arity == 1) {
                fprintf(fp, CGP_FUNC_CODE[n->function], input1);

            } else if (arity == 2) {
                fprintf(fp, CGP_FUNC_CODE[n->function], input1, input2);
            }

            fprintf(fp, ";\n");
        }
    }

    fprintf(fp, "\treturn n%02d;\n", genome->outputs[0] - CGP_INPUTS);
    fprintf(fp, "}");
}


/**
 * ASCII-art rendering helper: renders primary input
 * @param in_counter
 * @param chr
 * @param fp
 */
void cgp_dump_chr_asciiart_input(int *in_counter, ga_chr_t chr, FILE *fp)
{
    if (*in_counter < CGP_INPUTS) fprintf(fp, "[%2u]>| ", (*in_counter)++);
    else fprintf(fp, "     | ");
}


/**
 * ASCII-art rendering helper: renders primary output
 * @param in_counter
 * @param chr
 * @param fp
 */
void cgp_dump_chr_asciiart_output(int *out_counter, ga_chr_t chr, FILE *fp)
{
    cgp_genome_t genome = (cgp_genome_t) chr->genome;
    if (*out_counter < CGP_OUTPUTS) fprintf(fp, ">[%2u]", genome->outputs[(*out_counter)++]);
}


/**
 * Dumps chromosome to given file as an ASCII-art, which looks like this:
 *
 *      .------------------------------------.
 *      |      .----.            .----.      |
 * [ 0]>| [ 1]>|    |>[ 4]  [ 0]>|    |>[ 6] |>[ 6]
 * [ 1]>| [ 3]>|  a |       [ 4]>| xor|      |
 * [ 2]>|      '----'            '----'      |
 * [ 3]>|      .----.            .----.      |
 *      | [ 3]>|    |>[ 5]  [ 4]>|    |>[ 7] |
 *      | [ 0]>|a>>1|       [ 5]>|nand|      |
 *      |      '----'            '----'      |
 *      '------------------------------------'
 *
 * @param chr
 * @param fp
*/
void cgp_dump_chr_asciiart(ga_chr_t chr, FILE *fp, bool only_active_blocks)
{
    int in_counter = 0;
    int out_counter = 0;
    cgp_genome_t genome = (cgp_genome_t) chr->genome;

    cgp_dump_chr_header(chr, fp);

    /* top of the circuit */
    fprintf(fp, "     .--");
    for (int x = 0; x < CGP_COLS; x++) {
        fprintf(fp, "----------------");
        if (x == CGP_COLS - 1) fprintf(fp, ".\n");
        else fprintf(fp, "--");
    }

    for (int y = 0; y < CGP_ROWS; y++) {
        if (y != 0) cgp_dump_chr_asciiart_input(&in_counter, chr, fp);
        else fprintf(fp, "     | ");

        /* top of the blocks */
        fprintf(fp, " ");
        for (int x = 0; x < CGP_COLS; x++) {
            int i = cgp_node_index(x, y);
            cgp_node_t *n = &(genome->nodes[i]);

            if (only_active_blocks && !n->is_active) {
                fprintf(fp, "                ");
            } else {
                fprintf(fp, "    .----.      ");
            }

            if (x == CGP_COLS - 1) fprintf(fp, "|");
            else fprintf(fp, "  ");
        }

        if (y != 0) cgp_dump_chr_asciiart_output(&out_counter, chr, fp);
        fprintf(fp, "\n");
        cgp_dump_chr_asciiart_input(&in_counter, chr, fp);

        /* first line of blocks */
        for (int x = 0; x < CGP_COLS; x++) {
            int i = cgp_node_index(x, y);
            cgp_node_t *n = &(genome->nodes[i]);

            if (only_active_blocks && !n->is_active) {
                fprintf(fp, "                ");
            } else {
                if (only_active_blocks && (CGP_FUNC_ARITY[n->function] < 1 || n->is_constant)) {
                    fprintf(fp, "     |    |>[%2u]", CGP_INPUTS + i);
                } else {
                    fprintf(fp, "[%2u]>|    |>[%2u]", n->inputs[0], CGP_INPUTS + i);
                }
            }

            if (x == CGP_COLS - 1) fprintf(fp, " |");
            else fprintf(fp, "  ");
        }

        cgp_dump_chr_asciiart_output(&out_counter, chr, fp);
        fprintf(fp, "\n");
        cgp_dump_chr_asciiart_input(&in_counter, chr, fp);

        /* second line of blocks */
        for (int x = 0; x < CGP_COLS; x++) {
            int i = cgp_node_index(x, y);
            cgp_node_t *n = &(genome->nodes[i]);

            if (only_active_blocks && !n->is_active) {
                fprintf(fp, "                ");
            } else {

                char name[5];
                if (n->is_constant) {
                    snprintf(name, 5, CGP_ASCIIART_CONSTANT_FORMAT, n->constant_value);
                    name[4] = '\0';  // always null-terminated
                } else {
                    strncpy(name, CGP_FUNC_NAMES[n->function], 5);
                }

                if (only_active_blocks && (CGP_FUNC_ARITY[n->function] < 2 || n->is_constant)) {
                    fprintf(fp, "     |%s|     ", name);
                } else {
                    fprintf(fp, "[%2u]>|%s|     ", n->inputs[1], name);
                }
            }

            if (x == CGP_COLS - 1) fprintf(fp, " |");
            else fprintf(fp, "  ");
        }

        cgp_dump_chr_asciiart_output(&out_counter, chr, fp);
        fprintf(fp, "\n");
        cgp_dump_chr_asciiart_input(&in_counter, chr, fp);

        /* bottom of the blocks */
        fprintf(fp, " ");
        for (int x = 0; x < CGP_COLS; x++) {
            int i = cgp_node_index(x, y);
            cgp_node_t *n = &(genome->nodes[i]);

            if (only_active_blocks && !n->is_active) {
                fprintf(fp, "                ");
            } else {
                fprintf(fp, "    '----'      ");
            }

            if (x == CGP_COLS - 1) fprintf(fp, "|");
            else fprintf(fp, "  ");
        }

        cgp_dump_chr_asciiart_output(&out_counter, chr, fp);
        fprintf(fp, "\n");
    }

    /* bottom of the circuit */
    fprintf(fp, "     '--");
    for (int x = 0; x < CGP_COLS; x++) {
        fprintf(fp, "----------------");
        if (x == CGP_COLS - 1) fprintf(fp, "'\n");
        else fprintf(fp, "--");
    }
}


/**
 * Dumps whole population to given file pointer with chromosomes in
 * CGP-viewer compatible format
 * @param pop
 * @param fp
 */
void cgp_dump_pop_compat(ga_pop_t pop, FILE *fp)
{
    fprintf(fp, "Generation: %d\n", pop->generation);
    fprintf(fp, "Best chromosome: %d\n", pop->best_chr_index);
    fprintf(fp, "Chromosomes: %d\n", pop->size);

    for (int i = 0; i < pop->size; i++) {
        ga_chr_t chr = pop->chromosomes[i];
        if (chr->has_fitness) {
            fprintf(fp, "Fitness: Y " FITNESS_PRECISE_FMT "\n", chr->fitness);
        } else {
            fprintf(fp, "Fitness: N\n");
        }
        cgp_dump_chr_compat(chr, fp);
    }
}
