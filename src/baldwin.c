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
#include <assert.h>
#include <string.h>

#include "baldwin.h"
#include "predictors.h"
#include "logging.h"


/**
 * Initializes history data structure
 * @param history
 */
void bw_init_history(bw_history_t *history)
{
    history->last_change.generation = 0;
    history->last_change.fitness = 0;
    history->last_change.velocity = 0;
    history->last_change.delta_generation = 0;
    history->last_change.delta_fitness = 0;
    history->last_change.delta_velocity = 0;

    history->log[0].generation = 0;
    history->log[0].fitness = 0;
    history->log[0].velocity = 0;
    history->log[0].delta_generation = 0;
    history->log[0].delta_fitness = 0;
    history->log[0].delta_velocity = 0;

    history->stored = 1;
    history->pointer = 1;
}


/**
 * Adds entry to history
 * @param  history
 * @param  generation
 * @param  real_fitness
 * @param  predicted_fitness
 * @param  active_predictor_fitness
 * @return pointer to newly inserted entry
 */
bw_history_entry_t *bw_add_history(bw_history_t *history, int generation,
    ga_fitness_t real_fitness, ga_fitness_t predicted_fitness,
    ga_fitness_t active_predictor_fitness)
{
    bw_history_entry_t *prev = bw_get(history, -1);
    bw_history_entry_t *new = &history->log[history->pointer];

    new->generation = generation;
    new->fitness = real_fitness;
    new->predicted_fitness = predicted_fitness;
    new->active_predictor_fitness = active_predictor_fitness;

    new->delta_generation = generation - prev->generation;
    new->delta_fitness = new->fitness - prev->fitness;

    new->velocity = new->delta_fitness / new->delta_generation;
    new->delta_velocity = new->velocity - prev->velocity;

    if (new->delta_fitness != 0) {
        memcpy(&history->last_change, new, sizeof(bw_history_entry_t));
    }

    if (history->stored < BW_HISTORY_LENGTH) {
        history->stored++;
    }
    history->pointer = (history->pointer + 1) % BW_HISTORY_LENGTH;

    return new;
}


/**
 * Gets current velocity according to algorithm
 * @param  alg
 * @param  history
 * @return
 */
double bw_get_velocity(bw_algorithm_t alg, bw_history_t *history)
{
    if (alg == bwalg_last) {
        return bw_get(history, -1)->velocity;

    } else if (alg == bwalg_avg3) {
        // doesn't matter if there are less than 3 items, we get duplicates
        double a = bw_get(history, -1)->velocity;
        double b = bw_get(history, -2)->velocity;
        double c = bw_get(history, -3)->velocity;
        return (a + b + c) / 3;

    } else if (alg == bwalg_avg7w) {
        double sum = 0;
        double divider = 0;
        for (int i = 1; i <= history->stored; i++) {
            double velo = bw_get(history, -i)->velocity;
            int weight = 8 - i;
            sum += velo * weight;
            divider += weight;
            printf("%lf * %d\n", velo, weight);
        }
        printf("%lf / %lf = %lf\n", sum, divider, sum / divider);
        return sum / divider;

    } else if (alg == bwalg_median3) {
        // doesn't matter if there are less than 3 items, we get duplicates
        double a = bw_get(history, -1)->velocity;
        double b = bw_get(history, -2)->velocity;
        double c = bw_get(history, -3)->velocity;

        if (a >= b && a >= c) {
            // a is greatest
            return b > c? b : c;
        } else if (b >= a && b >= c) {
            // b is greatest
            return a > c? a : c;
        } else {
            // c is greatest
            return a > b? a : b;
        }

    } else {
        fprintf(stderr, "Unimplemented baldwin algorithm.\n");
        assert(false);
    }
}


/**
 * Gets coefficient according to some symreg function
 */
double bw_get_coef(bw_algorithm_t alg, bw_history_t *history)
{
    double a = bw_get(history, -1)->velocity;
    double b = bw_get(history, -2)->velocity;
    double c = bw_get(history, -3)->velocity;
    double d = bw_get(history, -4)->velocity;
    double e = bw_get(history, -5)->velocity;
    double f = bw_get(history, -6)->velocity;
    double g = bw_get(history, -7)->velocity;

    return (0.984805307321727
        + 2.92388275504055*e
        + 55.5973782292397*b*g
        + 11.5809571875034*b*d
        + 1.97691040282476*d*f
        - 0.144536309148617*a
        - 2.76098000498705*c*e
        - 1.97691040282476*d*d);
}


/**
 * Updates evolution parameters according to history
 * @param  history
 * @return Info about what has been changed and how
 */
void bw_update_params(bw_config_t *config, bw_history_t *history, bw_update_t *result)
{
    bw_history_entry_t *last = bw_get(history, -1);

    int old_length = pred_get_length();
    result->old_predictor_length = old_length;
    result->new_predictor_length = old_length;
    result->predictor_length_changed = false;

    double coefficcient = 1.0;
    double inaccuracy = last->predicted_fitness / last->fitness;

    if (inaccuracy > config->inaccuracy_tolerance) {
        coefficcient = config->inaccuracy_coef;
    }

    if (config->algorithm == bwalg_symreg) {
        coefficcient = bw_get_coef(config->algorithm, history);

    } else {
        double velocity = bw_get_velocity(config->algorithm, history);

        if (fabs(velocity) <= config->zero_epsilon) {
            // no change
            coefficcient = config->zero_coef;

        } else if (velocity < 0) {
            // decrease
            coefficcient = config->decrease_coef;

        } else if (velocity > 0 && velocity <= config->slow_threshold) {
            // slow increase
            coefficcient = config->increase_slow_coef;

        } else if (velocity > config->slow_threshold) {
            // fast increase
            coefficcient = config->increase_fast_coef;

        } else {
            fprintf(stderr, "Baldwin if-else fail. Velocity = %.10g\n", velocity);
            assert(false);
        }
    }

    int new_length = floor(old_length * coefficcient);
    if (config->min_length && new_length < config->min_length) {
        new_length = config->min_length;
    }
    if (config->max_length && new_length > config->max_length) {
        new_length = config->max_length;
    }

    if (new_length != old_length) {
        result->new_predictor_length = new_length;
        result->predictor_length_changed = true;
        pred_set_length(new_length);
    }
}


/**
 * Dumps history to file as ASCII art
 * @param fp
 * @param history
 */
void bw_dump_history_asciiart(FILE *fp, bw_history_t *history) {
    int len = history->stored;

    // boxes top
    fprintf(fp, "+--------+---------++");
    for (int i = 0; i < len; i++) {
        fprintf(fp, "---------+");
    }
    fprintf(fp, "\n");

    // generations
    fprintf(fp, "|      G |");
    fprintf(fp, " %7d ||", history->last_change.generation);
    for (int i = 0; i < len; i++) {
        fprintf(fp, " %7d |", bw_get(history, i)->generation);
    }
    fprintf(fp, "\n");

    // fitness
    fprintf(fp, "|     rf |");
    fprintf(fp, " %7.3lf ||", history->last_change.fitness);
    for (int i = 0; i < len; i++) {
        fprintf(fp, " %7.3lf |", bw_get(history, i)->fitness);
    }
    fprintf(fp, "\n");

    // predicted fitness
    fprintf(fp, "|     pf |");
    fprintf(fp, " %7.3lf ||", history->last_change.predicted_fitness);
    for (int i = 0; i < len; i++) {
        fprintf(fp, " %7.3lf |", bw_get(history, i)->predicted_fitness);
    }
    fprintf(fp, "\n");

    // predicted fitness
    fprintf(fp, "|  predf |");
    fprintf(fp, " %7.3lf ||", history->last_change.active_predictor_fitness);
    for (int i = 0; i < len; i++) {
        fprintf(fp, " %7.3lf |", bw_get(history, i)->active_predictor_fitness);
    }
    fprintf(fp, "\n");

    // divider
    fprintf(fp, "+--------+---------++");
    for (int i = 0; i < len; i++) {
        fprintf(fp, "---------+");
    }
    fprintf(fp, "\n");

    // delta generations
    fprintf(fp, "|     dG |");
    fprintf(fp, " %7d ||", history->last_change.delta_generation);
    for (int i = 0; i < len; i++) {
        fprintf(fp, " %7d |", bw_get(history, i)->delta_generation);
    }
    fprintf(fp, "\n");

    // delta fitness
    fprintf(fp, "|     df |");
    fprintf(fp, " %7.3lf ||", history->last_change.delta_fitness);
    for (int i = 0; i < len; i++) {
        fprintf(fp, " %7.3lf |", bw_get(history, i)->delta_fitness);
    }
    fprintf(fp, "\n");

    // divider
    fprintf(fp, "+--------+---------++");
    for (int i = 0; i < len; i++) {
        fprintf(fp, "---------+");
    }
    fprintf(fp, "\n");

    // velocity
    fprintf(fp, "|    f/G |");
    fprintf(fp, " %7.3lf ||", history->last_change.velocity);
    for (int i = 0; i < len; i++) {
        fprintf(fp, " %7.3lf |", bw_get(history, i)->velocity);
    }
    fprintf(fp, "\n");

    // delta velocity = acceleration
    fprintf(fp, "| d(f/G) |");
    fprintf(fp, " %7.3lf ||", history->last_change.delta_velocity);
    for (int i = 0; i < len; i++) {
        fprintf(fp, " %7.3lf |", bw_get(history, i)->delta_velocity);
    }
    fprintf(fp, "\n");

    // boxes bottom
    fprintf(fp, "+--------+---------++");
    for (int i = 0; i < len; i++) {
        fprintf(fp, "---------+");
    }
    fprintf(fp, "\n");
}
