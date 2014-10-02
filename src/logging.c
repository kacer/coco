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

#include "logging.h"


#define MAX_FILENAME_LENGTH 1000

/* predictors log to second column */
#define PRED_INDENT "                                                        "


/**
 * Print current CGP progress
 * @param  cgp_population
 */
void print_cgp_progress(ga_pop_t cgp_population)
{
    LOG_THREAD_IDENT(stdout);
    printf("CGP generation %4d: best fitness %.10g\n",
        cgp_population->generation, cgp_population->best_fitness);
}


/**
 * Print current predictors progress
 * @param  pred_population
 * @param  pred_archive
 */
void print_pred_progress(ga_pop_t pred_population,
    archive_t pred_archive)
{
    LOG_THREAD_IDENT(stdout);
    printf(PRED_INDENT);
    printf("PRED generation %4d: best fitness %.10g (archived %.10g)\n",
        pred_population->generation, pred_population->best_fitness,
        arc_get(pred_archive, 0)->fitness);
}


/**
 * Print current progress
 * @param  cgp_population
 * @param  pred_population
 * @param  pred_archive
 */
void print_progress(ga_pop_t cgp_population, ga_pop_t pred_population,
    archive_t pred_archive)
{
    LOG_THREAD_IDENT(stdout);
    printf("CGP generation %4d: best fitness %.10g\t\t",
        cgp_population->generation, cgp_population->best_fitness);
    if (pred_population != NULL) {
        printf("PRED generation %4d: best fitness %.10g",
            pred_population->generation, pred_population->best_fitness);
        if (pred_archive != NULL) {
            printf(" (archived %.10g)\n", arc_get(pred_archive, 0)->fitness);
        }
        printf("\n");
    }
}


/**
 * Print current progress and best found circuit
 * @param cgp_population
 * @param pred_population
 * @param pred_archive
 */
void print_results(ga_pop_t cgp_population, ga_pop_t pred_population,
    archive_t pred_archive)
{
    print_progress(cgp_population, pred_population, pred_archive);

    if (pred_population != NULL) {
        printf("\n"
               "Best predictor\n"
               "--------------\n");
        pred_dump_chr(pred_population->best_chromosome, stdout);
    }

    printf("\n"
           "Best circuit\n"
           "------------\n");
    cgp_dump_chr_asciiart(cgp_population->best_chromosome, stdout);
    printf("\n");
}


/**
 * Logs that CGP best fitness has changed
 * @param previous_best
 * @param new_best
 */
void log_cgp_change(ga_fitness_t previous_best, ga_fitness_t new_best)
{
    SLOWLOG("Best CGP circuit changed by %.10g, moving to archive",
                new_best - previous_best);
}


/**
 * Logs that predictors best fitness has changed
 * @param previous_best
 * @param new_best
 */
void log_pred_change(ga_fitness_t previous_best, ga_fitness_t new_best)
{
    SLOWLOG(PRED_INDENT "Best predictor changed by %.10g, moving to archive",
            new_best - previous_best);
}


/**
 * Saves image named as filtered to results directory
 * @param cgp_population
 * @param noisy
 */
void _save_filtered_image(char *dir, img_image_t noisy, int generation)
{
    char filename[MAX_FILENAME_LENGTH];
    snprintf(filename, MAX_FILENAME_LENGTH, "%s/img_filtered_%08d.bmp", dir, generation);
    img_save_bmp(noisy, filename);
}


/**
 * Saves original image to results directory
 * @param dir results directory
 * @param original
 */
void save_original_image(char *dir, img_image_t original)
{
    char filename[MAX_FILENAME_LENGTH];
    snprintf(filename, MAX_FILENAME_LENGTH, "%s/img_original.bmp", dir);
    img_save_bmp(original, filename);
}


/**
 * Saves input noisy image to results directory
 * @param dir results directory
 * @param noisy
 */
void save_noisy_image(char *dir, img_image_t noisy)
{
    _save_filtered_image(dir, noisy, 0);
}



/**
 * Saves image filtered by best filter to results directory
 * @param dir results directory
 * @param cgp_population
 * @param noisy
 */
void save_filtered_image(char *dir, ga_pop_t cgp_population, img_image_t noisy)
{
    img_image_t filtered = fitness_filter_image(cgp_population->best_chromosome);
    _save_filtered_image(dir, filtered, cgp_population->generation);
    img_destroy(filtered);
}


/**
 * Saves configuration to results directory
 */
void save_config(char *dir, config_t *config)
{
    char filename[MAX_FILENAME_LENGTH];
    snprintf(filename, MAX_FILENAME_LENGTH, "%s/config", dir);
    FILE *f = fopen(filename, "wt");
    config_save_file(f, config);
    fclose(f);
}
