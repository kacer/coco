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


#include "cgp.h"


#define VAULT_ERROR -10
#define VAULT_EMPTY -11


typedef struct {
    char *directory;
    ga_fitness_t cgp_fitness_func;
} vault_storage;


/**
 * Initialize vault storage
 * @param storage
 */
int vault_init(vault_storage *storage);


/**
 * Store running state to vault
 * @param storage
 * @param population
 */
int vault_store(vault_storage *storage, ga_pop_t cgp_population);


/**
 * Retrieve last stored state from vault
 * @param storage
 * @param population
 */
int vault_retrieve(vault_storage *storage, ga_pop_t *cgp_pop_ptr);


/**
 * Read stored state from file
 * @param file name
 * @param population
 */
int vault_read(char *fullname, ga_pop_t *cgp_pop_ptr);
