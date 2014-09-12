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


/**
 * Fitness value
 */
typedef double ga_fitness_t;


/**
 * Chromosome
 */
struct ga_chr {
    bool has_fitness;
    ga_fitness_t fitness;
    void *genome;
};
typedef struct ga_chr* ga_chr_t;


/* Required to solve circular dependency */
struct ga_pop;
typedef struct ga_pop* ga_pop_t;


/**
 * Genome allocation function
 *
 * This function should allocate all required memory and should not
 * perform initialization of any kind (e.g. generate random genes).
 *
 * @param  chromosome
 * @return
 */
typedef void* (*ga_alloc_genome_func_t)();


/**
 * Genome deallocation function.
 *
 * This function should release any memory resources allocated by
 * ga_free_genome_func_t.
 *
 * @param  chromosome
 * @return pointer to allocated genome
 */
typedef void (*ga_free_genome_func_t)(void *genome);


/**
 * Genome initialization function
 *
 * This function should initialize the genome, i.e. generate random
 * genes.
 *
 * @param  chromosome
 * @return 0 on success, any other value on error
 */
typedef int (*ga_init_genome_func_t)(ga_chr_t chromosome);


/**
 * Genome copy function.
 *
 * This function should copy genes from `src` to `dst`.
 *
 * @param  chromosome
 */
typedef void (*ga_copy_genome_func_t)(void *dst, void* src);


/**
 * Fitness function
 *
 * This function should calculate given chromosome (genome) fitness.
 * It must not skip calculation even if `has_fitness` attribute is set
 * to `true`.
 *
 * @param  chromosome
 * @return fitness value associated to given chromosome
 */
typedef ga_fitness_t (*ga_fitness_func_t)(ga_chr_t chromosome);


/**
 * New generation population generator function
 *
 * This function should modify given population chromosomes in place.
 * Parent selection, crossover, mutation, etc. happens here.
 *
 * It is important to reset `has_fitness` attributes, if genome changes!
 *
 * @param  parent chromosomes
 * @return
 */
typedef void (*ga_offspring_func_t)(ga_pop_t population);



/**
 * GA problem type
 */
typedef enum {
    minimize,
    maximize,
} ga_problem_type_t;


/**
 * User-defined methods
 */
typedef struct {
    /* memory allocation */
    ga_alloc_genome_func_t alloc_genome;
    ga_free_genome_func_t free_genome;

    /* random genome initialization */
    ga_init_genome_func_t init_genome;

    /* fitness function */
    ga_fitness_func_t fitness;

    /* children generator */
    ga_offspring_func_t offspring;
} ga_func_vect_t;


/**
 * Population
 */
struct ga_pop {
    /* basic info */
    int size;
    int generation;

    /* evolution settings */
    ga_problem_type_t problem_type;
    ga_func_vect_t methods;

    /* chromosomes */
    ga_fitness_t best_fitness;
    int best_chr_index;
    ga_chr_t *chromosomes;
};


/**
 * Create a new CGP population with given size. The genomes are not
 * initialized at this point.
 *
 * @param  size
 * @param  chromosomes_length
 * @return
 */
ga_pop_t ga_create_pop(int size, ga_problem_type_t type, ga_func_vect_t methods);


/**
 * Initializes population to random chromosomes
 * @param pop
 */
int ga_init_pop(ga_pop_t pop);


/**
 * Sets method vector
 */
void ga_set_methods(ga_pop_t pop, ga_func_vect_t methods);


/**
* Clear memory associated with given population (including its chromosomes)
* @param pop
*/
void ga_destroy_pop(ga_pop_t pop);


/**
 * Allocates memory for chromosome
 *
 * @param  problem-specific genome allocation function
 * @return pointer to allocated chromosome
 */
ga_chr_t ga_alloc_chr(ga_alloc_genome_func_t alloc_func);


/**
 * De-allocates memory for chromosome
 *
 * @param  problem-specific genome de-allocation function
 */
void ga_free_chr(ga_chr_t chr, ga_free_genome_func_t free_func);


/**
 * Copies `src` chromosome to `dst`.
 *
 * @param  problem-specific genome copying function
 */
void ga_copy_chr(ga_chr_t dst, ga_chr_t src, ga_copy_genome_func_t copy_func);


/**
 * Calculate fitness of given chromosome, but only if its `has_fitness`
 * attribute is set to `false`
 * @param pop
 * @param chr
 */
ga_fitness_t ga_evaluate_chr(ga_pop_t pop, ga_chr_t chr);


/**
 * Calculate fitness of given chromosome, regardless of its `has_fitness`
 * value
 * @param pop
 * @param chr
 */
ga_fitness_t ga_reevaluate_chr(ga_pop_t pop, ga_chr_t chr);


/**
 * Returns true if WHAT is better or same as COMPARED_TO
 * @param  what
 * @param  compared_to
 * @return
 */
static inline bool ga_is_better_or_same(ga_problem_type_t type, ga_fitness_t what, ga_fitness_t compared_to) {
    return (type == minimize)? what <= compared_to : what >= compared_to;
}


/**
 * Calculate fitness of whole population, using `ga_evaluate_chr`
 * in single thread
 * @param chr
 */
void _ga_evaluate_pop_simple(ga_pop_t pop);


/**
 * Calculate fitness of whole population, using `ga_evaluate_chr`
 * using one thread per chromosome
 * @param chr
 */
void _ga_evaluate_pop_pthread(ga_pop_t pop);


#if !defined(_OPENMP) && defined(GA_USE_PTHREAD)

    static inline void ga_evaluate_pop(ga_pop_t pop) {
        _ga_evaluate_pop_pthread(pop);
    }

#else

    static inline void ga_evaluate_pop(ga_pop_t pop) {
        _ga_evaluate_pop_simple(pop);
    }

#endif


/**
 * Advance population to next generation
 * @param pop
 * @param mutation_rate
 */
void ga_next_generation(ga_pop_t pop);
