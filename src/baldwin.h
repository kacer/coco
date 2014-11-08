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


#pragma once


#include "ga.h"

#define BW_HISTORY_LENGTH 7

typedef enum {
    bwalg_last = 0,
    bwalg_median3,
    bwalg_avg3,
    bwalg_avg7w,
    bwalg_symreg,
} bw_algorithm_t;


// multiple const to avoid "unused variable" warnings
static const char * const bw_algorithm_names[] = {
    "last",
    "median3",
    "avg3",
    "avg7w",
    "symreg",
};


typedef struct {
    bw_algorithm_t algorithm;
    bool use_absolute_increments;

    double inaccuracy_tolerance;
    double inaccuracy_coef;

    double zero_epsilon;
    double slow_threshold;

    double zero_coef;
    double decrease_coef;
    double increase_slow_coef;
    double increase_fast_coef;

    int absolute_increment_base;
    int zero_increment;
    int decrease_increment;
    int increase_slow_increment;
    int increase_fast_increment;

    int min_length;
    int max_length;
} bw_config_t;


typedef struct {
    int generation;
    int delta_generation;

    ga_fitness_t predicted_fitness;
    ga_fitness_t active_predictor_fitness;

    ga_fitness_t fitness;
    ga_fitness_t delta_fitness;

    double velocity;
    double delta_velocity;
} bw_history_entry_t;


typedef struct {
    bw_history_entry_t last_change;
    bw_history_entry_t log[BW_HISTORY_LENGTH];

    /* number of stored items */
    int stored;

    /* pointer to beginning of ring buffer - where new item will be
       stored */
    int pointer;
} bw_history_t;


typedef struct {
    bool predictor_length_changed;
    int old_predictor_length;
    int new_predictor_length;
} bw_update_t;


typedef struct {
    bw_history_t history;
    bool apply_now;
    int last_applied_generation;
} bw_state_t;


/**
 * Initializes history data structure
 * @param history
 */
void bw_init_history(bw_history_t *history);


/**
 * Calculate history entry values
 * @param  history
 * @param  generation
 * @param  real_fitness
 * @param  predicted_fitness
 * @param  active_predictor_fitness
 * @return pointer to newly inserted entry
 */
void bw_calc_history(bw_history_entry_t *entry, bw_history_entry_t *prev,
    int generation, ga_fitness_t real_fitness, ga_fitness_t predicted_fitness,
    ga_fitness_t active_predictor_fitness);


/**
 * Adds entry to history
 * @param  history
 * @param  entry
 * @return pointer to newly inserted entry
 */
bw_history_entry_t *bw_add_history_entry(bw_history_t *history,
    bw_history_entry_t *entry);


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
    ga_fitness_t active_predictor_fitness);


/**
 * Returns real index of item in ring buffer
 */
static inline int bw_real_index(bw_history_t *history, int index)
{
    if (history->stored < BW_HISTORY_LENGTH) {
        int real = index % history->stored;
        if (real < 0) real += history->stored;
        return real;

    } else {
        int real = (history->pointer + index) % BW_HISTORY_LENGTH;
        if (real < 0) real += BW_HISTORY_LENGTH;
        return real;
    }
}


/**
 * Returns item stored on given index
 */
static inline bw_history_entry_t *bw_get(bw_history_t *history, int index)
{
    return &history->log[bw_real_index(history, index)];
}


/**
 * Calculate absolute predictor length increments values
 */
void bw_init_absolute_increments(bw_config_t *config, int base);


/**
 * Updates evolution parameters according to history
 * @param  history
 * @return Info about what has been changed and how
 */
void bw_update_params(bw_config_t *config, bw_history_t *history, bw_update_t *result);


/**
 * Dumps history to file as ASCII art
 * @param fp
 * @param history
 */
void bw_dump_history_asciiart(FILE *fp, bw_history_t *history);
