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
    //char buf[MAX_FILENAME_LENGTH + 1];

    retval = mkdir(dir, S_IRWXU);
    if (retval != 0 && errno != EEXIST) {
        return retval;
    }

    // images directory
    /*
    snprintf(buf, MAX_FILENAME_LENGTH + 1, "%s/images", dir);
    retval = mkdir(buf, S_IRWXU);
    if (retval != 0 && errno != EEXIST) {
        return retval;
    }
    */

    // vault directory
    /*
    snprintf(buf, MAX_FILENAME_LENGTH + 1, "%s/vault", dir);
    retval = mkdir(buf, S_IRWXU);
    if (retval != 0 && errno != EEXIST) {
        return retval;
    }
    if (vault_dir != NULL) {
        strncpy(vault_dir, buf, vault_dir_buffer_size);
    }
    */

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
    fprintf(fp, "Storing state. CGP generation %4d, best fitness " FITNESS_FMT "\n",
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
 * Logs CGP history entry
 * @param history
 */
void log_bw_history_entry(FILE *fp, bw_history_entry_t *history)
{
    log_entry_prolog(fp, SECTION_CGP);
    fprintf(fp, "CGP velocity %.5lf (" FITNESS_FMT " / %d)\n",
        history->velocity,
        history->delta_fitness,
        history->delta_generation);
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
void log_cgp_circuit(FILE *fp, int generation, ga_chr_t circuit)
{
    fprintf(fp, "Generation: %d\n", generation);
    fprintf(fp, "Fitness: " FITNESS_FMT "\n\n", circuit->fitness);
    fprintf(fp, "CGP Viewer format:\n");
    cgp_dump_chr(circuit, fp, compat);
    fprintf(fp, "\nASCII Art:\n");
    cgp_dump_chr(circuit, fp, asciiart);
    fprintf(fp, "\nASCII Art without inactive blocks:\n");
    cgp_dump_chr(circuit, fp, asciiart_active);
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
 * @param best_circuit
 * @param noisy
 */
void save_best_image(const char *dir, ga_chr_t best_circuit, img_image_t noisy)
{
    img_image_t best = fitness_filter_image(best_circuit);
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
 * @param  log_start whether to insert initial log message
 * @return
 */
FILE *open_log_file(const char *dir, const char *file, bool log_start)
{
    char filename[MAX_FILENAME_LENGTH + 1];
    snprintf(filename, MAX_FILENAME_LENGTH + 1, "%s/%s", dir, file);
    FILE *fp = fopen(filename, "wt");
    if (fp && log_start) {
        log_entry_prolog(fp, SECTION_SYS);
        fprintf(fp, "Log started.\n");
    }
    return fp;
}


/**
 * Initializes CGP history CSV file and writes header to it
 * @param  dir
 * @param  file
 * @return fp
 */
FILE *init_cgp_history_file(const char *dir, const char *file)
{
    FILE *fp = open_log_file(dir, file, false);
    if (!fp) return NULL;

    fprintf(fp,
        "generation,"
        "predicted_fitness,real_fitness,inaccuracy (pred/real),best_fitness_ever,"
        "active_predictor_fitness,pred_length,pred_used_length,"
        "cgp_evals,"
        "velocity,"
        "delta_generation,delta_fitness,delta_velocity"
        "\n");

    return fp;
}


/**
 * Log CGP history entry in CSV format
 * @param fp
 * @param hist
 * @param cgp_evals
 * @param pred_length
 * @param pred_used_length
 * @param best_ever
 */
void log_cgp_history(FILE *fp, bw_history_entry_t *hist, long cgp_evals,
    int pred_length, int pred_used_length, ga_fitness_t best_ever)
{
    fprintf(fp, "%d,", hist->generation);
    fprintf(fp, FITNESS_FMT ",", hist->predicted_fitness);
    fprintf(fp, FITNESS_FMT ",", hist->fitness);
    fprintf(fp, FITNESS_FMT ",", hist->predicted_fitness / hist->fitness);
    fprintf(fp, FITNESS_FMT ",", best_ever);
    fprintf(fp, FITNESS_FMT ",", hist->active_predictor_fitness);
    fprintf(fp, "%d,", pred_length);
    fprintf(fp, "%d,", pred_used_length);
    fprintf(fp, "%ld,", cgp_evals);

    fprintf(fp, "%10g,", hist->velocity);

    fprintf(fp, "%d,", hist->delta_generation);
    fprintf(fp, FITNESS_FMT ",", hist->delta_fitness);
    fprintf(fp, "%10g\n", hist->delta_velocity);
}


/**
 * Log that predictors' length has changed
 * @param fp
 * @param old_length
 * @param new_length
 */
void log_predictors_length_change(FILE *fp, int old_length, int new_length)
{
    log_entry_prolog(fp, SECTION_BALDWIN);
    fprintf(fp, "Predictor length changed by %d from %d to %d\n",
                new_length - old_length,
                old_length,
                new_length);
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


/**
 * Logs spent time
 * @param fp
 * @param usertime_start
 * @param usertime_end
 * @param wallclock_start
 * @param wallclock_end
 */
void log_time(FILE *fp, struct timeval *usertime_start,
    struct timeval *usertime_end, struct timeval *wallclock_start,
    struct timeval *wallclock_end)
{
    struct timeval usertime_diff;
    timeval_subtract(&usertime_diff, usertime_end, usertime_start);

    struct timeval wallclock_diff;
    timeval_subtract(&wallclock_diff, wallclock_end, wallclock_start);

    struct tm tmp;
    char time_string[40];

    fprintf(fp, "Time in user mode:");
    fprintf(fp, "\n- start: ");
    fprint_timeval(fp, usertime_start);
    fprintf(fp, "\n- end:   ");
    fprint_timeval(fp, usertime_end);
    fprintf(fp, "\n- diff:  ");
    fprint_timeval(fp, &usertime_diff);
    fprintf(fp, "\n");

    fprintf(fp, "\nWall clock:");

    localtime_r(&wallclock_start->tv_sec, &tmp);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &tmp);
    fprintf(fp, "\n- start: %s", time_string);

    localtime_r(&wallclock_end->tv_sec, &tmp);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &tmp);
    fprintf(fp, "\n- end: %s", time_string);

    fprintf(fp, "\n- diff:  ");
    fprint_timeval(fp, &wallclock_diff);
    fprintf(fp, "\n");
}
