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


#include <stdlib.h>

#include "archive.h"



/**
 * Allocate memory for and initialize new archive
 *
 * @param  size Archive size
 * @param  problem-specific genome function pointers
 * @return pointer to created archive
 */
archive_t arc_create(int capacity, arc_func_vect_t methods)
{
    archive_t arc = (archive_t) malloc(sizeof(struct archive));
    if (arc == NULL) {
        return NULL;
    }

    ga_chr_t *items = (ga_chr_t*) malloc(sizeof(ga_chr_t) * capacity);

    for (int i = 0; i < capacity; i++) {
        items[i] = ga_alloc_chr(methods.alloc_genome);
        if (items[i] == NULL) {
            for (int x = i - 1; x >= 0; i--) {
                ga_free_chr(items[x], methods.free_genome);
            }
            free(arc);
            return NULL;
        }
    }

    arc->chromosomes = items;
    arc->capacity = capacity;
    arc->stored = 0;
    arc->pointer = 0;
    arc->methods = methods;
    return arc;
}


/**
 * Release given archive from memory
 */
void arc_destroy(archive_t arc)
{
    for (int i = 0; i < arc->capacity; i++) {
        ga_free_chr(arc->chromosomes[i], arc->methods.free_genome);
    }
    free(arc->chromosomes);
    free(arc);
}


/**
 * Insert chromosome into archive
 *
 * Chromosome is copied into place and pointer to it is returned.
 *
 * Chromosome is reevaluated using `arc->methods.fitness` (if set).
 *
 * @param  arc
 * @param  chr
 * @return pointer to stored chromosome in archive
 */
ga_chr_t arc_insert(archive_t arc, ga_chr_t chr)
{
    int target_index = arc_real_index(arc, 0);
    ga_chr_t dst = arc->chromosomes[target_index];
    ga_copy_chr(dst, chr, arc->methods.copy_genome);

    if (arc->stored < arc->capacity) {
        arc->stored++;
    }
    arc->pointer = (arc->pointer + 1) % arc->capacity;

    if (arc->methods.fitness != NULL) {
        dst->fitness = arc->methods.fitness(dst);
        dst->has_fitness = true;
    }

    return dst;
}
