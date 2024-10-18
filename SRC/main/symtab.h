#ifndef _SYMTAB_H_INCLUDED
#define _SYMTAB_H_INCLUDED

#include <sys/types.h>
#include <stdlib.h>

#include <utility.h>
#include <config.h>

/* An "atom" is a terminal symbol, e.g. terminal symbol.
 * 
 * A "symbol" is non-terminal symbol. It contains a list of atoms.
 */

struct atom {
	struct atom *(*clone)(const struct atom *);
	void (*destroy)(struct atom *);
};

struct symbol {
	char *name;
	list_t *atom_list;
};

struct symbol_table {
	list_t *symbols;
};

/* atom_list_destroy returns always NULL. */
list_t *atom_list_destroy(list_t *);
list_t *atom_list_clone(list_t *);

struct symbol *symbol_new(void);
void symbol_attach(struct symbol *, char *, list_t *);
void symbol_destroy(struct symbol *);

struct symbol_table *symbol_table_new(void);
void symbol_table_destroy(struct symbol_table *);
void symbol_table_insert(struct symbol_table *, struct symbol *);
struct symbol *symbol_table_lookup(struct symbol_table *, 
	const char *, size_t) __attr_bounded__ (__string__, 2, 3);

#endif /* _SYMTAB_H_INCLUDED */
