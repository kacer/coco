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

#define _XOPEN_SOURCE 700

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "vault.h"


#define MAX_FILENAME_LENGTH 255


/**
 * Initialize vault storage
 * @param storage
 */
int vault_init(vault_storage *storage)
{
    int retval = mkdir(storage->directory, S_IRWXU);
    if (retval && errno == EEXIST) {
        return 0;
    }
    return retval;
}


int _get_dest_filename(char *buffer, int buffer_size, char *directory, int generation)
{
    int dirname_length = strlen(directory);
    strncpy(buffer, directory, buffer_size);
    snprintf(buffer + dirname_length, buffer_size - dirname_length, "/state_%08d", generation);
    return 0;
}


/**
 * Store running state to vault
 * @param storage
 * @param population
 */
int vault_store(vault_storage *storage, cgp_pop population)
{
    char fullname[MAX_FILENAME_LENGTH];
    if (_get_dest_filename(fullname, MAX_FILENAME_LENGTH, storage->directory, population->generation) < 0) {
        return VAULT_ERROR;
    }

    FILE *fp = fopen(fullname, "w");
    if (!fp) return VAULT_ERROR;

    cgp_dump_pop_compat(population, fp);
    fclose(fp);
    return 0;
}


int _file_selector(const struct dirent *entry)
{
    return strncmp(entry->d_name, "state_", 6) == 0;
}


int _get_src_filename(char *buffer, int buffer_size, char *directory, char *filename)
{
    int dirname_length = strlen(directory);
    strncpy(buffer, directory, buffer_size);
    buffer[dirname_length] = '/';
    strncpy(buffer + dirname_length + 1, filename, buffer_size - dirname_length);
    return 0;
}


/**
 * Retrieve last stored state from vault
 * @param storage
 * @param population
 */
int vault_retrieve(vault_storage *storage, cgp_pop *pop_ptr)
{
    // find last existing file

    struct dirent **entries;
    int files_found = scandir(storage->directory, &entries, _file_selector, alphasort);
    if (files_found < 0) {
        return VAULT_ERROR;

    } else if (files_found == 0) {
        return VAULT_EMPTY;
    }

    char *last_state = entries[files_found - 1]->d_name;
    char fullname[MAX_FILENAME_LENGTH];
    if (_get_src_filename(fullname, MAX_FILENAME_LENGTH, storage->directory, last_state) < 0) {
        return VAULT_ERROR;
    }

    return vault_read(fullname, pop_ptr);
}


int vault_read(char *fullname, cgp_pop *pop_ptr)
{
    FILE *fp = fopen(fullname, "r");
    if (!fp) return VAULT_ERROR;

    int retval = cgp_load_pop_compat(pop_ptr, fp);
    fclose(fp);
    return retval;
}
