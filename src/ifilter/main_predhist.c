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
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "image.h"
#include "../utils.h"


const char* help =
    "Colearning in Coevolutionary Algorithms\n"
    "Bc. Michal Wiglasz <xwigla00@stud.fit.vutbr.cz>\n"
    "\n"
    "Master's Thesis\n"
    "2014/2015\n"
    "\n"
    "Supervisor: Ing. Michaela Šikulová <isikulova@fit.vutbr.cz>\n"
    "\n"
    "Faculty of Information Technologies\n"
    "Brno University of Technology\n"
    "http://www.fit.vutbr.cz/\n"
    "     _       _\n"
    "  __(.)=   =(.)__\n"
    "  \\___)     (___/\n"
    "\n"
    "\n"
    "Usage:\n"
    "    ./coco_predhist --width 256 --height 256 --generations 100000\n"
    "                    LOGFILES\n"
    "\n"
    "Prints pixel index and number of uses in predictors.\n"
    "\n"
    "Command line options:\n"
    "    --help, -h\n"
    "          Show this help and exit\n"
    "\n"
    "Required:\n"
    "    LOGFILES\n"
    "          One or more coco predictors log files (see --log-pred-file option)\n"
    "    --height N, -x N\n"
    "          Training image height\n"
    "    --width N, -y N\n"
    "          Training image width\n"
    "    --generations N, -g N\n"
    "          Stop after N generations\n"
    "\n"
    "Optional:\n"
    "    --output-image FILE, -o FILE\n"
    "          Render histogram as 2D image to given file\n"
    "";


/******************************************************************************/


int main(int argc, char *argv[])
{
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"height", required_argument, 0, 'y'},
        {"width", required_argument, 0, 'x'},
        {"output-image", required_argument, 0, 'o'},
        {"generations", required_argument, 0, 'g'},

        {0, 0, 0, 0}
    };

    static const char *short_options = "hl:x:y:o:g:";

    img_image_t output_image = NULL;
    char output_filename[MAX_FILENAME_LENGTH];
    bool render = false;
    long *histogram;
    bool *included;
    int image_size;
    int image_width = -1;
    int image_height = -1;
    int max_generations = -1;
    int option_index = -1;

    /*
        Parse command line
     */


    while (1) {
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (c == - 1) break;


        switch (c) {
            case 'h':
                puts(help);
                return 1;

            case 'x':
                image_width = atoi(optarg);
                break;

            case 'y':
                image_height = atoi(optarg);
                break;

            case 'g':
                max_generations = atoi(optarg);
                break;

            case 'o':
                if (strlen(optarg) > MAX_FILENAME_LENGTH - 1) {
                    fprintf(stderr, "Out dir name is too long (limit: %d chars)\n",
                        MAX_FILENAME_LENGTH - 1);
                    return 1;
                }
                strncpy(output_filename, optarg, MAX_FILENAME_LENGTH);
                render = true;
                break;

            default:
                fprintf(stderr, "Invalid arguments.\n");
                return 1;
        }
    }

    /*
        Check args
     */

    if (optind == argc) {
        fprintf(stderr, "No log files given.\n");
        return 1;
    }

    if (image_width <= 0) {
        fprintf(stderr, "No width given.\n");
        return 1;
    }

    if (image_height <= 0) {
        fprintf(stderr, "No height given.\n");
        return 1;
    }

    if (max_generations <= 0) {
        fprintf(stderr, "Number of generations not given.\n");
        return 1;
    }

    if (render) {
        output_image = img_create(image_width, image_height, 1);
        if (!output_image) {
            fprintf(stderr, "Failed to allocate memory for output image\n");
            return 1;
        }
    }

    image_size = image_height * image_width;
    histogram = (long*) calloc(image_size, sizeof(long));
    included = (bool*) malloc(image_size * sizeof(bool));

    /*
        Iterate over log files
     */

    while (optind < argc) {

        char *filename = argv[optind++];
        FILE *log_file = fopen(filename, "rt");
        bool first_entry = true;
        int last_generation = 0;

        /* fscanf outputs */
        int count, generation, length;
        int delta_generation;

        fprintf(stderr, "%s\n", filename);

        if (!log_file) {
            fprintf(stderr, "Cannot open %s\n", filename);
            return 1;
        }

        do {

            count = fscanf(log_file, "Generation %d: Predictor phenotype length %d [ ", &generation, &length);
            if (count < 2) break;

            if (generation > max_generations) {
                generation = max_generations;
            }

            delta_generation = generation - last_generation;

            if (!first_entry) {
                for (int i = 0; i < image_size; i++) {
                    if (included[i]) {
                        histogram[i] += delta_generation;
                    }
                }
            } else {
                first_entry = false;
            }

            if (generation == max_generations) {
                break;
            }

            // update histogram data
            memset(included, 0, sizeof(bool) * image_size);
            for (int i = 0; i < length; i++) {
                unsigned int index;
                count = fscanf(log_file, "%u", &index);
                if (count == 1) {
                    included[index] = true;
                } else {
                    fprintf(stderr, "Invalid log file.\n");
                    return 1;
                }
            }

            count = fscanf(log_file, " ] ");

            if (generation == max_generations) {
                break;
            }

            last_generation = generation;

        } while(true);

        fclose(log_file);

        if (generation != max_generations) {
            delta_generation = max_generations - last_generation;
            for (int i = 0; i < image_size; i++) {
                if (included[i]) {
                    histogram[i] += delta_generation;
                }
            }
        }
    }

    /**
     * Dump results
     */

    long max_count = 0;
    long min_count = LONG_MAX;

    if (render) {
        for (int i = 0; i < image_size; i++) {
            if (histogram[i] > max_count) {
                max_count = histogram[i];
            }
            if (histogram[i] < min_count) {
                min_count = histogram[i];
            }
        }
    }

    fprintf(stderr, "min count: %ld\n", min_count);
    fprintf(stderr, "max count: %ld\n", max_count);

    printf("index,usage\n");
    for (int i = 0; i < image_size; i++) {
        printf("%d,%ld\n", i, histogram[i]);
        if (render) {
            char shade = (histogram[i] / (double)max_count) * 255;
            output_image->data[i] = shade;
        }
    }


    // save png
    if (render) {
        img_save_png(output_image, output_filename);
    }

    img_destroy(output_image);
    free(histogram);
}
