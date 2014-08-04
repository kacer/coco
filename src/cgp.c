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
#include <string.h>

#include "cgp.h"
#include "random.h"


void cgp_randomize_gene(cgp_chr chr, int gene);


typedef struct {
    int size;
    int *values;
} int_array;

int_array allowed_vals[CGP_COLS];
cgp_fitness_func fitness_function;
cgp_problem_type problem_type;


#ifdef TEST_RANDOMIZE
    #define TEST_RANDOMIZE_PRINTF(...) printf(__VA_ARGS__)
#else
    #define TEST_RANDOMIZE_PRINTF(...) /* printf(__VA_ARGS__) */
#endif


/**
 * Returns true if WHAT is better or same as COMPARED_TO
 * @param  what
 * @param  compared_to
 * @return
 */
static inline bool cgp_is_better_or_same(cgp_fitness_t what, cgp_fitness_t compared_to) {
    return (problem_type == minimize)? what <= compared_to : what >= compared_to;
}



/**
 * Initialize CGP internals
 * @param Fitness function to use
 * @param Type of solved problem
 */
void cgp_init(cgp_fitness_func fitness, cgp_problem_type type)
{
    // remember fitness function and problem type
    fitness_function = fitness;
    problem_type = type;

    // calculate allowed values of node inputs in each column
    for (int x = 0; x < CGP_COLS; x++) {

        // range of outputs which can be connected to node in i-th column
        // maximum is actually by 1 larger than maximum allowed value
        // (so there is `val < maximum` in the for loop below instead of `<=`)
        int minimum = CGP_ROWS * (x - CGP_LBACK) + CGP_INPUTS;
        if (minimum < CGP_INPUTS) minimum = CGP_INPUTS;
        int maximum = CGP_ROWS * x + CGP_INPUTS;

        int size = CGP_INPUTS + maximum - minimum;
        allowed_vals[x].size = size;
        allowed_vals[x].values = (int*) malloc(sizeof(int) * size);

        int key = 0;
        // primary input indexes
        for (int val = 0; val < CGP_INPUTS; val++, key++) {
            allowed_vals[x].values[key] = val;
        }

        // nodes to the left output indexes
        for (int val = minimum; val < maximum; val++, key++) {
            allowed_vals[x].values[key] = val;
        }
    }


#ifdef TEST_INIT
    for (int x = 0; x < CGP_COLS; x++) {
        printf ("x = %d: ", x);
        for (int y = 0; y < allowed_vals[x].size; y++) {
            if (y > 0) printf(", ");
            printf ("%d", allowed_vals[x].values[y]);
        }
        printf("\n");
    }
#endif
}


/**
 * Deinitialize CGP internals
 */
void cgp_deinit()
{
    for (int x = 0; x < CGP_COLS; x++) {
        free(allowed_vals[x].values);
    }
}


/**
 * Create a new chromosome
 * @return
 */
cgp_chr cgp_create_chr()
{
    cgp_chr new_chr = (cgp_chr) malloc(sizeof(_cgp_chr));
    new_chr->has_fitness = false;

    for (int i = 0; i < CGP_CHR_LENGTH; i++) {
        cgp_randomize_gene(new_chr, i);
    }

    return new_chr;
}


/**
 * Clear memory associated with given chromosome
 * @param chr
 */
void cgp_destroy_chr(cgp_chr chr)
{
    free(chr);
}


/**
 * Replace chromosome genes with genes from other chromomose
 * @param  chr
 * @param  replacement
 * @return
 */
void cgp_replace_chr(cgp_chr chr, cgp_chr replacement)
{
    memcpy(chr->nodes, replacement->nodes, sizeof(_cgp_node) * CGP_NODES);
    memcpy(chr->outputs, replacement->outputs, sizeof(int) * CGP_OUTPUTS);
    chr->fitness = replacement->fitness;
    chr->has_fitness = replacement->has_fitness;
}


/**
 * Mutate given chromosome
 * @param chr
 * @param max_changed_genes
 */
void cgp_mutate_chr(cgp_chr chr, int max_changed_genes)
{
    int genes_to_change = rand_range(0, max_changed_genes);
    for (int i = 0; i < genes_to_change; i++) {
        int gene = rand_range(0, CGP_CHR_LENGTH - 1);
        cgp_randomize_gene(chr, gene);
    }
    chr->has_fitness = false;
}


#define SWAP(A, B) (((A & 0x0F) << 4) | ((B & 0x0F)))
#define ADD_SAT(A, B) ((A > 0xFF - B) ? 0xFF : A + B)
#define MAX(A, B) ((A > B) ? A : B)
#define MIN(A, B) ((A < B) ? A : B)


/**
 * Calculate output of given chromosome and inputs
 * @param chr
 */
void cgp_get_output(cgp_chr chr, cgp_value_t *inputs, cgp_value_t *outputs)
{
    cgp_value_t inner_outputs[CGP_INPUTS + CGP_NODES];

    // copy primary inputs to working array
    memcpy(inner_outputs, inputs, sizeof(cgp_value_t) * CGP_INPUTS);

    for (int i = 0; i < CGP_NODES; i++) {
        _cgp_node *n = &(chr->nodes[i]);
        cgp_value_t A = inner_outputs[n->inputs[0]];
        cgp_value_t B = inner_outputs[n->inputs[1]];
        cgp_value_t Y;

        switch (n->function) {
            case c255:          Y = 255;            break;
            case identity:      Y = A;              break;
            case inversion:     Y = 255 - A;        break;
            case b_or:          Y = A | B;          break;
            case b_not1or2:     Y = ~A | B;         break;
            case b_and:         Y = A & B;          break;
            case b_nand:        Y = ~(A & B);       break;
            case b_xor:         Y = A ^ B;          break;
            case rshift1:       Y = A >> 1;         break;
            case rshift2:       Y = A >> 2;         break;
            case swap:          Y = SWAP(A, B);     break;
            case add:           Y = A + B;          break;
            case add_sat:       Y = ADD_SAT(A, B);  break;
            case avg:           Y = (A + B) >> 1;   break;
            case max:           Y = MAX(A, B);      break;
            case min:           Y = MIN(A, B);      break;
        }

        inner_outputs[CGP_INPUTS + i] = Y;
    }

    for (int i = 0; i < CGP_OUTPUTS; i++) {
        outputs[i] = inner_outputs[chr->outputs[i]];
    }

#ifdef TEST_EVAL
    for (int i = 0; i < CGP_INPUTS + CGP_NODES; i++) {
        printf("%c: %u = %u\n", (i < CGP_INPUTS? 'I' : 'N'), i, inner_outputs[i]);
    }
    for (int i = 0; i < CGP_OUTPUTS; i++) {
        printf("O: %u = %u\n", i, outputs[i]);
    }
#endif
}


/**
 * Calculate fitness of given chromosome, but only if its `has_fitness`
 * attribute is set to `false`
 * @param chr
 */
cgp_fitness_t cgp_evaluate_chr(cgp_chr chr)
{
    if (!chr->has_fitness) {
        return cgp_reevaluate_chr(chr);
    } else {
        return chr->fitness;
    }
}


/**
 * Calculate fitness of given chromosome, regardless of its `has_fitness`
 * value
 * @param chr
 */
cgp_fitness_t cgp_reevaluate_chr(cgp_chr chr)
{
    chr->fitness = (*fitness_function)(chr);
    chr->has_fitness = true;
    return chr->fitness;
}


/**
 * Replace gene on given locus with random alele
 * @param chr
 * @param gene
 */
void cgp_randomize_gene(cgp_chr chr, int gene)
{
    if (gene >= CGP_CHR_LENGTH)
        return;

    if (gene < CGP_CHR_OUTPUTS_INDEX) {
        // mutating node input or function
        int node_index = gene / 3;
        int gene_index = gene % 3;
        int col = cgp_node_col(node_index);

        TEST_RANDOMIZE_PRINTF("gene %u, node %u, gene %u, col %u\n", gene, node_index, gene_index, col);

        if (gene_index == CGP_FUNC_INPUTS) {
            // mutating function
            chr->nodes[node_index].function = rand_range(0, CGP_FUNC_COUNT - 1);
            TEST_RANDOMIZE_PRINTF("func 0 - %u\n", CGP_FUNC_COUNT - 1);

        } else {
            // mutating input
            chr->nodes[node_index].inputs[gene_index] = rand_schoice(allowed_vals[col].size, allowed_vals[col].values);
            TEST_RANDOMIZE_PRINTF("input choice from %u\n", allowed_vals[col].size);
        }

    } else {
        // mutating primary output connection
        int index = gene - CGP_CHR_OUTPUTS_INDEX;
        chr->outputs[index] = rand_range(0, CGP_INPUTS + CGP_NODES - 1);
        TEST_RANDOMIZE_PRINTF("out 0 - %u\n", CGP_INPUTS + CGP_NODES - 1);
    }
}


/* population *****************************************************************/


/**
 * Create a new CGP population with given size
 * @param  size
 * @return
 */
cgp_pop cgp_create_pop(int size)
{
    cgp_pop new_pop = (cgp_pop) malloc(sizeof(_cgp_pop));
    if (new_pop == NULL) return NULL;

    new_pop->size = size;
    new_pop->best_chr_index = -1;
    new_pop->chromosomes = (cgp_chr*) malloc(sizeof(cgp_chr) * size);
    if (new_pop->chromosomes == NULL) {
        free(new_pop);
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        new_pop->chromosomes[i] = cgp_create_chr();
    }

    return new_pop;
}


/**
 * Clear memory associated with given population (including its chromosomes)
 * @param pop
 */
void cgp_destroy_pop(cgp_pop pop)
{
    if (pop != NULL) {
        for (int i = 0; i < pop->size; i++) {
            cgp_destroy_chr(pop->chromosomes[i]);
        }
        free(pop->chromosomes);
    }
    free(pop);
}


/**
 * Calculate fitness of whole population, using `cgp_evaluate_chr`
 * @param chr
 */
void cgp_evaluate_pop(cgp_pop pop)
{
    cgp_fitness_t best_fitness;
    int best_index;

    // reevaluate population
    for (int i = 0; i < pop->size; i++) {
        cgp_fitness_t f = cgp_evaluate_chr(pop->chromosomes[i]);
        if (i == 0 || cgp_is_better_or_same(f, best_fitness)) {
            best_fitness = f;
            best_index = i;
        }
    }

    // if best index hasn't changed, try to find different one with the same fitness
    if (best_index == pop->best_chr_index) {
        for (int i = 0; i < pop->size; i++) {
            cgp_fitness_t f = pop->chromosomes[i]->fitness;
            if (i != best_index && f == best_fitness) {
                best_index = i;
                break;
            }
        }
    }

    // set new best values
    pop->best_fitness = best_fitness;
    pop->best_chr_index = best_index;
}


/**
 * Advance population to next generation
 * @param pop
 * @param mutation_rate
 */
void cgp_next_generation(cgp_pop pop, int mutation_rate)
{
    cgp_chr parent = pop->chromosomes[pop->best_chr_index];

    for (int i = 0; i < pop->size; i++) {
        if (i == pop->best_chr_index) continue;
        cgp_replace_chr(pop->chromosomes[i], parent);
        cgp_mutate_chr(pop->chromosomes[i], mutation_rate);
    }

    cgp_evaluate_pop(pop);
}
