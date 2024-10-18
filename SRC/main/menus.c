#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <daydream.h>
#include <ddcommon.h>
#include <menucmd.h>
#include <symtab.h>

struct command_arg {
	struct atom callback_table;	
	union {
		char *string;
		int integer;
	} value;
	unsigned int type;
};

struct command {
	struct atom callback_table;
	
	int type;
	
	int arg_template;
	list_t *args; /* an atom list */
};

struct menu {
	struct symbol_table *commands;
	struct symbol_table *hotkeys;

	char *view_file;
	char *node_state;
	char *menu_prompt;
	char *menu_prompt_no_bases;
};

enum {
	EXEC_HANGUP	= 0,
	EXEC_OK		= 1,
	EXEC_NULL	= 2,
	EXEC_UNDEFINED	= 3,
	EXEC_ARG_ERROR	= 4
};

#define car_and_cdr(__list__, __type__, __default__, __what__) ({	\
	__type__ __retval__ = __default__;				\
	struct command_arg *__arg__ =					\
		car(struct command_arg *, __list__);			\
	__list__ = cdr(__list__);					\
	if (__arg__) 							\
		__retval__ = __arg__->value.__what__;			\
	__retval__;							\
})

#define string_car_and_cdr(__list__) __string_car_and_cdr(&__list__)
#define integer_car_and_cdr(__list__) car_and_cdr(__list__, int, 0, integer)
			
#define push(__list__, __x__) __list__ = push(__list__, __x__)

struct command_convert_string_to_int {
	const char *string;
	int code;
};

static struct command_convert_string_to_int cmd_strings[] = {
	{ "VER", CMD_VER },
	{ "EXPERT_MODE", CMD_EXPERT_MODE },
	{ "LOGOFF", CMD_LOGOFF },
	{ "CHANGE_INFO", CMD_CHANGE_INFO },
	{ "FILE_SCAN", CMD_FILE_SCAN },
	{ "DOWNLOAD", CMD_DOWNLOAD },
	{ "TAG_EDITOR", CMD_TAG_EDITOR },
	{ "BULLETINS", CMD_BULLETINS },
	{ "NEW_FILES", CMD_NEW_FILES },
	{ "ZIPPY_SEARCH", CMD_ZIPPY_SEARCH },
	{ "TIME", CMD_TIME },
	{ "STATS", CMD_STATS },
	{ "ENTER_MSG", CMD_ENTER_MSG },
	{ "COMMENT", CMD_COMMENT },
	{ "READ_MSGS", CMD_READ_MSGS },
	{ "GLOBAL_READ", CMD_GLOBAL_READ },
	{ "JOIN_CONF", CMD_JOIN_CONF },
	{ "CHANGE_MSGBASE", CMD_CHANGE_MSGBASE },
	{ "LOCAL_UPLOAD", CMD_LOCAL_UPLOAD },
	{ "UPLOAD", CMD_UPLOAD },
	{ "UPLOAD_RZ", CMD_UPLOAD_RZ },
	{ "VIEW_FILE", CMD_VIEW_FILE },
	{ "SCAN_MAIL", CMD_SCAN_MAIL },
	{ "GLOBAL_FSCAN", CMD_GLOBAL_FSCAN },
	{ "TAG_MSGBASES", CMD_TAG_MSGBASES },
	{ "TAG_CONFS", CMD_TAG_CONFS },
	{ "PREV_CONF", CMD_PREV_CONF },
	{ "NEXT_CONF", CMD_NEXT_CONF },
	{ "PREV_BASE", CMD_PREV_BASE },
	{ "NEXT_BASE", CMD_NEXT_BASE },
	{ "WHO", CMD_WHO },
	{ "HELP", CMD_HELP },
	{ "USERLIST", CMD_USERLIST },
	{ "MODE", CMD_MODE },
	{ "PAGE", CMD_PAGE },
	{ "OLM", CMD_OLM },
	{ "MOVE", CMD_MOVE },
	{ "COPY", CMD_COPY },
	{ "LINK", CMD_LINK },
	{ "SYSOP_DOWNLOAD", CMD_SYSOP_DOWNLOAD },
	{ "USERED", CMD_USERED },
	{ "TEXT_SEARCH", CMD_TEXT_SEARCH },
	{ "TEST_DOOR", CMD_TEST_DOOR },
	{ NULL, 0 }	       
};

/* ARGOPT is meaningful only just after the last mandatory
 * argument.
 */

#define COMMANDS							   \
	CREATE_CMD(default_menu, "load_defaults", ARG0()),		   \
	CREATE_CMD(push_menu, "push_menu", ARG2(ARGSTR | ARGOPT, ARGSTR)), \
        CREATE_CMD(pop_menu, "pop_menu", ARG1(ARGINT | ARGOPT)),	   \
	CREATE_CMD(bind_command, "bind_cmd", ARG1(ARGSTR | ARGOPT)),	   \
	CREATE_CMD(bind_hotkey, "bind_hotkey", ARG1(ARGSTR | ARGOPT)),	   \
	CREATE_CMD(substitute_args, "subst", ARG2(ARGSTR, ARGSTR)),	   \
	CREATE_CMD(do_command, "command", ARG2(ARGSTR, ARGSTR | ARGOPT)),  \
	CREATE_CMD(do_return, "return", ARG1(ARGINT)),		 	   \
	CREATE_CMD(source_file, "source", ARG1(ARGSTR)),		   \
	CREATE_CMD(exec_file, "exec", ARG1(ARGSTR)),			   \
	CREATE_CMD(chprompt, "prompt", ARG2(ARGSTR, ARGSTR | ARGOPT)),	   \
	CREATE_CMD(run_door, "door", ARG2(ARGSTR, ARGSTR | ARGOPT)),	   \
	CREATE_CMD(test_door, "test_door", ARG2(ARGSTR, ARGSTR | ARGOPT)), \
	CREATE_CMD(print, "print", ARG1(ARGSTR | ARGOPT)),		   \
	CREATE_CMD(kbd_stuff, "kbdstuff", ARG1(ARGSTR | ARGOPT)),	   \
	CREATE_CMD(internal_command, "internal", ARG2(ARGSTR, ARGSTR | ARGOPT))

#define ARG0()
#define ARG1(__x__)
#define ARG2(__x__, __y__)

#define REF_CMD(__x__) CMD##__x__
#define CREATE_CMD(__x__, __z__, __y__) REF_CMD(__x__)

enum {
	COMMANDS
};

#undef CREATE_CMD
#define CREATE_CMD(__x__, __z__, __y__) { 	\
	__z__, REF_CMD(__x__), __y__, __x__ 	\
}

#undef ARG0
#undef ARG1
#undef ARG2
#define ARG0() 0
#define ARG1(__x__) (__x__)
#define ARG2(__x__, __y__) ((__x__) | ((__y__) << ARGMASKLEN))

#define ARGMASKLEN	3
#define ARGMASK		((~0 << ARGMASKLEN) ^ ~0)
#define ARGTYPEMASK	(ARGSTR | ARGINT)
#define ARGSTR 		0x01
#define ARGINT		0x02
#define ARGOPT		0x04

struct command_string {
	const char *string;
	int code;
	int arg_template;
	int (*internal)(list_t *, list_t **);
};

static int default_menu(list_t *args, list_t **stmt_list);
static int push_menu(list_t *args, list_t **stmt_list);
static int pop_menu(list_t *args, list_t **stmt_list);
static int bind_command(list_t *args, list_t **stmt_list);
static int bind_hotkey(list_t *args, list_t **stmt_list);
static int substitute_args(list_t *args, list_t **stmt_list);
static int internal_command(list_t *args, list_t **stmt_list);
static int source_file(list_t *args, list_t **stmt_list);
static int exec_file(list_t *args, list_t **stmt_list);
static int run_door(list_t *args, list_t **stmt_list);
static int test_door(list_t *args, list_t **stmt_list);
static int chprompt(list_t *args, list_t **stmt_list);
static int print(list_t *args, list_t **stmt_list);
static int kbd_stuff(list_t *args, list_t **stmt_list);
static int do_command(list_t *args, list_t **stmt_list);
static int do_return(list_t *args, list_t **stmt_list);

static struct command_string command_strings[] = { 
	COMMANDS,
	{ NULL, 0, 0, NULL }
};

static list_t *menu_stack;
static struct menu *current_menu;
static list_t *scope_args;

static struct command_string *get_command_definition(int command_code);
static int tokenize_command(const char *command);
static int get_arg_template(int command_code);
static int check_arguments(struct atom *command_atom);
static int execute_command(list_t *command_list);

static struct atom *command_atom_new(int type, list_t *arg);
static struct atom *command_atom_clone(const struct atom *);
static void command_atom_destroy(struct atom *);

static struct atom *arg_new_integer(int i);
static struct atom *arg_new_string(const char *string);
static struct atom *arg_atom_clone(const struct atom *atom);
static void arg_atom_destroy(struct atom *);

static char *read_unquoted_string(const char **source);
static char *read_quoted_string(const char **source);
static int read_integer(const char **source);
static int expect_character(const char **source, char ch);
static const char *trim(const char *s);

static void menu_destroy(struct menu *menu);
static struct menu *menu_new(const char *view_file, const char *node_state);

static void makemainprompt(char *, size_t) __attr_bounded__ (__buffer__, 1, 2);

static char *__string_car_and_cdr(list_t **list)
{
	static char buffer[32];
	struct command_arg *arg = car(struct command_arg *, *list);
	*list = cdr(*list);
	if (!arg)
		return NULL;
	if ((arg->type & ARGTYPEMASK) == ARGSTR)
		return arg->value.string;
	else if ((arg->type & ARGTYPEMASK) == ARGINT) {
		snprintf(buffer, sizeof buffer, "%d", arg->value.integer);
		return buffer;
	}
	return NULL;
}
			

void init_menu_system(void)
{
	menu_stack = NULL;
	current_menu = NULL;
	scope_args = NULL;
	
	load_default_commands();
}

void fini_menu_system(void)
{
	list_t *dummy_list = NULL;
	while (menu_stack)
		pop_menu(NULL, &dummy_list);
}

static int default_menu(list_t *args, list_t **stmt_list) 
{
	*stmt_list = cdr(*stmt_list);
	if (load_default_commands())
		return EXEC_ARG_ERROR;
	
	return EXEC_OK;
}

static int push_menu(list_t *args, list_t **stmt_list)
{
	struct menu *menu;
	char *view_file, *node_state;
	view_file = string_car_and_cdr(args);
	node_state = string_car_and_cdr(args);	
	menu = menu_new(view_file, node_state);
	push(menu_stack, menu);
	current_menu = menu;
	*stmt_list = cdr(*stmt_list);
	return EXEC_OK;
}

static int pop_menu(list_t *args, list_t **stmt_list)
{
	struct menu *menu;
	int pop_count = integer_car_and_cdr(args);
	if (!pop_count)
		pop_count++;
	while (pop_count-- > 0) {
		menu = shift(struct menu *, menu_stack);
		menu_destroy(menu);		
		current_menu = car(struct menu *, menu_stack);
	}
	*stmt_list = cdr(*stmt_list);
	return EXEC_OK;
}

void parse_menu_command(const char **source)
{
	const char *ptr = *source;
	int first_command = 1, error_code = 0;
	char *command = NULL;
	list_t *args = NULL;
	list_t *command_list = NULL;

	while (*ptr && !error_code) {
		int first_arg = 1;

		/* sequence terminator and separators */
		if (expect_character(&ptr, '|'))
			break;
		if (first_command)
			first_command = 0;
		else
			if (!expect_character(&ptr, ';')) {
				error_code++;
				break;
			}

		/* command itself */
		if (!(command = read_unquoted_string(&ptr))) {
			error_code++;
			break;
		}

		/* args may be omitted */
		if (!expect_character(&ptr, '('))
			goto skip_args;
			
		for (;;) {
			char *p = NULL;
			int i;
			
			if (expect_character(&ptr, ')'))
				break;
			
			if (!first_arg && !expect_character(&ptr, ',')) {
				error_code++;
				break;
			} else
				first_arg = 0;
			
			if ((i = read_integer(&ptr)) != -1) {
				struct atom *atom = arg_new_integer(i);
				cons(args, atom);
				continue;
			}						
			
			if ((p = read_quoted_string(&ptr)) != NULL) {
				struct atom *atom = arg_new_string(p);
				cons(args, atom);
				g_free(p);
				continue;
			}
			
			/* some other datatypes? */
			
			error_code++;
			break;
		}
				
		if (error_code)
			break;
		
	skip_args:
		cons(command_list, 
		     command_atom_new(tokenize_command(command), args));

		args = atom_list_destroy(args);
		g_free(command);
		command = NULL;
	}

	atom_list_destroy(args);
	g_free(command);

	if (!error_code) 
		execute_command(command_list);

	atom_list_destroy(command_list);
	*source = ptr;
}

int try_hotkey_match(const char *sname, size_t snamelen)
{
	if (!current_menu)
		return 0;
	
	return (int) symbol_table_lookup(current_menu->hotkeys, 
		sname, snamelen);
}

static int check_arguments(struct atom *command_atom)
{	
	int optional = 0;
	struct command *command = (struct command *) command_atom;
	list_t *arg_atom_list = command->args;		
	unsigned int arg_template = get_arg_template(command->type);

	while (arg_atom_list) {
		struct command_arg *arg;

		arg = car(struct command_arg *, arg_atom_list);

		if (arg_template & ARGOPT)
			optional = 1;
		
		if ((arg_template & ARGTYPEMASK) != arg->type)
			return 0;

		arg_template >>= ARGMASKLEN;
		arg_atom_list = cdr(arg_atom_list);
	}

	if (optional)
		return 1;
	
	return (arg_template && !(arg_template & ARGOPT)) ? 0 : 1;
}

static int bind_command(list_t *args, list_t **stmt_list)
{
	struct symbol *symbol;
	
	if (!current_menu)
		return EXEC_ARG_ERROR;
	
	symbol = symbol_new();
	symbol_attach(symbol, string_car_and_cdr(args), cdr(*stmt_list));
	symbol_table_insert(current_menu->commands, symbol);
	symbol_destroy(symbol);

	*stmt_list = NULL;
	return EXEC_OK;
}

static int bind_hotkey(list_t *args, list_t **stmt_list)
{
	struct symbol *symbol;
	
	if (!current_menu)
		return EXEC_ARG_ERROR;
	
	symbol = symbol_new();
	symbol_attach(symbol, string_car_and_cdr(args), cdr(*stmt_list));
	symbol_table_insert(current_menu->hotkeys, symbol);
	symbol_destroy(symbol);

	*stmt_list = NULL;
	return EXEC_OK;
}

static char *get_parameters(list_t *args)
{
	string_t *str = NULL;
	char *p;
	
	/* concatenate parameters */
	do {
		p = string_car_and_cdr(args);
		if (p) {
			if (str)
				str = strappend_c(str, ' ');
			str = strappend(str, p);
		}
	} while (p);
	
	if (!str)
		return NULL;
	
	/* strip spaces */	
	p = str->str;
	while (isspace(*p))
		p++;
	if (!*p)
		p = NULL;
	else 
		p = strdup(p);
	
	strfree(str, 1);
	
	return p;
}

static int internal_command(list_t *args, list_t **stmt_list)
{
	int retcode;
	struct command_convert_string_to_int *t;	
	char *cmd_name = string_car_and_cdr(args), *param;
	*stmt_list = cdr(*stmt_list);
	
	for (t = cmd_strings; t->string; t++) 
		if (!strcasecmp(cmd_name, t->string))
			break;
		
	if (!t->string)
		return EXEC_UNDEFINED;
	
	param = get_parameters(args);
	retcode = primitive_docmd(t->code, param);
	
	g_free(param);
	return retcode;
}

static int test_door(list_t *args, list_t **stmt_list)
{
	char *params, *command;
	char *buffer;

	if (user.user_securitylevel != 255) {
		DDPut(sd[accessdeniedstr]);
		return(EXEC_ARG_ERROR);
	}
	
	*stmt_list = cdr(*stmt_list);
	command = string_car_and_cdr(args);
	
	buffer = g_new(char, strlen(command) + 4);
	if (!buffer)
		return EXEC_HANGUP;
	
	snprintf(buffer, strlen(command) + 4, "%s %%N", command);
	params = get_parameters(args);
	
	rundoor(buffer, params);
	
	g_free(buffer);
	g_free(params);
	return EXEC_OK;
}

static int ask_password_for_door(const char *doorname, const char *passwd)
{
	char temp[100];
	int i;

	if (!passwd[0])
		return 1;
	
	snprintf(temp, sizeof temp, "doorpasswd%s", doorname);
	for (i = 0; i < strlen(temp); i++)
		temp[i] = tolower(temp[i]);
	TypeFile(temp, TYPE_MAKE | TYPE_SEC | TYPE_CONF);

	temp[0] = 0;
	if (!Prompt(temp, 16, PROMPT_SECRET))
		return -1;
	return strcasecmp(temp, passwd) ? 0 : 1;
}	

static int run_door(list_t *args, list_t **stmt_list)
{
	int retvalue;
	struct DD_ExternalCommand *ext;
	char *command = string_car_and_cdr(args);
	char *params;
	*stmt_list = cdr(*stmt_list);
	
	params = get_parameters(args);
	
	ext = exts;
	while (ext->EXT_NAME[0] != 0) {
		if (!strcasecmp(command, ext->EXT_NAME)) {
			if (ext->EXT_SECLEVEL > user.user_securitylevel) {
				DDPut(sd[accessdeniedstr]);
				g_free(params);
				return EXEC_NULL; /* hmm? */
			}
			retvalue = ask_password_for_door(ext->EXT_NAME,
				ext->EXT_PASSWD);
			if (retvalue == -1)
				return EXEC_HANGUP;
			if (!retvalue) {
				DDPut(sd[accessdeniedstr]);
				return 1;
			}
			switch (ext->EXT_CMDTYPE) {
			case 1:
				rundoor(ext->EXT_COMMAND, params);
				break;
			case 2:
				runstdio(ext->EXT_COMMAND, -1, 1);
				break;
			case 3:
				TypeFile(ext->EXT_COMMAND,
					 TYPE_MAKE | TYPE_WARN);
				break;
			case 4:
				return docmd(ext->EXT_COMMAND, 
					strlen(ext->EXT_COMMAND), 1);
			case 5:
				stdioout(ext->EXT_COMMAND);
				break;
			case 6:
				runpython(ext->EXT_COMMAND, params);
				break;
			case 7: /* DORINFO%n.DEF dropfile DOS door */
				if (!rundosdoor(ext->EXT_COMMAND, 1)) 
					DDPut("Command failed.\n");
				break;
			case 8: /* DOOR.SYS dropfile DOS door */
				if (!rundosdoor(ext->EXT_COMMAND, 2))
					DDPut("Command failed.\n");
				break;
			}
			g_free(params);
			return 1;
		}
		ext++;
	}
	
	g_free(params);
	return EXEC_UNDEFINED;
}

static int print(list_t *args, list_t **stmt_list)
{
	char *str = string_car_and_cdr(args);       
	*stmt_list = cdr(*stmt_list);	
	if (str)
		DDPut(str);	
	return EXEC_OK;
}

static int kbd_stuff(list_t *args, list_t **stmt_list)
{
	char *str = string_car_and_cdr(args);
	*stmt_list = cdr(*stmt_list);
	if (str)
		keyboard_stuff(str);
	return EXEC_OK;
}

static int exec_file(list_t *args, list_t **stmt_list)
{
	int return_code = source_file(args, stmt_list);
	*stmt_list = NULL;
	return return_code;
}

static int chprompt(list_t *args, list_t **stmt_list)
{
	char *pr = string_car_and_cdr(args);
	char *prnmb = string_car_and_cdr(args);
	*stmt_list = cdr(*stmt_list);
	if (!current_menu)
		return EXEC_ARG_ERROR;	
	g_free(current_menu->menu_prompt);
	current_menu->menu_prompt = g_strdup(pr);
	g_free(current_menu->menu_prompt_no_bases);
	current_menu->menu_prompt_no_bases = g_strdup(prnmb);
	return EXEC_OK;
}

static int do_command(list_t *args, list_t **stmt_list)
{
	int retcode;
	char *params;
	
	params = get_parameters(args);
	retcode = docmd(params, strlen(params), 0);
	g_free(params);
	return retcode;
}

static int do_return(list_t *args, list_t **stmt_list)
{
	*stmt_list = NULL;
	return integer_car_and_cdr(args);
}

static int source_file(list_t *args, list_t **stmt_list)
{
	char *name = string_car_and_cdr(args);	
	*stmt_list = cdr(*stmt_list);	
	
	TypeFile(name, 0);	
	return EXEC_OK;
}

static int substitute_args(list_t *args, list_t **stmt_list)
{
	const char *command, *arg_template, *arg_string;
	struct command_arg *cmd_args;
	list_t *arg_list;
	int error_code = 0;
        
	*stmt_list = cdr(*stmt_list);
	command = string_car_and_cdr(args);
	arg_template = string_car_and_cdr(args);
	
	cmd_args = car(struct command_arg *, car(list_t *, scope_args));
	if (cmd_args->type != ARGSTR)
		return EXEC_ARG_ERROR;
	arg_string = cmd_args->value.string;
	
	if (!arg_string)
		arg_string = "";

	arg_list = NULL;

	while (*arg_template & !error_code) {
		const char *p;
		char *r;
		int i;	       

		if (*arg_template != '%') {
			error_code = EXEC_ARG_ERROR;
			break;
		}
		switch (*++arg_template) {
		case '\'':
			p = ++arg_template;
			while (*p && !isspace(*p)) {
				if (*p == '%' && p[1] != '%')
					break;
				p++;
			}
			r = (char *) xmalloc(p - arg_template + 1);
			memcpy(r, arg_template, p - arg_template);
			r[p - arg_template] = 0;
			cons(arg_list, arg_new_string(r));
			free(r);
			arg_template = trim(p);
			break;
		case 'S':
			arg_template++;
			arg_string = p = trim(arg_string);
			cons(arg_list, arg_new_string(arg_string));
			arg_string += strlen(arg_string);
			break;			
		case 's':
			arg_template++;
			arg_string = p = trim(arg_string);
			if (!*arg_string) {
				error_code = EXEC_ARG_ERROR;
				break;
			}
			while (*p && !isspace(*p))
				p++;
			r = (char *) xmalloc(p - arg_string + 1);
			memcpy(r, arg_string, p - arg_string);
			r[p - arg_string] = 0;
			cons(arg_list, arg_new_string(r));
			free(r);
			arg_string = p;
			break;
		case 'd':
			arg_template++;
			arg_string = trim(arg_string);
			i = read_integer(&arg_string);
			if (i == -1) {
				error_code = EXEC_ARG_ERROR;
				break;
			}
			cons(arg_list, arg_new_integer(i));
			break;
		default:
			error_code = EXEC_ARG_ERROR;
			break;
		}
	}

	if (!error_code) {
		list_t *command_list = NULL;
		cons(command_list, 
		     command_atom_new(tokenize_command(command), arg_list));
		push(scope_args, arg_list);
		error_code = execute_command(command_list);		
		arg_list = atom_list_destroy(shift(list_t *, scope_args));
		
		atom_list_destroy(command_list);
	}
	
	atom_list_destroy(arg_list);
	return error_code;	
}

int docmd(const char *command, size_t command_len, int skipd)
{
	struct symbol *binding;
	const char *params;
	size_t params_len;
	int error_code = 0;

	list_t *list = NULL;
	if (!current_menu)
		return EXEC_UNDEFINED;

	if (!command)
		return EXEC_NULL;

	while (command_len) {
		if (!isspace(*command))
			break;
		command++;
		command_len--;
	}
		
	params_len = command_len;
	params = command;
	while (params_len) {
		if (isspace(*params))
			break;
		params++;
		params_len--;
	}
	command_len = params - command;

	while (params_len) {
		if (!isspace(*params))
			break;
		params++;
		params_len--;
	}
	while (params_len > 0 && isspace(params[params_len - 1]))
		params_len--;
	
	if (!*params)
		params = NULL;
	if (!*command)
		command = NULL;
	
	if (!(binding = symbol_table_lookup(current_menu->hotkeys, command, command_len)))
		if (!(binding = symbol_table_lookup(current_menu->commands, command, command_len))) 
			return EXEC_UNDEFINED;
	
	cons(list, arg_new_string(params));
	push(scope_args, list);

	error_code = execute_command(binding->atom_list);
	atom_list_destroy(shift(list_t *, scope_args));
	
	return error_code;
}

int domenu(int pause)
{
	char buffer[512];
	const char *s;
	
	if (pause && !(user.user_toggles & (1L << 4))) {
		dpause();
		if (current_menu)
			s = current_menu->view_file;
		else 
			s = NULL;
		if (!s)
			s = "";		
		TypeFile(s, TYPE_SEC | TYPE_MAKE | TYPE_WARN | TYPE_CONF);
	}
	
	makemainprompt(buffer, sizeof buffer);
	DDPut(buffer);
	
	buffer[0] = 0;
	if (!(Prompt(buffer, 400, PROMPT_MAIN)))
		return EXEC_HANGUP;

	s = trim(buffer);
	return docmd(s, strlen(s), 0);
}

static int execute_command(list_t *command_list)
{
	int ret_value = EXEC_NULL;

	list_t *head, *work_list;
	
	work_list = head = atom_list_clone(command_list);
	
	for (;;) {
		struct command *command;
		struct command_string *definition;
		
		if (!work_list) 
			break;
		
		command = car(struct command *, work_list);
		definition = get_command_definition(command->type);
	
		if (check_arguments((struct atom *) command) != 1) {
			ret_value = EXEC_ARG_ERROR;
			break;
		}
		
		if(!definition) {
			break;
		}
		
		if (definition->internal) {
			ret_value = definition->internal(command->args, &work_list);
			if (ret_value != EXEC_OK && ret_value != EXEC_NULL)
				break; 
		}
	}
	
	atom_list_destroy(head);
	
	
	return ret_value;
}

static struct atom *command_atom_new(int type, list_t *arg)
{
	struct command *command;
	
	command = g_new(struct command, 1);
	command->callback_table.clone = command_atom_clone;
	command->callback_table.destroy = command_atom_destroy;
	command->type = type;
	command->args = atom_list_clone(arg);
	
	return (struct atom *) command;
}

static struct atom *command_atom_clone(const struct atom *atom)
{
	struct command *command, *cloned_command;
	
	command = (struct command *) atom;
	cloned_command = g_new(struct command, 1);       
	cloned_command->callback_table.clone = command_atom_clone;
	cloned_command->callback_table.destroy = command_atom_destroy;
	cloned_command->type = command->type;
	cloned_command->args = atom_list_clone(command->args);
	return (struct atom *) cloned_command;
}

static void command_atom_destroy(struct atom *atom)
{
	struct command *command = (struct command *) atom;
	atom_list_destroy(command->args);
	g_free(command);
}

static struct atom *arg_new_integer(int i)
{
	struct command_arg *arg = g_new(struct command_arg, 1);
	arg->callback_table.clone = arg_atom_clone;
	arg->callback_table.destroy = arg_atom_destroy;	
	arg->value.integer = i;
	arg->type = ARGINT;
	return (struct atom *) arg;
}

static struct atom *arg_new_string(const char *string)
{
	struct command_arg *arg = g_new(struct command_arg, 1);
	arg->callback_table.clone = arg_atom_clone;
	arg->callback_table.destroy = arg_atom_destroy;	
	arg->value.string = g_strdup(string);
	arg->type = ARGSTR;
	return (struct atom *) arg;
}

static struct atom *arg_atom_clone(const struct atom *atom)
{
	const struct command_arg *arg = (const struct command_arg *) atom;
	struct command_arg *cloned_arg = g_new(struct command_arg, 1);
	cloned_arg->callback_table.clone = arg_atom_clone;
	cloned_arg->callback_table.destroy = arg_atom_destroy;	
	cloned_arg->type = arg->type;
	switch (cloned_arg->type) {
	case ARGSTR:
		cloned_arg->value.string = g_strdup(arg->value.string);
		break;
	case ARGINT:
		cloned_arg->value.integer = arg->value.integer;
		break;
	}
	return (struct atom *) cloned_arg;
}

static void arg_atom_destroy(struct atom *atom)
{
	struct command_arg *arg = (struct command_arg *) atom;
	if (arg->type == ARGSTR)
		g_free(arg->value.string);
	g_free(arg);
}

static struct menu *menu_new(const char *view_file, const char *node_state)
{
	struct menu *menu = g_new0(struct menu, 1);

	menu->view_file = g_strdup(view_file);
	menu->node_state = g_strdup(node_state);	

	menu->commands = symbol_table_new();
	menu->hotkeys = symbol_table_new();
	return menu;
}

static void menu_destroy(struct menu *menu)
{
	if (!menu)
		return;
	
	g_free(menu->menu_prompt);
	g_free(menu->view_file);
	g_free(menu->node_state);
	symbol_table_destroy(menu->commands);
	symbol_table_destroy(menu->hotkeys);
	g_free(menu);
}

static int expect_character(const char **source, char ch)
{
	*source = trim(*source);
	if (**source == ch) {
		(*source)++;
		return 1;
	} else
		return 0;
}

static char *read_quoted_string(const char **source)
{
	int quote_mode = 0, empty_string = 1;
	const char *s;

	string_t *result = strnew();
	
	for (s = trim(*source); *s; s++) {
		int i;		
		if (*s == '"') {
			empty_string = 0;
			quote_mode = !quote_mode;
			continue;
		}
		if (!quote_mode && !isspace(*s))
			break;
		empty_string = 0;
		if (*s != '\\') {
			strappend_c(result, *s);
			continue;
		}
		/* switch-statement taken from glib's gscanner.c */
		switch (*++s) {			
		case 0:
			break;			
		case '\\':
			strappend_c (result, '\\');
			break;		
		case 'e':
			strappend_c (result, '\033');
			break;
		case 'n':
			strappend_c (result, '\n');
			break;			
		case 't':
			strappend_c (result, '\t');
			break;			
		case 'r':
			strappend_c (result, '\r');
			break;			
		case 'b':
			strappend_c (result, '\b');
			break;			
		case 'f':
			strappend_c (result, '\f');
			break;			
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			i = *s - '0';
			if (s[1] >= '0' && s[1] <= '7') {
				i = i * 8 + *++s - '0';
				if (s[1] >= '0' && s[1] <= '7') {
					i = i * 8 + *++s - '0';
				}
			}
			strappend_c (result, i);
			break;
			
		default:
			strappend_c (result, *s);
			break;
		}
	}
	
	*source = s;
	return strfree(result, quote_mode || empty_string);
}

static int read_integer(const char **source)
{
	int result = 0;
	const char *s, *p;
	
	for (s = *source; isspace(*s); s++);	
	for (p = s; isdigit(*p); result = result * 10 + *p - '0', p++);
	
	*source = p;
	if (s == p)
		return -1;
	else 
		return result;
}
		
static char *read_unquoted_string(const char **source)
{
	int empty_string = 1;
	const char *s;
	
	string_t *result = strnew();
		
	for (s = *source; *s; s++) {
		if (isspace(*s)) {
			if (empty_string)
				continue;
			else
				break;
		}
				
		if (!isalpha(*s) && *s != '_') {
			if (isdigit(*s)) {
				if (empty_string)
					break;
			} else 
				break;
		}
		
		empty_string = 0;
		strappend_c(result, *s);
	}

	*source = s;	
	return strfree(result, empty_string);
}

static const char *trim(const char *s)
{
	while (isspace(*s))
		s++;
	return s;
}

static int tokenize_command(const char *command)
{
	const struct command_string *cmd;
	
	for (cmd = command_strings; cmd->string; cmd++)
		if (!strcasecmp(command, cmd->string))
			break;
	
	if (!cmd->string)
		return -1;
	else
		return cmd->code;
}

static struct command_string *get_command_definition(int command_code)
{
	struct command_string *cmd;
	
	for (cmd = command_strings; cmd->string; cmd++)
		if (command_code == cmd->code)
			return cmd;
	
	return NULL;
}

static int get_arg_template(int command_code)
{
	struct command_string *cmd;
	
	for (cmd = command_strings; cmd->string; cmd++)
		if (command_code == cmd->code)
			break;
	
	if (!cmd->string)
		return -1;
	else
		return cmd->arg_template;
}

static void makemainprompt(char *buf, size_t bufsize)
{
	char tmp[16];
	char *template;
	char code;

	if (!bufsize)
		return;
	*buf = '\0';
	
	if (current_menu && current_menu->menu_prompt)
		template = current_menu->menu_prompt;
	else
		template = sd[mainmenustr];
	
	if (!conference()->conf.CONF_MSGBASES) {
		if (current_menu && current_menu->menu_prompt_no_bases)
			template = current_menu->menu_prompt_no_bases;
		else
			template = sd[mainmenustrnmb];
	}

	while (*template) {
		if (*template != '~') {
			tmp[0] = *template++;
			tmp[1] = '\0';
			strlcat(buf, tmp, bufsize);
			continue;
		}

		if (*++template == '\0')
			break;

		switch (tolower(code = *template++)) {
		case 'c':
			strlcat(buf, conference()->conf.CONF_NAME, bufsize);
			break;
		case 'n':
			snprintf(tmp, sizeof tmp, "%d", 
				conference()->conf.CONF_NUMBER);
			strlcat(buf, tmp, bufsize);
			break;
		case 't':
			snprintf(tmp, sizeof tmp, "%d", timeleft / 60);
			strlcat(buf, tmp, bufsize);
			break;
		case 'b':
			snprintf(tmp, sizeof tmp, "%d", node);
			strlcat(buf, tmp, bufsize);
			break;
		case 's':
			strlcat(buf, maincfg.CFG_BOARDNAME, bufsize);
			break;
		case '~':
			strlcat(buf, "~", bufsize);
			break;
		}

		if (!conference()->conf.CONF_MSGBASES) 
			continue;					
		
		switch (tolower(code)) {
		case 'a':
			snprintf(tmp, sizeof tmp, "%d", lrp);
			strlcat(buf, tmp, bufsize);
			break;
		case 'e':
			snprintf(tmp, sizeof tmp, "%d", highest);
			strlcat(buf, tmp, bufsize);
			break;
		case 'l':
			snprintf(tmp, sizeof tmp, "%d", 
				current_msgbase->MSGBASE_NUMBER);
			strlcat(buf, tmp, bufsize);
			break;
		case 'm':
			strlcat(buf, current_msgbase->MSGBASE_NAME, bufsize);
			break;
		}			
	}
}
