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

#include "ga.h"
#include "cgp_config.h"


#define CGP_FUNC_INPUTS 2
#define CGP_NODES (CGP_COLS * CGP_ROWS)
#define CGP_CHR_OUTPUTS_INDEX ((CGP_FUNC_INPUTS + 1) * CGP_NODES)
#define CGP_CHR_LENGTH (CGP_CHR_OUTPUTS_INDEX + CGP_OUTPUTS)

typedef unsigned char cgp_value_t;

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
} cgp_func_t;


/**
 * One CGP node (function block)
 */
typedef struct {
    int inputs[CGP_FUNC_INPUTS];
    cgp_func_t function;
} cgp_node_t;


/**
 * Chromosome
 */
struct cgp_genome {
    cgp_node_t nodes[CGP_COLS * CGP_ROWS];
    int outputs[CGP_OUTPUTS];
};
typedef struct cgp_genome* cgp_genome_t;


/**
 * Initialize CGP internals
 * @param Fitness function to use
 * @param Type of solved problem
 */
void cgp_init();


/**
 * Deinitialize CGP internals
 */
void cgp_deinit();


/**
 * Create a new CGP population with given size
 * @param  mutation rate (in number of genes)
 * @param  population size
 * @param  fitness function
 * @return
 */
ga_pop_t cgp_init_pop(int mutation_rate, int pop_size, ga_fitness_func_t fitness_func);


/**
 * Initialize new CGP genome
 * @param chromosome
 */
int cgp_init_chr(ga_chr_t chromosome);


/**
 * Deinitialize CGP genome
 * @param  chromosome
 * @return
 */
void cgp_deinit_chr(ga_chr_t chromosome);


/**
 * Replace gene on given locus with random alele
 * @param chr
 * @param gene
 */
void cgp_randomize_gene(cgp_genome_t genome, int gene);


/**
 * Mutate given chromosome
 * @param chr
 */
void cgp_mutate_chr(ga_chr_t chromosome);


/**
 * Replace chromosome genes with genes from other chromomose
 * @param  chr
 * @param  replacement
 * @return
 */
void cgp_replace_chr(ga_chr_t chromosome, ga_chr_t replacement);


/**
 * Calculate output of given chromosome and inputs
 * @param chr
 */
void cgp_get_output(ga_chr_t chromosome, cgp_value_t *inputs, cgp_value_t *outputs);


/**
 * Create new generation
 * @param pop
 * @param mutation_rate
 */
void cgp_offspring(ga_pop_t pop);


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



/**
 * Sets mutation rate
 * @param mutation_rate
 */
void cgp_set_mutation_rate(int mutation_rate);