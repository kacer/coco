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
 * @param  fitness
 * @return pointer to newly inserted entry
 */
bw_history_entry_t *bw_add_history(bw_history_t *history, int generation,
    ga_fitness_t fitness)
{
    bw_history_entry_t *prev = bw_get(history, -1);
    bw_history_entry_t *new = &history->log[history->pointer];

    new->generation = generation;
    new->fitness = fitness;

    new->delta_generation = generation - prev->generation;
    new->delta_fitness = fitness - prev->fitness;

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
    fprintf(fp, "|      f |");
    fprintf(fp, " %7.3lf ||", history->last_change.fitness);
    for (int i = 0; i < len; i++) {
        fprintf(fp, " %7.3lf |", bw_get(history, i)->fitness);
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


/**
 * Updates evolution parameters according to history
 * @param  history
 * @return Info about what has been changed and how
 */
void bw_update_params(bw_history_t *history, bw_update_t *result)
{
    bw_history_entry_t *last = bw_get(history, -1);
    double velocity = last->velocity;

    int old_length = pred_get_length();
    result->old_predictor_length = old_length;
    result->new_predictor_length = old_length;
    result->predictor_length_changed = false;

    double coefficcient = 1;

    if (fabs(velocity) <= BW_ZERO) {
        // no change
        coefficcient = BW_STEADY_COEF;

    } else if (velocity < 0) {
        // decrease
        coefficcient = BW_DECREASE_COEF;

    } else if (velocity > 0 && velocity <= BW_SLOW) {
        // slow increase
        coefficcient = BW_INCREASE_SLOW_COEF;

    } else if (velocity > BW_SLOW) {
        // fast increase
        coefficcient = BW_INCREASE_FAST_COEF;

    } else {
        fprintf(stderr, "Baldwin if-else fail. Velocity = %.10g\n", velocity);
        assert(false);
    }

    if (coefficcient != 100) {
        int new_length = (old_length * coefficcient) / 100;
        result->new_predictor_length = new_length;
        result->predictor_length_changed = true;
        pred_set_length(new_length);
    }
}
