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
#include <assert.h>
#include <immintrin.h>

#include "cgp_sse.h"


static inline void _set_output(cgp_value_t *output_data,
    int output_idx, __m128i value)
{
    __m128i *dest = (__m128i*)(&output_data[output_idx * SSE2_BLOCK_SIZE]);
    _mm_store_si128(dest, value);
}


static inline __m128i *_get_output(cgp_value_t *output_data,
    int output_idx)
{
    return (__m128i*)(&output_data[output_idx * SSE2_BLOCK_SIZE]);
}


#ifdef TEST_EVAL_SSE2
    #define UCFMT1 "%u"
    #define UCFMT4 UCFMT1 ", " UCFMT1 ", " UCFMT1 ", " UCFMT1
    #define UCFMT16 UCFMT4 ", " UCFMT4 ", " UCFMT4 ", " UCFMT4

    #define UCVAL1(n) _tmp[n]
    #define UCVAL4(n) UCVAL1(n), UCVAL1(n+1), UCVAL1(n+2), UCVAL1(n+3)
    #define UCVAL16(n) UCVAL4(n), UCVAL4(n+4), UCVAL4(n+8), UCVAL4(n+12)

    #define PRINT_REG(reg) do { \
        __m128i _tmpval = reg; \
        unsigned char *_tmp = (unsigned char*) &_tmpval; \
        printf(UCFMT16 "\n", UCVAL16(0)); \
    } while(0);
#endif



#ifdef SSE2

    /**
     * Calculate output of given chromosome and inputs using SSE instructions
     * @param chr
     * @param input_data
     * @param input_offset offset from which to copy input data to SSE registers
     * @param output_data will be written here
     * @return number of items written to output_data
     */
    int cgp_sse_get_output(
        ga_chr_t chromosome,
        cgp_value_t *input_data,
        int input_offset,
        cgp_value_t *output_data,
        int row_length,
        bool *should_restart)
    {
        cgp_genome_t genome = (cgp_genome_t) chromosome->genome;

        // previous and currently computed column
        __m128i_aligned inner_outputs[genome->inputs_count + cgp_nodes_count(genome)];

        // copy primary inputs to working array
        for (int i = 0; i < genome->inputs_count; i++) {
            inner_outputs[i] = _mm_load_si128((__m128i*)(&input_data[i * row_length + input_offset]));
        }

        for (int i = 0; i < cgp_nodes_count(genome); i++) {
            cgp_node_t *n = &(genome->nodes[i]);

            // skip inactive blocks
            if (!n->is_active) continue;

            __m128i A = inner_outputs[n->inputs[0]];
            __m128i B = inner_outputs[n->inputs[1]];
            __m128i Y = cgp_sse_get_node_output(n, A, B, should_restart);

            if (*should_restart) {
                return 0;
            }

            inner_outputs[genome->inputs_count + i] = Y;
        }

        for (int i = 0; i < genome->outputs_count; i++) {
            __m128i Y = inner_outputs[genome->outputs[i]];
            _set_output(output_data, i, Y);
        }


    #ifdef TEST_EVAL_SSE2
        for (int i = 0; i < genome->inputs_count + cgp_nodes_count(genome); i++) {
            unsigned char *_tmp = (unsigned char*) inner_outputs[i];
            printf("%c: %2d = " UCFMT16 "\n", (i < genome->inputs_count? 'I' : 'N'), i, UCVAL16(0));
        }
        for (int i = 0; i < genome->outputs_count; i++) {
            unsigned char *_tmp = (unsigned char*) _get_output(output_data, i);
            printf("O: %2d = " UCFMT16 "\n", i, UCVAL16(0));
        }
    #endif

        return SSE2_BLOCK_SIZE;
    }

#else

    int cgp_get_output_sse(
        ga_chr_t chromosome,
        cgp_value_t *input_data[genome->inputs_count],
        int input_offset,
        cgp_value_t *output_data,
        bool *should_restart)
    {
        assert(false);
    }

#endif
