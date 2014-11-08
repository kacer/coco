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
#include "algo.h"
#include "files.h"
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


// configuration
static config_t config = {
    .max_generations = 50000,
    .target_fitness = 0,
    .algorithm = predictors,

    .cgp_mutate_genes = 5,
    .cgp_population_size = 8,
    .cgp_archive_size = 10,

    .pred_size = 0.25,
    .pred_initial_size = 0,
    .pred_mutation_rate = 0.05,
    .pred_population_size = 10,
    .pred_offspring_elite = 0.25,
    .pred_offspring_combine = 0.5,
    .pred_genome_type = permuted,
    .pred_repeated_subtype = linear,

    .bw_interval = 0,
    .bw_config = {
        .algorithm = bwalg_last,
        .use_absolute_increments = false,
        .inaccuracy_tolerance = 1.2,
        .inaccuracy_coef = 2.0,
        .zero_epsilon = 0.001,
        .slow_threshold = 0.1,

        .zero_coef = 0.93,
        .decrease_coef = 0.97,
        .increase_slow_coef = 1.03,
        .increase_fast_coef = 1.00,

    },

    .bw_zero_increment_percent = -0.07,
    .bw_decrease_increment_percent = -0.03,
    .bw_increase_slow_increment_percent = 0.03,
    .bw_increase_fast_increment_percent = 0,

    .log_interval = 0,
    .log_dir = "cocolog",

    .vault_enabled = false,
    .vault_interval = 200,
};


/******************************************************************************/


int main(int argc, char *argv[])
{
    // cannot be set in initializer
    config.random_seed = rand_seed_from_time();

    // vault
    vault_storage_t vault;

    // populations and archives
    ga_pop_t cgp_population;
    ga_pop_t pred_population = NULL;
    archive_t cgp_archive = NULL;
    archive_t pred_archive = NULL;

    // baldwin
    bw_state_t baldwin_state = {};
    bw_init_history(&baldwin_state.history);

    // images
    img_image_t img_original;
    img_image_t img_noisy;

    // log files
    char best_circuit_file_name_txt[MAX_FILENAME_LENGTH + 1];
    char best_circuit_file_name_chr[MAX_FILENAME_LENGTH + 1];
    FILE *progress_log_file;
    FILE *cgp_history_log_file;

    // application exit code
    int retval = 0;


    /*
        Log system configuration
     */

    #ifdef _OPENMP
        printf("OpenMP is enabled. CPUs: %d. Max threads: %d.\n",
            omp_get_num_procs(), omp_get_max_threads());

            /*
                Nested parallelism is essential and must be explicitly
                enabled. Otherwise the population evaluation will be
                serialised.
             */
            omp_set_nested(true);

    #else

        printf("OpenMP is disabled, coevolution is not available.\n");

    #endif

    #ifdef AVX2
        if (can_use_intel_core_4th_gen_features()) {
            printf("AVX2 is enabled.\n");
        } else {
            printf("AVX2 is enabled, but not supported by CPU.\n");
        }
    #else
        printf("AVX2 is disabled. Recompile with -DAVX2 defined to enable.\n");
    #endif

    #ifdef SSE2
        if (can_use_sse2()) {
            printf("SSE2 is enabled.\n");
        } else {
            printf("SSE2 is enabled, but not supported by CPU.\n");
        }
    #else
        printf("SSE2 is disabled. Recompile with -DSSE2 defined to enable.\n");
    #endif

    /*
        Load configuration and images and init log directory and files
     */

    bool config_ok = true;
    config_retval_t cfg_status = config_load_args(argc, argv, &config);

    if (cfg_status == cfg_err) {
        fprintf(stderr, "Failed to load configuration.\n");
        config_ok = false;

    } else if (cfg_status == cfg_help) {
        return 1;
    }

    if ((img_original = img_load(config.input_image)) == NULL) {
        fprintf(stderr, "Failed to load original image or no filename given.\n");
        config_ok = false;
    }

    if ((img_noisy = img_load(config.noisy_image)) == NULL) {
        fprintf(stderr, "Failed to load noisy image or no filename given.\n");
        config_ok = false;
    }

    int log_create_dirs_retval = log_create_dirs(config.log_dir, vault.directory, MAX_FILENAME_LENGTH + 1);
    if (log_create_dirs_retval != 0) {
        fprintf(stderr, "Error initializing results directory: %s\n", strerror(log_create_dirs_retval));
        config_ok = false;
    }

    if ((progress_log_file = open_log_file(config.log_dir, "progress.log", true)) == NULL) {
        fprintf(stderr, "Failed to open 'progress.log' in results dir for writing.\n");
        config_ok = false;
    }

    if ((cgp_history_log_file = init_cgp_history_file(config.log_dir, "cgp_history.csv")) == NULL) {
        fprintf(stderr, "Failed to open 'cgp_history.csv' in results dir for writing.\n");
        config_ok = false;
    }

    if (config.pred_initial_size > config.pred_size) {
        fprintf(stderr, "Predictors' initial size cannot be larger than their full size\n");
        config_ok = false;
    }

    if (config.pred_min_size > config.pred_size) {
        fprintf(stderr, "Predictors' minimal size cannot be larger than their full size\n");
        config_ok = false;
    }

    if (config.pred_min_size > config.pred_initial_size) {
        fprintf(stderr, "Predictors' minimal size cannot be larger than their initial size\n");
        config_ok = false;
    }

    if (!config_ok) {
        fprintf(stderr, "Run %s --help or %s -h to see available options.\n", argv[0], argv[0]);
        return 1;
    }

    snprintf(best_circuit_file_name_txt, MAX_FILENAME_LENGTH + 1, "%s/best_circuit.txt", config.log_dir);
    snprintf(best_circuit_file_name_chr, MAX_FILENAME_LENGTH + 1, "%s/best_circuit.chr", config.log_dir);


    /*
        Initialize data structures etc.
     */

    // random number generator
    rand_init_seed(config.random_seed);

    // evolution
    cgp_init(config.cgp_mutate_genes, fitness_eval_or_predict_cgp);

    // vault
    if (config.vault_enabled) {
        vault_init(&vault);
    }

    // predictors population and CGP archive
    if (config.algorithm != simple_cgp) {

        // calculate absolute predictors sizes
        int img_size = img_original->width * img_original->height;
        int pred_min_size = config.pred_min_size * img_size;
        int pred_max_size = config.pred_size * img_size;
        int pred_initial_size;

        // allow to set different initial size only for baldwin or circular genotype
        bool is_circular = (config.pred_genome_type == repeated && config.pred_repeated_subtype == circular);
        if (config.pred_initial_size && (config.algorithm == baldwin || is_circular)) {
            pred_initial_size = config.pred_initial_size * img_size;
        } else {
            pred_initial_size = pred_max_size;
        }

        // predictors evolution
        pred_init(
            img_size - 1,  // max gene value
            pred_max_size,                 // max genome length
            pred_initial_size,             // initial genome length
            config.pred_mutation_rate,     // % of mutated genes
            config.pred_offspring_elite,   // % of elite children
            config.pred_offspring_combine, // % of "crossovered" children
            config.pred_genome_type,       // genome type
            config.pred_repeated_subtype   // repeated genome subtype
        );

        if (config.algorithm == baldwin) {
            // baldwin thresholds
            config.bw_config.min_length = pred_min_size;
            config.bw_config.max_length = pred_max_size;

            // baldwin absolute increments
            if (config.bw_config.use_absolute_increments) {
                config.bw_config.absolute_increment_base = pred_max_size;
                config.bw_config.zero_increment = pred_max_size * config.bw_zero_increment_percent;
                config.bw_config.decrease_increment = pred_max_size * config.bw_decrease_increment_percent;
                config.bw_config.increase_slow_increment = pred_max_size * config.bw_increase_slow_increment_percent;
                config.bw_config.increase_fast_increment = pred_max_size * config.bw_increase_fast_increment_percent;

                printf("Absolute increments are:\n"
                    "Base: %d pixels\n"
                    "Zero: %d pixels\n"
                    "Decrease: %d pixels\n"
                    "Slow increase: %d pixels\n"
                    "Fast increase: %d pixels\n",
                    config.bw_config.absolute_increment_base,
                    config.bw_config.zero_increment,
                    config.bw_config.decrease_increment,
                    config.bw_config.increase_slow_increment,
                    config.bw_config.increase_fast_increment
                );
            }
        }

        // cgp evolution
        arc_func_vect_t arc_cgp_methods = {
            .alloc_genome = cgp_alloc_genome,
            .free_genome = cgp_free_genome,
            .copy_genome = cgp_copy_genome,
            .fitness = fitness_eval_cgp,
        };
        cgp_archive = arc_create(config.cgp_archive_size, arc_cgp_methods, CGP_PROBLEM_TYPE);
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
        pred_archive = arc_create(1, arc_pred_methods, PRED_PROBLEM_TYPE);
        if (pred_archive == NULL) {
            fprintf(stderr, "Failed to initialize predictors archive.\n");
            return 1;
        }
    }

    // fitness function
    fitness_init(img_original, img_noisy, cgp_archive, pred_archive);

    /*
        Populations recovery / initialization
     */

    if (config.vault_enabled && vault_retrieve(&vault, &cgp_population) == 0) {
        printf("Population retrieved from vault.\n");

    } else {
        cgp_population = cgp_init_pop(config.cgp_population_size);
        if (cgp_population == NULL) {
            fprintf(stderr, "Failed to initialize CGP population.\n");
        }

        if (config.algorithm != simple_cgp) {
            pred_population = pred_init_pop(config.pred_population_size);
            if (pred_population == NULL) {
                fprintf(stderr, "Failed to initialize predictors population.\n");
            }
        }
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

    printf("Configuration:\n");
    config_save_file(stdout, &config);

    printf("Algorithm: %s\n", config_algorithm_names[config.algorithm]);
    printf("Stop at generation: %d\n", config.max_generations);
    printf("Initial \"PSNR\" value:         %.10g\n",
        img_psnr(img_original, img_noisy));

    printf("Evaluating CGP population...");
    ga_evaluate_pop(cgp_population);

    if (config.algorithm != simple_cgp) {
        arc_insert(cgp_archive, cgp_population->best_chromosome);

        printf("Archive: %d", cgp_archive->stored);

        printf("Evaluating PRED population...");
        ga_evaluate_pop(pred_population);
        arc_insert(pred_archive, pred_population->best_chromosome);

        printf("Best fitness: CGP %.10g, PRED %.10g",
            cgp_population->best_fitness, pred_population->best_fitness);

    } else {
        printf("Best fitness: %.10g", cgp_population->best_fitness);
    }

    save_original_image(config.log_dir, img_original);
    save_noisy_image(config.log_dir, img_noisy);
    save_config(config.log_dir, &config);


    /*
        Evolution itself
     */

    printf("Starting the big while loop.");
    log_init_time();

    // install signal handlers
    signal(SIGINT, sigint_handler);
    signal(SIGXCPU, sigxcpu_handler);

    // shared among threads
    bool finished = false;

    switch (config.algorithm) {

        case simple_cgp:
            retval = cgp_main(
                cgp_population,
                NULL,
                NULL,
                NULL,
                &config,
                &vault,
                img_noisy,
                &baldwin_state,  // required history field
                best_circuit_file_name_txt,
                best_circuit_file_name_chr,
                progress_log_file,
                cgp_history_log_file,
                &finished
            );
            break;

        case predictors:
        case baldwin:
            #pragma omp parallel
            {
                #pragma omp single
                {
                    #pragma omp task
                    {
                        retval = cgp_main(
                            cgp_population,
                            pred_population,
                            cgp_archive,
                            pred_archive,
                            &config,
                            &vault,
                            img_noisy,
                            &baldwin_state,
                            best_circuit_file_name_txt,
                            best_circuit_file_name_chr,
                            progress_log_file,
                            cgp_history_log_file,
                            &finished
                        );
                    }
                    #pragma omp task
                    {
                        pred_main(
                            cgp_population,
                            pred_population,
                            cgp_archive,
                            pred_archive,
                            &config,
                            &baldwin_state,
                            progress_log_file,
                            cgp_history_log_file,
                            &finished
                        );
                    }
                }
            }
            break;

        default:
            fprintf(stderr, "Algorithm '%s' is not supported.\n",
                config_algorithm_names[config.algorithm]);
            return -1;
    }


    /*
        Dump summary and clean-up
     */

    // dummy expression to avoid compiler warning
    printf("----\n");

    ga_fitness_t best_fitness;
    if (config.algorithm == simple_cgp) {
        best_fitness = cgp_population->best_fitness;
    } else {
        best_fitness = cgp_archive->best_chromosome_ever->fitness;
    }

    // dump best filter
    ga_chr_t best_filter;
    if (config.algorithm == simple_cgp) {
        best_filter = cgp_population->best_chromosome;
    } else {
        best_filter = cgp_archive->best_chromosome_ever;
    }

    FILE *circuit_file_txt = fopen(best_circuit_file_name_txt, "w");
    if (circuit_file_txt) {
        log_cgp_circuit(circuit_file_txt, cgp_population->generation, best_filter);
        fclose(circuit_file_txt);
    } else {
        fprintf(stderr, "Failed to open %s!\n", best_circuit_file_name_txt);
    }

    FILE *circuit_file_chr = fopen(best_circuit_file_name_chr, "w");
    if (circuit_file_chr) {
        cgp_dump_chr_compat(best_filter, circuit_file_chr);
        fclose(circuit_file_chr);
    } else {
        fprintf(stderr, "Failed to open %s!\n", best_circuit_file_name_chr);
    }

    save_best_image(config.log_dir, best_filter, img_noisy);

    // dump evolution summary
    log_final_summary(stdout, cgp_population->generation,
        best_fitness, fitness_get_cgp_evals());
    printf("\n");
    log_time(stdout);
    printf("\n");
    config_save_file(stdout, &config);

    FILE *summary_file;
    if ((summary_file = open_log_file(config.log_dir, "summary.log", false)) == NULL) {
        fprintf(stderr, "Failed to open 'summary.log' in results dir for writing.\n");

    } else {
        log_final_summary(summary_file, cgp_population->generation,
            best_fitness, fitness_get_cgp_evals());
        fprintf(summary_file, "\n");
        log_time(summary_file);
    }

    ga_destroy_pop(cgp_population);

    if (config.algorithm != simple_cgp) {
        ga_destroy_pop(pred_population);
        arc_destroy(cgp_archive);
        arc_destroy(pred_archive);
    }
    cgp_deinit();
    fitness_deinit();

    img_destroy(img_original);
    img_destroy(img_noisy);

    fclose(progress_log_file);

    return retval;
}


