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

#include <stdlib.h>
#include <time.h>

#include "types.h"

static inline void rand_init()
{
    srand(time(0));
}

static inline uint rnd()
{
    return rand();
}


/**
 * Generates random number between low and high, inclusive
 * @param  low
 * @param  high
 * @return
 */
static inline uint rand_range(uint low, uint high)
{
    return rand() % (high - low + 1) + low;
}


/**
 * Returns randomly chosen number from the list of signed integers
 * @param  length
 * @param  choices
 * @return
 */
static inline int rand_schoice(uint length, int choices[])
{
    uint index = rand_range(0, length - 1);
    return choices[index];
}



/**
 * Returns randomly chosen number from the list of unsigned integers
 * @param  length
 * @param  choices
 * @return
 */
static inline uint rand_uchoice(uint length, uint choices[])
{
    uint index = rand_range(0, length - 1);
    return choices[index];
}



/**
 * Returns randomly chosen number from the list of unsigned integers
 * @param  length
 * @param  choices
 * @return
 */
static inline uint rand_uachoice(uint_array *array)
{
    return rand_uchoice(array->size, array->values);
}
