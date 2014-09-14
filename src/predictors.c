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
#include <string.h>

#include "random.h"
#include "fitness.h"
#include "predictors.h"


static pred_gene_t _max_gene_value;
static pred_index_t _max_genome_length;
static pred_index_t _initial_genome_length;
static float _mutation_rate;


/**
 * Initialize predictor internals
 */
void pred_init(pred_gene_t max_gene_value, pred_index_t max_genome_length,
    pred_index_t initial_genome_length, float mutation_rate)
{
    _max_gene_value = max_gene_value;
    _max_genome_length = max_genome_length;
    _initial_genome_length = initial_genome_length;
    _mutation_rate = mutation_rate;
}


/**
 * Create a new predictors population with given size
 * @param  size
 * @param  problem-specific methods
 * @return
 */
ga_pop_t pred_init_pop(int pop_size)
{
    /* prepare methods vector */
    ga_func_vect_t methods = {
        .alloc_genome = pred_alloc_genome,
        .free_genome = pred_free_genome,
        .init_genome = pred_randomize_genome,

        .fitness = fitness_eval_predictor,
        .offspring = pred_offspring,
    };

    /* initialize GA */
    return ga_create_pop(pop_size, minimize, methods);
}


/**
 * Allocates memory for new predictor genome
 * @return pointer to newly allocated genome
 */
void* pred_alloc_genome()
{
    pred_genome_t genome = (pred_genome_t) malloc(sizeof(struct pred_genome));
    if (genome == NULL) {
        return NULL;
    }

    genome->genes = (pred_gene_t*) malloc(sizeof(pred_gene_t) * _max_genome_length);
    if (genome->genes == NULL) {
        free(genome);
        return NULL;
    }

    return genome;
}


/**
 * Deinitialize predictor genome
 * @param  genome
 * @return
 */
void pred_free_genome(void *_genome)
{
    pred_genome_t genome = (pred_genome_t) _genome;
    free(genome->genes);
    free(genome);
}


/**
 * Initializes predictor genome to random values
 * @param chromosome
 */
int pred_randomize_genome(ga_chr_t chromosome)
{
    pred_genome_t genome = (pred_genome_t) chromosome->genome;

    genome->used_genes = _initial_genome_length;
    for (int i = 0; i < _max_genome_length; i++) {
        genome->genes[i] = rand_urange(0, _max_gene_value);
    }

    return 0;
}


/**
 * Replace chromosome genes with genes from other chromomose
 * @param  chr
 * @param  replacement
 * @return
 */
void pred_copy_genome(void *_dst, void *_src)
{
    pred_genome_t dst = (pred_genome_t) _dst;
    pred_genome_t src = (pred_genome_t) _src;

    memcpy(dst->genes, src->genes, sizeof(pred_gene_t) * _max_genome_length);
    dst->used_genes = src->used_genes;
}



/**
 * Genome mutation function
 * @param  chromosome
 * @return
 */
void pred_mutate(ga_chr_t chromosome)
{
    pred_genome_t genome = (pred_genome_t) chromosome->genome;

    int max_changed_genes = _mutation_rate * genome->used_genes;
    int genes_to_change = rand_range(0, max_changed_genes);

    for (int i = 0; i < genes_to_change; i++) {
        int gene = rand_range(0, genome->used_genes - 1);
        genome->genes[gene] = rand_urange(0, _max_gene_value);
    }
}


/**
 * Create new generation
 * @param pop
 * @param mutation_rate
 */
void pred_offspring(ga_pop_t pop)
{

}
