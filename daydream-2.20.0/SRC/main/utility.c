#include <string.h>

#define COMPILING_UTILITY_C
#include <utility.h>
#include <daydream.h>
#include <ddcommon.h>

int int_sortfn(const void *a, const void *b)
{
	return (int) a - (int) b;
}

int exists_in_list(list_t *list, void *data, 
		   int (*comparefn)(const void *, const void *))
{
	while (list) {
		if (!comparefn((void *) car(list), data))
			return 1;
		list = cdr(list);
	}
	return 0;
}

list_t *sorted_insert(list_t *list, void *data, 
		      int (*sortfn)(const void *, const void *))
{
	list_t *node = list, *prev = NULL, *new_node;
	
	while (node) {
		if (sortfn(data, (void *) car(node)) <= 0)
			break;
		prev = node;
		node = cdr(node);
	}
	
	new_node = (list_t *) xmalloc(sizeof(list_t));
	new_node->next = node;
	new_node->data = data;
	
	if (prev)
		prev->next = new_node;
	else
		list = new_node;
	
	return list;
}

list_t *push(list_t *list, void *data)
{
	list_t *node = (list_t *) xmalloc(sizeof(list_t));
	node->next = list;
	node->data = data;
	return node;
}

list_t *cons(list_t *list, void *data)
{
	list_t *node;
	if (!list) 
		list = node = (list_t *) xmalloc(sizeof(list_t));
	else {
		node = list;
		while (node->next)
			node = node->next;
		node = node->next = (list_t *) xmalloc(sizeof(list_t));
	}
	node->next = NULL;
	node->data = data;
	return list;
}

void *car(list_t *list)
{
	if (!list)
		return list;
	return list->data;
}

list_t *cdr(list_t *list)
{
	if (!list)
		return list;
	return list->next;
}
	
list_t *delcar(list_t *list)
{
	list_t *next = cdr(list);
	if (list)
		free(list);
	return next;
}

string_t *strnew(void)
{
	string_t *str = (string_t *) xmalloc(sizeof(string_t));
	str->str = (char *) xmalloc(32);
	str->str[0] = 0;
	str->alloc = 32;
	return str;
}

string_t *strappend(string_t *str, const char *s)
{
	if (!str)
		str = strnew();
	if (str->alloc - strlen(str->str) - 1 < strlen(s)) {
		str->alloc += strlen(s);
		str->alloc *= 2;
		str->str = (char *) xrealloc(str->str, str->alloc);
	}
	strlcat(str->str, s, str->alloc);
	return str;
}

string_t *strappend_c(string_t *str, char ch)
{
	char tmp[2];
	tmp[0] = ch;
	tmp[1] = 0;
	return strappend(str, tmp);
}

char *strfree(string_t *str, int free_contents)
{
	char *p;
	if (free_contents) {
		p = NULL;
		free(str->str);
	} else
		p = str->str;
	free(str);
	return p;
}

void iterator_discard(struct iterator *iterator)
{
	iterator->impl->discard(iterator);
}

void *iterator_next(struct iterator *iterator)
{
	return iterator->impl->next(iterator);
}
