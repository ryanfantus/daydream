#include <stdlib.h>
#include <string.h>

#include <daydream.h>

static const char *default_command_bindings =
"~#MBpush_menu(\"commands\", \"Main menu\")|~#LF\n"
"~#MBbind_cmd(\"<\"); subst(\"internal\", \"%'prev_conf %S\")|~#LF\n"
"~#MBbind_cmd(\"<<\"); subst(\"internal\", \"%'prev_base %S\")|~#LF\n"
"~#MBbind_cmd(\">\"); subst(\"internal\", \"%'next_conf %S\")|~#LF\n"
"~#MBbind_cmd(\">>\"); subst(\"internal\", \"%'next_base %S\")|~#LF\n"
"~#MBbind_cmd(\"?\"); subst(\"internal\", \"%'help %S\")|~#LF\n"
"~#MBbind_cmd(\"a\"); subst(\"internal\", \"%'change_info %S\")|~#LF\n"
"~#MBbind_cmd(\"b\"); subst(\"internal\", \"%'bulletins %S\")|~#LF\n"
"~#MBbind_cmd(\"c\"); subst(\"internal\", \"%'comment %S\")|~#LF\n"
"~#MBbind_cmd(\"copy\"); subst(\"internal\", \"%'copy %S\")|~#LF\n"
"~#MBbind_cmd(\"d\"); subst(\"internal\", \"%'download %S\")|~#LF\n"
"~#MBbind_cmd(\"e\"); subst(\"internal\", \"%'enter_msg %S\")|~#LF\n"
"~#MBbind_cmd(\"f\"); subst(\"internal\", \"%'file_scan %S\")|~#LF\n"
"~#MBbind_cmd(\"g\"); subst(\"internal\", \"%'logoff %S\")|~#LF\n"
"~#MBbind_cmd(\"gr\"); subst(\"internal\", \"%'global_read %S\")|~#LF\n"
"~#MBbind_cmd(\"j\"); subst(\"internal\", \"%'join_conf %S\")|~#LF\n"
"~#MBbind_cmd(\"l\"); subst(\"internal\", \"%'time %S\")|~#LF\n"
"~#MBbind_cmd(\"link\"); subst(\"internal\", \"%'link %S\")|~#LF\n"
"~#MBbind_cmd(\"lu\"); subst(\"internal\", \"%'local_upload %S\")|~#LF\n"
"~#MBbind_cmd(\"m\"); subst(\"internal\", \"%'change_msgbase %S\")|~#LF\n"
"~#MBbind_cmd(\"mode\"); subst(\"internal\", \"%'mode %S\")|~#LF\n"
"~#MBbind_cmd(\"move\"); subst(\"internal\", \"%'move %S\")|~#LF\n"
"~#MBbind_cmd(\"ms\"); subst(\"internal\", \"%'scan_mail %S\")|~#LF\n"
"~#MBbind_cmd(\"n\"); subst(\"internal\", \"%'new_files %S\")|~#LF\n"
"~#MBbind_cmd(\"ns\"); subst(\"internal\", \"%'global_fscan %S\")|~#LF\n"
"~#MBbind_cmd(\"o\"); subst(\"internal\", \"%'page %S\")|~#LF\n"
"~#MBbind_cmd(\"olm\"); subst(\"internal\", \"%'olm %S\")|~#LF\n"
"~#MBbind_cmd(\"r\"); subst(\"internal\", \"%'read_msgs %S\")|~#LF\n"
"~#MBbind_cmd(\"rz\"); subst(\"internal\", \"%'upload_rz %S\")|~#LF\n"
"~#MBbind_cmd(\"s\"); subst(\"internal\", \"%'stats %S\")|~#LF\n"
"~#MBbind_cmd(\"sd\"); subst(\"internal\", \"%'sysop_download %S\")|~#LF\n"
"~#MBbind_cmd(\"sf\"); subst(\"internal\", \"%'tag_confs %S\")|~#LF\n"
"~#MBbind_cmd(\"sm\"); subst(\"internal\", \"%'tag_msgbases %S\")|~#LF\n"
"~#MBbind_cmd(\"t\"); subst(\"internal\", \"%'tag_editor %S\")|~#LF\n"
"~#MBbind_cmd(\"ts\"); subst(\"internal\", \"%'text_search %S\")|~#LF\n"
"~#MBbind_cmd(\"u\"); subst(\"internal\", \"%'upload %S\")|~#LF\n"
"~#MBbind_cmd(\"usered\"); subst(\"internal\", \"%'usered %S\")|~#LF\n"
"~#MBbind_cmd(\"userlist\"); subst(\"internal\", \"%'userlist %S\")|~#LF\n"
"~#MBbind_cmd(\"v\"); subst(\"internal\", \"%'view_file %S\")|~#LF\n"
"~#MBbind_cmd(\"ver\"); subst(\"internal\", \"%'ver %S\")|~#LF\n"
"~#MBbind_cmd(\"who\"); subst(\"internal\", \"%'who %S\")|~#LF\n"
"~#MBbind_cmd(\"x\"); subst(\"internal\", \"%'expert_mode %S\")|~#LF\n"
"~#MBbind_cmd(\"z\"); subst(\"internal\", \"%'zippy_search %S\")|~#LF\n"
"~#MBbind_cmd(\"$\"); subst(\"test_door\", \"%s %S\")|~#LF\n"
"~#MBbind_cmd; return(2)|~#LF\n"
"~#MBbind_cmd(\"cls\"); print(\"\033[2J\033[H\")|~#LF\n";

int load_default_commands(void)
{
	struct DD_ExternalCommand *ext;
	
	if (!formatted_print(default_command_bindings, 
		strlen(default_command_bindings), 0)) 
		return -1;
	
	for (ext = exts; ext->EXT_NAME[0]; ext++) {
		char buffer[1024];
		
		if (snprintf(buffer, 1024, "~#MBbind_cmd(\"%s\"); subst(\"door\", \"%%'%s %%S\")|~#LF",
			     ext->EXT_NAME, ext->EXT_NAME) == -1) 
			return -1;
			
		if (!formatted_print(buffer, strlen(buffer), 0))
			return -1;
	}
	
	return 0;
}
