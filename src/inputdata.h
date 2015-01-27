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


#include "config.h"


struct _input_data;
typedef struct _input_data input_data_t;

bool input_data_load(input_data_t *data, config_t *config);
void input_data_destroy(input_data_t *data);


#ifdef SYMREG
    #include "symreg/inputdata.h"
#else
    #include "ifilter/inputdata.h"
#endif
