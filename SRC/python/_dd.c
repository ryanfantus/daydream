#include <python2.7/Python.h>
#include <dd.h>
#include <ddlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

static struct dif *d;
static struct DayDream_MainConfig mcfg;

static PyObject * initdoor(PyObject *self, PyObject *args)
{
	char *node;
	int sts=0;
	int fd;
	char buf[512];
	if (!PyArg_ParseTuple(args, "s", &node))
	  return NULL;
	sprintf(buf,"%s/data/daydream.dat",getenv("DAYDREAM"));
	fd=open(buf,O_RDONLY);
	if (fd<0) return 0;
	read(fd,&mcfg,sizeof(struct DayDream_MainConfig));
	close(fd);
	d=dd_initdoor(node);
	if (d) sts=1;
	
	return Py_BuildValue("i", sts);
}

static PyObject * sendstring(PyObject *self, PyObject *args)
{
	char *s;
	
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	dd_sendstring(d,s);
	return Py_BuildValue("");

}

static PyObject * sendstring_noparse(PyObject *self, PyObject *args)
{
	char *s;
	
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	dd_sendstring_noparse(d,s);
	return Py_BuildValue("");
}

static PyObject * sendfmt(PyObject *self, PyObject *args)
{
	char *s;
	
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	return Py_BuildValue("i", dd_sendfmt(d, "%s", s));
}

static PyObject * hotkey(PyObject *self, PyObject *args)
{
	int fl;
	int foo;

	if (!PyArg_ParseTuple(args, "i", &fl))
	  return NULL;
	foo=dd_hotkey(d,fl);
	if ((fl & HOT_YESNO) || (fl & HOT_NOYES)) {
		return Py_BuildValue("i",foo);
	} 
	return Py_BuildValue("c",foo);

}

static PyObject * prompt(PyObject *self, PyObject *args)
{
	char s[1024];
	char *t;
	int i;
	int fl;
	if (!PyArg_ParseTuple(args, "sii", &t,&i,&fl))
	  return NULL;
	strncpy(s,t,1024);
	if (dd_prompt(d,s,i,fl)) return Py_BuildValue("s",s);
	return 0;
}

static PyObject * closedoor(PyObject *self, PyObject *args)
{
	dd_close(d);
	return Py_BuildValue("");
}

static PyObject * typefile(PyObject *self, PyObject *args)
{
	char *s;
	int fl;
	if (!PyArg_ParseTuple(args, "si", &s,&fl))
	  return NULL;

	return Py_BuildValue("i",dd_typefile(d,s,fl));
}

void confstostr(int confss, char *str)
{
        int i;
        for (i=0; i!=32; i++)
        {
                if (confss & (1L<<i))
                        *str++='X';
                else
                        *str++='_';
        }
        *str=0;
}

int strtoconfs(char *str)
{
        int i;
        int confd=0;
        for (i=0;i!=32;i++)
        {
                if (*str++=='X') confd |= (1L<<i);
        }
        return confd;
}

static PyObject * getvar(PyObject *self, PyObject *args)
{
	int i;
	char buf[512];
	
	if (!PyArg_ParseTuple(args, "i", &i))
	  return NULL;
	if ( (i>99 && i<109) || (i>128 && i < 131)) {
		dd_getstrlval(d, buf, sizeof buf, i);
		return Py_BuildValue("s",buf);
	} else if (i == 109 || (i > 112 && i < 124) || (i > 125 && i < 129)
		   || ( i == 131) || (i == 135) || (i > 137 && i < 144)) {
		return Py_BuildValue("i",dd_getintval(d,i));
	} else if (i==132 || i==133) {
		return Py_BuildValue("l",dd_getlintval(d,i));
	} else if (i==124 || i==125) {
		confstostr(dd_getintval(d,i),buf);
		return Py_BuildValue("s",buf);
	} else if (i==USER_PROTOCOL) {
		return Py_BuildValue("c",dd_getintval(d,i));
	} else if (i==136 || i==134) {
		time_t fooh;
		fooh=dd_getintval(d,i);
		return Py_BuildValue("s",ctime(&fooh));
	} else if (i>99999 && i <100100 ) {
		struct DayDream_Conference *co;
		co=dd_getconf(dd_getintval(d,VCONF_NUMBER));
		if (!co) return 0;
		switch (i) {
		case 100000:
			return Py_BuildValue("s",co->CONF_NAME);
			break;
		case 100001:
			return Py_BuildValue("s",co->CONF_PATH);
			break;
		case 100002:
			return Py_BuildValue("i",co->CONF_FILEAREAS);
			break;
		case 100003:
			return Py_BuildValue("i",co->CONF_UPLOADAREA);
			break;
		case 100004:
			return Py_BuildValue("i",co->CONF_MSGBASES);
			break;
		case 100005:
			return Py_BuildValue("i",co->CONF_COMMENTAREA);
			break;
		case 100006:
			return Py_BuildValue("s",co->CONF_ULPATH);
			break;
		case 100007:
			return Py_BuildValue("s",co->CONF_NEWSCANAREAS);
			break;
		case 100008:
			return Py_BuildValue("s",co->CONF_PASSWD);
			break;
		default:
			return 0;
		}
	} else if (i > 100099 && i < 100200) {
		struct DayDream_MsgBase *ba;
		ba=dd_getbase(dd_getintval(d,VCONF_NUMBER),dd_getintval(d,SYS_MSGBASE));
		if (!ba) return 0;
		switch (i) {
		case 100100:
			return Py_BuildValue("i",ba->MSGBASE_MSGLIMIT);
			break;
		case 100101:
			return Py_BuildValue("s",ba->MSGBASE_NAME);
			break;
		case 100102:
			return Py_BuildValue("s",ba->MSGBASE_FN_TAG);
			break;
		case 100103:
			return Py_BuildValue("s",ba->MSGBASE_FN_ORIGIN);
			break;
		case 100104:
			sprintf(buf,"%d:%d/%d.%d",ba->MSGBASE_FN_ZONE,
				ba->MSGBASE_FN_NET, ba->MSGBASE_FN_NODE,
				ba->MSGBASE_FN_POINT);
			return Py_BuildValue("s",buf);
			break;
		default:
			return 0;

		}

	} else return 0;
}

static PyObject * setvar(PyObject *self, PyObject *args)
{
	int i,k;

	uint64_t j;
	char *s;

	if (!PyArg_ParseTuple(args, "is", &i,&s))
	  return NULL;
	if ( (i>99 && i<109) || (i>128 && i < 131)) {
		dd_setstrval(d,s,i);
	} else if (i == 109 || (i > 112 && i < 124) || (i > 125 && i < 129)
		   || (i == 135) || (i > 137 && i < 142)) {
		dd_setintval(d,i,atoi(s));
	} else if (i==132 || i==133) {
		sscanf(s,"%Lu",&j);
		dd_setlintval(d,i,j);
	} else if (i==124 || i==125) {
		k=strtoconfs(s);
		dd_setintval(d,i,k);
	} else if (i==USER_PROTOCOL) {
		dd_setintval(d,i,*s);
	} else {
		return 0;
	}
	return Py_BuildValue("");
		

}

static PyObject * flagsingle(PyObject *self, PyObject *args)
{
	int i;
	char *s;
	if (!PyArg_ParseTuple(args, "si", &s,&i))
	  return NULL;
	
	return Py_BuildValue("i",dd_flagsingle(d,s,i));

}

static PyObject * finduser(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	return Py_BuildValue("i",dd_findusername(d,s));
}

static PyObject * dsystem(PyObject *self, PyObject *args)
{
	char *s;
	int i;
	if (!PyArg_ParseTuple(args, "si", &s,&i))
	  return NULL;
	return Py_BuildValue("i",dd_system(d,s,i));
}

static PyObject * cmd(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	return Py_BuildValue("i",dd_docmd(d,s));
}

static PyObject * writelog(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	dd_writelog(d,s);
	return Py_BuildValue("");
} 

static PyObject * changestatus(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	dd_changestatus(d,s);
	return Py_BuildValue("");
}

static PyObject * dpause(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
	  return NULL;
	dd_pause(d);
	return Py_BuildValue("");
}

static PyObject * joinconf(PyObject *self, PyObject *args)
{
	int co, fl;
	if (!PyArg_ParseTuple(args, "ii",&co,&fl))
	  return NULL;
	return Py_BuildValue("i",dd_joinconf(d,co,fl));
}

static PyObject * isfreedl(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s",&s))
	  return NULL;
	return Py_BuildValue("i",dd_isfreedl(d,s));
}

static PyObject * flagfile(PyObject *self, PyObject *args)
{
	char *s;
	int fl;
	if (!PyArg_ParseTuple(args, "s",&s,&fl))
	  return NULL;
	return Py_BuildValue("i",dd_flagfile(d,s,fl));
}

static PyObject * isconfaccess(PyObject *self, PyObject *args)
{
	int i;
	if (!PyArg_ParseTuple(args, "i",&i))
	  return NULL;
	return Py_BuildValue("i",dd_isconfaccess(d,i));
}

static PyObject * isanybasestagged(PyObject *self, PyObject *args)
{
	int i;
	if (!PyArg_ParseTuple(args, "i",&i))
	  return NULL;
	return Py_BuildValue("i",dd_isanybasestagged(d,i));
}
static PyObject * isconftagged(PyObject *self, PyObject *args)
{
	int i;
	if (!PyArg_ParseTuple(args, "i",&i))
	  return NULL;
	return Py_BuildValue("i",dd_isconftagged(d,i));
}

static PyObject * isbasetagged(PyObject *self, PyObject *args)
{
	int i, j;
	if (!PyArg_ParseTuple(args, "i",&i,&j))
	  return NULL;
	return Py_BuildValue("i",dd_isbasetagged(d,i,j));
}

static PyObject * changemsgbase(PyObject *self, PyObject *args)
{
	int i, j;
	if (!PyArg_ParseTuple(args, "i",&i,&j))
	  return NULL;
	return Py_BuildValue("i",dd_changemsgbase(d,i,j));
}

static PyObject * sendfiles(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s",&s))
	  return NULL;
	dd_sendfiles(d,s);
	return Py_BuildValue("");
}

static PyObject * getfiles(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s",&s))
	  return NULL;
	dd_sendfiles(d,s);
	return Py_BuildValue("");
}

static PyObject * unflagfiles(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s",&s))
	  return NULL;
	
	return Py_BuildValue("i",dd_unflagfile(d,s));
}

static PyObject * findfilestolist(PyObject *self, PyObject *args)
{
	char *s, *t;
	if (!PyArg_ParseTuple(args, "ss",&s,&t))
	  return NULL;
	
	return Py_BuildValue("i",dd_findfilestolist(d,s,t));
}

static PyObject * dumpfilestofile(PyObject *self, PyObject *args)
{
	char *s;
	char tmpf[256];
	int i;
	FILE* fhr;
	FILE* fhw;
	struct FFlag f;
	
	if (!PyArg_ParseTuple(args, "s",&s))
	  return NULL;
	
	sprintf(tmpf, "%s.tmp", s);
	
	i = dd_dumpfilestofile(d,tmpf);
	
	fhr = fopen(tmpf, "r");
	if(fhr) {
		fhw = fopen(s, "w");
		while(fread(&f, sizeof(struct FFlag), 1, fhr) == 1) {
			fprintf(fhw, "%s\t%s\n", f.f_filename, f.f_path);
		}
		fclose(fhr);
		fclose(fhw);
	}
	
	unlink(tmpf);
	
	return Py_BuildValue("i", i);
}

static PyObject * getlprs(PyObject *self, PyObject *args)
{
	struct DayDream_LRP lrp;
	
	if (!PyArg_ParseTuple(args, ""))
	  return NULL;
	dd_getlprs(d, &lrp);
	return Py_BuildValue("(ii)", lrp.lrp_read, lrp.lrp_scan);
}

static PyObject * setlprs(PyObject *self, PyObject *args)
{
	struct DayDream_LRP lrp;
	
	if (!PyArg_ParseTuple(args, "ii", &lrp.lrp_read, &lrp.lrp_scan))
	  return NULL;
	dd_setlprs(d, &lrp);
	return Py_BuildValue("");
}

static PyObject * getmprs(PyObject *self, PyObject *args)
{
	struct DayDream_MsgPointers mptrs;
	
	if (!PyArg_ParseTuple(args, ""))
	  return NULL;
	dd_getmprs(d, &mptrs);
	return Py_BuildValue("(ii)", mptrs.msp_low, mptrs.msp_high);
}

static PyObject * setmprs(PyObject *self, PyObject *args)
{
	struct DayDream_MsgPointers mptrs;
	
	if (!PyArg_ParseTuple(args, "ii", &mptrs.msp_low, &mptrs.msp_high))
	  return NULL;
	dd_setmprs(d, &mptrs);
	return Py_BuildValue("");
}

static PyObject * getfidounique(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
	  return NULL;
	return Py_BuildValue("i", dd_getfidounique());
}

static PyObject * fileattach(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
	  return NULL;
	return Py_BuildValue("i", dd_fileattach(d));
}

static PyObject * isfiletagged(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	return Py_BuildValue("i", dd_isfiletagged(d, s));
}

static PyObject * outputmask(PyObject *self, PyObject *args)
{
	int mask;
	if (!PyArg_ParseTuple(args, "i", &mask))
	  return NULL;
	dd_outputmask(d, mask);
	return Py_BuildValue("");
}

static PyObject * ansi_fg(PyObject *self, PyObject *args)
{
	char buf[32];
	int color;
	if (!PyArg_ParseTuple(args, "i", &color))
	  return NULL;
	dd_ansi_fg(buf, color);
	return Py_BuildValue("s", buf);
}

static PyObject * ansi_bg(PyObject *self, PyObject *args)
{
	char buf[32];
	int color;
	if (!PyArg_ParseTuple(args, "i", &color))
	  return NULL;
	dd_ansi_bg(buf, color);
	return Py_BuildValue("s", buf);
}

static PyObject * parsepipes(PyObject *self, PyObject *args)
{
	char *s;
	char buf[4096];
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	strncpy(buf, s, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	dd_parsepipes(buf);
	return Py_BuildValue("s", buf);
}

static PyObject * strippipes(PyObject *self, PyObject *args)
{
	char *s;
	char buf[4096];
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	strncpy(buf, s, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	dd_strippipes(buf);
	return Py_BuildValue("s", buf);
}

static PyObject * stripansi(PyObject *self, PyObject *args)
{
	char *s;
	char buf[4096];
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	strncpy(buf, s, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	dd_stripansi(buf);
	return Py_BuildValue("s", buf);
}

static PyObject * ansipos(PyObject *self, PyObject *args)
{
	int x, y;
	if (!PyArg_ParseTuple(args, "ii", &x, &y))
	  return NULL;
	dd_ansipos(d, x, y);
	return Py_BuildValue("");
}

static PyObject * clrscr(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
	  return NULL;
	dd_clrscr(d);
	return Py_BuildValue("");
}

static PyObject * center(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	dd_center(d, s);
	return Py_BuildValue("");
}

static PyObject * strlenansi(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	return Py_BuildValue("i", dd_strlenansi(s));
}

static PyObject * stripcrlf(PyObject *self, PyObject *args)
{
	char *s;
	char buf[4096];
	if (!PyArg_ParseTuple(args, "s", &s))
	  return NULL;
	strncpy(buf, s, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	dd_stripcrlf(buf);
	return Py_BuildValue("s", buf);
}

static PyObject * cursoron(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
	  return NULL;
	dd_cursoron(d);
	return Py_BuildValue("");
}

static PyObject * cursoroff(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
	  return NULL;
	dd_cursoroff(d);
	return Py_BuildValue("");
}

static PyObject * getconfdata(PyObject *self, PyObject *args)
{
	struct DayDream_Conference *conf;
	if (!PyArg_ParseTuple(args, ""))
	  return NULL;
	conf = dd_getconfdata();
	if (!conf)
	  return Py_BuildValue("");
	return Py_BuildValue("i", 1);  // Just return success for now
}

// Message library functions
static PyObject * py_ddmsg_open_base(PyObject *self, PyObject *args)
{
	char *conf;
	int msgbase_num, flags, mode;
	if (!PyArg_ParseTuple(args, "siii", &conf, &msgbase_num, &flags, &mode))
	  return NULL;
	return Py_BuildValue("i", ddmsg_open_base(conf, msgbase_num, flags, mode));
}

static PyObject * py_ddmsg_close_base(PyObject *self, PyObject *args)
{
	int fd;
	if (!PyArg_ParseTuple(args, "i", &fd))
	  return NULL;
	return Py_BuildValue("i", ddmsg_close_base(fd));
}

static PyObject * ddmsg_setptrs_py(PyObject *self, PyObject *args)
{
	char *conf;
	int msgbase_num;
	struct DayDream_MsgPointers ptrs;
	if (!PyArg_ParseTuple(args, "siii", &conf, &msgbase_num, &ptrs.msp_low, &ptrs.msp_high))
	  return NULL;
	return Py_BuildValue("i", ddmsg_setptrs(conf, msgbase_num, &ptrs));
}

static PyObject * ddmsg_getptrs_py(PyObject *self, PyObject *args)
{
	char *conf;
	int msgbase_num;
	struct DayDream_MsgPointers ptrs;
	if (!PyArg_ParseTuple(args, "si", &conf, &msgbase_num))
	  return NULL;
	if (ddmsg_getptrs(conf, msgbase_num, &ptrs) < 0)
	  return Py_BuildValue("");
	return Py_BuildValue("(ii)", ptrs.msp_low, ptrs.msp_high);
}

static PyObject * ddmsg_getfidounique_py(PyObject *self, PyObject *args)
{
	char *origdir;
	if (!PyArg_ParseTuple(args, "s", &origdir))
	  return NULL;
	return Py_BuildValue("i", ddmsg_getfidounique(origdir));
}

static PyObject * py_ddmsg_open_msg(PyObject *self, PyObject *args)
{
	char *conf;
	int msgbase_num, msgnum, flags, mode;
	if (!PyArg_ParseTuple(args, "siiii", &conf, &msgbase_num, &msgnum, &flags, &mode))
	  return NULL;
	return Py_BuildValue("i", ddmsg_open_msg(conf, msgbase_num, msgnum, flags, mode));
}

static PyObject * py_ddmsg_close_msg(PyObject *self, PyObject *args)
{
	int fd;
	if (!PyArg_ParseTuple(args, "i", &fd))
	  return NULL;
	return Py_BuildValue("i", ddmsg_close_msg(fd));
}

static PyObject * py_ddmsg_delete_msg(PyObject *self, PyObject *args)
{
	char *conf;
	int msgbase_num, msgnum;
	if (!PyArg_ParseTuple(args, "sii", &conf, &msgbase_num, &msgnum))
	  return NULL;
	return Py_BuildValue("i", ddmsg_delete_msg(conf, msgbase_num, msgnum));
}

static PyObject * py_ddmsg_rename_msg(PyObject *self, PyObject *args)
{
	char *conf;
	int msgbase_num, old_num, new_num;
	if (!PyArg_ParseTuple(args, "siii", &conf, &msgbase_num, &old_num, &new_num))
	  return NULL;
	return Py_BuildValue("i", ddmsg_rename_msg(conf, msgbase_num, old_num, new_num));
}

static PyObject * new(PyObject *self, PyObject *args)
{
	int i;
	if (!PyArg_ParseTuple(args, "i", &i))
	  return NULL;
	return Py_BuildValue("i",i);

}
static PyMethodDef DDMethods[] = {
	{"initdoor",  initdoor, 1},
	{"print", sendstring, 1},
	{"sendstring", sendstring, 1},
	{"sendstring_noparse", sendstring_noparse, 1},
	{"sendfmt", sendfmt, 1},
	{"closedoor", closedoor ,1},
	{"prompt", prompt, 1},
	{"hotkey", hotkey, 1},
	{"typefile", typefile, 1},
	{"getvar",getvar,1},
	{"setvar",setvar,1},
	{"flagsingle",flagsingle,1},
	{"finduser",finduser,1},
	{"system",dsystem,1},
	{"cmd",cmd,1},
	{"writelog",writelog,1},
	{"changestatus",changestatus,1},
	{"pause",dpause,1},
	{"joinconf",joinconf,1},
	{"isfreedl",isfreedl,1},
	{"flagfile",flagfile,1},
	{"isconfaccess",isconfaccess,1},
	{"isanybasestagged",isanybasestagged,1},
	{"isconftagged",isconftagged,1},
	{"isbasetagged",isbasetagged,1},
	{"changemsgbase",changemsgbase,1},
	{"sendfiles",sendfiles,1},
	{"getfiles",getfiles,1},
	{"unflagfiles",unflagfiles,1},
	{"findfilestolist",findfilestolist,1},
	{"dumpfilestolist",dumpfilestofile,1},
	{"getlprs", getlprs, 1},
	{"setlprs", setlprs, 1},
	{"getmprs", getmprs, 1},
	{"setmprs", setmprs, 1},
	{"getfidounique", getfidounique, 1},
	{"fileattach", fileattach, 1},
	{"isfiletagged", isfiletagged, 1},
	{"outputmask", outputmask, 1},
	{"ansi_fg", ansi_fg, 1},
	{"ansi_bg", ansi_bg, 1},
	{"parsepipes", parsepipes, 1},
	{"strippipes", strippipes, 1},
	{"stripansi", stripansi, 1},
	{"ansipos", ansipos, 1},
	{"clrscr", clrscr, 1},
	{"center", center, 1},
	{"strlenansi", strlenansi, 1},
	{"stripcrlf", stripcrlf, 1},
	{"cursoron", cursoron, 1},
	{"cursoroff", cursoroff, 1},
	{"getconfdata", getconfdata, 1},
	{"ddmsg_open_base", py_ddmsg_open_base, 1},
	{"ddmsg_close_base", py_ddmsg_close_base, 1},
	{"ddmsg_setptrs", ddmsg_setptrs_py, 1},
	{"ddmsg_getptrs", ddmsg_getptrs_py, 1},
	{"ddmsg_getfidounique", ddmsg_getfidounique_py, 1},
	{"ddmsg_open_msg", py_ddmsg_open_msg, 1},
	{"ddmsg_close_msg", py_ddmsg_close_msg, 1},
	{"ddmsg_delete_msg", py_ddmsg_delete_msg, 1},
	{"ddmsg_rename_msg", py_ddmsg_rename_msg, 1},
	{NULL,      NULL}        /* Sentinel */
};

PyMODINIT_FUNC initddp(void)
{
	PyObject* m;

	m = Py_InitModule("ddp", DDMethods);
	
	//return m;
}


/*
#define BBS_NAME 100
#define BBS_SYSOP 101
#define USER_REALNAME 102
#define USER_HANDLE 103
#define USER_ORGANIZATION 104
#define USER_ZIPCITY 105
#define USER_VOICEPHONE 106
#define USER_COMPUTERMODEL 107
#define USER_SIGNATURE 108
#define USER_SCREENLENGTH 109
#define USER_TOGGLES 110
#define USER_ULFILES 113
#define USER_DLFILES 114
#define USER_PUBMESSAGES 115
#define USER_PVTMESSAGES 116
#define USER_CONNECTIONS 117
#define USER_FILERATIO 118
#define USER_BYTERATIO 119
#define USER_FREEDLBYTES 120
#define USER_FREEDLFILES 121
#define USER_SECURITYLEVEL 122
#define USER_JOINCONFERENCE 123
#define USER_CONFERENCEACC1 124
#define USER_CONFERENCEACC2 125
#define USER_DAILYTIMELIMIT 126
#define USER_ACCOUNT_ID 127
#define USER_TIMELEFT 128
#define DOOR_PARAMS 129
#define DD_ORIGDIR 130
#define VCONF_NUMBER 131
#define USER_ULBYTES 132
#define USER_DLBYTES 133
#define USER_FIRSTCALL 134
#define USER_FLINES 135
#define USER_LASTCALL 136
#define USER_PROTOCOL 137
#define USER_FAKEDFILES 138
#define USER_FAKEDBYTES 139
#define SYS_FLAGGEDFILES 140
#define SYS_FLAGGEDBYTES 141
#define SYS_FLAGERROR 142
*/
