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

#include "types.h"
#include "cgp_config.h"

#define CGP_FUNC_INPUTS 2
#define CGP_NODES (CGP_COLS * CGP_ROWS)
#define CGP_CHR_OUTPUTS_INDEX ((CGP_FUNC_INPUTS + 1) * CGP_NODES)
#define CGP_CHR_LENGTH (CGP_CHR_OUTPUTS_INDEX + CGP_OUTPUTS)

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


static char *func_names[] = {
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


typedef struct {
    uint inputs[CGP_FUNC_INPUTS];
    _cgp_func function;
} _cgp_node;


typedef struct {
    bool has_fitness;
    uint fitness;
    _cgp_node nodes[CGP_COLS * CGP_ROWS];
    uint outputs[CGP_OUTPUTS];
} _cgp_chr;
typedef _cgp_chr* cgp_chr;


typedef struct {
    uint size;
    uint best_fitness;
    uint best_chr_index;
    cgp_chr *chromosomes;
} _cgp_pop;
typedef _cgp_pop* cgp_pop;


typedef enum {
    asciiart,
    compat,
    readable,
} cgp_dump_format;


/**
 * Initialize CGP internals
 */
void cgp_init();


/**
 * Deinitialize CGP internals
 */
void cgp_deinit();


/**
 * Create a new CGP population with given size
 * @param  size
 * @return
 */
cgp_pop cgp_create_pop(uint size);


/**
 * Clear memory associated with given population (including its chromosomes)
 * @param pop
 */
void cgp_destroy_pop(cgp_pop pop);



/**
 * Create a new chromosome, according to specification
 * @return
 */
cgp_chr cgp_create_chr();


/**
 * Clear memory associated with given chromosome
 * @param chr
 */
void cgp_destroy_chr(cgp_chr chr);


/**
 * Create and return a copy of given chromosome
 * @param  chr
 * @return
 */
cgp_chr cgp_clone_chr(cgp_chr chr);


/**
 * Mutate given chromosome
 * @param chr
 */
void cgp_mutate_chr(cgp_chr chr);


/**
 * Randomize given chromosome (e.g. replace with new random individual)
 * @param chr
 */
void cgp_randomize_chr(cgp_chr chr);


/**
 * Calculate fitness of given chromosome, but only if its `has_fitness`
 * attribute is set to `false`
 * @param chr
 */
void cgp_evaluate_chr(cgp_chr chr);


/**
 * Calculate fitness of given chromosome, regardless of its `has_fitness`
 * value
 * @param chr
 */
void cgp_reevaluate_chr(cgp_chr chr);


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



/**
 * Returns index of node in given column and row
 * @param  col
 * @param  row
 * @return
 */
static inline uint cgp_node_index(uint col, uint row)
{
    return CGP_ROWS * col + row;
}


/**
 * Returns node column based on its index
 * @param  index
 * @return
 */
static inline uint cgp_node_col(uint index)
{
    return index / CGP_ROWS;
}


/**
 * Returns node row based on its index
 * @param  index
 * @return
 */
static inline uint cgp_node_row(uint index)
{
    return index % CGP_ROWS;
}
