#ifndef COMMON_H
#define COMMON_H

#include "rpg.nb.h"

#include <stdio.h>
#include <stdlib.h>

#define LOG_FATAL(fmt, ...) \
	(fprintf(stderr, "%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__), \
	exit(1))

#define CHECK(exp) (exp) ? (void) 0 : LOG_FATAL("CHECK(%s) failed", #exp)

extern rpg_GameConfig config_;
extern rpg_GameState state_;

extern bool resume(void);
extern void save_state(void);
extern void load_state(void);
extern void load_config(void);

extern bool entry_level(void);

#endif  /* COMMON_H */
