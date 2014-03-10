#ifndef _UTILITY_H_INCLUDED
#define _UTILITY_H_INCLUDED

#include <sys/types.h>
#include <stdlib.h>

typedef struct list_tag {
	struct list_tag *next;
	void *data;
} list_t;

/* utility.c */
int int_sortfn(const void *a, const void *b);
int exists_in_list(list_t *list, void *data, int (*comparefn)(const void *, const void *));
list_t *sorted_insert(list_t *list, void *data, int (*sortfn)(const void *, const void *));
list_t *push(list_t *list, void *data);
list_t *cons(list_t *list, void *data);
void *car(list_t *list);
list_t *cdr(list_t *list);
list_t *delcar(list_t *list);

#ifndef COMPILING_UTILITY_C
/* This is non-destructive, like Lisp's car */
#define car(__type__, __list__) ((__type__) car(__list__))
/* This returns the first item of a list and shortens the list,
 * just like Perl's shift. */
#define shift(__type__, __list__) ({			\
	__type__ __data__ = car(__type__, __list__);	\
	__list__ = delcar(__list__);			\
	__data__;					\
})

/* Note that value is appended, not prepended. */
#define cons(__list__, __value__) __list__ = cons(__list__, __value__)
#endif /* COMPILING_UTILITY_C */

#define g_new0(__type__, __count__) (__type__ *) \
	calloc(__count__, sizeof(__type__))
#define g_new(__type__, __count__) (__type__ *) \
	xmalloc(__count__ * sizeof(__type__))
#define g_free(__x__) do { if (__x__) free(__x__); } while (0)

#define g_strdup(__x__) ((char *) (__x__ ? strdup(__x__) : __x__))

typedef struct {
	char *str;
	int alloc;
} string_t;

string_t *strnew(void);
string_t *strappend(string_t *, const char *);
string_t *strappend_c(string_t *, char);
string_t *strnappend(string_t *, char *, size_t);
char *strfree(string_t *, int);

struct vector {
	void *data;
	int member_size;
	int allocated;
	int size;
};

struct vector *vec_new(int member_size);
void *vec_nth(struct vector *vec, int n);
void *vec_append(struct vector *vec, void *data);
size_t vec_size(struct vector *vec);
void vec_destroy(struct vector *vec);

struct iterator;

struct iterator_impl {
	void (*discard)(struct iterator *);
	void *(*next)(struct iterator *);
};

struct iterator {
	struct iterator_impl *impl;
	void *data;
};

void iterator_discard(struct iterator *);
void *iterator_next(struct iterator *);

#endif /* _UTILITY_H_INCLUDED */

