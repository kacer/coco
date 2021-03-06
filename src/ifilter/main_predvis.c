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

#include "image.h"
#include "../utils.h"


#define MAX_OUTDIR_LENGTH (MAX_FILENAME_LENGTH - 14)
#define MAX_OUTFILE_LENGTH (MAX_OUTDIR_LENGTH + 1 + MAX_FILENAME_LENGTH)


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
    "To visualise predictors:\n"
    "    ./coco_predvis --log predictors.log --image noisy.png --outdir images\n"
    "\n"
    "To list entries in log file:\n"
    "    ./coco_predvis --list --log predictors.log\n"
    "\n"
    "Various input formats are supported. Output image will always be in PNG file\n"
    "format.\n"
    "\n"
    "Prints generation number and phenotype length for each entry in log file.\n"
    "\n"
    "Command line options:\n"
    "    --help, -h\n"
    "          Show this help and exit\n"
    "    --list\n"
    "          Only list predictors included in log file to stdout.\n"
    "\n"
    "Required:\n"
    "    --log FILE, -l FILE\n"
    "          Predictors log file generated by coco (see --log-pred-file option).\n"
    "\n"
    "Required for visualisation only:\n"
    "    --image FILE, -i FILE\n"
    "          Input image filename\n"
    "    --outdir FILE, -o FILE\n"
    "          Output directory where images will be written (created if missing).\n"
    "\n"
    "Optional:"
    "    --color RRGGBB, -c RRGGBB\n"
    "          Pixel highlight color in hex format, default is red (FF0000).\n"
    "";


/******************************************************************************/


int main(int argc, char *argv[])
{
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},

        {"log", required_argument, 0, 'l'},
        {"image", required_argument, 0, 'i'},
        {"outdir", required_argument, 0, 'o'},
        {"color", required_argument, 0, 'c'},
        {"list", no_argument, 0, 1000},

        {0, 0, 0, 0}
    };

    static const char *short_options = "hl:i:o:c:";

    img_image_t input_image = NULL;
    img_image_t output_image = NULL;
    int image_length = 0;
    FILE *log_file = NULL;
    char out_dir[MAX_OUTDIR_LENGTH];
    bool list_only = false;
    unsigned char red = 0xff;
    unsigned char green = 0x00;
    unsigned char blue = 0x00;

    /*
        Parse command line
     */


    while (1) {
        int option_index;
        int count, tmp;
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (c == - 1) break;

        switch (c) {
            case 'h':
                puts(help);
                return 1;

            case 'l':
                log_file = fopen(optarg, "rt");
                break;

            case 'i':
                input_image = img_load(optarg);
                assert(input_image->comp == 1);
                break;

            case 'o':
                if (strlen(optarg) > MAX_OUTDIR_LENGTH - 1) {
                    fprintf(stderr, "Out dir name is too long (limit: %d chars)\n",
                        MAX_OUTDIR_LENGTH - 1);
                    return 1;
                }
                strncpy(out_dir, optarg, MAX_OUTDIR_LENGTH);
                break;

            case 'c':
                count = sscanf(optarg + 4, "%2x", &tmp);
                if (count != 1) { fprintf(stderr, "Invalid color\n"); return 1; }
                blue = tmp & 0xFF;
                optarg[4] = '\0';
                count = sscanf(optarg + 2, "%x", &tmp);
                green = tmp & 0xFF;
                if (count != 1) { fprintf(stderr, "Invalid color\n"); return 1; }
                optarg[2] = '\0';
                count = sscanf(optarg, "%2x", &tmp);
                if (count != 1) { fprintf(stderr, "Invalid color\n"); return 1; }
                red = tmp & 0xFF;
                break;

            case 1000:
                list_only = true;
                break;

            default:
                fprintf(stderr, "Invalid arguments.\n");
                return 1;
        }
    }

    /*
        Check args
     */

    if (!log_file) {
        fprintf(stderr, "Failed to open log file or no file given.\n");
        return 1;
    }

    if (!list_only) {
        if (!input_image) {
            fprintf(stderr, "Failed to load input image or no file given.\n");
            return 1;
        }

        if (!strlen(out_dir)) {
            fprintf(stderr, "No output directory given.\n");
            return 1;
        }
        int create_dir_retval = create_dir(out_dir);
        if (create_dir_retval != 0) {
            fprintf(stderr, "Error creating output directory: %s\n", strerror(create_dir_retval));
            return 1;
        }

        output_image = img_create(input_image->width, input_image->height, 3);
        if (!output_image) {
            fprintf(stderr, "Failed to allocate memory for output image\n");
            return 1;
        }

        image_length = input_image->width * input_image->height;
    }

    /*
        Iterate over log file
     */

    do {
        int generation, length;
        int count;
        char output_filename[MAX_OUTFILE_LENGTH];

        count = fscanf(log_file, "Generation %d: Predictor phenotype length %d [ ", &generation, &length);
        if (count < 2) break;

        printf("Generation %d, length %d\n", generation, length);

        if (!list_only) {
            snprintf(output_filename, MAX_OUTFILE_LENGTH, "%s/%06d.png", out_dir, generation);

            // copy input image to output image (and convert it to 24bit)
            for (int i = 0; i < image_length; i++) {
                int offset = i * 3;
                output_image->data[offset] = input_image->data[i];
                output_image->data[offset + 1] = input_image->data[i];
                output_image->data[offset + 2] = input_image->data[i];
            }
        }

        // set selected pixels to specified color
        for (int i = 0; i < length; i++) {
            unsigned int index;
            count = fscanf(log_file, "%u", &index);
            if (!list_only) {
                if (count == 1) {
                    index = index * 3;
                    output_image->data[index] = red;
                    output_image->data[index + 1] = green;
                    output_image->data[index + 2] = blue;
                } else {
                    fprintf(stderr, "Invalid log file.\n");
                    return 1;
                }
            }
        }

        count = fscanf(log_file, " ] ");

        // save png
        if (!list_only) {
            img_save_png(output_image, output_filename);
        }

    } while(true);

    fclose(log_file);
    img_destroy(output_image);
    img_destroy(input_image);
}
