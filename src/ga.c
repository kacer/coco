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


#define GA_USE_PTHREAD

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef GA_USE_PTHREAD
    #include <pthread.h>
#endif

#include "ga.h"
#include "random.h"


/* population *****************************************************************/


/**
 * Create a new CGP population with given size
 * @param  size
 * @param  chromosomes_length
 * @return
 */
ga_pop_t ga_create_pop(int size, ga_problem_type_t type, ga_func_vect_t methods)
{
    assert(methods.init != NULL);
    assert(methods.deinit != NULL);
    assert(methods.fitness != NULL);
    assert(methods.offspring != NULL);
    assert(methods.crossover != NULL || methods.mutate != NULL);

    ga_pop_t new_pop = (ga_pop_t) malloc(sizeof(struct ga_pop));
    if (new_pop == NULL) {
        return NULL;
    }

    /* general attributes */
    new_pop->size = size;
    new_pop->generation = 0;
    new_pop->problem_type = type;
    new_pop->methods = methods;
    new_pop->best_chr_index = -1;

    /* allocate chromosome array */
    new_pop->chromosomes = (ga_chr_t*) malloc(sizeof(ga_chr_t) * size);
    if (new_pop->chromosomes == NULL) {
        free(new_pop);
        return NULL;
    }

    /* initialize chromosomes */
    for (int i = 0; i < size; i++) {
        new_pop->chromosomes[i] = (ga_chr_t) malloc(sizeof(struct ga_chr));
        int retval = new_pop->methods.init(new_pop->chromosomes[i]);
        if (retval != 0) {
           for (int x = i - 1; x >= 0; x--) {
               new_pop->methods.deinit(new_pop->chromosomes[i]);
           }
           return NULL;
        }
    }

    return new_pop;
}


/**
 * Clear memory associated with given population (including its chromosomes)
 * @param pop
 */
void ga_destroy_pop(ga_pop_t pop)
{
    if (pop != NULL) {
        for (int i = 0; i < pop->size; i++) {
            pop->methods.deinit(pop->chromosomes[i]);
        }
        free(pop->chromosomes);
    }
    free(pop);
}


/* chromosome *****************************************************************/


/**
 * Mutate given chromosome
 * @param pop
 * @param chr
 */
void ga_mutate_chr(ga_pop_t pop, ga_chr_t chr)
{
    if (pop->methods.mutate != NULL) {
        pop->methods.mutate(chr);
        chr->has_fitness = false;
    }
}


/**
 * Calculate fitness of given chromosome, but only if its `has_fitness`
 * attribute is set to `false`
 * @param pop
 * @param chr
 */
ga_fitness_t ga_evaluate_chr(ga_pop_t pop, ga_chr_t chr)
{
    if (!chr->has_fitness) {
        return ga_reevaluate_chr(pop, chr);
    } else {
        return chr->fitness;
    }
}


/**
 * Calculate fitness of given chromosome, regardless of its `has_fitness`
 * value
 * @param pop
 * @param chr
 */
ga_fitness_t ga_reevaluate_chr(ga_pop_t pop, ga_chr_t chr)
{
    chr->fitness = pop->methods.fitness(chr);
    chr->has_fitness = true;
    return chr->fitness;
}


/* fitness calculation ********************************************************/


/**
 * Calculate fitness of whole population, using `ga_evaluate_chr`
 * @param chr
 */
void _ga_evaluate_pop_simple(ga_pop_t pop)
{
    ga_fitness_t best_fitness;
    int best_index;

    // reevaluate population
    for (int i = 0; i < pop->size; i++) {
        ga_fitness_t f = ga_evaluate_chr(pop, pop->chromosomes[i]);
        if (i == 0 || ga_is_better_or_same(pop->problem_type, f, best_fitness)) {
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


    typedef struct
    {
        ga_pop_t pop;
        int chr_index;
    } _thread_job;


    /**
     * _cgp_evaluate_pop_pthread helper - thread source code
     * @param chromosome to evaluate
     */
    void *_ga_evaluate_pop_pthread_worker(void *_job)
    {
        _thread_job *job = (_thread_job*) _job;
        ga_evaluate_chr(job->pop, job->pop->chromosomes[job->chr_index]);
        return NULL;
    }


    /**
     * Calculate fitness of whole population, using `ga_evaluate_chr`
     * @param chr
     */
    void _ga_evaluate_pop_pthread(ga_pop_t pop)
    {
        ga_fitness_t best_fitness;
        int best_index;

        /* reevaluate population */

        _thread_job jobs[pop->size];
        pthread_t threads[pop->size];

        for (int i = 0; i < pop->size; i++) {
            jobs[i].pop = pop;
            jobs[i].chr_index = i;
            pthread_create(&threads[i], NULL, _ga_evaluate_pop_pthread_worker, (void*)&jobs[i]);
        }
        for (int i = 0; i < pop->size; i++) {
            pthread_join(threads[i], NULL);
        }

        /* find best fitness */
        for (int i = 0; i < pop->size; i++) {
            ga_fitness_t f = pop->chromosomes[i]->fitness;
            if (i == 0 || ga_is_better_or_same(pop->problem_type, f, best_fitness)) {
                best_fitness = f;
                best_index = i;
            }
        }

        /* if best index hasn't changed,
           try to find different one with the same fitness */
        if (best_index == pop->best_chr_index) {
            for (int i = 0; i < pop->size; i++) {
                ga_fitness_t f = pop->chromosomes[i]->fitness;
                if (i != best_index && f == best_fitness) {
                    best_index = i;
                    break;
                }
            }
        }

        /* set new best values */
        pop->best_fitness = best_fitness;
        pop->best_chr_index = best_index;
    }


#endif /* GA_USE_PTHREAD */



/**
 * Advance population to next generation
 * @param pop
 * @param mutation_rate
 */
void ga_next_generation(ga_pop_t pop)
{
    pop->methods.offspring(pop);
    ga_evaluate_pop(pop);
    pop->generation++;
}
