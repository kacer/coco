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


/**
 * Dumps chromosome to given file pointer
 * @param fp
 */
void cgp_dump_chr(cgp_chr chr, FILE *fp, cgp_dump_format fmt)
{
    if (fmt == readable) {
        cgp_dump_chr_readable(chr, fp);

    } else if (fmt == asciiart) {
        cgp_dump_chr_asciiart(chr, fp);

    } else {
        cgp_dump_chr_compat(chr, fp);
    }

}


/**
 * Dumps primary outputs to given file in "(X, Y, Z)"" format
 * @param chr
 * @param fp
 */
void cgp_dump_chr_outputs(cgp_chr chr, FILE *fp)
{
    fprintf(fp, "(");
    for (int i = 0; i < CGP_OUTPUTS; i++) {
        if (i > 0) fprintf(fp, ", ");
        fprintf(fp, "%u", chr->outputs[i]);
    }
    fprintf(fp, ")");
}


/**
 * Dumps basic info in human-friendly format
 * @param chr
 * @param fp
 */
void cgp_dump_chr_header(cgp_chr chr, FILE *fp)
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
void cgp_dump_chr_compat(cgp_chr chr, FILE *fp)
{
    fprintf(fp, "{%u, %u, %u, %u, %u, %u, %u}",
        CGP_INPUTS, CGP_OUTPUTS, CGP_COLS, CGP_ROWS, CGP_FUNC_INPUTS,
        1, CGP_FUNC_COUNT);

    for (int i = 0; i < CGP_NODES; i++) {
        _cgp_node *n = &(chr->nodes[i]);
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
void cgp_dump_chr_readable(cgp_chr chr, FILE *fp)
{
    cgp_dump_chr_header(chr, fp);

    for (int y = 0; y < CGP_ROWS; y++) {
        for (int x = 0; x < CGP_COLS; x++) {
            int i = cgp_node_index(x, y);
            _cgp_node *n = &(chr->nodes[i]);

            fprintf(fp, "([%2u] %2u, %2u, %2u)  ",
                CGP_INPUTS + i, n->inputs[0], n->inputs[1], n->function);
        }
        if (CGP_OUTPUTS <= CGP_ROWS && y < CGP_OUTPUTS) {
            fprintf(fp, "  (%2u)", chr->outputs[y]);
        }
        fprintf(fp, "\n");
    }

    if (CGP_OUTPUTS > CGP_ROWS) {
        fprintf(fp, "Primary outputs: ");
        cgp_dump_chr_outputs(chr, fp);
    }
}


/**
 * ASCII-art rendering helper: renders primary input
 * @param in_counter
 * @param chr
 * @param fp
 */
void cgp_dump_chr_asciiart_input(int *in_counter, cgp_chr chr, FILE *fp)
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
void cgp_dump_chr_asciiart_output(int *out_counter, cgp_chr chr, FILE *fp)
{
    if (*out_counter < CGP_OUTPUTS) fprintf(fp, ">[%2u]", chr->outputs[(*out_counter)++]);
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
void cgp_dump_chr_asciiart(cgp_chr chr, FILE *fp)
{
    int in_counter = 0;
    int out_counter = 0;

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
            fprintf(fp, "    .----.      ");
            if (x == CGP_COLS - 1) fprintf(fp, "|");
            else fprintf(fp, "  ");
        }

        if (y != 0) cgp_dump_chr_asciiart_output(&out_counter, chr, fp);
        fprintf(fp, "\n");
        cgp_dump_chr_asciiart_input(&in_counter, chr, fp);

        /* first line of blocks */
        for (int x = 0; x < CGP_COLS; x++) {
            int i = cgp_node_index(x, y);
            _cgp_node *n = &(chr->nodes[i]);
            fprintf(fp, "[%2u]>|    |>[%2u]", n->inputs[0], CGP_INPUTS + i);
            if (x == CGP_COLS - 1) fprintf(fp, " |");
            else fprintf(fp, "  ");
        }

        cgp_dump_chr_asciiart_output(&out_counter, chr, fp);
        fprintf(fp, "\n");
        cgp_dump_chr_asciiart_input(&in_counter, chr, fp);

        /* second line of blocks */
        for (int x = 0; x < CGP_COLS; x++) {
            int i = cgp_node_index(x, y);
            _cgp_node *n = &(chr->nodes[i]);
            fprintf(fp, "[%2u]>|%s|     ", n->inputs[1], cgp_func_name(n->function));
            if (x == CGP_COLS - 1) fprintf(fp, " |");
            else fprintf(fp, "  ");
        }

        cgp_dump_chr_asciiart_output(&out_counter, chr, fp);
        fprintf(fp, "\n");
        cgp_dump_chr_asciiart_input(&in_counter, chr, fp);

        /* bottom of the blocks */
        fprintf(fp, " ");
        for (int x = 0; x < CGP_COLS; x++) {
            fprintf(fp, "    '----'      ");
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
