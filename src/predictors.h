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


#include "ga.h"


typedef unsigned int pred_gene_t;
typedef unsigned int pred_index_t;

struct pred_genome {
    pred_index_t used_genes;
    pred_gene_t *genes;
};
typedef struct pred_genome* pred_genome_t;


/**
 * Initialize predictor internals
 */
void pred_init(pred_gene_t max_gene_value, pred_index_t max_genome_length,
    pred_index_t initial_genome_length, float mutation_rate);


/**
 * Create a new predictors population with given size
 * @param  size
 * @param  problem-specific methods
 * @return
 */
ga_pop_t pred_init_pop(int pop_size);


/**
 * Allocates memory for new predictor genome
 * @return pointer to newly allocated genome
 */
void* pred_alloc_genome();


/**
 * Deinitialize predictor genome
 * @param  genome
 * @return
 */
void pred_free_genome(void *genome);


/**
 * Initializes predictor genome to random values
 * @param chromosome
 */
int pred_randomize_genome(ga_chr_t chromosome);


/**
 * Replace chromosome genes with genes from other chromomose
 * @param  chr
 * @param  replacement
 * @return
 */
void pred_copy_genome(void *_dst, void *_src);


/**
 * Genome mutation function
 * @param  chromosome
 * @return
 */
void pred_mutate(ga_chr_t chromosome);


/**
 * Create new generation
 * @param pop
 * @param mutation_rate
 */
void pred_offspring(ga_pop_t pop);
