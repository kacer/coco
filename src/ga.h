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
 * Genome allocation/initialization function
 * @param  chromosome
 * @return
 */
typedef int (*ga_init_func_t)(ga_chr_t chromosome);


/**
 * Genome deallocation/deinitialization function
 * @param  chromosome
 * @return
 */
typedef void (*ga_deinit_func_t)(ga_chr_t chromosome);


/**
 * Fitness function
 */
typedef ga_fitness_t (*ga_fitness_func_t)(ga_chr_t chromosome);


/**
 * Genome mutation function
 * @param  chromosome
 * @return
 */
typedef void (*ga_mutate_func_t)(ga_chr_t chromosome);


/**
 * Population offspring generator function - should modify chromosomes in place
 * @param  parent chromosomes
 * @return
 */
typedef void (*ga_offspring_func_t)(ga_pop_t population);


/**
 * Genome crossover function
 * @param  child
 * @param  mom
 * @param  dad
 * @return
 */
typedef void (*ga_crossover_func_t)(ga_chr_t child, ga_chr_t mom, ga_chr_t dad);


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
    /* required */
    ga_init_func_t init;
    ga_deinit_func_t deinit;
    ga_fitness_func_t fitness;
    ga_offspring_func_t offspring;

    /* at least one of "mutate", "crossover" must be set */
    ga_mutate_func_t mutate;
    ga_crossover_func_t crossover;
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
 * Create a new CGP population with given size
 * @param  size
 * @param  problem-specific methods
 * @return
 */
ga_pop_t ga_create_pop(int size, ga_problem_type_t type, ga_func_vect_t methods);


/**
* Clear memory associated with given population (including its chromosomes)
* @param pop
*/
void ga_destroy_pop(ga_pop_t pop);


/**
 * Mutate given chromosome
 * @param pop
 * @param chr
 */
void ga_mutate_chr(ga_pop_t pop, ga_chr_t chr);


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


#ifdef GA_USE_PTHREAD

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
