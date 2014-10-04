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


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "vault.h"


/**
 * Initialize vault storage
 * @param storage
 */
int vault_init(vault_storage_t *storage)
{
    if (storage->directory == NULL)
        return 0;

    int retval = mkdir(storage->directory, S_IRWXU);
    if (retval && errno == EEXIST) {
        return 0;
    }
    return retval;
}


/**
 * Store running state to vault
 * @param storage
 * @param population
 */
int vault_store(vault_storage_t *storage, ga_pop_t cgp_population)
{
    if (storage->directory == NULL)
        return 0;

    char fullname[MAX_FILENAME_LENGTH + 1];
    snprintf(fullname, MAX_FILENAME_LENGTH + 1, "%s/state_%08d",
        storage->directory, cgp_population->generation);

    FILE *fp = fopen(fullname, "w");
    if (!fp) return VAULT_ERROR;

    cgp_dump_pop_compat(cgp_population, fp);
    fclose(fp);
    return 0;
}


int _file_selector(const struct dirent *entry)
{
    return strncmp(entry->d_name, "state_", 6) == 0;
}


/**
 * Retrieve last stored state from vault
 * @param storage
 * @param population
 */
int vault_retrieve(vault_storage_t *storage, ga_pop_t *cgp_pop_ptr)
{
    if (storage->directory == NULL)
        return VAULT_EMPTY;

    // find last existing file

    struct dirent **entries;
    int files_found = scandir(storage->directory, &entries, _file_selector, alphasort);
    if (files_found < 0) {
        return VAULT_ERROR;

    } else if (files_found == 0) {
        return VAULT_EMPTY;
    }

    char *last_state = entries[files_found - 1]->d_name;
    char fullname[MAX_FILENAME_LENGTH + 1];
    snprintf(fullname, MAX_FILENAME_LENGTH + 1, "%s/%s", storage->directory, last_state);

    return vault_read(fullname, cgp_pop_ptr);
}


int vault_read(char *fullname, ga_pop_t *cgp_pop_ptr)
{
    FILE *fp = fopen(fullname, "r");
    if (!fp) return VAULT_ERROR;

    int retval = cgp_load_pop_compat(cgp_pop_ptr, fp);
    fclose(fp);
    return retval;
}
