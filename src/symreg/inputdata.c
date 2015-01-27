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


#ifndef SYMREG
    /* fixes linter */
    #define SYMREG
#endif


#include <stdlib.h>
#include <stdio.h>

#include "../inputdata.h"
#include "inputdata.h"


#define FAIL() do {\
    fprintf(stderr, "Failed to open source data file %s\n", config->input_data); \
    return false; \
} while(false);


bool input_data_load(input_data_t *data, config_t *config)
{
    FILE *datafile = fopen(config->input_data, "rt");
    if (datafile == NULL) {
        FAIL();
    }

    /* read header */

    int variables;
    if (fscanf(datafile, "%u %u", &data->fitness_cases, &variables) != 2) {
        FAIL();
    }

    if (variables != CGP_INPUTS) {
        fprintf(stderr, "Unsupported number of independent variables (must be %d, %d given)\n", CGP_INPUTS, variables);
        return false;
    }

    /* allocate memory */

    data->inputs = (cgp_value_t*) malloc(sizeof(cgp_value_t) * data->fitness_cases * CGP_INPUTS);
    if (!data->inputs) {
        fprintf(stderr, "Failed to allocate memory for input data.\n");
        return false;
    }

    data->outputs = (cgp_value_t*) malloc(sizeof(cgp_value_t) * data->fitness_cases);
    if (!data->outputs) {
        fprintf(stderr, "Failed to allocate memory for input data.\n");
        return false;
    }

    /* load data */

    for (unsigned int case_idx = 0; case_idx < data->fitness_cases; case_idx++) {
        double tmp;
        for (int in_idx = 0; in_idx < CGP_INPUTS; in_idx++) {
            if (fscanf(datafile, "%lf", &tmp) != 1) {
                FAIL();
            }
            data->inputs[INPUT_IDX(case_idx, in_idx)] = tmp;
        }

        if (fscanf(datafile, "%lf", &tmp) != 1) {
            FAIL();
        }
        data->outputs[case_idx] = tmp;
    }

    fclose(datafile);
    return true;
}


void input_data_destroy(input_data_t *data)
{
    free(data->outputs);
    free(data->inputs);
}


void input_data_save(input_data_t *data, FILE *file)
{
    fprintf(file, "%u  %u\n", data->fitness_cases, CGP_INPUTS);
    for (unsigned int case_idx = 0; case_idx < data->fitness_cases; case_idx++) {
        for (int in_idx = 0; in_idx < CGP_INPUTS; in_idx++) {
            fprintf(file, "%lg\t", data->inputs[INPUT_IDX(case_idx, in_idx)]);
        }
        fprintf(file, "%lg\n", data->outputs[case_idx]);
    }
}
