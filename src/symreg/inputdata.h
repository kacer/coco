/*
 * Colearning in Coevolutionary Algorithms
 * Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>
 *
 * Master's Thesis
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


#include <stdio.h>

#include "cgp_symreg.h"


#define INPUT_IDX(case_idx, in_idx) (case_idx * CGP_INPUTS + in_idx)


struct _input_data {
    unsigned int fitness_cases;
    cgp_value_t *inputs;  // two-dimensional [fitnesscase][input]
    cgp_value_t *outputs;
};


void input_data_save(struct _input_data *data, FILE *file);
