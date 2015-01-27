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

#include <assert.h>

#include "../cpu.h"
#include "../inputdata.h"


bool input_data_load(input_data_t *data, config_t *config)
{
    if ((data->img_original = img_load(config->input_image)) == NULL) {
        fprintf(stderr, "Failed to load original image or no filename given.\n");
        return false;
    }

    if ((data->img_noisy = img_load(config->noisy_image)) == NULL) {
        fprintf(stderr, "Failed to load noisy image or no filename given.\n");
        return false;
    }

    assert(data->img_original->width == data->img_noisy->width);
    assert(data->img_original->height == data->img_noisy->height);
    assert(data->img_original->comp == data->img_noisy->comp);

    data->img_noisy_windows = img_split_windows(data->img_noisy);

    if (can_use_simd()) {
        img_split_windows_simd(data->img_noisy, data->img_noisy_simd);
    }

    data->fitness_cases = data->img_original->width * data->img_original->height;
    return true;
}



void input_data_destroy(input_data_t *data)
{
    img_destroy(data->img_original);
    img_destroy(data->img_noisy);
}
