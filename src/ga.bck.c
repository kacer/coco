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
#include <assert.h>

#ifdef GA_USE_PTHREAD
    #include <pthread.h>
#endif

#include "ga.h"
#include "random.h"


unsigned int max_gene_value;
ga_fitness_func fitness_function;
ga_problem_type problem_type;


inline void ga_randomize_gene(ga_chr chr, int gene)
{
    chr->genes[gene] = rand_range(0, max_gene_value);
}


/**
 * Returns true if WHAT is better or same as COMPARED_TO
 * @param  what
 * @param  compared_to
 * @return
 */
static inline bool ga_is_better_or_same(ga_fitness_t what, ga_fitness_t compared_to) {
    return (problem_type == minimize)? what <= compared_to : what >= compared_to;
}


/**
 * Initialize GA internals
 * @param Maximum value each gene can have
 * @param Fitness function to use
 * @param Type of solved problem
 */
void ga_init(unsigned int max_value, ga_fitness_func fitness, ga_problem_type type)
{
    // remember fitness function, problem type etc.
    fitness_function = fitness;
    problem_type = type;
    max_gene_value = max_value;
}


/**
 * Deinitialize CGP internals
 */
void ga_deinit()
{
    // void here
}


/**
 * Create a new chromosome
 * @return
 */
ga_chr ga_create_chr(unsigned int length)
{
    ga_chr new_chr = (ga_chr) malloc(sizeof(_ga_chr));
    if (new_chr == NULL) return NULL;

    new_chr->has_fitness = false;
    new_chr->length = length;
    new_chr->genes = (ga_gene_t*) malloc(sizeof(ga_gene_t) * length);
    if (new_chr->genes == NULL) {
        free(new_chr);
        return NULL;
    }

    for (int i = 0; i < length; i++) {
        ga_randomize_gene(new_chr, i);
    }

    return new_chr;
}


/**
 * Clear memory associated with given chromosome
 * @param chr
 */
void ga_destroy_chr(ga_chr chr)
{
    if (chr) free(chr->genes);
    free(chr);
}


/**
 * Replace chromosome genes with genes from other chromomose.
 * Chromosomes must have same length.
 * @param  chr
 * @param  replacement
 * @return
 */
void ga_replace_chr(ga_chr chr, ga_chr replacement)
{
    assert(chr->length == replacement->length);
    memcpy(chr->genes, replacement->genes, sizeof(ga_gene_t) * chr->length);
    chr->fitness = replacement->fitness;
    chr->has_fitness = replacement->has_fitness;
}


/**
 * Mutate given chromosome
 * @param chr
 * @param max_changed_genes
 */
void ga_mutate_chr(ga_chr chr, int max_changed_genes)
{
    int genes_to_change = rand_range(0, max_changed_genes);
    for (int i = 0; i < genes_to_change; i++) {
        int gene = rand_range(0, chr->length - 1);
        ga_randomize_gene(chr, gene);
    }
    chr->has_fitness = false;
}


/**
 * Calculate fitness of given chromosome, but only if its `has_fitness`
 * attribute is set to `false`
 * @param chr
 */
ga_fitness_t ga_evaluate_chr(ga_chr chr)
{
    if (!chr->has_fitness) {
        return ga_reevaluate_chr(chr);
    } else {
        return chr->fitness;
    }
}


/**
 * Calculate fitness of given chromosome, regardless of its `has_fitness`
 * value
 * @param chr
 */
ga_fitness_t ga_reevaluate_chr(ga_chr chr)
{
    chr->fitness = (*fitness_function)(chr);
    chr->has_fitness = true;
    return chr->fitness;
}


/* population *****************************************************************/


/**
 * Create a new CGP population with given size
 * @param  size
 * @param  chromosomes_length
 * @return
 */
ga_pop ga_create_pop(int size, int chromosomes_length)
{
    ga_pop new_pop = (ga_pop) malloc(sizeof(_ga_pop));
    if (new_pop == NULL) return NULL;

    new_pop->size = size;
    new_pop->generation = 0;
    new_pop->best_chr_index = -1;
    new_pop->chromosomes = (ga_chr*) malloc(sizeof(ga_chr) * size);
    if (new_pop->chromosomes == NULL) {
        free(new_pop);
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        new_pop->chromosomes[i] = ga_create_chr(chromosomes_length);
    }

    return new_pop;
}


/**
 * Clear memory associated with given population (including its chromosomes)
 * @param pop
 */
void ga_destroy_pop(ga_pop pop)
{
    if (pop != NULL) {
        for (int i = 0; i < pop->size; i++) {
            ga_destroy_chr(pop->chromosomes[i]);
        }
        free(pop->chromosomes);
    }
    free(pop);
}


/**
 * Calculate fitness of whole population, using `ga_evaluate_chr`
 * @param chr
 */
void _ga_evaluate_pop_simple(ga_pop pop)
{
    ga_fitness_t best_fitness;
    int best_index;

    // reevaluate population
    for (int i = 0; i < pop->size; i++) {
        ga_fitness_t f = ga_evaluate_chr(pop->chromosomes[i]);
        if (i == 0 || ga_is_better_or_same(f, best_fitness)) {
            best_fitness = f;
            best_index = i;
        }
    }

    // if best index hasn't changed, try to find different one with the same fitness
    if (best_index == pop->best_chr_index) {
        for (int i = 0; i < pop->size; i++) {
            ga_fitness_t f = pop->chromosomes[i]->fitness;
            if (i != best_index && f == best_fitness) {
                best_index = i;
                break;
            }
        }
    }

    // set new best values
    pop->best_fitness = best_fitness;
    pop->best_chr_index = best_index;
}


#ifdef GA_USE_PTHREAD


/**
 * _cgp_evaluate_pop_pthread helper - thread source code
 * @param chromosome to evaluate
 */
void* _ga_evaluate_pop_pthread_worker(void *chromosome)
{
    ga_evaluate_chr((ga_chr)chromosome);
    return NULL;
}


/**
 * Calculate fitness of whole population, using `ga_evaluate_chr`
 * @param chr
 */
void _ga_evaluate_pop_pthread(ga_pop pop)
{
    ga_fitness_t best_fitness;
    int best_index;

    // reevaluate population

    pthread_t threads[pop->size];
    for (int i = 0; i < pop->size; i++) {
        pthread_create(&threads[i], NULL, _ga_evaluate_pop_pthread_worker, (void*)pop->chromosomes[i]);
    }
    for (int i = 0; i < pop->size; i++) {
        pthread_join(threads[i], NULL);
    }

    // find best fitness
    for (int i = 0; i < pop->size; i++) {
        ga_fitness_t f = pop->chromosomes[i]->fitness;
        if (i == 0 || ga_is_better_or_same(f, best_fitness)) {
            best_fitness = f;
            best_index = i;
        }
    }

    // if best index hasn't changed, try to find different one with the same fitness
    if (best_index == pop->best_chr_index) {
        for (int i = 0; i < pop->size; i++) {
            ga_fitness_t f = pop->chromosomes[i]->fitness;
            if (i != best_index && f == best_fitness) {
                best_index = i;
                break;
            }
        }
    }

    // set new best values
    pop->best_fitness = best_fitness;
    pop->best_chr_index = best_index;
}


#endif /* GA_USE_PTHREAD */


/**
 * Advance population to next generation
 * @param pop
 * @param mutation_rate
 */
void cgp_next_generation(ga_pop pop, int mutation_rate)
{
    ga_chr parent = pop->chromosomes[pop->best_chr_index];

    for (int i = 0; i < pop->size; i++) {
        if (i == pop->best_chr_index) continue;
        ga_replace_chr(pop->chromosomes[i], parent);
        ga_mutate_chr(pop->chromosomes[i], mutation_rate);
    }

    ga_evaluate_pop(pop);
    pop->generation++;
}
