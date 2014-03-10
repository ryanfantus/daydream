/* Some handy macros */

#define NEW(x) malloc(sizeof(x))
#define FREE(x) (x != NULL)  ? free(x) : 0
#define COPY(x) memcpy(malloc(sizeof(*x)), x, sizeof(*x))

#define MKDIR_DEFS 0770

#define IN_NP 0
#define IN_P 1
#define IN_L 2

typedef enum {FALSE, TRUE} bool;
