/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: text.c
 *
 * Description: text UI for cdebconf
 * Some notes on the implementation - this is meant to be an accessibility-
 * friendly implementation. I've taken care to make the prompts work well
 * with screen readers and the like.
 *
 * $Id: text.c,v 1.66 2004/03/11 22:17:46 barbier Exp $
 *
 * cdebconf is (c) 2000-2001 Randolph Chung and others under the following
 * license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ***********************************************************************/
#include "common.h"
#include "template.h"
#include "question.h"
#include "frontend.h"
#include "database.h"
#include "strutl.h"

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define q_get_extended_description(q)   question_get_field((q), "", "extended_description")
#define q_get_description(q)		question_get_field((q), "", "description")
#define q_get_choices(q)		question_get_field((q), "", "choices")
#define q_get_choices_vals(q)		question_get_field((q), NULL, "choices")
#define q_get_indices(q)		question_get_field((q), "", "indices")

#define MAKE_UPPER(C) do { if (islower((int) C)) { C = (char) toupper((int) C); } } while(0)

#define CHAR_GOBACK '<'
#define CHAR_HELP '?'
#define CHAR_CLEAR '!'

#if defined(__s390__) || defined (__s390x__)
#define ISEMPTY(buf) (buf[0] == 0 || (buf[0] == '.' && buf[1] == 0))
#else
#define ISEMPTY(buf) (buf[0] == 0)
#endif

/*
 * Function: getwidth
 * Input: none
 * Output: int - width of screen
 * Description: get the width of the current terminal
 * Assumptions: doesn't handle resizing; caches value on first call
 */
static const int getwidth(void)
{
	static int res = 80;
	static int inited = 0;
	int fd;
	struct winsize ws;

	if (inited == 0)
	{
		inited = 1;
		if ((fd = open("/dev/tty", O_RDONLY)) > 0)
		{
			if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
				res = ws.ws_col;
			close(fd);
		}
	}
	return res;
}

/*
 * Function: get_text
 * Input: struct frontend *obj - frontend object
 *        const char *template - template name
 *        const char *fallback - string to use if not available
 * Output: const char * - ptr to string, translated if possible
 * Description: get the translated version of a string
 * Assumptions: None.
 */
static const char *
get_text(struct frontend *obj, const char *template, const char *fallback)
{
	struct question *q = obj->qdb->methods.get(obj->qdb, template);
	return q ? q_get_description(q) : fallback;
}

/*
 * Function: wrap_print
 * Input: const char *str - string to display
 * Output: none
 * Description: prints a string to the screen with word wrapping 
 * Assumptions: string fits in <500 lines
 */
static void wrap_print(const char *str)
{
	/* Simple greedy line-wrapper */
	int i, lc;
	char *lines[500];

	lc = strwrap(str, getwidth() - 1, lines, DIM(lines));

	for (i = 0; i < lc; i++)
	{
		printf("%s\n", lines[i]);
		DELETE(lines[i]);
	}
}

/*
 * Function: text_handler_displaydesc
 * Input: struct frontend *obj - UI object
 *        struct question *q - question for which to display the description
 * Output: none
 * Description: displays the description for a given question 
 * Assumptions: none
 */
static void text_handler_displaydesc(struct frontend *obj, struct question *q) 
{
	char *descr = q_get_description(q);
	char *ext_descr = q_get_extended_description(q);
	if (strcmp(q->template->type, "note") == 0 ||
	    strcmp(q->template->type, "error") == 0)
	{
		if (strcmp(q->template->type, "error") == 0)
			printf(get_text(obj, "debconf/text-error", "!! ERROR: %s"), descr);
		else
			printf("%s", descr);
		printf("\n\n");
		if (*ext_descr)
			wrap_print(ext_descr);
	}
	else
	{
		if (*ext_descr)
			wrap_print(ext_descr);
		wrap_print(descr);
	}
	free(descr);
	free(ext_descr);
}

static void
get_answer(char *answer, int size)
{
	fgets(answer, size, stdin);
	CHOMP(answer);
}

static void
show_help (struct frontend *obj, struct question *q)
{
	char *descr = q_get_description(q);
	printf("%s", get_text(obj, "debconf/text-help-keystrokes", "KEYSTROKES:"));
	printf("\n  %c  ", CHAR_HELP);
	printf("%s", get_text(obj, "debconf/text-help-help", "Display this help message"));
	if (obj->methods.can_go_back (obj, q)) {
		printf("\n  %c  ", CHAR_GOBACK);
		printf("%s", get_text(obj, "debconf/text-help-goback", "Go back to previous question"));
	}
	if (strcmp(q->template->type, "string") == 0 ||
	    strcmp(q->template->type, "passwd") == 0 ||
	    strcmp(q->template->type, "multiselect") == 0) {
		printf("\n  %c  ", CHAR_CLEAR);
		printf("%s", get_text(obj, "debconf/text-help-clear", "Select an empty entry"));
	}
	printf("\n");
	wrap_print(descr);
	free(descr);
}

static void
printlist (struct frontend *obj, struct question *q, int count, char **choices_translated, int *tindex, char *selected)
{
	int choice_min = -1;
	int num_cols, num_lines;
	int i, k, l;
	int logcount = 0;
	int *col_width;
	int total_width = 0;
	char **output;
	int line = 0, max_len = 0, col = 0;
	int width = getwidth();
	char **fchoices = malloc(sizeof(char *) * count);
	i = count;
	do {
		i /= 10;
		logcount++;
	} while (i > 0);

	/*  Set string arrays  */
	for (i=0; i < count; i++)
	{
		asprintf(&(fchoices[i]), "  %d. %s", i+1, choices_translated[i]);
		if (strwidth(fchoices[i]) < choice_min || choice_min == -1)
			choice_min = strwidth(fchoices[i]);
		if (strwidth(fchoices[i]) > width)
			width = strwidth(fchoices[i]);
	}
	num_cols = width / choice_min;
	if (num_cols > count)
		num_cols = count;
	col_width = malloc (sizeof(int) * num_cols);
	num_cols++;
	while (1)
	{
  COLUMN:
		num_cols--;
		if (num_cols == 0)
			break;
		num_lines = (count - 1) / num_cols + 1;
		for (i = 0; i < num_cols; i++)
			col_width[i] = 0;
		for (i = 0; i < count; i++)
		{
			int current_col = i / num_lines;
			if (strwidth(fchoices[i]) > col_width[current_col])
			{
				col_width[current_col] = strwidth(fchoices[i]);
				total_width = 0;
				for (k = 0; k < num_cols; k++)
					total_width += col_width[k];
				if (total_width > width)
					goto COLUMN;
			}
		}
		break;
	}
	if (num_cols == 0)
	{
		num_lines = count;
		num_cols = 1;
	}
	output = malloc(sizeof(char *) * num_lines);
	for (i = 0; i < num_lines; i++)
	{
		output[i] = malloc(MB_LEN_MAX * width + 1);
		*(output[i]) = '\0';
	}
	for (i = 0; i < count; i++)
	{
		strcat(output[line], fchoices[i]);
		if (strwidth(output[line]) > max_len)
			max_len = strwidth(output[line]);
		line++;
		if (line >= num_lines)
		{
			col++;
			if (col != num_cols)
			{
				for (l = 0; l < num_lines; l++)
					strpad(output[l], max_len);
			}

			line = 0;
			max_len = 0;
		}
	}
	for (l = 0; l < num_lines; l++)
	{
		printf("%s\n", output[l]);
		free(output[l]);
	}
	free(output);
	free(col_width);
	for (i = 0; i < count; i++)
		free(fchoices[i]);
	free(fchoices);
}

/*
 * Function: text_handler_boolean
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK, DC_GOBACK
 * Description: handler for the boolean question type
 * Assumptions: none
 */
static int text_handler_boolean(struct frontend *obj, struct question *q)
{
	char buf[30];
	int ans = 0;
	int def = 0;
	const char *defval;

	defval = question_getvalue(q, "");
	if (defval)
	{
		if (strcmp(defval, "true") == 0)
			def = 1;
		else 
			def = 2;
	}

	do {
		printf("  %d. %s", 1, get_text(obj, "debconf/yes", "Yes"));
		printf("  %d. %s", 2, get_text(obj, "debconf/no", "No"));
		printf("\n");
		if (def)
			printf(get_text(obj, "debconf/text-prompt-default",
					"Prompt: '%c' for help, default=%d> "), CHAR_HELP, def);
		else
			printf(get_text(obj, "debconf/text-prompt",
					"Prompt: '%c' for help> "), CHAR_HELP);
		get_answer(buf, sizeof(buf));
		if (buf[0] == CHAR_HELP && buf[1] == 0)
			show_help(obj, q);
		else if (obj->methods.can_go_back (obj, q) &&
		         buf[0] == CHAR_GOBACK && buf[1] == 0)
			return DC_GOBACK;
		else if (buf[0] == '1')
			ans = 1;
		else if (buf[0] == '2')
			ans = 2;
		else if (defval != 0 && ISEMPTY(buf))
			ans = def;
	} while (ans == 0);

	question_setvalue(q, (ans == 1 ? "true" : "false"));
	return DC_OK;
}

/*
 * Function: text_handler_multiselect
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the multiselect question type
 * Assumptions: none
 *
 * TODO: factor common code with select
 */
static int text_handler_multiselect(struct frontend *obj, struct question *q)
{
	char **choices;
	char **choices_translated;
	char **defaults;
	char *selected, *defval, *cp;
	char answer[4096] = {0};
	int i, j, count = 0, dcount, choice;
	int *tindex = NULL;
	int ret = DC_OK;
	const char *indices = q_get_indices(q);

	count = strgetargc(q_get_choices_vals(q));
	if (count <= 0)
		return DC_NOTOK;
	choices = malloc(sizeof(char *) * count);
	choices_translated = malloc(sizeof(char *) * count);
	tindex = malloc(sizeof(int) * count);
	if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
	{
		ret = DC_NOTOK;
		goto CleanUp_TINDEX;
	}

	defaults = malloc(sizeof(char *) * count);
	dcount = strchoicesplit(question_getvalue(q, ""), defaults, count);
	selected = calloc(1, sizeof(char) * count);

	for (j = 0; j < dcount; j++)
		for (i = 0; i < count; i++) {
			if (strcmp(choices[tindex[i]], defaults[j]) == 0)
				selected[tindex[i]] = 1;
		}

	i = 0;

	defval = malloc(10*count);
	*defval = '\0';
	for (i = 0; i < count; i++)
		if (selected[tindex[i]])
		{
			char buf[10];
			if (*defval != '\0')
				strcat(defval, " ");
			sprintf(buf, "%d", i+1);
			strcat(defval, buf);
		}

  DISPLAY:
	printlist (obj, q, count, choices_translated, tindex, selected);
	printf(get_text(obj, "debconf/text-prompt-default", 
		"Prompt: '%c' for help, default=%s> "), CHAR_HELP, defval);
	get_answer(answer, sizeof(answer));
	if (answer[0] == CHAR_HELP && answer[1] == 0)
	{
		show_help(obj, q);
		goto DISPLAY;
	}
	else if (answer[0] == CHAR_CLEAR && answer[1] == 0)
	{
		for (i = 0; i < count; i++)
			selected[i] = 0;
	}
	else if (obj->methods.can_go_back (obj, q) &&
	         answer[0] == CHAR_GOBACK && answer[1] == 0)
	{
		ret = DC_GOBACK;
		goto CleanUp_DEFVAL;
	}

	if (!(ISEMPTY(answer)))
	{
		for (i = 0; i < count; i++)
			selected[i] = 0;
		for (cp = answer; cp != NULL && *cp != '\0'; cp = strchr(cp+1, ' '))
		{
			choice = atoi(cp) - 1;
			if (choice >= 0 && choice < count)
				selected[tindex[choice]] = 1;
		}
	}

	answer[0] = 0;
	for (i = 0; i < count; i++)
	{
		if (selected[i])
		{
			if (answer[0] != 0)
				strvacat(answer, sizeof(answer), ", ", NULL);
			strvacat(answer, sizeof(answer), choices[i], NULL);
		}
	}
	question_setvalue(q, answer);

  CleanUp_DEFVAL:
	free(defval);
	free(selected);
	for (i = 0; i < dcount; i++)
		free(defaults[i]);
	free(defaults);
  CleanUp_TINDEX:
	free(tindex);
	for (i = 0; i < count; i++)
	{
		free(choices_translated[i]);
		free(choices[i]);
	}
	free(choices_translated);
	free(choices);
	
	return ret;
}

/*
 * Function: text_handler_select
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the select question type
 * Assumptions: none
 *
 * TODO: factor common code with multiselect
 */
static int text_handler_select(struct frontend *obj, struct question *q)
{
	char **choices;
	char **choices_translated;
	char answer[10];
	char *selected;
	int i, count = 0, choice = 1, def = -1;
	const char *defval = question_getvalue(q, "");
	int *tindex = NULL;
	int ret = DC_OK;
	const char *indices = q_get_indices(q);

	count = strgetargc(q_get_choices_vals(q));
	if (count <= 0)
		return DC_NOTOK;
	choices = malloc(sizeof(char *) * count);
	choices_translated = malloc(sizeof(char *) * count);
	tindex = malloc(sizeof(int) * count);
	if (strchoicesplitsort(q_get_choices_vals(q), q_get_choices(q), indices, choices, choices_translated, tindex, count) != count)
	{
		ret = DC_NOTOK;
		goto CleanUp_TINDEX;
	}

	selected = calloc(1, sizeof(char) * count);
	if (count == 1)
		defval = choices[0];

	if (defval != NULL)
	{
		for (i = 0; i < count; i++)
			if (strcmp(choices[tindex[i]], defval) == 0)
			{
				selected[tindex[i]] = 1;
				def = i;
			}
	}

	i = 0;
	choice = -1;
	do {
		printlist (obj, q, count, choices_translated, tindex, selected);
		if (def >= 0 && choices_translated[def]) {
			printf(get_text(obj, "debconf/text-prompt-default", 
				"Prompt: '%c' for help, default=%d> "),
					CHAR_HELP, def+1);
		} else {
			printf(get_text(obj, "debconf/text-prompt",
				"Prompt: '%c' for help> "), CHAR_HELP);
		}
		get_answer(answer, sizeof(answer));
		if (answer[0] == CHAR_HELP)
		{
			show_help(obj, q);
			continue;
		}
		if (obj->methods.can_go_back (obj, q) &&
		    answer[0] == CHAR_GOBACK && answer[1] == 0)
		{
			ret = DC_GOBACK;
			goto CleanUp_SELECTED;
		}
		if (ISEMPTY(answer))
			choice = def;
		else
			choice = atoi(answer) - 1;
	} while (choice < 0 || choice >= count);
	question_setvalue(q, choices[tindex[choice]]);

  CleanUp_SELECTED:
	free(selected);

  CleanUp_TINDEX:
	free(tindex);
	for (i = 0; i < count; i++) 
	{
		free(choices_translated[i]);
		free(choices[i]);
	}
	free(choices_translated);
	free(choices);
	
	return ret;
}

/*
 * Function: text_handler_note
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK, DC_GOBACK
 * Description: handler for the note question type
 * Assumptions: none
 */
static int text_handler_note(struct frontend *obj, struct question *q)
{
	char buf[100] = {0};
	printf("%s\n", get_text(obj, "debconf/cont-prompt",
				"[Press enter to continue]"));
	while (1)
	{
		get_answer(buf, sizeof(buf));
		if (buf[0] == CHAR_HELP && buf[1] == 0)
			show_help(obj, q);
		else if (obj->methods.can_go_back (obj, q) &&
		         buf[0] == CHAR_GOBACK && buf[1] == 0)
			return DC_GOBACK;
		else
			break;
	}
	return DC_OK;
}

/*
 * Function: text_handler_password
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the password question type
 * Assumptions: none
 *
 * TODO: this can be *MUCH* improved. no editing is possible right now
 */
static int text_handler_password(struct frontend *obj, struct question *q)
{
	struct termios oldt, newt;
	char passwd[256] = {0};
	int i, c;

	tcgetattr(0, &oldt);
	memcpy(&newt, &oldt, sizeof(struct termios));
	cfmakeraw(&newt);
	while (1)
	{
		tcsetattr(0, TCSANOW, &newt);
		i = 0;
		while ((c = fgetc(stdin)) != EOF)
		{
			if (c == '\r' || c == '\n')
				break;
			else if (c == '\b')
			{
				if (i > 0)
					i--;
				continue;
			}
			passwd[i] = (char)c;
			i++;
		}
		passwd[i] = 0;
		tcsetattr(0, TCSANOW, &oldt);
		if (passwd[0] == CHAR_HELP && passwd[1] == 0)
			show_help(obj, q);
		else
			break;
	}
	if (obj->methods.can_go_back (obj, q) &&
	    passwd[0] == CHAR_GOBACK && passwd[1] == 0)
		return DC_GOBACK;
	if (passwd[0] == CHAR_CLEAR && passwd[1] == 0)
		question_setvalue(q, "");
	else
		question_setvalue(q, passwd);
	return DC_OK;
}

/*
 * Function: text_handler_string
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK
 * Description: handler for the string question type
 * Assumptions: none
 */
static int text_handler_string(struct frontend *obj, struct question *q)
{
	char buf[1024] = {0};
	const char *defval = question_getvalue(q, "");
	while (1) {
		if (defval)
			printf(get_text(obj, "debconf/text-prompt-default", "Prompt: '%c' for help, default=%s> "), CHAR_HELP, defval);
		else
			printf(get_text(obj, "debconf/text-prompt", "Prompt: '%c' for help> "), CHAR_HELP);
		fflush(stdout);
		get_answer(buf, sizeof(buf));
		if (buf[0] == CHAR_HELP && buf[1] == 0)
			show_help(obj, q);
		else
			break;
	}
	if (obj->methods.can_go_back (obj, q) &&
	    buf[0] == CHAR_GOBACK && buf[1] == 0)
		return DC_GOBACK;
	if (ISEMPTY(buf) && defval == 0)
		question_setvalue(q, "");
	else if (ISEMPTY(buf) && defval != 0)
		question_setvalue(q, defval);
	else if (buf[0] == CHAR_CLEAR && buf[1] == 0)
		question_setvalue(q, "");
	else
		question_setvalue(q, buf);
	return DC_OK;
}

/*
 * Function: text_handler_text
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK, DC_GOBACK
 * Description: handler for the text question type
 * Assumptions: none
 */
static int text_handler_text(struct frontend *obj, struct question *q)
{
	return text_handler_note(obj, q);
}

/*
 * Function: text_handler_error
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question to ask
 * Output: int - DC_OK, DC_NOTOK, DC_GOBACK
 * Description: handler for the error question type. Currently equal to _note
 * Assumptions: none
 */
static int text_handler_error(struct frontend *obj, struct question *q)
{
	return text_handler_note(obj, q);
}

/* ----------------------------------------------------------------------- */
struct question_handlers {
	const char *type;
	int (*handler)(struct frontend *obj, struct question *q);
} question_handlers[] = {
	{ "boolean",	text_handler_boolean },
	{ "multiselect", text_handler_multiselect },
	{ "note",	text_handler_note },
	{ "password",	text_handler_password },
	{ "select",	text_handler_select },
	{ "string",	text_handler_string },
	{ "text",	text_handler_text },
	{ "error",	text_handler_error },
};

/*
 * Function: text_intitialize
 * Input: struct frontend *obj - frontend UI object
 *        struct configuration *cfg - configuration parameters
 * Output: int - DC_OK, DC_NOTOK
 * Description: initializes the text UI
 * Assumptions: none
 *
 * TODO: SIGINT is ignored by the text UI, otherwise it interfers with
 * the way question/answers are handled. this is probably not optimal
 */
static int text_initialize(struct frontend *obj, struct configuration *conf)
{
	obj->interactive = 1;
	signal(SIGINT, SIG_IGN);
	return DC_OK;
}

/*
 * Function: text_can_go_back
 * Input: struct frontend *obj - frontend object
 *        struct question *q - question object
 * Output: int - DC_OK, DC_NOTOK
 * Description: tells whether confmodule supports backing up
 * Assumptions: none
 */
static int
text_can_go_back(struct frontend *obj, struct question *q)
{
	return (obj->capability & DCF_CAPB_BACKUP);
}

/*
 * Function: text_go
 * Input: struct frontend *obj - frontend object
 * Output: int - DC_OK, DC_NOTOK
 * Description: asks all pending questions
 * Assumptions: none
 */
static int text_go(struct frontend *obj)
{
	struct question *q = obj->questions;
	int i;
	int ret = DC_OK;
	int display_title = 1;

	while (q != NULL) {
		for (i = 0; i < DIM(question_handlers); i++) {
			if (strcmp(q->template->type, question_handlers[i].type) == 0) 
			{

				if (display_title)
				{
					printf("%s\n\n", obj->title);
					display_title = 0;
				}
				text_handler_displaydesc(obj, q);
				ret = question_handlers[i].handler(obj, q);
				if (ret == DC_OK)
					obj->qdb->methods.set(obj->qdb, q);
				else if (ret == DC_GOBACK && q->prev != NULL)
					q = q->prev;
				else
					return ret;
				break;
			}
		}
		if (ret == DC_OK)
			q = q->next;
	}
	return DC_OK;
}

static void text_progress_start(struct frontend *obj, int min, int max, const char *title)
{
	DELETE(obj->progress_title);
	obj->progress_title = STRDUP(title);
	obj->progress_min = min;
	obj->progress_max = max;
	obj->progress_cur = min;

	printf("%s  ", title);
	fflush(stdout);
}

static void text_progress_set(struct frontend *obj, int val)
{
	static int last = 0;
	int new;

	obj->progress_cur = val;
	/*  Prevent verbose output  */
	if (obj->progress_cur < last)
		last = 0;
	new = ((double)(obj->progress_cur - obj->progress_min) / 
		(double)(obj->progress_max - obj->progress_min) * 100.0);
	if (new / 10 == last / 10)
		return;
	last = new;
	printf("..%d%%", new);
	fflush(stdout);
}

static void text_progress_stop(struct frontend *obj)
{
	INFO(INFO_DEBUG, "%s\n", __FUNCTION__);
	printf("\n");
	fflush(stdout);
}

struct frontend_module debconf_frontend_module =
{
	initialize: text_initialize,
	can_go_back: text_can_go_back,
	go: text_go,
	progress_start: text_progress_start,
	progress_set: text_progress_set,
	progress_stop: text_progress_stop,
};
