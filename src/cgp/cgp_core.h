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


#pragma once

#include "../ga.h"


#ifdef SYMREG
  #include "../symreg/cgp.h"
#else
  #include "../ifilter/cgp.h"
#endif


typedef struct {
    int inputs;
    int outputs;
    int cols;
    int rows;
    int lback;
    int mutation_rate;
    ga_fitness_func_t fitness_function;
} cgp_settings_t;


static const ga_problem_type_t CGP_PROBLEM_TYPE = maximize;


/**
 * One CGP node (function block)
 */
typedef struct {
    int inputs[CGP_FUNC_INPUTS];
    cgp_func_t function;
    bool is_active;
    bool is_constant;
    cgp_value_t constant_value;
} cgp_node_t;


/**
 * Chromosome
 */
struct cgp_genome {
    cgp_node_t *nodes;
    int *outputs;

    int inputs_count;
    int outputs_count;
    int cols;
    int rows;
};
typedef struct cgp_genome* cgp_genome_t;


/**
 * Initialize CGP internals
 */
void cgp_init(cgp_settings_t *settings);


/**
 * Deinitialize CGP internals
 */
void cgp_deinit();


/**
 * Create a new CGP population with given size
 * @param  mutation rate (in number of genes)
 * @return
 */
ga_pop_t cgp_init_pop(int pop_size);


/**
 * Allocates memory for new CGP genome
 * @return pointer to newly allocated genome
 */
void* cgp_alloc_genome();


/**
 * Initializes CGP genome to random values
 * @param chromosome
 * @return 0 on success, other value on error
 */
int cgp_randomize_genome(ga_chr_t chromosome);


/**
 * Deinitialize CGP genome
 * @param  genome
 * @return
 */
void cgp_free_genome(void *genome);


/**
 * Replace chromosome genes with genes from other chromomose
 * @param  chr
 * @param  replacement
 * @return
 */
void cgp_copy_genome(void *_dst, void *_src);


/**
 * Replace gene on given locus with random alele
 * @param chr
 * @param gene
 * @return whether active node was changed or not (phenotype has changed)
 */
bool cgp_randomize_gene(cgp_genome_t genome, int gene);


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
 * @return Whether CGP function has changed and evaluation should be restarted
 */
bool cgp_get_output(ga_chr_t chromosome, cgp_value_t *inputs, cgp_value_t *outputs);


/**
 * Calculates CGP node output value.
 *
 * Defined in symreg/cgp.c or ifilter/cgp.c
 *
 * @param  n CGP node
 * @param  A Input value A
 * @param  B Input value B
 * @param  Y Output value
 * @return Whether CGP function changed and evaluation should be restarted
 */
bool cgp_get_node_output(cgp_node_t *n, cgp_value_t A,
    cgp_value_t B, cgp_value_t *Y);


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
static inline int cgp_nodes_count(cgp_genome_t genome)
{
    return genome->rows * genome->rows;
}


/**
 * Returns index of node in given column and row
 * @param  col
 * @param  row
 * @return
 */
static inline int cgp_output_genes_offset(cgp_genome_t genome)
{
    return ((CGP_FUNC_INPUTS + 1) * cgp_nodes_count(genome));
}


/**
 * Returns index of node in given column and row
 * @param  col
 * @param  row
 * @return
 */
static inline int cgp_genome_length(cgp_genome_t genome)
{
    return cgp_output_genes_offset(genome) + genome->outputs_count;
}



/**
 * Returns index of node in given column and row
 * @param  col
 * @param  row
 * @return
 */
static inline int cgp_node_index(cgp_genome_t genome, int col, int row)
{
    return genome->rows * col + row;
}


/**
 * Returns node column based on its index
 * @param  index
 * @return
 */
static inline int cgp_node_col(cgp_genome_t genome, int index)
{
    return index / genome->rows;
}


/**
 * Returns node row based on its index
 * @param  index
 * @return
 */
static inline int cgp_node_row(cgp_genome_t genome, int index)
{
    return index % genome->rows;
}


/**
 * Finds which blocks are active.
 * @param chromosome
 * @param active
 */
void cgp_find_active_blocks(ga_chr_t chromosome);


/**
 * Returns l-back parameter value
 */
int cgp_get_lback();
