/*	$Id: out.c,v 1.8 2009/11/07 08:26:45 kristaps Exp $ */
/*
 * Copyright (c) 2009 Kristaps Dzonsons <kristaps@kth.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "out.h"

#ifdef __linux__
extern	size_t	  strlcat(char *, const char *, size_t);
#endif

/* 
 * Convert a `scaling unit' to a consistent form, or fail.  Scaling
 * units are documented in groff.7, mdoc.7, man.7.
 */
int
a2roffsu(const char *src, struct roffsu *dst, enum roffscale def)
{
	char		 buf[BUFSIZ], hasd;
	int		 i;
	enum roffscale	 unit;

	if ('\0' == *src)
		return(0);

	i = hasd = 0;

	switch (*src) {
	case ('+'):
		src++;
		break;
	case ('-'):
		buf[i++] = *src++;
		break;
	default:
		break;
	}

	if ('\0' == *src)
		return(0);

	while (i < BUFSIZ) {
		if ( ! isdigit((u_char)*src)) {
			if ('.' != *src)
				break;
			else if (hasd)
				break;
			else
				hasd = 1;
		}
		buf[i++] = *src++;
	}

	if (BUFSIZ == i || (*src && *(src + 1)))
		return(0);

	buf[i] = '\0';

	switch (*src) {
	case ('c'):
		unit = SCALE_CM;
		break;
	case ('i'):
		unit = SCALE_IN;
		break;
	case ('P'):
		unit = SCALE_PC;
		break;
	case ('p'):
		unit = SCALE_PT;
		break;
	case ('f'):
		unit = SCALE_FS;
		break;
	case ('v'):
		unit = SCALE_VS;
		break;
	case ('m'):
		unit = SCALE_EM;
		break;
	case ('\0'):
		if (SCALE_MAX == def)
			return(0);
		unit = SCALE_BU;
		break;
	case ('u'):
		unit = SCALE_BU;
		break;
	case ('M'):
		unit = SCALE_MM;
		break;
	case ('n'):
		unit = SCALE_EN;
		break;
	default:
		return(0);
	}

	if ((dst->scale = atof(buf)) < 0)
		dst->scale = 0;
	dst->unit = unit;
	dst->pt = hasd;

	return(1);
}


/*
 * Correctly writes the time in nroff form, which differs from standard
 * form in that a space isn't printed in lieu of the extra %e field for
 * single-digit dates.
 */
void
time2a(time_t t, char *dst, size_t sz)
{
	struct tm	 tm;
	char		 buf[5];
	char		*p;
	size_t		 nsz;

	assert(sz > 1);
	localtime_r(&t, &tm);

	p = dst;
	nsz = 0;

	dst[0] = '\0';

	if (0 == (nsz = strftime(p, sz, "%B ", &tm)))
		return;

	p += (int)nsz;
	sz -= nsz;

	if (0 == strftime(buf, sizeof(buf), "%e, ", &tm))
		return;

	nsz = strlcat(p, buf + (' ' == buf[0] ? 1 : 0), sz);

	if (nsz >= sz)
		return;

	p += (int)nsz;
	sz -= nsz;

	(void)strftime(p, sz, "%Y", &tm);
}


/* Returns length of parsed string. */
int
a2roffdeco(enum roffdeco *d,
		const char **word, size_t *sz)
{
	int		 j, type, sv, t, lim;
	const char	*wp;

	*d = DECO_NONE;
	wp = *word;
	type = 1;

	switch (*wp) {
	case ('\0'):
		return(0);

	case ('('):
		wp++;
		if ('\0' == *wp)
			return(1);
		if ('\0' == *(wp + 1))
			return(2);

		*d = DECO_SPECIAL;
		*sz = 2;
		*word = wp;
		return(3);

	case ('*'):
		wp++;

		switch (*wp) {
		case ('\0'):
			return(1);

		case ('('):
			wp++;
			if ('\0' == *wp)
				return(2);
			if ('\0' == *(wp + 1))
				return(3);

			*d = DECO_RESERVED;
			*sz = 2;
			*word = wp;
			return(4);

		case ('['):
			type = 0;
			break;

		default:
			*d = DECO_RESERVED;
			*sz = 1;
			*word = wp;
			return(2);
		}
		break;

#if 0
	case ('s'):
		wp++;

		/* This closely follows mandoc_special(). */
		if ('\0' == *wp) 
			return(1);

		t = 0;
		lim = 1;

		if (*wp == '\'') {
			lim = 0;
			t = 1;
			++wp;
		} else if (*wp == '[') {
			lim = 0;
			t = 2;
			++wp;
		} else if (*wp == '(') {
			lim = 2;
			t = 3;
			++wp;
		}

		if (*wp == '+' || *wp == '-')
			++wp;

		if (*wp == '\'') {
			if (t) {
				*word = wp;
				return;
			}
			lim = 0;
			t = 1;
			++wp;
		} else if (*wp == '[') {
			if (t) {
				*word = wp;
				return;
			}
			lim = 0;
			t = 2;
			++wp;
		} else if (*wp == '(') {
			if (t) {
				*word = wp;
				return;
			}
			lim = 2;
			t = 3;
			++wp;
		}

		if ( ! isdigit((u_char)*wp)) {
			*word = --wp;
			return;
		}

		for (j = 0; isdigit((u_char)*wp); j++) {
			if (lim && j >= lim)
				break;
			++wp;
		}

		if (t && t < 3) {
			if (1 == t && *wp != '\'') {
				*word = --wp;
				return;
			}
			if (2 == t && *wp != ']') {
				*word = --wp;
				return;
			}
			++wp;
		}
		*word = --wp;
		return;
#endif

	case ('f'):
		wp++;

		switch (*wp) {
		case ('\0'):
			return(1);
		case ('3'):
			/* FALLTHROUGH */
		case ('B'):
			*d = DECO_BOLD;
			break;
		case ('2'):
			/* FALLTHROUGH */
		case ('I'):
			*d = DECO_ITALIC;
			break;
		case ('P'):
			*d = DECO_PREVIOUS;
			break;
		case ('1'):
			/* FALLTHROUGH */
		case ('R'):
			*d = DECO_ROMAN;
			break;
		default:
			break;
		}

		return(2);

	case ('['):
		break;

	default:
		*d = DECO_SPECIAL;
		*word = wp;
		*sz = 1;
		return(1);
	}

	*word = ++wp;
	for (j = 0; *wp && ']' != *wp; wp++, j++)
		/* Loop... */ ;

	if ('\0' == *wp)
		return(j + 1);

	*d = type ? DECO_SPECIAL : DECO_RESERVED;
	*sz = j;
	return (j + 2);
}
