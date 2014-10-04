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

#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _OPENMP
    #include <omp.h>
#endif

#include "logging.h"


/**
 * Write log line prolog (contains time, thread ident, section ident)
 * @param fp
 */
void log_entry_prolog(FILE *fp, const char *section)
{
    time_t now = time(NULL);
    char timestr[200];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S %z", localtime(&now));
    fprintf(fp, "[%s] [%s] ", timestr, section);
    #ifdef _OPENMP
        fprintf(fp, "[thread-%02d] ", omp_get_thread_num());
    #endif
}


/**
 * Creates log directories
 * @param  dir
 * @param  vault_dir If not NULL, stores vault directory here
 * @param  vault_dir_buffer_size
 */
int log_create_dirs(const char *dir, char *vault_dir, int vault_dir_buffer_size)
{
    // main directory
    int retval;
    char buf[MAX_FILENAME_LENGTH + 1];

    retval = mkdir(dir, S_IRWXU);
    if (retval != 0 && errno != EEXIST) {
        return retval;
    }

    // images directory
    snprintf(buf, MAX_FILENAME_LENGTH + 1, "%s/images", dir);
    retval = mkdir(buf, S_IRWXU);
    if (retval != 0 && errno != EEXIST) {
        return retval;
    }

    // vault directory
    snprintf(buf, MAX_FILENAME_LENGTH + 1, "%s/vault", dir);
    retval = mkdir(buf, S_IRWXU);
    if (retval != 0 && errno != EEXIST) {
        return retval;
    }
    if (vault_dir != NULL) {
        strncpy(vault_dir, buf, vault_dir_buffer_size);
    }

    return 0;
}


/**
 * Print current CGP progress
 * @param  cgp_population
 */
void log_cgp_progress(FILE *fp, ga_pop_t cgp_population)
{
    log_entry_prolog(fp, SECTION_CGP);
    fprintf(fp, "CGP generation %4d: best fitness " FITNESS_FMT "\n",
        cgp_population->generation, cgp_population->best_fitness);
}


/**
 * Print current predictors progress
 * @param  pred_population
 * @param  pred_archive
 * @param  indent
 */
    void log_pred_progress(FILE *fp, ga_pop_t pred_population,
    archive_t pred_archive, bool indent)
{
    log_entry_prolog(fp, SECTION_PRED);
    if (indent) fprintf(fp, PRED_INDENT);
    fprintf(fp, "PRED generation %4d: best fitness " FITNESS_FMT " (archived " FITNESS_FMT ")\n",
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
    log_entry_prolog(stdout, "combined");
    printf("CGP generation %4d: best fitness " FITNESS_FMT "\t\t",
        cgp_population->generation, cgp_population->best_fitness);
    if (pred_population != NULL) {
        printf("PRED generation %4d: best fitness " FITNESS_FMT "",
            pred_population->generation, pred_population->best_fitness);
        if (pred_archive != NULL) {
            printf(" (archived " FITNESS_FMT ")\n", arc_get(pred_archive, 0)->fitness);
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
void log_cgp_change(FILE *fp, ga_fitness_t previous_best, ga_fitness_t new_best)
{
    log_entry_prolog(fp, SECTION_CGP);
    fprintf(fp, "Best CGP circuit changed by " FITNESS_FMT "\n", new_best - previous_best);
}


/**
 * Logs that CGP was moved to archive
 * @param predicted
 * @param real
 */
void log_cgp_archived(FILE *fp, ga_fitness_t predicted, ga_fitness_t real)
{
    log_entry_prolog(fp, SECTION_CGP);
    fprintf(fp, "Moving CGP circuit to archive. "
        "Fitness predicted / real: " FITNESS_FMT " / " FITNESS_FMT "\n",
        predicted, real);
}


/**
 * Logs that predictors best fitness has changed
 * @param previous_best
 * @param new_best
 */
void log_pred_change(FILE *fp, ga_fitness_t previous_best, ga_fitness_t new_best, bool indent)
{
    log_entry_prolog(fp, SECTION_PRED);
    if (indent) fprintf(fp, PRED_INDENT);
    fprintf(fp, "Best predictor changed by " FITNESS_FMT "\n", new_best - previous_best);
}


/**
 * Logs best circuit to file.
 */
void log_cgp_circuit(FILE *fp, ga_pop_t pop)
{
    fprintf(fp, "Generation: %d\n", pop->generation);
    fprintf(fp, "Fitness: " FITNESS_FMT "\n\n", pop->best_fitness);
    fprintf(fp, "CGP Viewer format:\n");
    cgp_dump_chr(pop->best_chromosome, fp, compat);
    fprintf(fp, "\nASCII Art:\n");
    cgp_dump_chr(pop->best_chromosome, fp, asciiart);
}


/**
 * Saves image named as filtered to results directory
 * @param cgp_population
 * @param noisy
 */
void _save_filtered_image(const char *dir, img_image_t noisy, int generation)
{
    char filename[MAX_FILENAME_LENGTH + 1];
    snprintf(filename, MAX_FILENAME_LENGTH + 1, "%s/images/img_filtered_%08d.bmp", dir, generation);
    img_save_bmp(noisy, filename);
}


/**
 * Saves original image to results directory
 * @param dir results directory
 * @param original
 */
void save_original_image(const char *dir, img_image_t original)
{
    char filename[MAX_FILENAME_LENGTH + 1];
    snprintf(filename, MAX_FILENAME_LENGTH + 1, "%s/images/img_original.bmp", dir);
    img_save_bmp(original, filename);
}


/**
 * Saves input noisy image to results directory
 * @param dir results directory
 * @param noisy
 */
void save_noisy_image(const char *dir, img_image_t noisy)
{
    _save_filtered_image(dir, noisy, 0);
}



/**
 * Saves image filtered by best filter to results directory
 * @param dir results directory
 * @param cgp_population
 * @param noisy
 */
void save_filtered_image(const char *dir, ga_pop_t cgp_population, img_image_t noisy)
{
    img_image_t filtered = fitness_filter_image(cgp_population->best_chromosome);
    _save_filtered_image(dir, filtered, cgp_population->generation);
    img_destroy(filtered);
}


/**
 * Saves configuration to results directory
 */
void save_config(const char *dir, config_t *config)
{
    char filename[MAX_FILENAME_LENGTH + 1];
    snprintf(filename, MAX_FILENAME_LENGTH + 1, "%s/config.log", dir);
    FILE *f = fopen(filename, "wt");
    config_save_file(f, config);
    fclose(f);
}


/**
 * Open specified file for writing. Caller is responsible for closing.
 * @param  dir
 * @param  file
 * @return
 */
FILE *open_log_file(const char *dir, const char *file)
{
    char filename[MAX_FILENAME_LENGTH + 1];
    snprintf(filename, MAX_FILENAME_LENGTH + 1, "%s/%s", dir, file);
    FILE *fp = fopen(filename, "wt");
    if (fp) {
        log_entry_prolog(fp, SECTION_SYS);
        fprintf(fp, "Log started.");
    }
    return fp;
}
