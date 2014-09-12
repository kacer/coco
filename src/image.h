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


#pragma once


#define WINDOW_SIZE 9


struct img_image {
    unsigned char *data;
    int width;
    int height;
    int comp;
};
typedef struct img_image* img_image_t;


typedef struct {
    int pos_x;
    int pos_y;
    unsigned char pixels[WINDOW_SIZE];
} img_window_t;


typedef struct img_window_array {
    int size;
    img_window_t *windows;
} _img_window_array;
typedef struct img_window_array* img_window_array_t;


/**
 * Create new image - image data are not initialized!
 * @param  filename
 * @return
 */
img_image_t img_create(int width, int height, int comp);


/**
 * Loads image from file
 * @param  filename
 * @return
 */
img_image_t img_load(char const *filename);


/**
 * Splits image into windows
 * @param  filename
 * @return
 */
img_window_array_t img_split_windows(img_image_t img);


/**
 * Store image to BMP file
 * @param  img
 * @return 0 on failure, non-zero on success
 */
int img_save_bmp(img_image_t img, char const *filename);


/**
 * Clears all data associated with image from memory
 * @param img
 */
void img_destroy(img_image_t img);


/**
 * Clears all data associated with image windows from memory
 * @param img
 */
void img_windows_destroy(img_window_array_t arr);


/**
 * Returns 1-D index in data array of given pixel
 * @param img
 */
static inline int img_pixel_index(img_image_t img, int x, int y) {
    return (y * img->width) + x;
}


/**
 * Returns value of given pixel
 * @param img
 */
static inline unsigned char img_get_pixel(img_image_t img, int x, int y) {
    return img->data[img_pixel_index(img, x, y)];
}


/**
 * Sets value of given pixel
 * @param img
 */
static inline void img_set_pixel(img_image_t img, int x, int y, unsigned char value) {
    img->data[img_pixel_index(img, x, y)] = value;
}
