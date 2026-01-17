#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include <daydream.h>
#include <ddcommon.h>

// External function declarations
extern void dd_strippipes(char *);

// Function declarations for word wrapping
static char *read_entire_message(FILE *msgfd, long *total_size);
static char **split_into_lines(const char *buffer, int *line_count);
static char *wrap_lines(char **lines, int line_count, int wrap_length);
static void free_line_array(char **lines, int line_count);

int replymessage(struct DayDream_Message *msgd)
{
	char qbuffer[4096];
	char input[2048];
	char msgin[10];
	FILE *msgfd;
	FILE *quotefd;
	char *s, *t;
	int i;
	int hola;
	int l = 0;
	int fd;

	struct DayDream_Message header;

	s = (char *) &header;
	for (hola = 0; hola < sizeof(struct DayDream_Message); hola++) {
		*s++ = 0;
	}

	fd = ddmsg_open_msg(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, msgd->MSG_NUMBER, O_RDONLY, 0);

	if(fd == -1)
		return 0;

	msgfd = fdopen(fd, "r");

	// Save original message content for re-quoting in dded
	{
		char orig_filename[256];
		FILE *orig_fd;
		char line_buffer[1024];
		
		snprintf(orig_filename, sizeof(orig_filename), "%s/daydream%d.full.msg", DDTMP, node);
		unlink(orig_filename);
		
		if ((orig_fd = fopen(orig_filename, "w"))) {
			while (fgets(line_buffer, sizeof(line_buffer), msgfd)) {
				fputs(line_buffer, orig_fd);
			}
			fclose(orig_fd);
			fseek(msgfd, 0, SEEK_SET); // Reset file pointer for normal processing
		}
	}

	snprintf(qbuffer, sizeof qbuffer, "%s/daydream%d.mtm", DDTMP, node);
	unlink(qbuffer);

	if (!(quotefd = fopen(qbuffer, "w"))) {
		fclose(msgfd);
		return 0;
	}
	if (current_msgbase->MSGBASE_FLAGS & (1L << 3)) {
		if (current_msgbase->MSGBASE_FLAGS & (1L << 4)) {
			s = msgd->MSG_AUTHOR;
			t = msgin;
			*t++ = *s++;
			while (*s) {
				if (*s == ' ' && *(s + 1)) {
					s++;
					*t++ = *s;
				}
				s++;
			}
			*t++ = '>';
			*t = 0;
		} else {
			msgin[0] = '>';
			msgin[1] = 0;
		}
	} else {
		msgin[0] = 0;
	}


	// New 4-step process with smart word wrapping (similar to readmsgs.c)
	{
		long msg_size;
		char *message_buffer;
		char **lines;
		int line_count;
		char *wrapped_message;
		int screenl = user.user_screenlength - 1;
		
		// Step 1: Read entire message into dynamically allocated buffer
		message_buffer = read_entire_message(msgfd, &msg_size);
		if (!message_buffer) {
			fprintf(quotefd, "%s Error reading message.\n", msgin);
			goto cleanup_reply;
		}
		
		// Step 2: Split buffer into array of lines
		lines = split_into_lines(message_buffer, &line_count);
		if (!lines) {
			free(message_buffer);
			fprintf(quotefd, "%s Error processing message.\n", msgin);
			goto cleanup_reply;
		}
		
		// Step 3: Wrap the lines with quote prefix (wrap 2 chars before screen edge)
		wrapped_message = wrap_lines(lines, line_count, 73 - strlen(msgin));
		if (!wrapped_message) {
			free_line_array(lines, line_count);
			free(message_buffer);
			fprintf(quotefd, "%s Error wrapping message.\n", msgin);
			goto cleanup_reply;
		}
		
		// Step 4: Write wrapped message to quote file with prefixes
		char *line_start = wrapped_message;
		char *line_end;
		
		while ((line_end = strchr(line_start, '\n')) != NULL) {
			*line_end = '\0';
			// Strip ANSI and pipes, then add quote prefix
			stripansi(line_start);
			dd_strippipes(line_start);
			fprintf(quotefd, "%s %s\n", msgin, line_start);
			*line_end = '\n';
			line_start = line_end + 1;
		}
		// Handle any remaining text after last newline
		if (*line_start) {
			stripansi(line_start);
			dd_strippipes(line_start);
			fprintf(quotefd, "%s %s\n", msgin, line_start);
		}
		
		// Cleanup
		free(wrapped_message);
		free_line_array(lines, line_count);
		free(message_buffer);
		
		cleanup_reply:;
	}

	if (!msgin[0]) {
		snprintf(qbuffer, sizeof qbuffer, "---[ %s ]", 
			msgd->MSG_AUTHOR);

		for (i = strlen(qbuffer); i < 75; i++) {
			strlcat(qbuffer, "-", sizeof qbuffer);
		}
		strlcat(qbuffer, "\n\n", sizeof qbuffer);
		fputs(qbuffer, quotefd);
	}
	fclose(msgfd);
	ddmsg_close_msg(fd);
	fclose(quotefd);

	strlcpy(header.MSG_RECEIVER, msgd->MSG_AUTHOR, sizeof header.MSG_RECEIVER);
	strlcpy(header.MSG_SUBJECT, msgd->MSG_SUBJECT, sizeof header.MSG_SUBJECT);
	header.MSG_ORIGINAL = msgd->MSG_NUMBER;
	if (msgd->MSG_FLAGS & (1L << 0))
		header.MSG_FLAGS |= (1L << 0);
	if (toupper(current_msgbase->MSGBASE_FN_FLAGS) == 'N') {
		if (msgd->MSG_FN_ORIG_ZONE) {
			header.MSG_FN_DEST_ZONE = msgd->MSG_FN_ORIG_ZONE;
		} else {
			header.MSG_FN_DEST_ZONE = msgd->MSG_FN_PACKET_ORIG_ZONE;
		}
		header.MSG_FN_DEST_NET = msgd->MSG_FN_ORIG_NET;
		header.MSG_FN_DEST_NODE = msgd->MSG_FN_ORIG_NODE;
		header.MSG_FN_DEST_POINT = msgd->MSG_FN_ORIG_POINT;

	}
	i = entermsg(&header, 1, 0);
	getmsgptrs();
	return i;
}

/* FIXME! better to move this to entermsg.c */
int askqlines(void)
{
	char qbuffer[200];
	char input[100];
	int ql;
	int lcount;
	int outp;
	int startn;
	int endn;
	int line;
	FILE *qfd, *msgfd;

	snprintf(qbuffer, sizeof qbuffer, "%s/daydream%d.mtm", DDTMP, node);
	if (!(qfd = fopen(qbuffer, "r")))
		return 0;

	DDPut("\n");

	for (;;) {
		ql = 0;
		lcount = user.user_screenlength - 1;
		outp = 1;

		while (fgets(input, 78, qfd)) {
			ql++;
			if (outp) {
				ddprintf(sd[lel2str], ql, input);
				lcount--;
			}
			if (lcount == 0) {
				int hot;

				DDPut(sd[morepromptstr]);
				hot = HotKey(0);
				DDPut("\r                                                         \r");
				if (hot == 'N' || hot == 'n') {
					outp = 0;
					lcount = -1;
				} else if (hot == 'C' || hot == 'c') {
					lcount = -1;
				} else {
					lcount = user.user_screenlength - 1;
				}
			}
		}
		ddprintf(sd[lequotestr], ql);
		qbuffer[0] = 0;
		if (!(Prompt(qbuffer, 3, PROMPT_NOCRLF))) {
			fclose(qfd);
			return 0;
		}
		if (!strcasecmp(qbuffer, "l")) {
			DDPut("\n\n");
			fseek(qfd, 0, SEEK_SET);
		} else if ((!strcasecmp(qbuffer, "*")) || *qbuffer == 0) {
			startn = 1;
			endn = ql;
			break;
		} else if ((startn = atoi(qbuffer))) {
			DDPut(sd[ledeltostr]);
			qbuffer[0] = 0;
			if (!(Prompt(qbuffer, 3, PROMPT_NOCRLF))) {
				fclose(qfd);
				return 0;
			}
			if ((endn = atoi(qbuffer))) {
				break;
			} else {
				fclose(qfd);
				return 0;
			}
		} else {
			fclose(qfd);
			return 0;
		}

	}

	fseek(qfd, 0, SEEK_SET);
	if (startn < 1 || endn > ql) {
		fclose(qfd);
		return 0;
	}
	line = 1;

	snprintf(qbuffer, sizeof qbuffer, "%s/daydream%d.msg", DDTMP, node);
	if (!(msgfd = fopen(qbuffer, "w"))) {
		fclose(qfd);
		return 0;
	}

	while (fgets(input, 78, qfd)) {
		if (startn <= line && endn >= line) {
			fputs(input, msgfd);
		}
		line++;
	}
	fclose(qfd);
	fclose(msgfd);
	return 1;
}

int getreplyid(int msg, char *de, size_t delen)
{
	FILE *msgf;
	char buf[1024];
	int fd;
	int found = 0;

	fd = ddmsg_open_msg(conference()->conf.CONF_PATH, current_msgbase->MSGBASE_NUMBER, msg, O_RDONLY, 0);

	if(fd == -1)
		return 0;

	msgf = fdopen(fd, "r");

	while (fgets(buf, 1024, msgf)) {
		if (!strncmp(buf, "\1MSGID:", 7)) {
			memcpy(buf, "\1REPLY:", 6);
			strlcpy(de, buf, delen);
			found = 1;
			break;
		}
	}
	fclose(msgf);
	ddmsg_close_msg(fd);
	return found;
}

// Step 1: Read entire message into dynamically allocated buffer
static char *read_entire_message(FILE *msgfd, long *total_size) {
    long start_pos = ftell(msgfd);
    char *buffer = NULL;
    long size = 0;
    long capacity = 1024;
    char line_buffer[500];
    
    buffer = (char *) xmalloc(capacity);
    buffer[0] = '\0';
    
    while (fgets(line_buffer, sizeof(line_buffer), msgfd)) {
        long line_len = strlen(line_buffer);
        
        // Check if we need to expand buffer
        if (size + line_len + 1 > capacity) {
            capacity *= 2;
            buffer = (char *) realloc(buffer, capacity);
            if (!buffer) {
                *total_size = 0;
                return NULL;
            }
        }
        
        strcat(buffer, line_buffer);
        size += line_len;
    }
    
    *total_size = size;
    return buffer;
}

// Step 2: Split buffer into array of lines
static char **split_into_lines(const char *buffer, int *line_count) {
    int capacity = 100;
    int count = 0;
    char **lines = (char **) xmalloc(capacity * sizeof(char *));
    const char *start = buffer;
    const char *current = buffer;
    
    while (*current) {
        if (*current == '\n' || *current == '\0') {
            int line_len = current - start;
            
            // Expand array if needed
            if (count >= capacity) {
                capacity *= 2;
                lines = (char **) realloc(lines, capacity * sizeof(char *));
            }
            
            // Allocate and copy line
            lines[count] = (char *) xmalloc(line_len + 1);
            strncpy(lines[count], start, line_len);
            lines[count][line_len] = '\0';
            
            count++;
            start = current + 1;
        }
        current++;
    }
    
    // Handle case where buffer doesn't end with newline
    if (start < current) {
        if (count >= capacity) {
            capacity++;
            lines = (char **) realloc(lines, capacity * sizeof(char *));
        }
        int line_len = current - start;
        lines[count] = (char *) xmalloc(line_len + 1);
        strncpy(lines[count], start, line_len);
        lines[count][line_len] = '\0';
        count++;
    }
    
    *line_count = count;
    return lines;
}

// Step 3: Wrap the lines
static char *wrap_lines(char **lines, int line_count, int wrap_length) {
    long total_capacity = 1024;
    char *result = (char *) xmalloc(total_capacity);
    long result_len = 0;
    result[0] = '\0';
    
    for (int i = 0; i < line_count; i++) {
        char *line = lines[i];
        
        // Skip kludge lines for FidoNet messages
        if (*line == 1 || !strncmp("AREA:", line, 5) || 
            !strncmp("SEEN-BY:", line, 8)) {
            continue;
        }
        
        // Handle @ character replacement
        if (*line == 1) {
            *line = '@';
        }
        
        // Remove trailing newlines and carriage returns
        int line_len = strlen(line);
        while (line_len > 0 && (line[line_len-1] == '\n' || line[line_len-1] == '\r')) {
            line[line_len-1] = '\0';
            line_len--;
        }
        
        // Handle empty lines - preserve as paragraph breaks
        if (line_len == 0) {
            // Ensure we have space for newline
            if (result_len + 2 > total_capacity) {
                total_capacity *= 2;
                result = (char *) realloc(result, total_capacity);
            }
            strcat(result, "\n");
            result_len++;
            continue;
        }
        
        // Apply word wrapping to this line
        int pos = 0;
        int current_line_len = 0;
        
        // If this isn't the first line of output, we need to check current line length
        if (result_len > 0 && result[result_len-1] != '\n') {
            // Find the length of the current line
            int last_newline = result_len - 1;
            while (last_newline >= 0 && result[last_newline] != '\n') {
                last_newline--;
            }
            current_line_len = result_len - last_newline - 1;
        }
        
        while (pos < line_len) {
            // Find the next word
            int word_start = pos;
            while (pos < line_len && line[pos] != ' ') {
                pos++;
            }
            int word_len = pos - word_start;
            
            // Check if adding this word would exceed wrap length
            int space_needed = (current_line_len > 0) ? 1 : 0; // space before word
            if (current_line_len + space_needed + word_len > wrap_length && current_line_len > 0) {
                // Need to wrap - add newline
                if (result_len + 2 > total_capacity) {
                    total_capacity *= 2;
                    result = (char *) realloc(result, total_capacity);
                }
                strcat(result, "\n");
                result_len++;
                current_line_len = 0;
                space_needed = 0; // no space needed at start of new line
            }
            
            // Add space if needed
            if (space_needed > 0) {
                if (result_len + 2 > total_capacity) {
                    total_capacity *= 2;
                    result = (char *) realloc(result, total_capacity);
                }
                strcat(result, " ");
                result_len++;
                current_line_len++;
            }
            
            // Add the word
            if (result_len + word_len + 1 > total_capacity) {
                total_capacity = (result_len + word_len + 1) * 2;
                result = (char *) realloc(result, total_capacity);
            }
            strncat(result, line + word_start, word_len);
            result_len += word_len;
            current_line_len += word_len;
            
            // Skip spaces after the word
            while (pos < line_len && line[pos] == ' ') {
                pos++;
            }
        }
        
        // Add newline after each original line to preserve paragraph structure
        if (result_len + 2 > total_capacity) {
            total_capacity *= 2;
            result = (char *) realloc(result, total_capacity);
        }
        strcat(result, "\n");
        result_len++;
    }
    
    return result;
}

// Step 4: Free line array
static void free_line_array(char **lines, int line_count) {
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
}
