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

#include <sys/time.h>
#include <sys/resource.h>

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
 * Calculates diff of two `struct timeval`
 *
 * http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 *
 * @param  result
 * @param  x
 * @param  y
 * @return
 */
int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y);


/**
 * Prints `struct timeval` value in "12m34.567s" format
 * @param fp
 * @param time
 */
void fprint_timeval(FILE *fp, struct timeval *time);


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


/******************************************************************************/


int main(int argc, char *argv[])
{
    // configuration
    static config_t config = {
        .max_generations = 50000,
        .target_fitness = 0,
        .algorithm = predictors,

        .cgp_mutate_genes = 5,
        .cgp_population_size = 8,
        .cgp_archive_size = 10,

        .pred_size = 0.25,
        .pred_mutation_rate = 0.05,
        .pred_population_size = 10,
        .pred_offspring_elite = 0.25,
        .pred_offspring_combine = 0.5,
        .pred_genome_type = permuted,

        .log_interval = 0,
        .log_dir = "cocolog",

        .vault_enabled = false,
        .vault_interval = 200,
    };

    // cannot be set in initializer
    config.random_seed = rand_seed_from_time();

    // vault
    vault_storage_t vault;

    // populations and archives
    ga_pop_t cgp_population;
    ga_pop_t pred_population = NULL;
    archive_t cgp_archive = NULL;
    archive_t pred_archive = NULL;

    // images
    img_image_t img_original;
    img_image_t img_noisy;

    // log files
    char best_circuit_file_name_txt[MAX_FILENAME_LENGTH + 1];
    char best_circuit_file_name_chr[MAX_FILENAME_LENGTH + 1];
    FILE *progress_log_file;

    // application exit code
    int retval = 0;

    // resource usage
    struct rusage resource_usage;
    struct timeval usertime_start, usertime_end;


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

    if ((progress_log_file = open_log_file(config.log_dir, "progress.log")) == NULL) {
        fprintf(stderr, "Failed to open 'progress.log' in results dir for writing.\n");
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
        int img_size = img_original->width * img_original->height;
        pred_init(
            img_size - 1,  // max gene value
            config.pred_size * img_size,   // max genome length
            config.pred_size * img_size,   // initial genome length
            config.pred_mutation_rate,     // % of mutated genes
            config.pred_offspring_elite,   // % of elite children
            config.pred_offspring_combine, // % of "crossovered" children
            repeated //(config.algorithm == baldwin)? repeated : permuted  // genome type
        );

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

    SLOWLOG("Algorithm: %s", config_algorithm_names[config.algorithm]);
    SLOWLOG("Stop at generation: %d", config.max_generations);
    SLOWLOG("Initial \"PSNR\" value:         %.10g",
        img_psnr(img_original, img_noisy));

    DEBUGLOG("Evaluating CGP population...");
    ga_evaluate_pop(cgp_population);

    if (config.algorithm != simple_cgp) {
        arc_insert(cgp_archive, cgp_population->best_chromosome);

        DEBUGLOG("Evaluating PRED population...");
        ga_evaluate_pop(pred_population);
        arc_insert(pred_archive, pred_population->best_chromosome);

        DEBUGLOG("Best fitness: CGP %.10g, PRED %.10g",
            cgp_population->best_fitness, pred_population->best_fitness);

    } else {
        DEBUGLOG("Best fitness: %.10g", cgp_population->best_fitness);
    }

    save_original_image(config.log_dir, img_original);
    save_noisy_image(config.log_dir, img_noisy);
    save_config(config.log_dir, &config);


    /*
        Evolution itself
     */

    DEBUGLOG("Starting the big while loop.");

    getrusage(RUSAGE_SELF, &resource_usage);
    usertime_start = resource_usage.ru_utime;


    // shared among threads
    bool finished = false;

    switch (config.algorithm) {

        case simple_cgp:
            // install signal handlers
            signal(SIGINT, sigint_handler);
            signal(SIGXCPU, sigxcpu_handler);

            retval = simple_cgp_main(
                cgp_population,
                &config,
                &vault,
                img_noisy,
                best_circuit_file_name_txt,
                best_circuit_file_name_chr,
                progress_log_file
            );
            goto done;

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
                        &config,
                        &vault,
                        img_noisy,
                        best_circuit_file_name_txt,
                        best_circuit_file_name_chr,
                        progress_log_file,
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
                        progress_log_file,
                        &finished
                    );
                }
            }
            goto done;

        default:
            fprintf(stderr, "Algorithm '%s' is not supported yet.\n",
                config_algorithm_names[config.algorithm]);
            return -1;

    }


    /*
        Dump summary and clean-up
     */


done:

    // dump evolution summary
    getrusage(RUSAGE_SELF, &resource_usage);
    usertime_end = resource_usage.ru_utime;

    struct timeval usertime_diff;
    timeval_subtract(&usertime_diff, &usertime_end, &usertime_start);

    log_final_summary(stdout, cgp_population, fitness_get_cgp_evals());
    fprintf(stdout, "\nTime in user mode:");
    fprintf(stdout, "\n- start: ");
    fprint_timeval(stdout, &usertime_start);
    fprintf(stdout, "\n- end:   ");
    fprint_timeval(stdout, &usertime_end);
    fprintf(stdout, "\n- diff:  ");
    fprint_timeval(stdout, &usertime_diff);
    fprintf(stdout, "\n");

    FILE *summary_file;
    if ((summary_file = open_log_file(config.log_dir, "summary.log")) == NULL) {
        fprintf(stderr, "Failed to open 'summary.log' in results dir for writing.\n");

    } else {


        log_final_summary(summary_file, cgp_population, fitness_get_cgp_evals());

        fprintf(summary_file, "\nTime in user mode:");
        fprintf(summary_file, "\n- start: ");
        fprint_timeval(summary_file, &usertime_start);
        fprintf(summary_file, "\n- end:   ");
        fprint_timeval(summary_file, &usertime_end);
        fprintf(summary_file, "\n- diff:  ");
        fprint_timeval(summary_file, &usertime_diff);
        fprintf(summary_file, "\n");
    }

    // dump config once again
    config_save_file(stdout, &config);

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


/**
 * Calculates difference of two `struct timeval` values
 *
 * http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 *
 * @param  result
 * @param  x
 * @param  y
 * @return 1 if the difference is negative, otherwise 0.
 */
int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}


/**
 * Prints `struct timeval` value in "12m34.567s" format
 * @param fp
 * @param time
 */
void fprint_timeval(FILE *fp, struct timeval *time)
{
    long minutes = time->tv_sec / 60;
    long seconds = time->tv_sec % 60;
    long microseconds = time->tv_usec;

    if (microseconds < 0) {
        microseconds = 1 - microseconds;
        seconds--;
    }

    fprintf(fp, "%ldm%ld.%06lds", minutes, seconds, microseconds);
}
