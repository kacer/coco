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
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <signal.h>

#ifdef _OPENMP
  #include <omp.h>
#endif

#include "cpu.h"
#include "cgp.h"
#include "image.h"
#include "vault.h"
#include "random.h"
#include "config.h"
#include "fitness.h"
#include "archive.h"
#include "logging.h"
#include "predictors.h"

#include <limits.h>

// signal handlers
volatile sig_atomic_t interrupted = 0;
volatile sig_atomic_t cpu_limit_reached = 0;

// when SIGINT was received last time
static long interrupted_generation = -1;

// if SIGINT is received twice within this gap, program exits
const int SIGINT_GENERATIONS_GAP = 1000;

// special `check_signals` return value indicating first catch of SIGINT
const int SIGINT_FIRST = -1;


/**
 * Handles SIGINT. Sets `interrupted` flag.
 */
void sigint_handler(int _)
{
    signal(SIGINT, sigint_handler);
    interrupted = 1;
}


/**
 * Handles SIGXCPU. Sets `cpu_limit_reached` flag.
 */
void sigxcpu_handler(int _)
{
    signal(SIGXCPU, sigxcpu_handler);
    cpu_limit_reached = 1;
}


/**
 * Checks for SIGXCPU and SIGINT signals
 * @return Received signal code
 */
int check_signals(int current_generation)
{
    // SIGXCPU
    if (cpu_limit_reached) {
        DEBUGLOG("SIGXCPU received!");
        return SIGXCPU;
    }

    // SIGINT
    if (interrupted) {
        if (interrupted_generation >= 0
            &&  interrupted_generation > current_generation - SIGINT_GENERATIONS_GAP) {
            DEBUGLOG("SIGINT received and terminating!");
            return SIGINT;
        }

        DEBUGLOG("SIGINT received!");
        interrupted = 0;
        interrupted_generation = current_generation;
        return SIGINT_FIRST;
    }

    return 0;
}


/**
 * Simple CGP (no coevolution) main loop
 * @param  cgp_population
 * @param  config
 * @param  vault
 * @param  img_noisy Noisy image (to store filtered img to results)
 * @return Program return value
 */
int simple_cgp_main(
    ga_pop_t cgp_population,
    config_t *config,
    vault_storage_t *vault,
    img_image_t img_noisy
);


/**
 * Coevolutionary or simple CGP main loop
 * @param  cgp_population
 * @param  pred_population
 * @param  cgp_archive CGP archive
 * @param  pred_archive Predictors archive
 * @param  cgp_current_best Pointer to shared variable holding best CGP fitness found
 * @param  config
 * @param  vault
 * @param  img_noisy Noisy image (to store filtered img to results)
 * @param  finished Pointer to shared variable indicating that the program
 *                  should terminate
 * @return Program return value
 */
int coev_cgp_main(
    // populations
    ga_pop_t cgp_population,
    ga_pop_t pred_population,

    // archives
    archive_t cgp_archive,
    archive_t pred_archive,

    // best found fitness value
    ga_fitness_t *cgp_current_best,

    // config
    config_t *config,
    vault_storage_t *vault,

    // input
    img_image_t img_noisy,

    // status
    bool *finished
);


/**
 * Coevolutionary predictors main loop
 * @param  cgp_population
 * @param  pred_population
 * @param  pred_archive Predictors archive
 * @param  config
 * @param  finished Pointer to shared variable indicating that the program
 *                  should terminate
 */
void coev_pred_main(
    // populations
    ga_pop_t cgp_population,
    ga_pop_t pred_population,

    // archive
    archive_t pred_archive,

    // config
    config_t *config,

    // status
    bool *finished
);


/******************************************************************************/


int main(int argc, char *argv[])
{
    // configuration
    static config_t config = {
        .max_generations = 50000,
        .algorithm = predictors,

        .cgp_mutate_genes = 5,
        .cgp_population_size = 8,
        .cgp_archive_size = 10,

        .pred_size = 0.25,
        .pred_mutation_rate = 0.05,
        .pred_population_size = 10,
        .pred_offspring_elite = 0.25,
        .pred_offspring_combine = 0.5,

        .log_interval = 20,
        .results_dir = "results",

        .vault_enabled = false,
        .vault_interval = 200,
    };

    // vault
    vault_storage_t vault;

    // populations and archives
    ga_pop_t cgp_population;
    ga_pop_t pred_population;
    archive_t cgp_archive;
    archive_t pred_archive;

    // images
    img_image_t img_original;
    img_image_t img_noisy;

    // current best fitness achieved
    ga_fitness_t cgp_current_best;

    // application exit code
    int retval = 0;


    /*
        Log system configuration
     */

    #ifdef _OPENMP
        SLOWLOG("OpenMP is enabled. CPUs: %d. Max threads: %d.",
            omp_get_num_procs(), omp_get_max_threads());

            /*
                Nested parallelism is essential and must be explicitly
                enabled. Otherwise the population evaluation will be
                serialised.
             */
            omp_set_nested(true);

    #else

        SLOWLOG("OpenMP is disabled, coevolution is not available.");

    #endif

    #ifdef FITNESS_AVX
        if (can_use_intel_core_4th_gen_features()) {
            SLOWLOG("AVX2 is enabled.")
        } else {
            SLOWLOG("AVX2 is enabled, but not supported by CPU.")
        }
    #else
        SLOWLOG("AVX2 is disabled. Recompile with FITNESS_AVX defined to enable.");
    #endif


    /*
        Load configuration
     */


    if (config_load_args(argc, argv, &config) != 0) {
        fprintf(stderr, "Failed to load configuration.\n");
        return 1;
    }

    if ((img_original = img_load(config.input_image)) == NULL) {
        fprintf(stderr, "Failed to load original image or no filename given.\n");
        return 1;
    }

    if ((img_noisy = img_load(config.noisy_image)) == NULL) {
        fprintf(stderr, "Failed to load noisy image or no filename given.\n");
        return 1;
    }


    /*
        Initialize data structures etc.
     */

    // random number generator
    rand_init();

    // evolution
    cgp_init(config.cgp_mutate_genes, fitness_eval_or_predict_cgp);
    int img_size = img_original->width * img_original->height;
    pred_init(
        img_size - 1,  // max gene value
        config.pred_size * img_size,   // max genome length
        config.pred_size * img_size,   // initial genome length
        config.pred_mutation_rate,     // % of mutated genes
        config.pred_offspring_elite,   // % of elite children
        config.pred_offspring_combine  // % of "crossovered" children
    );

    // vault
    if (config.vault_enabled) {
        vault_init(&vault);
    }

    // CGP archive
    arc_func_vect_t arc_cgp_methods = {
        .alloc_genome = cgp_alloc_genome,
        .free_genome = cgp_free_genome,
        .copy_genome = cgp_copy_genome,
        .fitness = fitness_eval_cgp,
    };
    cgp_archive = arc_create(config.cgp_archive_size, arc_cgp_methods);
    if (cgp_archive == NULL) {
        fprintf(stderr, "Failed to initialize CGP archive.\n");
        return 1;
    }

    // predictor archive
    arc_func_vect_t arc_pred_methods = {
        .alloc_genome = pred_alloc_genome,
        .free_genome = pred_free_genome,
        .copy_genome = pred_copy_genome,
        .fitness = NULL,
    };
    pred_archive = arc_create(1, arc_pred_methods);
    if (pred_archive == NULL) {
        fprintf(stderr, "Failed to initialize predictors archive.\n");
        return 1;
    }

    // fitness function
    fitness_init(img_original, img_noisy, cgp_archive, pred_archive);


    /*
        Populations recovery / initialization
     */

    if (config.vault_enabled && vault_retrieve(&vault, &cgp_population) == 0) {
        SLOWLOG("Population retrieved from vault.");

    } else {
        cgp_population = cgp_init_pop(config.cgp_population_size);
        pred_population = pred_init_pop(config.pred_population_size);
    }


    /*
        Log initial info
     */

    #ifndef _OPENMP
        if (config.algorithm != simple_cgp) {
            fprintf(stderr, "Only simple CGP is available.\n");
            fprintf(stderr, "Please recompile program with OpenMP (-fopenmp for gcc) to run coevolution.\n");
            return 1;
        }
    #endif

    SLOWLOG("Algorithm: %s", config_algorithm_names[config.algorithm]);
    SLOWLOG("Stop at generation: %d", config.max_generations);
    SLOWLOG("Initial PSNR value:           %.10g",
        fitness_psnr(img_original, img_noisy));

    DEBUGLOG("Evaluating CGP population...");
    ga_evaluate_pop(cgp_population);
    arc_insert(cgp_archive, cgp_population->best_chromosome);

    DEBUGLOG("Evaluating PRED population...");
    ga_evaluate_pop(pred_population);
    arc_insert(pred_archive, pred_population->best_chromosome);

    DEBUGLOG("Best fitness: CGP %.10g, PRED %.10g",
        cgp_population->best_fitness, pred_population->best_fitness);

    print_progress(cgp_population, pred_population, pred_archive);
    save_original_image(config.results_dir, img_original);
    save_noisy_image(config.results_dir, img_noisy);
    save_config(config.results_dir, &config);


    /*
        Evolution itself
     */

    DEBUGLOG("Starting the big while loop.");

    bool finished = false;
    cgp_current_best = cgp_population->best_fitness;

    switch (config.algorithm) {

        case simple_cgp:
            // install signal handlers
            signal(SIGINT, sigint_handler);
            signal(SIGXCPU, sigxcpu_handler);

            retval = simple_cgp_main(cgp_population, &config, &vault, img_noisy);
            goto cleanup;

        case predictors:
            #pragma omp parallel sections num_threads(2)
            {
                #pragma omp section
                {
                    // install signal handlers only in this thread
                    signal(SIGINT, sigint_handler);
                    signal(SIGXCPU, sigxcpu_handler);

                    retval = coev_cgp_main(
                        cgp_population,
                        pred_population,
                        cgp_archive,
                        pred_archive,
                        &cgp_current_best,
                        &config,
                        &vault,
                        img_noisy,
                        &finished
                    );
                }
                #pragma omp section
                {
                    coev_pred_main(
                        cgp_population,
                        pred_population,
                        pred_archive,
                        &config,
                        &finished
                    );
                }
            }
            goto cleanup;

        default:
            fprintf(stderr, "Algorithm '%s' is not supported yet.\n",
                config_algorithm_names[config.algorithm]);
            return -1;

    }


    /*
        Clean-up
     */


cleanup:

    ga_destroy_pop(cgp_population);
    ga_destroy_pop(pred_population);
    arc_destroy(cgp_archive);
    arc_destroy(pred_archive);
    cgp_deinit();
    fitness_deinit();

    img_destroy(img_original);
    img_destroy(img_noisy);

    return retval;
}


/******************************************************************************/


/**
 * Simple CGP (no coevolution) main loop
 * @param  cgp_population
 * @param  config
 * @param  vault
 * @param  img_noisy Noisy image (to store filtered img to results)
 * @return Program return value
 */
int simple_cgp_main(
    ga_pop_t cgp_population,
    config_t *config,
    vault_storage_t *vault,
    img_image_t img_noisy)
{
    while (cgp_population->generation < config->max_generations) {

        VERBOSELOG("One iteration of CGP.");

        // create children and evaluate new generation
        ga_next_generation(cgp_population);

        // log progress
        if ((cgp_population->generation % config->log_interval) == 0) {
            print_cgp_progress(cgp_population);
        }

        int signal = check_signals(cgp_population->generation);
        bool store_now = (cgp_population->generation % config->vault_interval) == 0;

        // store to vault
        if (signal || store_now) {
            print_results(cgp_population, NULL, NULL);
            save_filtered_image(config->results_dir, cgp_population, img_noisy);
            if (config->vault_enabled) {
                vault_store(vault, cgp_population);
            }
            SLOWLOG("Current output image and state stored.");
        }

        if (signal > 0) return signal;
    }

    return 0;
}



/**
 * Coevolutionary CGP main loop
 * @param  cgp_population
 * @param  pred_population
 * @param  cgp_archive CGP archive
 * @param  pred_archive Predictors archive
 * @param  cgp_current_best Pointer to shared variable holding best CGP fitness found
 * @param  pred_current_best Pointer to shared variable holding best predictor fitness found
 * @param  config
 * @param  vault
 * @param  img_noisy Noisy image (to store filtered img to results)
 * @param  finished Pointer to shared variable indicating that the program
 *                  should terminate
 * @return Program return value
 */
int coev_cgp_main(
    // populations
    ga_pop_t cgp_population,
    ga_pop_t pred_population,

    // archives
    archive_t cgp_archive,
    archive_t pred_archive,

    // best found fitness value
    ga_fitness_t *cgp_current_best,

    // config
    config_t *config,
    vault_storage_t *vault,

    // input
    img_image_t img_noisy,

    // status
    bool *finished)
{
    while (!(*finished)) {
        VERBOSELOG("One iteration of CGP.");

        // create children and evaluate new generation
        ga_create_children(cgp_population);

        #pragma omp critical (PRED_ARCHIVE)
        {
            ga_evaluate_pop(cgp_population);
        }

        if (ga_is_better(cgp_population->problem_type, cgp_population->best_fitness, *cgp_current_best)) {
            // log
            print_cgp_progress(cgp_population);
            log_cgp_change(*cgp_current_best, cgp_population->best_fitness);

            // store and recalculate predictors fitness
            ga_chr_t archived;
            #pragma omp critical (CGP_ARCHIVE)
            {
                archived = arc_insert(cgp_archive, cgp_population->best_chromosome);
                ga_invalidate_fitness(pred_population);
                ga_reevaluate_chr(pred_population, arc_get(pred_archive, 0));
            }

            SLOWLOG("Predicted fitness: %.20g", cgp_population->best_fitness);
            SLOWLOG("Real fitness: %.20g", archived->fitness);
            *cgp_current_best = cgp_population->best_fitness;

            // drop fitness values
            ga_invalidate_fitness(pred_population);
            ga_reevaluate_chr(pred_population, arc_get(pred_archive, 0));

        }

        // log progress
        if ((cgp_population->generation % config->log_interval) == 0) {
            print_cgp_progress(cgp_population);
        }

        int signal = check_signals(cgp_population->generation);
        bool store_now = (cgp_population->generation % config->vault_interval) == 0;

        // last generation?
        if (cgp_population->generation >= config->max_generations) {
            SLOWLOG("Generations limit reached (%d), terminating.", config->max_generations);
            *finished = true;
        }

        // store to vault
        if (*finished || signal || store_now) {
            print_results(cgp_population, pred_population, pred_archive);
            save_filtered_image(config->results_dir, cgp_population, img_noisy);
            if (config->vault_enabled) {
                // freeze predictors archive during save
                #pragma omp critical (PRED_ARCHIVE)
                {
                    vault_store(vault, cgp_population);
                }
            }
            SLOWLOG("Current output image and state stored.");
        }

        if (signal > 0) {
            // stop other threads
            *finished = true;
            return signal;
        }
    }

    return 0;
}


/**
 * Coevolutionary predictors main loop
 * @param  cgp_population
 * @param  pred_population
 * @param  pred_archive Predictors archive
 * @param  config
 * @param  finished Pointer to shared variable indicating that the program
 *                  should terminate
 */
void coev_pred_main(
    // populations
    ga_pop_t cgp_population,
    ga_pop_t pred_population,

    // archive
    archive_t pred_archive,

    // config
    config_t *config,

    // status
    bool *finished)
{
    while (!(*finished)) {
        VERBOSELOG("One iteration of predictors.");

        ga_create_children(pred_population);
        #pragma omp critical (CGP_ARCHIVE)
        {
            ga_evaluate_pop(pred_population);
        }

        // log progress
        if ((pred_population->generation % config->log_interval) == 0) {
            print_pred_progress(pred_population, pred_archive);
        }

        // copy to archive
        if (ga_is_better(pred_population->problem_type, pred_population->best_fitness, arc_get(pred_archive, 0)->fitness)) {
            // log
            print_pred_progress(pred_population, pred_archive);
            log_pred_change(arc_get(pred_archive, 0)->fitness, pred_population->best_fitness);

            // store and invalidate CGP fitness
            #pragma omp critical (PRED_ARCHIVE)
            {
                arc_insert(pred_archive, pred_population->best_chromosome);
                ga_invalidate_fitness(cgp_population);
            }
        }
    }
}
