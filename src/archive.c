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


static inline int arc_real_index(archive_t arc, int index)
{
    if (arc->stored < arc->capacity) {
        return index;

    } else {
        int real = (arc->pointer + index) % arc->capacity;
        if (real < 0) real += arc->capacity;
        return real;
    }
}


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
    ga_free_genome_func_t free_func, ga_copy_genome_func_t copy_func)
{
    archive_t arc = (archive_t) malloc(sizeof(struct archive));
    if (arc == NULL) {
        return NULL;
    }

    ga_chr_t *items = (ga_chr_t*) malloc(sizeof(ga_chr_t) * capacity);

    for (int i = 0; i < capacity; i++) {
        items[i] = ga_alloc_chr(alloc_func);
        if (items[i] == NULL) {
            for (int x = i - 1; x >= 0; i--) {
                ga_free_chr(items[x], free_func);
            }
            free(arc);
            return NULL;
        }
    }

    arc->chromosomes = items;
    arc->capacity = capacity;
    arc->stored = 0;
    arc->pointer = 0;
    arc->free_func = free_func;
    arc->copy_func = copy_func;
    return arc;
}


/**
 * Release given archive from memory
 */
void arc_destroy(archive_t arc)
{
    for (int i = 0; i < arc->capacity; i++) {
        ga_free_chr(arc->chromosomes[i], arc->free_func);
    }
    free(arc->chromosomes);
    free(arc);
}


/**
 * Insert chromosome into archive
 *
 * Chromosome is copied into place and pointer to it is returned.
 *
 * @param  arc
 * @param  chr
 * @return pointer to stored chromosome in archive
 */
ga_chr_t arc_insert(archive_t arc, ga_chr_t chr)
{
    int target_index = arc_real_index(arc, 0);
    ga_chr_t dst = arc->chromosomes[target_index];
    ga_copy_chr(dst, chr, arc->copy_func);

    if (arc->stored < arc->capacity) {
        arc->stored++;
    }
    arc->pointer = (arc->pointer + 1) % arc->capacity;

    return dst;
}
