/* SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026, kurumihere
 *
 * See LICENSE for details.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct
{
	bool num_nonblank; /* -b */
	bool show_ends;	   /* -e */
	bool num_all;	   /* -n */
	bool squeeze;	   /* -s */
	bool show_tabs;	   /* -t */
} cat_opts;

static char buf[BUFSIZ];

static void cat_err(const char *prog, const char *file)
{
	dprintf(STDERR_FILENO, "%s: %s: %s\n", prog, file, strerror(errno));
}

static void cat_n_args(int fd, const char *name, const char *prog)
{
	ssize_t n;
	while ((n = read(fd, buf, sizeof(buf))) > 0)
	{
		if (write(STDOUT_FILENO, buf, n) != n)
		{
			cat_err(prog, "stdout");
			return;
		}
	}
	if (n < 0)
		cat_err(prog, name);
}

static void cat_args(int fd, const char *name, const char *prog, cat_opts *opts)
{
	static int l_num = 1;
	static bool bol = true; /* Beginning Of Line */
	static int b_cnt = 0;	/* counter for blank lines, to '-s' */
	ssize_t n, i;
	while ((n = read(fd, buf, sizeof(buf))) > 0)
	{
		for (i = 0; i < n; i++)
		{
			unsigned char ch = buf[i];
			if (bol)
			{
				if (ch == '\n')
				{
					b_cnt++;
					if (opts->squeeze && b_cnt > 1)
						continue;
				}
				else
				{
					b_cnt = 0;
				}
				if (opts->num_nonblank)
				{
					if (ch != '\n')
					{
						printf("%6d\t", l_num++);
						bol = false;
					}
				}
				else if (opts->num_all)
				{
					printf("%6d\t", l_num++);
					bol = false;
				}
			}
			if (ch == '\n')
			{
				if (opts->show_ends)
					putchar('$');
				putchar('\n');
				bol = true;
			}
			else if (ch == '\t' && opts->show_tabs)
			{
				printf("^I");
				bol = false;
			}
			else
			{
				putchar(ch);
				bol = false;
			}
		}
	}
	if (n < 0)
		cat_err(prog, name);
}

static int parse_args(int c, char **av, cat_opts *opts)
{
	int i, j;
	for (i = 1; i < c; i++)
	{
		if (av[i][0] != '-' || av[i][1] == '\0')
			continue;
		if (av[i][1] == '-')
		{
			if (strcmp(av[i], "--number-nonblank") == 0)
				opts->num_nonblank = true;
			else if (strcmp(av[i], "--number") == 0)
				opts->num_all = true;
			else if (strcmp(av[i], "--squeeze-blank") == 0)
				opts->squeeze = true;
			else
				return -1;
			continue;
		}
		for (j = 1; av[i][j]; j++)
		{
			switch (av[i][j])
			{
				case 'b':
					opts->num_nonblank = true;
					break;
				case 'e':
					opts->show_ends = true;
					break;
				case 'n':
					opts->num_all = true;
					break;
				case 's':
					opts->squeeze = true;
					break;
				case 't':
					opts->show_tabs = true;
					break;
				default:
					return -1;
			}
		}
	}
	if (opts->num_nonblank)
		opts->num_all = false;
	return 0;
}

int main(int c, char **av)
{
	cat_opts opts = {0};
	int i, fd;
	bool has_files = false;
	bool use_flags;
	if (parse_args(c, av, &opts) < 0)
	{
		dprintf(STDERR_FILENO, "usage: %s [-benst] [file ...]\n",
			av[0]);
		return EXIT_FAILURE;
	}
	use_flags = (opts.num_all || opts.num_nonblank || opts.show_ends ||
		     opts.squeeze || opts.show_tabs);
	for (i = 1; i < c; i++)
	{
		if (av[i][0] == '-' && av[i][1] != '\0')
			continue;
		has_files = true;
		if (strcmp(av[i], "-") == 0)
		{
			fd = STDIN_FILENO;
		}
		else
		{
			fd = open(av[i], O_RDONLY);
		}
		if (fd < 0)
		{
			cat_err(av[0], av[i]);
			continue;
		}
		if (use_flags)
			cat_args(fd, av[i], av[0], &opts);
		else
			cat_n_args(fd, av[i], av[0]);

		if (fd != STDIN_FILENO)
			close(fd);
	}
	if (!has_files)
	{
		if (use_flags)
			cat_args(STDIN_FILENO, "stdin", av[0], &opts);
		else
			cat_n_args(STDIN_FILENO, "stdin", av[0]);
	}

	return EXIT_SUCCESS;
}
