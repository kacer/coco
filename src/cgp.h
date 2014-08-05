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


#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "cgp_config.h"


#define CGP_FUNC_INPUTS 2
#define CGP_NODES (CGP_COLS * CGP_ROWS)
#define CGP_CHR_OUTPUTS_INDEX ((CGP_FUNC_INPUTS + 1) * CGP_NODES)
#define CGP_CHR_LENGTH (CGP_CHR_OUTPUTS_INDEX + CGP_OUTPUTS)

typedef unsigned char cgp_value_t;
typedef double cgp_fitness_t;

#define CGP_FUNC_COUNT 16
typedef enum
{
    c255 = 0,   // 255
    identity,   // a
    inversion,  // 255 - a
    b_or,       // a or b
    b_not1or2,  // (not a) or b
    b_and,      // a and b
    b_nand,     // not (a and b)
    b_xor,      // a xor b
    rshift1,    // a >> 1
    rshift2,    // a >> 2
    swap,       // a <-> b
    add,        // a + b
    add_sat,    // a +S b
    avg,        // (a + b) >> 1
    max,        // max(a, b)
    min,        // min(a, b)
} _cgp_func;


static inline char* cgp_func_name(_cgp_func f) {
    char *func_names[] = {
        " FF ",     // 255
        "  a ",     // a
        "FF-a",     // 255 - a
        " or ",     // a or b
        "~1|2",     // (not a) or b
        " and",     // a and b
        "nand",     // not (a and b)
        " xor",     // a xor b
        "a>>1",     // a >> 1
        "a>>2",     // a >> 2
        "swap",     // a <-> b
        " +  ",     // a + b
        " +S ",     // a +S b
        " avg",     // (a + b) >> 1
        " max",     // max(a, b)
        " min",     // min(a, b)
    };
    return func_names[f];
}


/**
 * One CGP node (function block)
 */
typedef struct {
    int inputs[CGP_FUNC_INPUTS];
    _cgp_func function;
} _cgp_node;


/**
 * Chromosome
 */
typedef struct {
    bool has_fitness;
    cgp_fitness_t fitness;
    _cgp_node nodes[CGP_COLS * CGP_ROWS];
    int outputs[CGP_OUTPUTS];
} _cgp_chr;
typedef _cgp_chr* cgp_chr;


/**
 * Population
 */
typedef struct {
    int size;
    int generation;
    cgp_fitness_t best_fitness;
    int best_chr_index;
    cgp_chr *chromosomes;
} _cgp_pop;
typedef _cgp_pop* cgp_pop;


/**
 * Dump formats
 */
typedef enum {
    asciiart,
    compat,
    readable,
} cgp_dump_format;


/**
 * CGP problem type
 */
typedef enum {
    minimize,
    maximize,
} cgp_problem_type;


/**
 * Fitness function
 */
typedef cgp_fitness_t (*cgp_fitness_func)(cgp_chr chromosome);


/**
 * Initialize CGP internals
 * @param Fitness function to use
 * @param Type of solved problem
 */
void cgp_init(cgp_fitness_func fitness, cgp_problem_type type);


/**
 * Deinitialize CGP internals
 */
void cgp_deinit();


/**
 * Create a new chromosome
 * @return
 */
cgp_chr cgp_create_chr();


/**
 * Clear memory associated with given chromosome
 * @param chr
 */
void cgp_destroy_chr(cgp_chr chr);


/**
 * Replace chromosome genes with genes from other chromomose
 * @param  chr
 * @param  replacement
 * @return
 */
void cgp_replace_chr(cgp_chr chr, cgp_chr replacement);


/**
 * Mutate given chromosome
 * @param chr
 * @param max_changed_genes
 */
void cgp_mutate_chr(cgp_chr chr, int max_changed_genes);


/**
 * Calculate output of given chromosome and inputs
 * @param chr
 */
void cgp_get_output(cgp_chr chr, cgp_value_t *inputs, cgp_value_t *outputs);


/**
 * Calculate fitness of given chromosome, but only if its `has_fitness`
 * attribute is set to `false`
 * @param chr
 */
cgp_fitness_t cgp_evaluate_chr(cgp_chr chr);


/**
 * Calculate fitness of given chromosome, regardless of its `has_fitness`
 * value
 * @param chr
 */
cgp_fitness_t cgp_reevaluate_chr(cgp_chr chr);


/**
 * Create a new CGP population with given size
 * @param  size
 * @return
 */
cgp_pop cgp_create_pop(int size);


/**
 * Clear memory associated with given population (including its chromosomes)
 * @param pop
 */
void cgp_destroy_pop(cgp_pop pop);


/**
 * Calculate fitness of whole population, using `cgp_evaluate_chr`
 * @param chr
 */
static inline void cgp_evaluate_pop(cgp_pop pop);


/**
 * Calculate fitness of whole population, using `cgp_evaluate_chr`
 * in single thread
 * @param chr
 */
void _cgp_evaluate_pop_simple(cgp_pop pop);


/**
 * Calculate fitness of whole population, using `cgp_evaluate_chr`
 * using one thread per chromosome
 * @param chr
 */
void _cgp_evaluate_pop_pthread(cgp_pop pop);


#ifdef CGP_USE_PTHREAD

    static inline void cgp_evaluate_pop(cgp_pop pop) {
        _cgp_evaluate_pop_pthread(pop);
    }

#else

    static inline void cgp_evaluate_pop(cgp_pop pop) {
        _cgp_evaluate_pop_simple(pop);
    }

#endif


/**
 * Advance population to next generation
 * @param pop
 * @param mutation_rate
 */
void cgp_next_generation(cgp_pop pop, int mutation_rate);


/**
 * Dumps chromosome to given file pointer in given format
 * @param fp
 */
void cgp_dump_chr(cgp_chr chr, FILE *fp, cgp_dump_format fmt);


/**
 * Dumps chromosome to given file in CGP-viewer compatible format
 * @param chr
 * @param fp
 */
void cgp_dump_chr_compat(cgp_chr chr, FILE *fp);


/**
 * Dumps chromosome to given file in more human-friendly fashion
 * @param chr
 * @param fp
 */
void cgp_dump_chr_readable(cgp_chr chr, FILE *fp);


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
void cgp_dump_chr_asciiart(cgp_chr chr, FILE *fp);


static inline int cgp_dump_chr_asciiart_width() {
    return 5 + CGP_COLS * 18 + 5;
}


static inline int cgp_dump_chr_asciiart_height() {
    return 1 + CGP_ROWS * 4 + 1;
}


/**
 * Loads chromosome from given file stored in CGP-viewer compatible format
 * @param chr
 * @param fp
 * @return 0 on success, -1 on file format error, -2 on incompatible CGP config
 */
int cgp_load_chr_compat(cgp_chr chr, FILE *fp);


/**
 * Dumps whole population to given file pointer with chromosomes in
 * CGP-viewer compatible format
 * @param pop
 * @param fp
 */
void cgp_dump_pop_compat(cgp_pop pop, FILE *fp);


/**
 * Loads whole population from given file with chromosomes stored in
 * CGP-viewer compatible format
 * @param pop
 * @param fp
 * @return 0 on success, -1 on file format error, -2 on incompatible CGP config
 */
int cgp_load_pop_compat(cgp_pop *pop_ptr, FILE *fp);


/**
 * Returns index of node in given column and row
 * @param  col
 * @param  row
 * @return
 */
static inline int cgp_node_index(int col, int row)
{
    return CGP_ROWS * col + row;
}


/**
 * Returns node column based on its index
 * @param  index
 * @return
 */
static inline int cgp_node_col(int index)
{
    return index / CGP_ROWS;
}


/**
 * Returns node row based on its index
 * @param  index
 * @return
 */
static inline int cgp_node_row(int index)
{
    return index % CGP_ROWS;
}
