
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 41 "ftpcmd.y"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/ftp.h>
#include <signal.h>
#include <setjmp.h>
#include <syslog.h>
#include <time.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <ddftp.h>

struct tab {
	const char *name;
	short token;
	short state;
	short implemented;	/* 1 if command is implemented */
	const char *help;
};

static struct tab cmdtab[];
static struct tab sitetab[];

extern	struct sockaddr_in data_dest;
extern	int logged_in;
extern	struct passwd *pw;
extern	int type;
extern	int form;
extern	int debug;
extern	int timeout;
extern	int maxtimeout;
extern  int pdata;
extern  int ulconf;
extern	char hostname[], remotehost[];
extern	char proctitle[];
extern	char *globerr;
extern	int usedefault;
extern  int transflag;
extern  char tmpline[];

off_t	restart_point;

static	int cmd_type;
static	int cmd_form;
static	int cmd_bytesz;
char	cbuf[512];
char	*fromname;

static void help(struct tab *ctab, char *s);
static void sizecmd(char *filename);
static int yylex(void);


/* Line 189 of yacc.c  */
#line 135 "ftpcmd.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     A = 258,
     B = 259,
     C = 260,
     E = 261,
     F = 262,
     I = 263,
     L = 264,
     N = 265,
     P = 266,
     R = 267,
     S = 268,
     T = 269,
     SP = 270,
     CRLF = 271,
     COMMA = 272,
     STRING = 273,
     NUMBER = 274,
     USER = 275,
     PASS = 276,
     ACCT = 277,
     REIN = 278,
     QUIT = 279,
     PORT = 280,
     PASV = 281,
     TYPE = 282,
     STRU = 283,
     MODE = 284,
     RETR = 285,
     STOR = 286,
     APPE = 287,
     MLFL = 288,
     MAIL = 289,
     MSND = 290,
     MSOM = 291,
     MSAM = 292,
     MRSQ = 293,
     MRCP = 294,
     ALLO = 295,
     REST = 296,
     RNFR = 297,
     RNTO = 298,
     ABOR = 299,
     DELE = 300,
     CWD = 301,
     LIST = 302,
     NLST = 303,
     SITE = 304,
     STAT = 305,
     HELP = 306,
     NOOP = 307,
     MKD = 308,
     RMD = 309,
     PWD = 310,
     CDUP = 311,
     STOU = 312,
     SMNT = 313,
     SYST = 314,
     SIZE = 315,
     MDTM = 316,
     UMASK = 317,
     IDLE = 318,
     CHMOD = 319,
     UPLOAD = 320,
     LEXERR = 321
   };
#endif
/* Tokens.  */
#define A 258
#define B 259
#define C 260
#define E 261
#define F 262
#define I 263
#define L 264
#define N 265
#define P 266
#define R 267
#define S 268
#define T 269
#define SP 270
#define CRLF 271
#define COMMA 272
#define STRING 273
#define NUMBER 274
#define USER 275
#define PASS 276
#define ACCT 277
#define REIN 278
#define QUIT 279
#define PORT 280
#define PASV 281
#define TYPE 282
#define STRU 283
#define MODE 284
#define RETR 285
#define STOR 286
#define APPE 287
#define MLFL 288
#define MAIL 289
#define MSND 290
#define MSOM 291
#define MSAM 292
#define MRSQ 293
#define MRCP 294
#define ALLO 295
#define REST 296
#define RNFR 297
#define RNTO 298
#define ABOR 299
#define DELE 300
#define CWD 301
#define LIST 302
#define NLST 303
#define SITE 304
#define STAT 305
#define HELP 306
#define NOOP 307
#define MKD 308
#define RMD 309
#define PWD 310
#define CDUP 311
#define STOU 312
#define SMNT 313
#define SYST 314
#define SIZE 315
#define MDTM 316
#define UMASK 317
#define IDLE 318
#define CHMOD 319
#define UPLOAD 320
#define LEXERR 321




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 309 "ftpcmd.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   199

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  15
/* YYNRULES -- Number of rules.  */
#define YYNRULES  76
/* YYNRULES -- Number of states.  */
#define YYNSTATES  209

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   321

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,    10,    15,    20,    25,    28,
      33,    38,    43,    48,    57,    63,    69,    75,    79,    85,
      89,    95,   101,   104,   110,   115,   118,   122,   128,   131,
     136,   139,   145,   151,   155,   159,   164,   171,   177,   185,
     195,   200,   207,   212,   219,   225,   228,   234,   240,   243,
     246,   252,   257,   259,   260,   262,   264,   276,   278,   280,
     282,   284,   288,   290,   294,   296,   298,   302,   305,   307,
     309,   311,   313,   315,   317,   319,   321
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      68,     0,    -1,    -1,    68,    69,    -1,    68,    70,    -1,
      20,    15,    71,    16,    -1,    21,    15,    72,    16,    -1,
      25,    15,    74,    16,    -1,    26,    16,    -1,    27,    15,
      76,    16,    -1,    28,    15,    77,    16,    -1,    29,    15,
      78,    16,    -1,    40,    15,    19,    16,    -1,    40,    15,
      19,    15,    12,    15,    19,    16,    -1,    30,    81,    15,
      79,    16,    -1,    31,    81,    15,    79,    16,    -1,    32,
      81,    15,    79,    16,    -1,    48,    81,    16,    -1,    48,
      81,    15,    18,    16,    -1,    47,    81,    16,    -1,    47,
      81,    15,    79,    16,    -1,    50,    81,    15,    79,    16,
      -1,    50,    16,    -1,    45,    81,    15,    79,    16,    -1,
      43,    15,    79,    16,    -1,    44,    16,    -1,    46,    81,
      16,    -1,    46,    81,    15,    79,    16,    -1,    51,    16,
      -1,    51,    15,    18,    16,    -1,    52,    16,    -1,    53,
      81,    15,    79,    16,    -1,    54,    81,    15,    79,    16,
      -1,    55,    81,    16,    -1,    56,    81,    16,    -1,    49,
      15,    51,    16,    -1,    49,    15,    51,    15,    18,    16,
      -1,    49,    15,    62,    81,    16,    -1,    49,    15,    62,
      81,    15,    80,    16,    -1,    49,    15,    64,    81,    15,
      80,    15,    79,    16,    -1,    49,    15,    63,    16,    -1,
      49,    15,    63,    15,    19,    16,    -1,    49,    15,    65,
      16,    -1,    49,    15,    65,    15,    19,    16,    -1,    57,
      81,    15,    79,    16,    -1,    59,    16,    -1,    60,    81,
      15,    79,    16,    -1,    61,    81,    15,    79,    16,    -1,
      24,    16,    -1,     1,    16,    -1,    42,    81,    15,    79,
      16,    -1,    41,    15,    73,    16,    -1,    18,    -1,    -1,
      18,    -1,    19,    -1,    19,    17,    19,    17,    19,    17,
      19,    17,    19,    17,    19,    -1,    10,    -1,    14,    -1,
       5,    -1,     3,    -1,     3,    15,    75,    -1,     6,    -1,
       6,    15,    75,    -1,     8,    -1,     9,    -1,     9,    15,
      73,    -1,     9,    73,    -1,     7,    -1,    12,    -1,    11,
      -1,    13,    -1,     4,    -1,     5,    -1,    18,    -1,    19,
      -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   124,   124,   125,   129,   132,   136,   140,   148,   151,
     185,   196,   207,   210,   213,   219,   225,   231,   235,   241,
     245,   251,   257,   260,   266,   276,   279,   283,   289,   292,
     306,   309,   315,   321,   325,   329,   332,   335,   344,   358,
     369,   374,   387,   392,   403,   409,   429,   445,   465,   469,
     473,   482,   490,   493,   496,   499,   502,   514,   517,   520,
     525,   529,   533,   537,   541,   544,   548,   553,   559,   562,
     565,   570,   573,   576,   581,   584,   608
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "A", "B", "C", "E", "F", "I", "L", "N",
  "P", "R", "S", "T", "SP", "CRLF", "COMMA", "STRING", "NUMBER", "USER",
  "PASS", "ACCT", "REIN", "QUIT", "PORT", "PASV", "TYPE", "STRU", "MODE",
  "RETR", "STOR", "APPE", "MLFL", "MAIL", "MSND", "MSOM", "MSAM", "MRSQ",
  "MRCP", "ALLO", "REST", "RNFR", "RNTO", "ABOR", "DELE", "CWD", "LIST",
  "NLST", "SITE", "STAT", "HELP", "NOOP", "MKD", "RMD", "PWD", "CDUP",
  "STOU", "SMNT", "SYST", "SIZE", "MDTM", "UMASK", "IDLE", "CHMOD",
  "UPLOAD", "LEXERR", "$accept", "cmd_list", "cmd", "rcmd", "username",
  "password", "byte_size", "host_port", "form_code", "type_code",
  "struct_code", "mode_code", "pathname", "octal_number", "check_login", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    67,    68,    68,    68,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      70,    70,    71,    72,    72,    73,    74,    75,    75,    75,
      76,    76,    76,    76,    76,    76,    76,    76,    77,    77,
      77,    78,    78,    78,    79,    80,    81
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     4,     4,     4,     2,     4,
       4,     4,     4,     8,     5,     5,     5,     3,     5,     3,
       5,     5,     2,     5,     4,     2,     3,     5,     2,     4,
       2,     5,     5,     3,     3,     4,     6,     5,     7,     9,
       4,     6,     4,     6,     5,     2,     5,     5,     2,     2,
       5,     4,     1,     0,     1,     1,    11,     1,     1,     1,
       1,     3,     1,     3,     1,     1,     3,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    76,    76,    76,     0,     0,    76,     0,     0,
      76,    76,    76,    76,     0,    76,     0,     0,    76,    76,
      76,    76,    76,     0,    76,    76,     3,     4,    49,     0,
      53,    48,     0,     8,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    25,     0,     0,     0,     0,     0,
      22,     0,     0,    28,    30,     0,     0,     0,     0,     0,
      45,     0,     0,    52,     0,    54,     0,     0,     0,    60,
      62,    64,    65,     0,    68,    70,    69,     0,    72,    73,
      71,     0,     0,     0,     0,     0,    55,     0,     0,    74,
       0,     0,     0,    26,     0,    19,     0,    17,     0,    76,
       0,    76,     0,     0,     0,     0,     0,    33,    34,     0,
       0,     0,     5,     6,     0,     7,     0,     0,     0,    67,
       9,    10,    11,     0,     0,     0,     0,    12,    51,     0,
      24,     0,     0,     0,     0,     0,    35,     0,     0,    40,
       0,     0,    42,     0,    29,     0,     0,     0,     0,     0,
       0,    59,    57,    58,    61,    63,    66,    14,    15,    16,
       0,    50,    23,    27,    20,    18,     0,     0,    37,     0,
       0,     0,    21,    31,    32,    44,    46,    47,     0,     0,
      36,    75,     0,    41,     0,    43,     0,     0,    38,     0,
       0,    13,     0,     0,    39,     0,     0,     0,    56
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    36,    37,    74,    76,    97,    78,   164,    83,
      87,    91,   100,   192,    47
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -93
static const yytype_int16 yypact[] =
{
     -93,    35,   -93,   -11,    -7,    29,    27,    39,    56,    78,
      95,    96,   -93,   -93,   -93,    97,    98,   -93,    99,   100,
     -93,   -93,   -93,   -93,   102,   103,     4,   104,   -93,   -93,
     -93,   -93,   -93,   105,   -93,   -93,   -93,   -93,   -93,   106,
     107,   -93,   108,   -93,    65,    46,    28,   114,   115,   116,
     113,   117,   118,   119,   -93,   120,    10,    54,    82,   -48,
     -93,   123,   121,   -93,   -93,   125,   126,   127,   128,   130,
     -93,   131,   132,   -93,   133,   -93,   134,   101,   135,   137,
     138,   -93,    -8,   139,   -93,   -93,   -93,   140,   -93,   -93,
     -93,   141,   119,   119,   119,    84,   -93,   142,   119,   -93,
     143,   119,   119,   -93,   119,   -93,   124,   -93,    86,   -93,
      88,   -93,    90,   119,   144,   119,   119,   -93,   -93,   119,
     119,   119,   -93,   -93,   129,   -93,     8,     8,   117,   -93,
     -93,   -93,   -93,   145,   146,   147,   110,   -93,   -93,   148,
     -93,   149,   150,   151,   152,   136,   -93,    93,   153,   -93,
     154,   155,   -93,   157,   -93,   159,   160,   161,   162,   163,
     164,   -93,   -93,   -93,   -93,   -93,   -93,   -93,   -93,   -93,
     156,   -93,   -93,   -93,   -93,   -93,   166,   165,   -93,   167,
     165,   169,   -93,   -93,   -93,   -93,   -93,   -93,   168,   170,
     -93,   -93,   172,   -93,   171,   -93,   173,   175,   -93,   119,
     174,   -93,   176,   177,   -93,   178,   179,   180,   -93
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -93,   -93,   -93,   -93,   -93,   -93,   -78,   -93,   -12,   -93,
     -93,   -93,   -92,   -57,    17
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
     133,   134,   135,   108,   129,    38,   139,   128,    39,   141,
     142,    96,   143,   161,   109,   110,   111,   112,   162,    62,
      63,   153,   163,   155,   156,   102,   103,   157,   158,   159,
      48,    49,    88,    89,    52,     2,     3,    55,    56,    57,
      58,    90,    61,    41,    40,    65,    66,    67,    68,    69,
     166,    71,    72,    84,    42,     4,     5,    85,    86,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    79,   104,
     105,    80,    43,    81,    82,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    44,    33,    34,    35,   106,   107,   136,
     137,   145,   146,   148,   149,   151,   152,   202,   177,   178,
      45,    46,    50,    51,    53,   165,    54,    59,   124,    60,
      64,    70,   170,   194,    73,    75,   147,    77,   150,    92,
      93,    94,    95,    98,     0,   101,    96,    99,   113,   114,
     115,   116,   144,   117,   118,   119,   120,   121,   160,   122,
     123,   125,   126,   127,   176,   130,   131,   132,   138,   140,
     154,   167,   168,   169,   171,   172,   173,   174,   175,   180,
       0,   189,   179,   182,   181,   183,   184,   185,   186,   187,
       0,   188,   190,   193,   191,   195,   199,   196,   198,   197,
     200,   201,   204,   203,   205,     0,   207,   206,     0,   208
};

static const yytype_int16 yycheck[] =
{
      92,    93,    94,    51,    82,    16,    98,    15,    15,   101,
     102,    19,   104,     5,    62,    63,    64,    65,    10,    15,
      16,   113,    14,   115,   116,    15,    16,   119,   120,   121,
      13,    14,     4,     5,    17,     0,     1,    20,    21,    22,
      23,    13,    25,    16,    15,    28,    29,    30,    31,    32,
     128,    34,    35,     7,    15,    20,    21,    11,    12,    24,
      25,    26,    27,    28,    29,    30,    31,    32,     3,    15,
      16,     6,    16,     8,     9,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    15,    59,    60,    61,    15,    16,    15,
      16,    15,    16,    15,    16,    15,    16,   199,    15,    16,
      15,    15,    15,    15,    15,   127,    16,    15,    17,    16,
      16,    16,    12,   180,    18,    18,   109,    19,   111,    15,
      15,    15,    19,    15,    -1,    15,    19,    18,    15,    18,
      15,    15,    18,    16,    16,    15,    15,    15,    19,    16,
      16,    16,    15,    15,    18,    16,    16,    16,    16,    16,
      16,    16,    16,    16,    16,    16,    16,    16,    16,    15,
      -1,    15,    19,    16,    19,    16,    16,    16,    16,    16,
      -1,    17,    16,    16,    19,    16,    15,    19,    16,    19,
      17,    16,    16,    19,    17,    -1,    17,    19,    -1,    19
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    68,     0,     1,    20,    21,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    59,    60,    61,    69,    70,    16,    15,
      15,    16,    15,    16,    15,    15,    15,    81,    81,    81,
      15,    15,    81,    15,    16,    81,    81,    81,    81,    15,
      16,    81,    15,    16,    16,    81,    81,    81,    81,    81,
      16,    81,    81,    18,    71,    18,    72,    19,    74,     3,
       6,     8,     9,    76,     7,    11,    12,    77,     4,     5,
      13,    78,    15,    15,    15,    19,    19,    73,    15,    18,
      79,    15,    15,    16,    15,    16,    15,    16,    51,    62,
      63,    64,    65,    15,    18,    15,    15,    16,    16,    15,
      15,    15,    16,    16,    17,    16,    15,    15,    15,    73,
      16,    16,    16,    79,    79,    79,    15,    16,    16,    79,
      16,    79,    79,    79,    18,    15,    16,    81,    15,    16,
      81,    15,    16,    79,    16,    79,    79,    79,    79,    79,
      19,     5,    10,    14,    75,    75,    73,    16,    16,    16,
      12,    16,    16,    16,    16,    16,    18,    15,    16,    19,
      15,    19,    16,    16,    16,    16,    16,    16,    17,    15,
      16,    19,    80,    16,    80,    16,    19,    19,    16,    15,
      17,    16,    79,    19,    16,    17,    19,    17,    19
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:

/* Line 1455 of yacc.c  */
#line 125 "ftpcmd.y"
    {
			fromname = (char *) 0;
			restart_point = (off_t) 0;
		}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 132 "ftpcmd.y"
    {
			user((char *) (yyvsp[(3) - (4)]));
			free((char *) (yyvsp[(3) - (4)]));
		}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 136 "ftpcmd.y"
    {
			pass((char *) (yyvsp[(3) - (4)]));
			free((char *) (yyvsp[(3) - (4)]));
		}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 140 "ftpcmd.y"
    {
			usedefault = 0;
			if (pdata >= 0) {
				close(pdata);
				pdata = -1;
			}
			reply(200, "PORT command successful.");
		}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 148 "ftpcmd.y"
    {
			passive();
		}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 151 "ftpcmd.y"
    {
			switch (cmd_type) {

			case TYPE_A:
				if (cmd_form == FORM_N) {
					reply(200, "Type set to A.");
					type = cmd_type;
					form = cmd_form;
				} else
					reply(504, "Form must be N.");
				break;

			case TYPE_E:
				reply(504, "Type E not implemented.");
				break;

			case TYPE_I:
				reply(200, "Type set to I.");
				type = cmd_type;
				break;

			case TYPE_L:
#if NBBY == 8
				if (cmd_bytesz == 8) {
					reply(200,
					    "Type set to L (byte size 8).");
					type = cmd_type;
				} else
					reply(504, "Byte size must be 8.");
#else /* NBBY == 8 */
				UNIMPLEMENTED for NBBY != 8
#endif /* NBBY == 8 */
			}
		}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 185 "ftpcmd.y"
    {
			switch ((yyvsp[(3) - (4)])) {

			case STRU_F:
				reply(200, "STRU F ok.");
				break;

			default:
				reply(504, "Unimplemented STRU type.");
			}
		}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 196 "ftpcmd.y"
    {
			switch ((yyvsp[(3) - (4)])) {

			case MODE_S:
				reply(200, "MODE S ok.");
				break;

			default:
				reply(502, "Unimplemented MODE type.");
			}
		}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 207 "ftpcmd.y"
    {
			reply(202, "ALLO command ignored.");
		}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 210 "ftpcmd.y"
    {
			reply(202, "ALLO command ignored.");
		}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 213 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				retrieve((char *) 0, (char *) (yyvsp[(4) - (5)]));
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 219 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				_store((char *) (yyvsp[(4) - (5)]), "w", 0);
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 225 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				_store((char *) (yyvsp[(4) - (5)]), "a", 0);
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 231 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (3)]))
				send_file_list(".");
		}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 235 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL) 
				send_file_list((char *) (yyvsp[(4) - (5)]));
			if ((char *) (char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 241 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (3)]))
				retrieve("/bin/ls -lgA", "");
		}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 245 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				retrieve("/bin/ls -lgA %s", (char *) (yyvsp[(4) - (5)]));
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 251 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				statfilecmd((char *) (yyvsp[(4) - (5)]));
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 257 "ftpcmd.y"
    {
			statcmd();
		}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 260 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				_delete((char *) (yyvsp[(4) - (5)]));
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 266 "ftpcmd.y"
    {
			if (fromname) {
				renamecmd(fromname, (char *) (yyvsp[(3) - (4)]));
				free(fromname);
				fromname = (char *) 0;
			} else {
				reply(503, "Bad sequence of commands.");
			}
			free((char *) (yyvsp[(3) - (4)]));
		}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 276 "ftpcmd.y"
    {
			reply(225, "ABOR command successful.");
		}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 279 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (3)]))
				cwd(pw->pw_dir);
		}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 283 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				cwd((char *) (yyvsp[(4) - (5)]));
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 289 "ftpcmd.y"
    {
			help(cmdtab, (char *) 0);
		}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 292 "ftpcmd.y"
    {
			char *cp = (char *)(yyvsp[(3) - (4)]);

			if (strncasecmp(cp, "SITE", 4) == 0) {
				cp = (char *)(yyvsp[(3) - (4)]) + 4;
				if (*cp == ' ')
					cp++;
				if (*cp)
					help(sitetab, cp);
				else
					help(sitetab, (char *) 0);
			} else
				help(cmdtab, (char *) (yyvsp[(3) - (4)]));
		}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 306 "ftpcmd.y"
    {
			reply(200, "NOOP command successful.");
		}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 309 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				makedir((char *) (yyvsp[(4) - (5)]));
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 315 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				removedir((char *) (yyvsp[(4) - (5)]));
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 321 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (3)]))
				pwd();
		}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 325 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (3)]))
				cwd("..");
		}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 329 "ftpcmd.y"
    {
			help(sitetab, (char *) 0);
		}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 332 "ftpcmd.y"
    {
			help(sitetab, (char *) (yyvsp[(5) - (6)]));
		}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 335 "ftpcmd.y"
    {
			int oldmask;

			if ((yyvsp[(4) - (5)])) {
				oldmask = umask(0);
				umask(oldmask);
				reply(200, "Current UMASK is %03o", oldmask);
			}
		}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 344 "ftpcmd.y"
    {
			int oldmask;

			if ((yyvsp[(4) - (7)])) {
				if (((yyvsp[(6) - (7)]) == -1) || ((yyvsp[(6) - (7)]) > 0777)) {
					reply(501, "Bad UMASK value");
				} else {
					oldmask = umask((yyvsp[(6) - (7)]));
					reply(200,
					    "UMASK set to %03o (was %03o)",
					    (yyvsp[(6) - (7)]), oldmask);
				}
			}
		}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 358 "ftpcmd.y"
    {
			if ((yyvsp[(4) - (9)]) && ((char *) (yyvsp[(8) - (9)]) != NULL)) {
				if ((yyvsp[(6) - (9)]) > 0777)
					reply(501,
				"CHMOD: Mode value must be between 0 and 0777");
				else
					reply(200, "NO!");
			}
			if ((char *) (yyvsp[(8) - (9)]) != NULL)
				free((char *) (yyvsp[(8) - (9)]));
		}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 369 "ftpcmd.y"
    {
			reply(200,
			    "Current IDLE time limit is %d seconds; max %d",
				timeout, maxtimeout);
		}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 374 "ftpcmd.y"
    {
			if ((yyvsp[(5) - (6)]) < 30 || (yyvsp[(5) - (6)]) > maxtimeout) {
				reply(501,
			"Maximum IDLE time must be between 30 and %d seconds",
				    maxtimeout);
			} else {
				timeout = (yyvsp[(5) - (6)]);
				alarm((unsigned) timeout);
				reply(200,
				    "Maximum IDLE time set to %d seconds",
				    timeout);
			}
		}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 387 "ftpcmd.y"
    {
			reply(200,
			    "Current upload conference is %d",
				ulconf);
		}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 392 "ftpcmd.y"
    {
			if (!setulconf((yyvsp[(5) - (6)]))) {
				reply(501,
			"Couldn't set the conference (unknown or no access)"
				    );
			} else {
				reply(200,
				    "upload conference set to %d",
				    ulconf);
			}
		}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 403 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				_store((char *) (yyvsp[(4) - (5)]), "w", 1);
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 409 "ftpcmd.y"
    {
#ifdef unix
#ifdef BSD
			reply(215, "UNIX Type: L%d Version: BSD-%d",
				NBBY, BSD);
#else /* BSD */
			reply(215, "UNIX Type: L%d", NBBY);
#endif /* BSD */
#else /* unix */
			reply(215, "UNKNOWN Type: L%d", NBBY);
#endif /* unix */
		}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 429 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL)
				sizecmd((char *) (yyvsp[(4) - (5)]));
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 445 "ftpcmd.y"
    {
			if ((yyvsp[(2) - (5)]) && (char *) (yyvsp[(4) - (5)]) != NULL) {
				struct stat stbuf;
				if (stat((char *) (yyvsp[(4) - (5)]), &stbuf) < 0)
					perror_reply(550, (char *) (yyvsp[(4) - (5)]));
				else if ((stbuf.st_mode&S_IFMT) != S_IFREG) {
					reply(550, "%s: not a plain file.",
						(char *) (yyvsp[(4) - (5)]));
				} else {
					struct tm *t;
					t = gmtime(&stbuf.st_mtime);
					reply(213,
					    "%04d%02d%02d%02d%02d%02d",
					    t->tm_year + 1900, t->tm_mon+1, t->tm_mday,
					    t->tm_hour, t->tm_min, t->tm_sec);
				}
			}
			if ((char *) (yyvsp[(4) - (5)]) != NULL)
				free((char *) (yyvsp[(4) - (5)]));
		}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 465 "ftpcmd.y"
    {
			reply(221, "Goodbye.");
			dologout(0);
		}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 469 "ftpcmd.y"
    {
			yyerrok;
		}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 473 "ftpcmd.y"
    {
			restart_point = (off_t) 0;
			if ((yyvsp[(2) - (5)]) && (yyvsp[(4) - (5)])) {
				fromname = renamefrom((char *) (yyvsp[(4) - (5)]));
				if (fromname == (char *) 0 && (yyvsp[(4) - (5)])) {
					free((char *) (yyvsp[(4) - (5)]));
				}
			}
		}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 482 "ftpcmd.y"
    {
			fromname = (char *) 0;
			restart_point = (yyvsp[(3) - (4)]);
			reply(350, "Restarting at %ld. %s", restart_point,
			    "Send STORE or RETRIEVE to initiate transfer.");
		}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 493 "ftpcmd.y"
    {
			*(char **)&((yyval)) = (char *)calloc(1, sizeof(char));
		}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 503 "ftpcmd.y"
    {
			char *a, *p;

			a = (char *)&data_dest.sin_addr;
			a[0] = (yyvsp[(1) - (11)]); a[1] = (yyvsp[(3) - (11)]); a[2] = (yyvsp[(5) - (11)]); a[3] = (yyvsp[(7) - (11)]);
			p = (char *)&data_dest.sin_port;
			p[0] = (yyvsp[(9) - (11)]); p[1] = (yyvsp[(11) - (11)]);
			data_dest.sin_family = AF_INET;
		}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 514 "ftpcmd.y"
    {
		(yyval) = FORM_N;
	}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 517 "ftpcmd.y"
    {
		(yyval) = FORM_T;
	}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 520 "ftpcmd.y"
    {
		(yyval) = FORM_C;
	}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 525 "ftpcmd.y"
    {
		cmd_type = TYPE_A;
		cmd_form = FORM_N;
	}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 529 "ftpcmd.y"
    {
		cmd_type = TYPE_A;
		cmd_form = (yyvsp[(3) - (3)]);
	}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 533 "ftpcmd.y"
    {
		cmd_type = TYPE_E;
		cmd_form = FORM_N;
	}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 537 "ftpcmd.y"
    {
		cmd_type = TYPE_E;
		cmd_form = (yyvsp[(3) - (3)]);
	}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 541 "ftpcmd.y"
    {
		cmd_type = TYPE_I;
	}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 544 "ftpcmd.y"
    {
		cmd_type = TYPE_L;
		cmd_bytesz = NBBY;
	}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 548 "ftpcmd.y"
    {
		cmd_type = TYPE_L;
		cmd_bytesz = (yyvsp[(3) - (3)]);
	}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 553 "ftpcmd.y"
    {
		cmd_type = TYPE_L;
		cmd_bytesz = (yyvsp[(2) - (2)]);
	}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 559 "ftpcmd.y"
    {
		(yyval) = STRU_F;
	}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 562 "ftpcmd.y"
    {
		(yyval) = STRU_R;
	}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 565 "ftpcmd.y"
    {
		(yyval) = STRU_P;
	}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 570 "ftpcmd.y"
    {
		(yyval) = MODE_S;
	}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 573 "ftpcmd.y"
    {
		(yyval) = MODE_B;
	}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 576 "ftpcmd.y"
    {
		(yyval) = MODE_C;
	}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 584 "ftpcmd.y"
    {
		int ret, dec, multby, digit;

		/*
		 * Convert a number that was read as decimal number
		 * to what it would be if it had been read as octal.
		 */
		dec = (yyvsp[(1) - (1)]);
		multby = 1;
		ret = 0;
		while (dec) {
			digit = dec%10;
			if (digit > 7) {
				ret = -1;
				break;
			}
			ret += digit * multby;
			multby *= 8;
			dec /= 10;
		}
		(yyval) = ret;
	}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 608 "ftpcmd.y"
    {
		if (logged_in)
			(yyval) = 1;
		else {
			reply(530, "Please login with USER and PASS.");
			(yyval) = 0;
		}
	}
    break;



/* Line 1455 of yacc.c  */
#line 2522 "ftpcmd.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 618 "ftpcmd.y"


extern jmp_buf errcatch;

#define	CMD	0	/* beginning of command */
#define	ARGS	1	/* expect miscellaneous arguments */
#define	STR1	2	/* expect SP followed by STRING */
#define	STR2	3	/* expect STRING */
#define	OSTR	4	/* optional SP then STRING */
#define	ZSTR1	5	/* SP then optional STRING */
#define	ZSTR2	6	/* optional STRING after SP */
#define	SITECMD	7	/* SITE command */
#define	NSTR	8	/* Number followed by a string */

static struct tab cmdtab[] = {		/* In order defined in RFC 765 */
	{ "USER", USER, STR1, 1,	"<sp> username" },
	{ "PASS", PASS, ZSTR1, 1,	"<sp> password" },
	{ "ACCT", ACCT, STR1, 0,	"(specify account)" },
	{ "SMNT", SMNT, ARGS, 0,	"(structure mount)" },
	{ "REIN", REIN, ARGS, 0,	"(reinitialize server state)" },
	{ "QUIT", QUIT, ARGS, 1,	"(terminate service)", },
	{ "PORT", PORT, ARGS, 1,	"<sp> b0, b1, b2, b3, b4" },
	{ "PASV", PASV, ARGS, 1,	"(set server in passive mode)" },
	{ "TYPE", TYPE, ARGS, 1,	"<sp> [ A | E | I | L ]" },
	{ "STRU", STRU, ARGS, 1,	"(specify file structure)" },
	{ "MODE", MODE, ARGS, 1,	"(specify transfer mode)" },
	{ "RETR", RETR, STR1, 1,	"<sp> file-name" },
	{ "STOR", STOR, STR1, 1,	"<sp> file-name" },
	{ "APPE", APPE, STR1, 1,	"<sp> file-name" },
	{ "MLFL", MLFL, OSTR, 0,	"(mail file)" },
	{ "MAIL", MAIL, OSTR, 0,	"(mail to user)" },
	{ "MSND", MSND, OSTR, 0,	"(mail send to terminal)" },
	{ "MSOM", MSOM, OSTR, 0,	"(mail send to terminal or mailbox)" },
	{ "MSAM", MSAM, OSTR, 0,	"(mail send to terminal and mailbox)" },
	{ "MRSQ", MRSQ, OSTR, 0,	"(mail recipient scheme question)" },
	{ "MRCP", MRCP, STR1, 0,	"(mail recipient)" },
	{ "ALLO", ALLO, ARGS, 1,	"allocate storage (vacuously)" },
	{ "REST", REST, ARGS, 1,	"(restart command)" },
	{ "RNFR", RNFR, STR1, 1,	"<sp> file-name" },
	{ "RNTO", RNTO, STR1, 1,	"<sp> file-name" },
	{ "ABOR", ABOR, ARGS, 1,	"(abort operation)" },
	{ "DELE", DELE, STR1, 1,	"<sp> file-name" },
	{ "CWD",  CWD,  OSTR, 1,	"[ <sp> directory-name ]" },
	{ "XCWD", CWD,	OSTR, 1,	"[ <sp> directory-name ]" },
	{ "LIST", LIST, OSTR, 1,	"[ <sp> path-name ]" },
	{ "NLST", NLST, OSTR, 1,	"[ <sp> path-name ]" },
	{ "SITE", SITE, SITECMD, 1,	"site-cmd [ <sp> arguments ]" },
	{ "SYST", SYST, ARGS, 1,	"(get type of operating system)" },
	{ "STAT", STAT, OSTR, 1,	"[ <sp> path-name ]" },
	{ "HELP", HELP, OSTR, 1,	"[ <sp> <string> ]" },
	{ "NOOP", NOOP, ARGS, 1,	"" },
	{ "MKD",  MKD,  STR1, 1,	"<sp> path-name" },
	{ "XMKD", MKD,  STR1, 1,	"<sp> path-name" },
	{ "RMD",  RMD,  STR1, 1,	"<sp> path-name" },
	{ "XRMD", RMD,  STR1, 1,	"<sp> path-name" },
	{ "PWD",  PWD,  ARGS, 1,	"(return current directory)" },
	{ "XPWD", PWD,  ARGS, 1,	"(return current directory)" },
	{ "CDUP", CDUP, ARGS, 1,	"(change to parent directory)" },
	{ "XCUP", CDUP, ARGS, 1,	"(change to parent directory)" },
	{ "STOU", STOU, STR1, 1,	"<sp> file-name" },
	{ "SIZE", SIZE, OSTR, 1,	"<sp> path-name" },
	{ "MDTM", MDTM, OSTR, 1,	"<sp> path-name" },
	{ NULL,   0,    0,    0,	0 }
};

static struct tab sitetab[] = {
	{ "UMASK", UMASK, ARGS, 1,	"[ <sp> umask ]" },
	{ "IDLE", IDLE, ARGS, 1,	"[ <sp> maximum-idle-time ]" },
	{ "CHMOD", CHMOD, NSTR, 1,	"<sp> mode <sp> file-name" },
	{ "UPLOAD", UPLOAD, ARGS, 1,	"[ <sp> upload-conference ]" },
	{ "HELP", HELP, OSTR, 1,	"[ <sp> <string> ]" },
	{ NULL,   0,    0,    0,	0 }
};

struct tab *lookup(struct tab *p, char *cmd)
{
	for (; p->name != NULL; p++)
		if (strcmp(cmd, p->name) == 0)
			return (p);
	return (0);
}

#include <arpa/telnet.h>

/*
 * ftpgetline - a hacked up version of fgets to ignore TELNET escape codes.
 */
char *ftpgetline(char *s, int n, FILE *iop)
{
	int c;
	char *cs;

	cs = s;
/* tmpline may contain saved command from urgent mode interruption */
	for (c = 0; tmpline[c] != '\0' && --n > 0; ++c) {
		*cs++ = tmpline[c];
		if (tmpline[c] == '\n') {
			*cs++ = '\0';
			if (debug)
				syslog(LOG_DEBUG, "command: %s", s);
			tmpline[0] = '\0';
			return(s);
		}
		if (c == 0)
			tmpline[0] = '\0';
	}
	while ((c = getc(iop)) != EOF) {
		c &= 0377;
		if (c == IAC) {
		    if ((c = getc(iop)) != EOF) {
			c &= 0377;
			switch (c) {
			case WILL:
			case WONT:
				c = getc(iop);
				printf("%c%c%c", IAC, DONT, 0377&c);
				fflush(stdout);
				continue;
			case DO:
			case DONT:
				c = getc(iop);
				printf("%c%c%c", IAC, WONT, 0377&c);
				fflush(stdout);
				continue;
			case IAC:
				break;
			default:
				continue;	/* ignore command */
			}
		    }
		}
		*cs++ = c;
		if (--n <= 0 || c == '\n')
			break;
	}
	if (c == EOF && cs == s)
		return (NULL);
	*cs++ = '\0';
	if (debug)
		syslog(LOG_DEBUG, "command: %s", s);
	return (s);
}

static void toolong()
{
	time_t now;

	reply(421,
	  "Timeout (%d seconds): closing control connection.", timeout);
	time(&now);
	syslog(LOG_INFO,
		"User %s timed out after %d seconds at %s",
		(pw ? pw -> pw_name : "unknown"), timeout, ctime(&now));
	dologout(1);
}

static int yylex(void)
{
	static int cpos, state;
	char *cp, *cp2;
	struct tab *p;
	int n;
	char c, *copy();

	for (;;) {
		switch (state) {

		case CMD:
			signal(SIGALRM, toolong);
			alarm((unsigned) timeout);
			if (ftpgetline(cbuf, sizeof(cbuf)-1, stdin) == NULL) {
				reply(221, "You could at least say goodbye.");
				dologout(0);
			}
			alarm(0);
			if (strncasecmp(cbuf, "PASS", 4) != 0)
				setproctitle("%s: %s", proctitle, cbuf);
			if ((cp = index(cbuf, '\r'))) {
				*cp++ = '\n';
				*cp = '\0';
			}
			if ((cp = strpbrk(cbuf, " \n")))
				cpos = cp - cbuf;
			if (cpos == 0)
				cpos = 4;
			c = cbuf[cpos];
			cbuf[cpos] = '\0';
			upper(cbuf);
			p = lookup(cmdtab, cbuf);
			cbuf[cpos] = c;
			if (p != 0) {
				if (p->implemented == 0) {
					nack(p->name);
					longjmp(errcatch,0);
					/* NOTREACHED */
				}
				state = p->state;
				*(char **)&yylval = p->name;
				return (p->token);
			}
			break;

		case SITECMD:
			if (cbuf[cpos] == ' ') {
				cpos++;
				return (SP);
			}
			cp = &cbuf[cpos];
			if ((cp2 = strpbrk(cp, " \n")))
				cpos = cp2 - cbuf;
			c = cbuf[cpos];
			cbuf[cpos] = '\0';
			upper(cp);
			p = lookup(sitetab, cp);
			cbuf[cpos] = c;
			if (p != 0) {
				if (p->implemented == 0) {
					state = CMD;
					nack(p->name);
					longjmp(errcatch,0);
					/* NOTREACHED */
				}
				state = p->state;
				*(char **)&yylval = p->name;
				return (p->token);
			}
			state = CMD;
			break;

		case OSTR:
			if (cbuf[cpos] == '\n') {
				state = CMD;
				return (CRLF);
			}
			/* FALLTHROUGH */

		case STR1:
		case ZSTR1:
		dostr1:
			if (cbuf[cpos] == ' ') {
				cpos++;
				state = state == OSTR ? STR2 : ++state;
				return (SP);
			}
			break;

		case ZSTR2:
			if (cbuf[cpos] == '\n') {
				state = CMD;
				return (CRLF);
			}
			/* FALLTHROUGH */

		case STR2:
			cp = &cbuf[cpos];
			n = strlen(cp);
			cpos += n - 1;
			/*
			 * Make sure the string is nonempty and \n terminated.
			 */
			if (n > 1 && cbuf[cpos] == '\n') {
				cbuf[cpos] = '\0';
				*(char **)&yylval = copy(cp);
				cbuf[cpos] = '\n';
				state = ARGS;
				return (STRING);
			}
			break;

		case NSTR:
			if (cbuf[cpos] == ' ') {
				cpos++;
				return (SP);
			}
			if (isdigit(cbuf[cpos])) {
				cp = &cbuf[cpos];
				while (isdigit(cbuf[++cpos]))
					;
				c = cbuf[cpos];
				cbuf[cpos] = '\0';
				yylval = atoi(cp);
				cbuf[cpos] = c;
				state = STR1;
				return (NUMBER);
			}
			state = STR1;
			goto dostr1;

		case ARGS:
			if (isdigit(cbuf[cpos])) {
				cp = &cbuf[cpos];
				while (isdigit(cbuf[++cpos]))
					;
				c = cbuf[cpos];
				cbuf[cpos] = '\0';
				yylval = atoi(cp);
				cbuf[cpos] = c;
				return (NUMBER);
			}
			switch (cbuf[cpos++]) {

			case '\n':
				state = CMD;
				return (CRLF);

			case ' ':
				return (SP);

			case ',':
				return (COMMA);

			case 'A':
			case 'a':
				return (A);

			case 'B':
			case 'b':
				return (B);

			case 'C':
			case 'c':
				return (C);

			case 'E':
			case 'e':
				return (E);

			case 'F':
			case 'f':
				return (F);

			case 'I':
			case 'i':
				return (I);

			case 'L':
			case 'l':
				return (L);

			case 'N':
			case 'n':
				return (N);

			case 'P':
			case 'p':
				return (P);

			case 'R':
			case 'r':
				return (R);

			case 'S':
			case 's':
				return (S);

			case 'T':
			case 't':
				return (T);

			}
			break;

		default:
			fatal("Unknown state in scanner.");
		}
		yyerror((char *) 0);
		state = CMD;
		longjmp(errcatch,0);
	}
}

void upper(char *s)
{
	while (*s != '\0') {
		if (islower(*s))
			*s = toupper(*s);
		s++;
	}
}

char *copy(char *s)
{
	char *p;

	p = malloc((unsigned) strlen(s) + 1);
	if (p == NULL)
		fatal("Ran out of memory.");
	strcpy(p, s);
	return (p);
}

static void help(struct tab *ctab, char *s)
{
	struct tab *c;
	int width, NCMDS;
	char *type;

	if (ctab == sitetab)
		type = "SITE ";
	else
		type = "";
	width = 0, NCMDS = 0;
	for (c = ctab; c->name != NULL; c++) {
		int len = strlen(c->name);

		if (len > width)
			width = len;
		NCMDS++;
	}
	width = (width + 8) &~ 7;
	if (s == 0) {
		int i, j, w;
		int columns, lines;

		lreply(214, "The following %scommands are recognized %s.",
		    type, "(* =>'s unimplemented)");
		columns = 76 / width;
		if (columns == 0)
			columns = 1;
		lines = (NCMDS + columns - 1) / columns;
		for (i = 0; i < lines; i++) {
			printf("   ");
			for (j = 0; j < columns; j++) {
				c = ctab + j * lines + i;
				printf("%s%c", c->name,
					c->implemented ? ' ' : '*');
				if (c + lines >= &ctab[NCMDS])
					break;
				w = strlen(c->name) + 1;
				while (w < width) {
					putchar(' ');
					w++;
				}
			}
			printf("\r\n");
		}
		fflush(stdout);
		reply(214, "Direct comments to ftp-bugs@%s.", hostname);
		return;
	}
	upper(s);
	c = lookup(ctab, s);
	if (c == (struct tab *)0) {
		reply(502, "Unknown command %s.", s);
		return;
	}
	if (c->implemented)
		reply(214, "Syntax: %s%s %s", type, c->name, c->help);
	else
		reply(214, "%s%-*s\t%s; unimplemented.", type, width,
		    c->name, c->help);
}

static void sizecmd(char *filename)
{
	switch (type) {
	case TYPE_L:
	case TYPE_I: {
		struct stat stbuf;
		if (stat(filename, &stbuf) < 0 ||
		    (stbuf.st_mode&S_IFMT) != S_IFREG)
			reply(550, "%s: not a plain file.", filename);
		else
			reply(213, "%lu", stbuf.st_size);
		break;}
	case TYPE_A: {
		FILE *fin;
		int c;
		long count;
		struct stat stbuf;
		fin = fopen(filename, "r");
		if (fin == NULL) {
			perror_reply(550, filename);
			return;
		}
		if (fstat(fileno(fin), &stbuf) < 0 ||
		    (stbuf.st_mode&S_IFMT) != S_IFREG) {
			reply(550, "%s: not a plain file.", filename);
			fclose(fin);
			return;
		}

		count = 0;
		while((c=getc(fin)) != EOF) {
			if (c == '\n')	/* will get expanded to \r\n */
				count++;
			count++;
		}
		fclose(fin);

		reply(213, "%ld", count);
		break;}
	default:
		reply(504, "SIZE not implemented for Type %c.", "?AEIL"[type]);
	}
}

