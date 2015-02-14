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


#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cgp_core.h"
#include "../random.h"
#include "../fitness.h"


typedef struct {
    int size;
    int *values;
} int_array;


static int_array *_allowed_gene_vals;
static cgp_settings_t *_settings;


#ifdef CGP_LIMIT_FUNCS
    static int _allowed_functions_list[] = {
        c255,
        identity,
        add,
        add_sat,
        inversion,
        max,
        min
    };

    static int_array _allowed_functions = {
        .size = 7,
        .values = _allowed_functions_list
    };
#endif


#ifdef TEST_RANDOMIZE
    #define TEST_RANDOMIZE_PRINTF(...) printf(__VA_ARGS__)
#else
    #define TEST_RANDOMIZE_PRINTF(...) /* printf(__VA_ARGS__) */
#endif



/**
 * Initialize CGP internals
 */
void cgp_init(cgp_settings_t *settings)
{
    _settings = settings;
    _allowed_gene_vals = (int_array*) malloc(sizeof(int_array) * settings->cols);

    // calculate allowed values of node inputs in each column
    for (int x = 0; x < _settings->cols; x++) {
        // range of outputs which can be connected to node in i-th column
        // maximum is actually by 1 larger than maximum allowed value
        // (so there is `val < maximum` in the for loop below instead of `<=`)
        int minimum = _settings->rows * (x - _settings->lback) + _settings->inputs;
        if (minimum < _settings->inputs) minimum = _settings->inputs;
        int maximum = _settings->rows * x + _settings->inputs;

        int size = _settings->inputs + maximum - minimum;
        _allowed_gene_vals[x].size = size;
        _allowed_gene_vals[x].values = (int*) malloc(sizeof(int) * size);

        int key = 0;
        // primary inputs
        for (int val = 0; val < _settings->inputs; val++, key++) {
            _allowed_gene_vals[x].values[key] = val;
        }

        // nodes to the left
        for (int val = minimum; val < maximum; val++, key++) {
            _allowed_gene_vals[x].values[key] = val;
        }
    }


#ifdef TEST_INIT
    for (int x = 0; x < _settings->cols; x++) {
        printf ("x = %d: ", x);
        for (int y = 0; y < _allowed_gene_vals[x].size; y++) {
            if (y > 0) printf(", ");
            printf ("%d", _allowed_gene_vals[x].values[y]);
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
    for (int x = 0; x < _settings->cols; x++) {
        free(_allowed_gene_vals[x].values);
    }
}


/**
 * Create a new cgp population with given size
 * @param  mutation rate (in number of genes)
 * @param  population size
 * @return
 */
ga_pop_t cgp_init_pop(int pop_size)
{
    /* prepare methods vector */
    ga_func_vect_t methods = {
        .alloc_genome = cgp_alloc_genome,
        .free_genome = cgp_free_genome,
        .init_genome = cgp_randomize_genome,

        .fitness = _settings->fitness_function,
        .offspring = cgp_offspring,
    };

    /* initialize GA */
    ga_pop_t pop = ga_create_pop(pop_size, CGP_PROBLEM_TYPE, methods);
    return pop;
}


/**
 * Returns l-back parameter value
 */
int cgp_get_lback()
{
    return _settings->lback;
}


/* chromosome *****************************************************************/


/**
 * Allocates memory for new CGP genome
 * @param chromosome
 */
void* cgp_alloc_genome()
{
    cgp_genome_t genome = malloc(sizeof(struct cgp_genome));
    if (genome) {
        genome->inputs_count = _settings->inputs;
        genome->outputs_count = _settings->outputs;
        genome->cols = _settings->cols;
        genome->rows = _settings->rows;

        genome->nodes = (cgp_node_t*)
            malloc(sizeof(cgp_node_t) * cgp_nodes_count(genome));
        genome->outputs = (int*) malloc(sizeof(int) * genome->outputs_count);
    }
    return genome;
}


/**
 * Initializes CGP genome to random values
 * @param chromosome
 */
int cgp_randomize_genome(ga_chr_t chromosome)
{
    cgp_genome_t genome = (cgp_genome_t) chromosome->genome;

    for (int i = 0; i < cgp_genome_length(genome); i++) {
        cgp_randomize_gene(genome, i);
    }

    cgp_find_active_blocks(chromosome);
    chromosome->has_fitness = false;

    return 0;
}


/**
 * Deinitialize CGP genome
 * @param  chromosome
 * @return
 */
void cgp_free_genome(void *genome)
{
    if (genome) {
        cgp_genome_t _genome = (cgp_genome_t) genome;
        free(_genome->nodes);
        free(_genome->outputs);
    }
    free(genome);
}


/* mutation *******************************************************************/


/**
 * Replace gene on given locus with random alele
 * @param chr
 * @param gene
 * @return whether active node was changed or not (phenotype has changed)
 */
bool cgp_randomize_gene(cgp_genome_t genome, int gene)
{
    if (gene >= cgp_genome_length(genome))
        return false;

    if (gene < cgp_output_genes_offset(genome)) {
        // mutating node input or function
        int node_index = gene / 3;
        int gene_index = gene % 3;
        int col = cgp_node_col(genome, node_index);

        TEST_RANDOMIZE_PRINTF("gene %u, node %u, gene %u, col %u\n", gene, node_index, gene_index, col);

        if (gene_index == CGP_FUNC_INPUTS) {
            // mutating function
            #ifdef CGP_LIMIT_FUNCS
                genome->nodes[node_index].function = (cgp_func_t) rand_schoice(_allowed_functions.size, _allowed_functions.values);
            #else
                genome->nodes[node_index].function = (cgp_func_t) rand_range(0, CGP_FUNC_COUNT - 1);
            #endif
            genome->nodes[node_index].is_constant = false;
            TEST_RANDOMIZE_PRINTF("func 0 - %u\n", CGP_FUNC_COUNT - 1);
            return genome->nodes[node_index].is_active;

        } else {
            // mutating input
            genome->nodes[node_index].inputs[gene_index] = rand_schoice(_allowed_gene_vals[col].size, _allowed_gene_vals[col].values);
            genome->nodes[node_index].is_constant = false;
            TEST_RANDOMIZE_PRINTF("input choice from %u\n", _allowed_gene_vals[col].size);
            return genome->nodes[node_index].is_active;
        }

    } else {
        // mutating primary output connection
        int index = gene - cgp_output_genes_offset(genome);
        genome->outputs[index] = rand_range(genome->inputs_count, genome->inputs_count + cgp_nodes_count(genome) - 1);
        TEST_RANDOMIZE_PRINTF("out %u - %u\n", genome->inputs_count, genome->inputs_count + cgp_nodes_count(genome) - 1);
        return true;
    }
}


/**
 * Mutate given chromosome
 * @param chr
 */
void cgp_mutate_chr(ga_chr_t chromosome)
{
    cgp_genome_t genome = (cgp_genome_t) chromosome->genome;
    assert(_settings->mutation_rate <= cgp_genome_length(genome));

    int genes_to_change = rand_range(1, _settings->mutation_rate);
    for (int i = 0; i < genes_to_change; i++) {
        int gene = rand_range(0, cgp_genome_length(genome) - 1);
        cgp_randomize_gene(genome, gene);
    }

    cgp_find_active_blocks(chromosome);
    chromosome->has_fitness = false;
}


/**
 * Replace chromosome genes with genes from other chromomose
 * @param  chr
 * @param  replacement
 * @return
 */
void cgp_copy_genome(void *_dst, void *_src)
{
    cgp_genome_t dst = (cgp_genome_t) _dst;
    cgp_genome_t src = (cgp_genome_t) _src;

    memcpy(dst->nodes, src->nodes, sizeof(cgp_node_t) * cgp_nodes_count(src));
    memcpy(dst->outputs, src->outputs, sizeof(int) * src->outputs_count);
}


#define SWAP(A, B) (((A & 0x0F) << 4) | ((B & 0x0F)))
#define ADD_SAT(A, B) ((A > 0xFF - B) ? 0xFF : A + B)
#define MAX(A, B) ((A > B) ? A : B)
#define MIN(A, B) ((A < B) ? A : B)


/* evaluation *****************************************************************/


/**
 * Calculate output of given chromosome and inputs
 * @param chr
 * @return Whether CGP function has changed and evaluation should be restarted
 */
bool cgp_get_output(ga_chr_t chromosome, cgp_value_t *inputs, cgp_value_t *outputs)
{
    cgp_genome_t genome = (cgp_genome_t) chromosome->genome;
    cgp_value_t inner_outputs[genome->inputs_count + cgp_nodes_count(genome)];

    // copy primary inputs to working array
    memcpy(inner_outputs, inputs, sizeof(cgp_value_t) * genome->inputs_count);

    for (int i = 0; i < cgp_nodes_count(genome); i++) {
        cgp_node_t *n = &(genome->nodes[i]);

        // skip inactive blocks
        if (!n->is_active) continue;

        cgp_value_t A = inner_outputs[n->inputs[0]];
        cgp_value_t B = inner_outputs[n->inputs[1]];
        cgp_value_t Y;

        if (n->is_constant) {
            Y = n->constant_value;

        } else {
            bool should_restart = cgp_get_node_output(n, A, B, &Y);
            if (should_restart) {
                return true;
            }
        }

        inner_outputs[genome->inputs_count + i] = Y;
    }

    for (int i = 0; i < genome->outputs_count; i++) {
        outputs[i] = inner_outputs[genome->outputs[i]];
    }

#ifdef TEST_EVAL
    for (int i = 0; i < genome->inputs_count + cgp_nodes_count(genome); i++) {
        printf("%c: %u = %u\n", (i < genome->inputs_count? 'I' : 'N'), i, inner_outputs[i]);
    }
    for (int i = 0; i < genome->outputs_count; i++) {
        printf("O: %u = %u\n", i, outputs[i]);
    }
#endif

    return false;
}


/**
 * Finds which blocks are active.
 * @param chromosome
 * @param active
 */
void cgp_find_active_blocks(ga_chr_t chromosome)
{
    cgp_genome_t genome = (cgp_genome_t) chromosome->genome;

    // first mark all nodes as inactive
    for (int i = cgp_nodes_count(genome) - 1; i >= 0; i--) {
        genome->nodes[i].is_active = false;
    }

    // mark inputs of primary outputs as active
    for (int i = 0; i < genome->outputs_count; i++) {
        int index = genome->outputs[i] - genome->inputs_count;
        // index may be negative (primary input), so do the check...
        if (index >= 0) {
            genome->nodes[index].is_active = true;
        }
    }

    // then walk nodes backwards and mark inputs of active nodes
    // as active nodes
    for (int i = cgp_nodes_count(genome) - 1; i >= 0; i--) {
        if (!genome->nodes[i].is_active) continue;
        cgp_node_t *n = &(genome->nodes[i]);

        int arity = CGP_FUNC_ARITY[n->function];
        for (int k = 0; k < arity; k++) {
            int index = n->inputs[k] - genome->inputs_count;
            // index may be negative (primary input), so do the check...
            if (index >= 0) {
                genome->nodes[index].is_active = true;
            }
        }
    }
}



/* population *****************************************************************/


/**
 * Create new generation
 * @param pop
 * @param mutation_rate
 */
void cgp_offspring(ga_pop_t pop)
{
    ga_chr_t parent = pop->best_chromosome;

    #pragma omp parallel for
    for (int i = 0; i < pop->size; i++) {
        ga_chr_t chr = pop->chromosomes[i];
        if (chr == parent) continue;
        ga_copy_chr(chr, parent, cgp_copy_genome);
        cgp_mutate_chr(chr);
    }
}
