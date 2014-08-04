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
#include <stdio.h>

#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"


const int COMP = 1;


/**
 * Create new image - image data are not initialized!
 * @param  filename
 * @return
 */
img_image img_create(int width, int height, int comp) {
    img_image img = (img_image) malloc(sizeof(_img_image));
    if (img == NULL) return NULL;

    img->width = width;
    img->height = height;
    img->comp = comp;
    img->data = (unsigned char *) malloc(sizeof(unsigned char) * width * height * comp);

    return img;
}


/**
 * Loads image from file
 * @param  filename
 * @return
 */
img_image img_load(char const *filename) {
    img_image img = (img_image) malloc(sizeof(_img_image));
    if (img == NULL) return NULL;

    img->data = stbi_load(filename, &(img->width), &(img->height), &(img->comp), COMP);
    if (img->data == NULL) {
        printf("%s", stbi__g_failure_reason);
        free(img);
        return NULL;
    }

    img->comp = COMP;
    return img;
}


/**
 * Store image to BMP file
 * @param  img
 * @return 0 on failure, non-zero on success
 */
int img_save_bmp(img_image img, char const *filename) {
    return stbi_write_bmp(filename, img->width, img->height, img->comp, img->data);
}


/**
 * Clears all data associated with image from memory
 * @param img
 */
void img_destroy(img_image img) {
    if (img != NULL) free(img->data);
    free(img);
}


/**
 * Clears all data associated with image windows from memory
 * @param img
 */
void img_windows_destroy(img_window_array arr) {
    if (arr != NULL) free(arr->windows);
    free(arr);
}



/**
 * Returns index of neighbour with given offset
 * @param  baseX
 * @param  baseY
 * @param  image width
 * @param  image height
 * @param  offX
 * @param  offY
 * @return
 */
static inline int get_neighbour_index(int baseX, int baseY, int width, int height, int offX, int offY) {
    int x = baseX + offX;
    int y = baseY + offY;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= width) x = width - 1;
    if (y >= height) y = height - 1;
    return (y * width) + x;
}


/**
 * Splits image into windows
 * @param  filename
 * @return
 */
img_window_array img_split_windows(img_image img) {
    int size = img->width * img->height;
    img_window *windows = (img_window*) malloc(sizeof(img_window) * size);
    if (windows == NULL) return NULL;

    img_window_array arr = (img_window_array) malloc(sizeof(_img_window_array));
    if (arr == NULL) {
        free(windows);
        return NULL;
    }

    arr->size = size;
    arr->windows = windows;

    for (int x = 0; x < img->width; x++) {
        for (int y = 0; y < img->height; y++) {
            int index = img_pixel_index(img, x, y);
            windows[index].pos_x = x;
            windows[index].pos_y = y;
            windows[index].pixels[0] = img->data[get_neighbour_index(x, y, img->width, img->height, -1, -1)];
            windows[index].pixels[1] = img->data[get_neighbour_index(x, y, img->width, img->height,  0, -1)];
            windows[index].pixels[2] = img->data[get_neighbour_index(x, y, img->width, img->height, +1, -1)];
            windows[index].pixels[3] = img->data[get_neighbour_index(x, y, img->width, img->height, -1,  0)];
            windows[index].pixels[4] = img->data[get_neighbour_index(x, y, img->width, img->height,  0,  0)];
            windows[index].pixels[5] = img->data[get_neighbour_index(x, y, img->width, img->height, +1,  0)];
            windows[index].pixels[6] = img->data[get_neighbour_index(x, y, img->width, img->height, -1, +1)];
            windows[index].pixels[7] = img->data[get_neighbour_index(x, y, img->width, img->height,  0, +1)];
            windows[index].pixels[8] = img->data[get_neighbour_index(x, y, img->width, img->height, +1, +1)];
        }
    }

    return arr;
}
