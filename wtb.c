/* MIT/X Consortium License
 *
 * © 2011-2016 Laurent Arnoud <laurent@spkdev.net>
 *
 * See LICENSE file for copyright and license details.
 *
 * wtb - print currently open bug ID
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
    /*
     * Trac #bugid (title)
     * https://trac.edgewall.org/ticket/8813
     */
    "^#([[:digit:]]+) \\(",
    /*
     * Redmine project - Bug #bugid: title
     * https://www.redmine.org/issues/24641
     */
    ".* #([[:digit:]]+):",
    /*
     * Debian BTS #bugid - title
     * https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=845558
     */
    "^#([[:digit:]]+) -",
    /*
     * MantisBT bugid: title
     * https://www.mantisbt.org/bugs/view.php?id=22049
     */
    "^([[:digit:]]+): (.*) -",
    /*
     * Bugzilla Bug bugid – title ! use PCRE ?
     * https://bugzilla.mozilla.org/show_bug.cgi?id=1143712
     */
    "^([[:digit:]]+) ",
    /*
     * Jira [#CODE-bugid] title
     * https://jira.mongodb.org/browse/DOCS-1515
     */
    "^\\[.*-([[:digit:]]+)\\] ",
    /*
     * Flyspray FS#bugid : title
     * https://bugs.flyspray.org/index.php?do=details&task_id=2319
     */
    "^FS#([[:digit:]]+) : ",
    /*
     * PHP Bug Tracking System
     * https://bugs.php.net/bug.php?id=73734
     */
    "^PHP :: .* #([[:digit:]]+) :: ",
    /* Github Issues */
    ".* ? .* #([[:digit:]]+) ? ",
    /* GitLab Issues */
    ".* \\(#([[:digit:]]+)\\)",
};

struct bug {
    char *bugid;
};

static const char usage[] =
APP " " VERSION " - print currently open bug ID\n"
"\n"
"Usage: " APP "      [options]\n"
"\n"
"Options:\n"
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
        free(b);
    }
}

    void
get_bugid(const char *string)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(bts); i++) {
        if (match(string, bts[i]) != 0) {
            struct bug *b = parse_bug(string, bts[i]);
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
        static const char opts[] = "tvh";
        static struct option lopts[] = {
            {"version", no_argument, 0, 'v'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        int idx = 0;
        while ((c = getopt_long(argc, argv, opts, lopts, &idx)) != -1) {
            switch (c) {
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
                get_bugid((const char *)tp.value);
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
