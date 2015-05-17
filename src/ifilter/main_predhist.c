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


void get_stats(long *histogram, int length, long *min, long *min_nonzero, long *max) {
    *min = LONG_MAX;
    *min_nonzero = LONG_MAX;
    *max = 0;

    for (int i = 0; i < length; i++) {
        if (histogram[i] > *max) {
            *max = histogram[i];
        }
        if (histogram[i] < *min) {
            *min = histogram[i];
        }
        if (histogram[i] && histogram[i] < *min_nonzero) {
            *min_nonzero = histogram[i];
        }
    }
}


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
    int image_size;
    int image_width = -1;
    int image_height = -1;
    int max_generations = -1;
    int option_index = -1;
    int file_count = 0;
    int first_file_arg;

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

    file_count = argc - optind;
    first_file_arg = optind;

    if (!file_count) {
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
    long *histogram = (long*) calloc(image_size, sizeof(long));
    bool *included = (bool*) malloc(image_size * sizeof(bool));
    long *file_histogram[file_count];

    long max_count;
    long min_count;
    long min_nonzero_count;

    for (int i = 0; i < file_count; i++) {
        file_histogram[i] = (long*) calloc(image_size, sizeof(long));
    }

    /*
        Iterate over log files
     */

    for (int file_index = 0; file_index < file_count; file_index++) {

        char *filename = argv[first_file_arg + file_index];
        FILE *log_file = fopen(filename, "rt");
        bool first_entry = true;
        int last_generation = 0;

        /* fscanf outputs */
        int count, generation, length;
        int delta_generation;

        fprintf(stderr, "%s: ", filename);

        if (!log_file) {
            fprintf(stderr, "Cannot open\n");
            return 1;
        }

        do {

            count = fscanf(log_file, "Generation %d: Predictor phenotype length %d [ ", &generation, &length);
            if (count == EOF) {
                break;
            }
            if (count < 2) {
                fprintf(stderr, "Invalid log file? Cannot fscanf start of line (loaded %d values)\n", count);
                break;
            }

            if (generation > max_generations) {
                generation = max_generations;
            }

            //fprintf(stderr, "\r%s: Generation %6d, length %6d", filename, generation, length);

            delta_generation = generation - last_generation;

            if (!first_entry) {
                for (int i = 0; i < image_size; i++) {
                    if (included[i]) {
                        histogram[i] += delta_generation;
                        file_histogram[file_index][i] += delta_generation;
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
                    fprintf(stderr, "Invalid log file? Cannot fscanf pixel %d (loaded %d values)\n", i, count);
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
                    file_histogram[file_index][i] += delta_generation;
                }
            }
        }

        get_stats(file_histogram[file_index], image_size, &min_count, &min_nonzero_count, &max_count);
        fprintf(stderr, "min %ld / %ld, max %ld\n", min_count, min_nonzero_count, max_count);
    }


    /**
     * Dump results
     */

    get_stats(histogram, image_size, &min_count, &min_nonzero_count, &max_count);
    fprintf(stderr, "TOTAL: min %ld / %ld, max %ld\n", min_count, min_nonzero_count, max_count);

    printf("index,total");
    for (int i = 0; i < file_count; i++) {
        printf(",\"%s\"", argv[first_file_arg + i]);
    }
    printf("\n");

    for (int i = 0; i < image_size; i++) {
        printf("%d,%ld", i, histogram[i]);
        for (int fi = 0; fi < file_count; fi++) {
            printf(",%ld", file_histogram[fi][i]);
        }
        printf("\n");

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
