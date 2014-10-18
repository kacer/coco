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


#include <math.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _OPENMP
    #include <omp.h>
#endif

#include "logging.h"
#include "fitness.h"


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


void _evals_readable(char *buf, int bufsize, long cgp_evals)
{
    char *suffix = "";
    double evals = cgp_evals;
    if (evals > 1000) {
        evals /= 1000;
        suffix = "k";
    }
    if (evals > 1000) {
        evals /= 1000;
        suffix = "M";
    }
    if (evals > 1000) {
        evals /= 1000;
        suffix = "G";
    }
    if (evals > 1000) {
        evals /= 1000;
        suffix = "T";
    }
    snprintf(buf, bufsize, "%.2lf %s", evals, suffix);
}


/**
 * Logs current CGP progress
 * @param  cgp_population
 */
void log_cgp_progress(FILE *fp, ga_pop_t cgp_population, long cgp_evals)
{
    char human_evals[50];
    _evals_readable(human_evals, 50, cgp_evals);
    log_entry_prolog(fp, SECTION_CGP);
    fprintf(fp, "CGP generation %4d: best fitness " FITNESS_FMT ", %ld evals (%s)\n",
        cgp_population->generation, cgp_population->best_fitness, cgp_evals, human_evals);
}


/**
 * Logs that CGP has finished
 * @param  cgp_population
 */
void log_cgp_finished(FILE *fp, ga_pop_t cgp_population)
{
    log_entry_prolog(fp, SECTION_SYS);
    fprintf(fp, "Evolution finished. Best PSNR %.2f dB\n",
        fitness_to_psnr(cgp_population->best_fitness));
}


/**
 * Logs that evolution is stored to vault + prints best fitness
 * @param  fp
 * @param  cgp_population
 */
void log_vault(FILE *fp, ga_pop_t cgp_population)
{
    log_entry_prolog(fp, SECTION_SYS);
    fprintf(fp, "Storing to vault. CGP generation %4d, best fitness " FITNESS_FMT "\n",
        cgp_population->generation, cgp_population->best_fitness);
}


/**
 * Logs current predictors progress
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
 * Logs that CGP best fitness has changed
 * @param previous_best
 * @param new_best
 */
void log_cgp_change(FILE *fp, ga_fitness_t previous_best, ga_fitness_t new_best)
{
    log_entry_prolog(fp, SECTION_CGP);
    fprintf(fp, "Best CGP circuit changed by " FITNESS_FMT
                " from " FITNESS_FMT " to " FITNESS_FMT "\n",
                new_best - previous_best, previous_best, new_best);
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
void log_pred_change(FILE *fp, ga_fitness_t previous_best,
    ga_fitness_t new_best, pred_genome_t new_genome, bool indent)
{
    log_entry_prolog(fp, SECTION_PRED);
    if (indent) fprintf(fp, PRED_INDENT);
    fprintf(fp, "Best predictor changed by " FITNESS_FMT
                " from " FITNESS_FMT " to " FITNESS_FMT " using %u/%u pixels\n",
                new_best - previous_best,
                previous_best,
                new_best,
                new_genome->used_pixels,
                pred_get_length());
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
    fprintf(fp, "\nASCII Art without inactive blocks:\n");
    cgp_dump_chr(pop->best_chromosome, fp, asciiart_active);
}


/**
 * Logs final summary
 */
void log_final_summary(FILE *fp, ga_pop_t cgp_population, long cgp_evals)
{
    fprintf(fp, "Final summary:\n\n");
    fprintf(fp, "Generation: %d\n", cgp_population->generation);
    fprintf(fp, "Best fitness: " FITNESS_FMT "\n", cgp_population->best_fitness);
    fprintf(fp, "PSNR: %.2f\n", fitness_to_psnr(cgp_population->best_fitness));
    fprintf(fp, "CGP evaluations: %ld\n", cgp_evals);
}


/**
 * Saves image named as filtered to results directory
 * @param cgp_population
 * @param noisy
 */
void _save_filtered_image(const char *dir, img_image_t noisy, int generation)
{
    char filename[MAX_FILENAME_LENGTH + 1];
    snprintf(filename, MAX_FILENAME_LENGTH + 1, "%s/images/img_filtered_%08d.png", dir, generation);
    img_save_png(noisy, filename);
}


/**
 * Saves original image to results directory
 * @param dir results directory
 * @param original
 */
void save_original_image(const char *dir, img_image_t original)
{
    char filename[MAX_FILENAME_LENGTH + 1];
    snprintf(filename, MAX_FILENAME_LENGTH + 1, "%s/img_original.png", dir);
    img_save_png(original, filename);
}


/**
 * Saves best found image to results directory
 * @param cgp_population
 * @param noisy
 */
void save_best_image(const char *dir, ga_pop_t cgp_population, img_image_t noisy)
{
    img_image_t best = fitness_filter_image(cgp_population->best_chromosome);
    char filename[MAX_FILENAME_LENGTH + 1];
    snprintf(filename, MAX_FILENAME_LENGTH + 1, "%s/img_best.png", dir);
    img_save_png(best, filename);
    img_destroy(best);
}


/**
 * Saves input noisy image to results directory
 * @param dir results directory
 * @param noisy
 */
void save_noisy_image(const char *dir, img_image_t noisy)
{
    char filename[MAX_FILENAME_LENGTH + 1];
    snprintf(filename, MAX_FILENAME_LENGTH + 1, "%s/img_noisy.png", dir);
    img_save_png(noisy, filename);
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
        fprintf(fp, "Log started.\n");
    }
    return fp;
}
