#include <string.h>

#include <symtab.h>

list_t *atom_list_destroy(list_t *atom_list)
{
	while (atom_list) {
		struct atom *atom = shift(struct atom *, atom_list);
		atom->destroy(atom);
	}
	return NULL;
}

list_t *atom_list_clone(list_t *atom_list)
{
	list_t *cloned_atom_list = NULL;
	while (atom_list) {
		struct atom *cloned_atom, *atom;
		
		atom = car(struct atom *, atom_list);
		cloned_atom = atom->clone(atom);
		cons(cloned_atom_list, cloned_atom);
		atom_list = cdr(atom_list);
	}
	return cloned_atom_list;
}

struct symbol *symbol_new(void)
{
	return g_new0(struct symbol, 1);
}

struct symbol *symbol_clone(struct symbol *symbol)
{
	struct symbol *cloned_symbol;
	
	cloned_symbol = symbol_new();
	symbol_attach(cloned_symbol, symbol->name, symbol->atom_list);
	
	return cloned_symbol;
}

void symbol_attach(struct symbol *sym, char *name, list_t *atom_list)
{
	g_free(sym->name);
	sym->name = g_strdup(name);
	atom_list_destroy(sym->atom_list);
	sym->atom_list = atom_list_clone(atom_list);
}

void symbol_destroy(struct symbol *sym)
{
	g_free(sym->name);
	atom_list_destroy(sym->atom_list);
	g_free(sym);
}

struct symbol_table *symbol_table_new(void)
{
	struct symbol_table *symbol_table;
	
	symbol_table = g_new0(struct symbol_table, 1);
	symbol_table->symbols = NULL;
	
	return symbol_table;
}

void symbol_table_destroy(struct symbol_table *sym_table)
{
	while (sym_table->symbols) 
		symbol_destroy(shift(struct symbol *, sym_table->symbols));
	g_free(sym_table);
}
	
void symbol_table_insert(struct symbol_table *sym_table, struct symbol *symbol)
{
	list_t *iterator, *prev;
	struct symbol *cloned_symbol = symbol_clone(symbol);
	
	for (iterator = sym_table->symbols, prev = NULL; iterator;
	     prev = iterator, iterator = cdr(iterator)) {
		char *a = cloned_symbol->name;
		char *b = car(struct symbol *, iterator)->name;
		if ((a == b && !a) || (a && b && !strcasecmp(a, b))) {
			symbol_destroy(shift(struct symbol *, iterator));
			if (prev)
				prev->next = iterator;
			else 
				sym_table->symbols = NULL;
			break;
		}
	}
	
	cons(sym_table->symbols, cloned_symbol);
}

struct symbol *symbol_table_lookup(struct symbol_table *sym_table, 
				   const char *sname, size_t snamelen)
{
	list_t *iterator;
	
	for (iterator = sym_table->symbols; iterator;
	     iterator = cdr(iterator)) {
		const char *a = sname;
		const char *b = car(struct symbol *, iterator)->name;
		if (!a && !b)
			break;
		if (!a || !b)
			continue;
		if (strlen(b) != snamelen)
			continue;
		if (!strncasecmp(a, b, snamelen)) 
			break;
	}

	return iterator ? car(struct symbol *, iterator) : NULL;
}
