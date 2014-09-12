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


/* population *****************************************************************/


/**
 * Create a new CGP population with given size. The genomes are not
 * initialized at this point.
 *
 * @param  size
 * @param  chromosomes_length
 * @return
 */
ga_pop_t ga_create_pop(int size, ga_problem_type_t type, ga_func_vect_t methods)
{
    /* only alloc/free are required for initialization */
    assert(methods.alloc_genome != NULL);
    assert(methods.free_genome != NULL);

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
        ga_chr_t new_chr = ga_alloc_chr(methods.alloc_genome);

        if (new_chr == NULL) {
            for (int x = i - 1; x >= 0; x--) {
                ga_free_chr(new_pop->chromosomes[x], methods.free_genome);
            }
            free(new_pop);
            return NULL;
        }
        new_pop->chromosomes[i] = new_chr;
    }

    return new_pop;
}


/**
 * Initializes population to random chromosomes
 * @param pop
 */
int ga_init_pop(ga_pop_t pop)
{
    assert(pop->methods.init_genome != NULL);

    for (int i = 0; i < pop->size; i++) {
        int retval = pop->methods.init_genome(pop->chromosomes[i]);
        if (retval != 0) {
            return retval;
        }
    }
    return 0;
}


/**
 * Sets method vector
 */
void ga_set_methods(ga_pop_t pop, ga_func_vect_t methods)
{
    assert(methods.alloc_genome != NULL);
    assert(methods.free_genome != NULL);
    assert(methods.init_genome != NULL);
    assert(methods.fitness != NULL);
    assert(methods.offspring != NULL);

    pop->methods = methods;
}


/**
 * Clear memory associated with given population (including its chromosomes)
 * @param pop
 */
void ga_destroy_pop(ga_pop_t pop)
{
    if (pop != NULL) {
        for (int i = 0; i < pop->size; i++) {
            pop->methods.free_genome(pop->chromosomes[i]->genome);
            free(pop->chromosomes[i]);
        }
        free(pop->chromosomes);
    }
    free(pop);
}


/* chromosome *****************************************************************/


/**
 * Allocates memory for chromosome
 *
 * @param  problem-specific genome allocation function
 * @return pointer to allocated chromosome
 */
ga_chr_t ga_alloc_chr(ga_alloc_genome_func_t alloc_func)
{
    ga_chr_t new_chr = (ga_chr_t) malloc(sizeof(struct ga_chr));
    if (new_chr == NULL) {
        return NULL;
    }
    void *new_genome = alloc_func();
    if (new_genome == NULL) {
        free(new_chr);
        return NULL;
    }

    new_chr->has_fitness = false;
    new_chr->genome = new_genome;
    return new_chr;
}


/**
 * De-allocates memory for chromosome
 *
 * @param  problem-specific genome de-allocation function
 */
void ga_free_chr(ga_chr_t chr, ga_free_genome_func_t free_func)
{
    free_func(chr->genome);
    free(chr);
}


/**
 * Copies `src` chromosome to `dst`.
 *
 * @param  problem-specific genome copying function
 */
void ga_copy_chr(ga_chr_t dst, ga_chr_t src, ga_copy_genome_func_t copy_func)
{
    copy_func(dst->genome, src->genome);
    dst->has_fitness = src->has_fitness;
    dst->fitness = src->fitness;
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
    assert(pop->methods.fitness != NULL);
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
