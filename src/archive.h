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


struct archive
{
    /* archive capacity */
    int capacity;

    /* number of stored items */
    int stored;

    /* stored items - ring buffer */
    ga_chr_t *chromosomes;

    /* pointer to beginning of ring buffer - where new item will be
       stored */
    int pointer;

    /* genome-specific functions */
    ga_copy_genome_func_t copy_func;
    ga_free_genome_func_t free_func;
};
typedef struct archive* archive_t;


/**
 * Allocate memory for and initialize new archive
 *
 * @param  size Archive size
 * @param  problem-specific genome allocation function
 * @param  problem-specific genome de-allocation function
 * @param  problem-specific genome copying function
 * @return pointer to created archive
 */
archive_t arc_create(int capacity, ga_alloc_genome_func_t alloc_func,
    ga_free_genome_func_t free_func, ga_copy_genome_func_t copy_func);


/**
 * Release given archive from memory
 */
void arc_destroy(archive_t arc);


/**
 * Insert chromosome into archive
 *
 * Chromosome is copied into place and pointer to it is returned.
 *
 * @param  arc
 * @param  chr
 * @return pointer to stored chromosome in archive
 */
ga_chr_t arc_insert(archive_t arc, ga_chr_t chr);

