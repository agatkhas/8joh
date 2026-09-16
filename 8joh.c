#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#define PATTERN "▼▼ ▼▼ ›› ▲▲▲▲ ▲▲▲▲ ‹ ▼▼ ▼▼ › ▲▲▲ ▲▲▲    "
#define GREY "\033[90m"
#define MAXCELLS 64
#define ROWS 3

struct cell { const char *p; int len; };

static struct cell cells[MAXCELLS];
static int ncells;
static int color = 1;
static volatile sig_atomic_t stop;

static void split(const char *s)
{
	while (*s && ncells < MAXCELLS) {
		int len = 1;

		while ((s[len] & 0xc0) == 0x80)
			len++;
		cells[ncells].p = s;
		cells[ncells].len = len;
		ncells++;
		s += len;
	}
}

static int is(struct cell c, const char *glyph)
{
	return c.len == (int)strlen(glyph) && !memcmp(c.p, glyph, c.len);
}

static int rowof(struct cell c)
{
	if (is(c, "▼"))
		return 0;
	if (is(c, "▲"))
		return 2;
	return 1;
}

static void frame(int off, int w)
{
	int r, x;

	for (r = 0; r < ROWS; r++) {
		if (color)
			fputs(GREY, stdout);
		for (x = 0; x < w; x++) {
			struct cell c = cells[(off + x) % ncells];

			if (rowof(c) == r && c.p[0] != ' ')
				fwrite(c.p, 1, c.len, stdout);
			else
				putchar(' ');
		}
		if (color)
			fputs("\033[0m", stdout);
		if (r < ROWS - 1)
			putchar('\n');
	}
}

static void nap(long ms)
{
	struct timespec t = { ms / 1000, (ms % 1000) * 1000000L };

	nanosleep(&t, NULL);
}

static void onsig(int sig)
{
	(void)sig;
	stop = 1;
}

static void usage(FILE *f)
{
	fputs("usage: 8joh [-1] [-n] [-s ms] [-w cols]\n"
	      "  -1       print one frame and quit, for your shell rc or fastfetch\n"
	      "  -n       no color\n"
	      "  -s ms    delay per frame, default 150\n"
	      "  -w cols  widen the box past the pattern\n", f);
}

int main(int argc, char **argv)
{
	struct sigaction sa;
	long ms = 150;
	int off = 0, w = 0, once = 0, opt;

	while ((opt = getopt(argc, argv, "1ns:w:h")) != -1) {
		switch (opt) {
		case '1':
			once = 1;
			break;
		case 'n':
			color = 0;
			break;
		case 's':
			ms = strtol(optarg, NULL, 10);
			if (ms < 0)
				ms = 0;
			break;
		case 'w':
			w = atoi(optarg);
			break;
		case 'h':
			usage(stdout);
			return 0;
		default:
			usage(stderr);
			return 1;
		}
	}

	split(PATTERN);
	if (w < 1)
		w = ncells;

	if (once) {
		frame(0, w);
		putchar('\n');
		return 0;
	}

	memset(&sa, 0, sizeof sa);
	sa.sa_handler = onsig;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	fputs("\033[?25l", stdout);
	while (!stop) {
		frame(off, w);
		off = (off + 1) % ncells;
		printf("\033[%dA\r", ROWS - 1);
		fflush(stdout);
		nap(ms);
	}
	printf("\033[%dB\033[?25h\n", ROWS - 1);
	return 0;
}
