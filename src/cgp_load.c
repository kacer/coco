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
#include <stdarg.h>

#include "cgp.h"


/**
 * Loads chromosome from given file stored in CGP-viewer compatible format
 * @param chr
 * @param fp
 * @return 0 on success, -1 on file format error, -2 on incompatible CGP config
 */
int cgp_load_chr_compat(cgp_chr chr, FILE *fp)
{
    int inputs, outputs, cols, rows, func_inputs, func_outputs, func_count;
    int count;

    count = fscanf(fp, "{%u, %u, %u, %u, %u, %u, %u}", &inputs, &outputs,
        &cols, &rows, &func_inputs, &func_outputs, &func_count);


    if (count != 7) return -1;
    if (inputs != CGP_INPUTS) return -2;
    if (outputs != CGP_OUTPUTS) return -2;
    if (cols != CGP_COLS) return -2;
    if (rows != CGP_ROWS) return -2;
    if (func_inputs != CGP_FUNC_INPUTS) return -2;
    if (func_outputs != 1) return -2;
    if (func_count != CGP_FUNC_COUNT) return -2;

    // nodes


    for (int i = 0; i < CGP_NODES; i++) {
        _cgp_node *n = &(chr->nodes[i]);

        int nodeid;
        count = fscanf(fp, "([%u] %u, %u, %u)",
            &nodeid, &n->inputs[0], &n->inputs[1], &n->function);
        if (count != 4) return -1;
        if (nodeid != CGP_INPUTS + i) return -1;
    }

    // primary outputs


    fscanf(fp, "(");
    for (int i = 0; i < CGP_OUTPUTS; i++) {
        if (i > 0) fscanf(fp, ",");
        count = fscanf(fp, "%u", &chr->outputs[i]);
        if (count != 1) return -1;
    }
    fscanf(fp, ")\n");

    return 0;
}


/**
 * Loads whole population from given file with chromosomes stored in
 * CGP-viewer compatible format
 * @param pop_ptr
 * @param fp
 * @return 0 on success, -1 on file format error, -2 on incompatible CGP config
 */
int cgp_load_pop_compat(cgp_pop *pop_ptr, FILE *fp)
{
    int generation;
    int size;
    int best_index;

    int count;
    count = fscanf(fp, "Generation: %d\n"
                       "Best chromosome: %d\n"
                       "Chromosomes: %d\n", &generation, &best_index, &size);
    if (count != 3) {
        return -1;
    }

    cgp_pop population = cgp_create_pop(size);
    population->generation = generation;
    population->best_chr_index = best_index;

    for (int i = 0; i < size; i++) {
        int retval = cgp_load_chr_compat(population->chromosomes[i], fp);
        if (retval != 0) {
            return retval;
        }
    }

    cgp_evaluate_pop(population);
    *pop_ptr = population;
    return 0;
}
