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

#include <assert.h>

#include "base.h"
#include "devnull.h"
#include "text.h"


/* maximal number of active loggers */
#define LOGGERS_MAX 3


typedef struct {
    int count;
    logger_t loggers[LOGGERS_MAX];
} logger_list_t;


static inline void logger_init_list(logger_list_t *list)
{
    list->count = 0;
}


static inline void logger_destroy_list(logger_list_t *list)
{
    for (int i = 0; i < list->count; i++) {
        logger_destroy(list->loggers[i]);
    }
    list->count = 0;
}


static inline void logger_add(logger_list_t *list, logger_t logger)
{
    assert(list->count < LOGGERS_MAX);
    assert(logger != NULL);
    list->loggers[list->count] = logger;
    list->count++;
}


#define logger_fire(listptr, event, ...) do { \
    for (int i = 0; i < (listptr)->count; i++) { \
        (listptr)->loggers[i]->handler_ ## event ((listptr)->loggers[i], __VA_ARGS__); \
    } \
} while(0);
