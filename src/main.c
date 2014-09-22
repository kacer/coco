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
#include "predictors.h"

#include <limits.h>

// signal handlers
volatile sig_atomic_t interrupted = 0;
volatile sig_atomic_t cpu_limit_reached = 0;


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
 * Saves image filtered by best filter to results directory
 * @param cgp_population
 * @param noisy
 */
void save_image(ga_pop_t cgp_population, img_image_t noisy)
{
    char filename[25 + 8 + 1];
    snprintf(filename, 25 + 8 + 1, "results/img_filtered_%08d.bmp", cgp_population->generation);
    img_image_t filtered = fitness_filter_image(cgp_population->best_chromosome);
    img_save_bmp(filtered, filename);
    img_destroy(filtered);
}


/* standard console outputing */

#ifdef DEBUG
    #define DEBUGLOG(...) { printf(__VA_ARGS__); printf("\n"); }
#else
    #define DEBUGLOG(...)
#endif

#define SLOWLOG(...) { printf(__VA_ARGS__); printf("\n"); }


void print_progress(ga_pop_t cgp_population, ga_pop_t pred_population,
    archive_t pred_archive)
{
    printf("CGP generation %4d: best fitness %.20g\t\t",
        cgp_population->generation, cgp_population->best_fitness);
    printf("PRED generation %4d: best fitness %.20g (archived %.20g)\n",
        pred_population->generation, pred_population->best_fitness,
        arc_get(pred_archive, 0)->fitness);
}


void print_results(ga_pop_t cgp_population, ga_pop_t pred_population,
    archive_t pred_archive)
{
    print_progress(cgp_population, pred_population, pred_archive);

    printf("\n"
           "Best predictor\n"
           "--------------\n");
    pred_dump_chr(pred_population->best_chromosome, stdout);

    printf("\n"
           "Best circuit\n"
           "------------\n");
    cgp_dump_chr_asciiart(cgp_population->best_chromosome, stdout);
    printf("\n");
}


#define SAVE_IMAGE_AND_STATE() { \
    print_results(cgp_population, pred_population, pred_archive); \
    save_image(cgp_population, img_noisy); \
    if (config.vault_enabled) vault_store(&vault, cgp_population); \
    SLOWLOG("Current output image and state stored."); \
}


int main(int argc, char *argv[])
{
    // application return code
    int retval = 0;

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
    ga_fitness_t pred_current_best;

    // stored when SIGINT was received last time
    long interrupted_generation = -1;


    /*
        Load configuration
     */


    retval = config_load_args(argc, argv, &config);
    if (retval != 0) {
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


    // signal handlers
    signal(SIGINT, sigint_handler);
    signal(SIGXCPU, sigxcpu_handler);

    // random number generator
    rand_init();

    // evolution
    cgp_init(config.cgp_mutate_genes, fitness_eval_or_predict_cgp);
    int img_size = img_original->width * img_original->height;
    pred_init(
        img_size - 1,  // max gene value
        img_size,  // max genome length
        config.pred_size * img_size,  // initial genome length
        config.pred_mutation_rate,  // % of mutated genes
        config.pred_offspring_elite,  // % of elite children
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
        Evolution recovery / initialization
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


    #ifdef _OPENMP
        SLOWLOG("OpenMP is enabled. CPUs: %d. Max threads: %d.",
            omp_get_num_procs(), omp_get_max_threads());
    #endif

    #ifdef FITNESS_AVX
        if (can_use_intel_core_4th_gen_features()) {
            SLOWLOG("AVX2 is enabled.")
        } else {
            SLOWLOG("AVX2 is enabled, but not supported by CPU.")
        }
    #endif

    SLOWLOG("Mutation rate: %d genes max", config.cgp_mutate_genes);
    SLOWLOG("Stop at generation: %d", config.max_generations);
    SLOWLOG("Initial PSNR value:           %.20g",
        fitness_psnr(img_original, img_noisy));

    DEBUGLOG("Evaluating CGP population...");
    ga_evaluate_pop(cgp_population);
    arc_insert(cgp_archive, cgp_population->best_chromosome);

    DEBUGLOG("Evaluating PRED population...");
    ga_evaluate_pop(pred_population);
    arc_insert(pred_archive, pred_population->best_chromosome);

    DEBUGLOG("Best fitness: CGP %.20g, PRED %.20g",
        cgp_population->best_fitness, pred_population->best_fitness);

    print_progress(cgp_population, pred_population, pred_archive);
    img_save_bmp(img_original, "results/img_original.bmp");
    img_save_bmp(img_noisy, "results/img_noisy.bmp");
    cgp_current_best = cgp_population->best_fitness;
    pred_current_best = pred_population->best_fitness;


    /*
        Evolution itself
     */


    if (interrupted) {
        // SIGINT received during initialization
        retval = -SIGINT;
        goto cleanup;
    }


    while (cgp_population->generation < config.max_generations) {

        // next generation
        for (int _ = 0; _ < 4; _++) {
            ga_next_generation(cgp_population);
        }
        ga_next_generation(pred_population);

        // log progress
        if ((cgp_population->generation % config.log_interval) == 0) {
            print_progress(cgp_population, pred_population, pred_archive);
        }

        // check SIGXCPU
        if (cpu_limit_reached) {
            SAVE_IMAGE_AND_STATE();
            retval = -SIGXCPU;
            goto cleanup;
        }

        // store to vault
        if ((cgp_population->generation % config.vault_interval) == 0) {
            SAVE_IMAGE_AND_STATE();
        }

        // copy to archive
        bool cgp_better = ga_is_better(cgp_population->problem_type,
            cgp_population->best_fitness, cgp_current_best);
        bool pred_better = ga_is_better(pred_population->problem_type,
            pred_population->best_fitness, arc_get(pred_archive, 0)->fitness);

        if (cgp_better) {
            print_progress(cgp_population, pred_population, pred_archive);
            SLOWLOG("Best CGP circuit changed by %.20g, moving to archive",
                cgp_population->best_fitness - cgp_current_best);
            ga_chr_t best = cgp_population->best_chromosome;
            ga_chr_t archived = arc_insert(cgp_archive, best);
            SLOWLOG("Predicted fitness: %.20g", best->fitness);
            SLOWLOG("Real fitness: %.20g", archived->fitness);
            cgp_current_best = cgp_population->best_fitness;

            // drop fitness values
            ga_invalidate_fitness(pred_population);
            ga_reevaluate_chr(pred_population, arc_get(pred_archive, 0));
            pred_current_best = ga_worst_fitness(pred_population->problem_type);
        }

        if (pred_better)
        {
            print_progress(cgp_population, pred_population, pred_archive);
            SLOWLOG("                                                       \t\t"
                "Best predictor changed by %.20g, moving to archive",
                pred_population->best_fitness - pred_current_best);
            ga_chr_t best = pred_population->best_chromosome;
            ga_chr_t archived = arc_insert(pred_archive, best);
            SLOWLOG("                                                       \t\t"
                "Fitness: %.20g", archived->fitness);
            pred_current_best = pred_population->best_fitness;
            ga_invalidate_fitness(cgp_population);
        }

        // SIGINT
        if (interrupted) {

            if (interrupted_generation >= 0 && interrupted_generation > cgp_population->generation - 1000) {
                retval = -SIGINT;
                goto cleanup;
            }

            SAVE_IMAGE_AND_STATE();

            interrupted = 0;
            interrupted_generation = cgp_population->generation;
        }

    } // while loop

    SAVE_IMAGE_AND_STATE();


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
