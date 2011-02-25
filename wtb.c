/* MIT/X Consortium License
 *
 * © 2011 Laurent Arnoud <laurent@spkdev.net>
 *
 * See LICENSE file for copyright and license details.
 *
 * wtb - print id or title of currently open bug
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <limits.h>
#include <getopt.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#define APP "wtb"
#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(x[0]))

/* globals */
static char *bts[] = {
	/* Trac #bugid (title) */
	"^#([[:digit:]]+) \\((.*)\\)",
	/* Redmine project - Bug #bugid: title */
	"^.* - \\w+ #([[:digit:]]+): (.*) - ",
	/* Debian BTS #bugid - title */
	"^#([[:digit:]]+) - (.*) -",
	/* MantisBT bugid: title */
	"^([[:digit:]]+): (.*) -",
	/* Bugzilla Bug bugid – title ! use PCRE ? */
	"^Bug ([[:digit:]]+) (.*) -",
	/* Jira [#CODE-bugid] title */
	"^\\[#[[:alnum:]]+-([[:digit:]]+)\\] (.*) - ",
	/* Flyspray FS#bugid : title */
	"^FS#([[:digit:]]+) : (.*) - ",
	/* PHP Bug Tracking System */
	"^PHP :: \\w+ #([[:digit:]]+) :: (.*) - ",
};

struct bug {
	char *bugid;
	char *title;
};

static const char usage[] =
APP " " VERSION " - print id or title of currently open bug\n"
"\n"
"Usage: " APP "      [options]\n"
"\n"
"Options:\n"
"  -t, --title                 Show bug title.\n"
"  -v, --version               Show version.\n"
"  -h, --help                  Show help message.";


/* function prototypes */
static Window *get_client_list(Display *dpy, Window root,
		unsigned long *nclients);

bool
match(const char *string, char *pattern)
{
	int    status;
	regex_t    re;

	if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0)
		return false;

	status = regexec(&re, string, (size_t) 0, NULL, 0);
	regfree(&re);
	if (status != 0)
		return false;

	return true;
}

struct bug *
parse_bug(const char *str, char *pattern)
{
	regex_t *preg;
	int i, n;
	regmatch_t *r = NULL;
	struct bug *b;

	preg = malloc(sizeof *preg);
	if (preg == NULL)
		return NULL;

	if (regcomp(preg, pattern, REG_EXTENDED|REG_ICASE) != 0) {
		free(preg);
		return NULL;
	}
	n = preg->re_nsub + 1;
	r = malloc(sizeof(*r) * n);
	i = regexec(preg, str, n, r, 0);
	if (i == 0) {
		/* bug found */
		b = malloc(sizeof(*b));
		int start_bugid = r[1].rm_so;
		int end_bugid = r[1].rm_eo;
		size_t size_bugid = end_bugid - start_bugid;

		b->bugid = malloc(sizeof(*b->bugid) * (size_bugid + 1));
		if (b->bugid) {
			strncpy(b->bugid, &str[start_bugid], size_bugid);
			b->bugid[size_bugid] = '\0';
		}

		int start_title = r[2].rm_so;
		int end_title = r[2].rm_eo;
		size_t size_title = end_title - start_title;
		b->title = malloc(sizeof(*b->title) * (size_title + 1));
		if (b->title) {
			strncpy(b->title, &str[start_title], size_title);
			b->title[size_title] = '\0';
		}
	} else {
		b = NULL;
	}

	regfree(preg);
	free(preg);
	free(r);
	return b;
}

void
free_bug(struct bug *b)
{
	if (b) {
		if (b->bugid)
			free(b->bugid);
		if (b->title)
			free(b->title);
		free(b);
	}
}

void
get_bugid(const char *string, bool title)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(bts); i++) {
		if (match(string, bts[i]) != 0) {
			struct bug *b = parse_bug(string, bts[i]);
			if (title)
				printf("%s\n", b->title);
			else
				printf("%s\n", b->bugid);

			free_bug(b);
			return;
		}

	}
}

int
main(int argc, char **argv)
{
	int i;
	unsigned long len = 0;
	Display *disp = XOpenDisplay(NULL);
	XTextProperty tp;
	Window root = DefaultRootWindow(disp), *list;

	if (!disp) {
		fprintf(stderr, "Failed to open display\n");
		return EXIT_FAILURE;
	}

	list = get_client_list(disp, root, &len);

	if (list) {
		/* command line options */
		static int c;
		static const char opts[] = "itavh";
		static struct option lopts[] = {
			{"title", no_argument, 0, 't'},
			{"version", no_argument, 0, 'v'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		static bool title;
		int idx = 0;
		while ((c = getopt_long(argc, argv, opts, lopts, &idx)) != -1) {
			switch (c) {
			case 't':
				title = true;
				break;
			case 'v':
				printf("%s %s\n", APP, VERSION);
				exit(EXIT_SUCCESS);
				break;
			case 'h':
				printf("%s\n", usage);
				exit(EXIT_SUCCESS);
				break;
			default:
				printf("%s\n", usage);
				abort();
			}
		}

		/* check clients WM_NAME */
		for (i = 0; i < len; i++) {
			if (XGetWMName(disp, list[i], &tp) != 0) {
				get_bugid((const char *)tp.value, title);
				XFree(tp.value);
			}
		}
	}

	XFree(list);
	XCloseDisplay(disp);
	return EXIT_SUCCESS;
}

/*
 * thanks Brian Tarricone
 * see http://spurint.org/files/xfce4/trans-inactive.c
 */
static Window *
get_client_list(Display *dpy, Window root, unsigned long *nclients)
{
	Window *clients = NULL;
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop_ret = NULL;

	if (XGetWindowProperty(dpy, root,
				XInternAtom(dpy, "_NET_CLIENT_LIST", False),
				0L, ULONG_MAX, False, XA_WINDOW,
				&actual_type, &actual_format, &nitems,
				&bytes_after, &prop_ret) == Success) {
		clients = (Window *)prop_ret;
		*nclients = nitems;
	}

	return clients;
}
