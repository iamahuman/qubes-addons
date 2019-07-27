/*
 * Copyright (c) 2019 Jinoh Kang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static char buf[4040]; // NOTE adjust so that there's no unused trailing memory in the last page

#ifdef __GNUC__
#define UNLIKELY(x) __builtin_expect((x), 0)
#else
#define UNLIKELY(x) (x)
#endif

static ssize_t write_all(int fd, void const *buf, size_t len)
{
	char const *ptr = (char const *) buf;
	size_t n = len;

	while (n > 0)
	{
		ssize_t k = write(fd, (void const *) ptr, n);
		if (UNLIKELY(k <= 0))
		{
			return k;
		}
		ptr += k, n -= k;
	}

	return 1;
}

#define WRITE_MSG(fd, msg) write_all(fd, msg, sizeof(msg)-1)

#define STR_CAPABL(F) F("pabilities\n")
#define STR_CNTGIT(F) F("nnect git-")
#define STR_RCVPAK(F) F("eceive-pack\n")
#define STR_UPLOAD(F) F("pload-")
#define STR_ARCHVE(F) F("rchive\n")
#define STR_PACKLN(F) F("ack\n")

#define STRM_ALL_TOKENS(_) CAPABL _ CNTGIT _ RCVPAK _ UPLOAD _ ARCHVE _ PACKLN
#define STRM_PREFIX_STR(N) STR_ ## N
#define STRM_DISCARD(...)
#define STRM_OPN (
#define STRM_CSE )
#define STRM_COMMA() ,
#define STRM_IDENTITY(X) X
#define STRM_EVAL0(X) X
#define STRM_TABLE(T) STRM_PREFIX_STR STRM_DISCARD(~) (T(STRM_CSE (STRM_IDENTITY) STRM_PREFIX_STR STRM_OPN)(STRM_IDENTITY))
#define STRM_CDAR(_, a, ...) a
#define STRM_COMPARE(A, B) (A (B STRM_DISCARD(~) (0 STRM_COMMA STRM_DISCARD(~) () STRM_CONT_0 STRM_COMMA STRM_DISCARD(~) () STRM_DISCARD)), STRM_CONT_1)
#define STRM_OFF_CONT(A, B) STRM_EVAL0(STRM_CDAR STRM_COMPARE(A, STR_ ## B))(STR_ ## B)
#define STRM_CONT_0(N) N (STRM_IDENTITY)
#define STRM_CONT_1(N) STRM_DISCARD STRM_OPN
#define STRM_OFFSET_(T, N) STRM_OFF_CONT STRM_OPN N STRM_COMMA() T(STRM_CSE STRM_OFF_CONT STRM_DISCARD STRM_DISCARD(~) (~) STRM_OPN N STRM_COMMA()) STRM_CSE STRM_CSE
#define STRM_EVALV(...) __VA_ARGS__
#define STRM_OFFSET(T, N) (sizeof("" STRM_EVALV(STRM_EVALV(STRM_OFFSET_(T, N)))) - 1)

static char const strtab[] = STRM_EVAL0(STRM_TABLE(STRM_ALL_TOKENS));
#define TOK_GET(x) (strtab + STRM_OFFSET(STRM_ALL_TOKENS, x))
#define TOK_SIZ(x) (x(sizeof) - 1)
#define TOK_END(x) (STRM_OFF_CONT(STRM_ALL_TOKENS, x) + x(sizeof) - 1)

static size_t scan_tok(char const *ptr, char const *end, char const *tok, size_t tok_siz, unsigned char base, unsigned char state)
{
	size_t off = state - base;
	size_t rem_siz = end - ptr;
	size_t rtk_siz = tok_siz - off;
	size_t cmp_siz = rem_siz < rtk_siz ? rem_siz : rtk_siz;
	return memcmp(ptr, tok + off, cmp_siz) ? 0 : cmp_siz;
}
#define SCAN_TOK(buf, siz, off, tok) scan_tok(buf, siz, TOK_GET(tok) + off, TOK_SIZ(tok) - off)

enum p_state {
	s_initial,

	p_c,
	p_ca,
	k_capabilities_ = p_ca + TOK_SIZ(STR_CAPABL) - 1,
	p_co,
	p_connect_git_ = p_co + TOK_SIZ(STR_CNTGIT),
	p_connect_git_r,
	k_connect_git_receive_pack = p_connect_git_r + TOK_SIZ(STR_RCVPAK) - 1,
	p_connect_git_u,
	p_connect_git_upload_ = p_connect_git_u + TOK_SIZ(STR_UPLOAD),
	p_connect_git_upload_a,
	k_connect_git_upload_archive_ = p_connect_git_upload_a + TOK_SIZ(STR_ARCHVE) - 1,
	p_connect_git_upload_p,
	k_connect_git_upload_pack_ = p_connect_git_upload_p + TOK_SIZ(STR_PACKLN) - 1,

	s_max
};
#define BRANCH_1(c0, d0) do { char x = *ptr++; state = x == (c0) ? d0 : s_max; } while (0)
#define BRANCH_2(c0, d0, c1, d1) do { char x = *ptr++; state = x == (c0) ? d0 : x == (c1) ? d1 : s_max; } while (0)
#define BRANCH_S(bas, tok) do { size_t r = scan_tok(ptr, end, TOK_GET(tok), TOK_SIZ(tok), bas, state); ptr += r; state = r ? state + r : s_max; } while (0)

static char const target_progname[] = "/usr/bin/qrexec-client-vm";

int main (int argc, char **argv)
{
	static char const prefix[] = "xawsip.GitSSH+", newline = '\n';

	char const *url, *spc;
	char *tgt, *nurl, *narg, cmd, *new_argv[4];
	unsigned char state = s_initial;
	size_t tgt_siz, pat_siz, spn, tot_siz;

	if (UNLIKELY(argc != 3 || (spc = strchr(url = argv[2], ' ')) == NULL))
	{
		WRITE_MSG(STDERR_FILENO, "usage: git remote-xawsip <remote> <url>\n");
		return 129;
	}
	tgt_siz = spc - url;
	pat_siz = strlen(++spc);

	for (;;)
	{
		char const *ptr, *end;
		ssize_t k = read(STDIN_FILENO, buf, sizeof buf);
		if (UNLIKELY(k <= 0)) return EXIT_FAILURE;

		end = buf + k;
		for (ptr = buf; ptr != end; )
		{
			if (state == s_initial)	BRANCH_1('c', p_c);
			else if (state == p_c)	BRANCH_2('a', p_ca, 'o', p_co);
			else if (state <= k_capabilities_)
			{
				BRANCH_S(p_ca, STR_CAPABL);
				if (state == 1 + k_capabilities_)
				{
					if (WRITE_MSG(STDOUT_FILENO, "*connect\n\n") <= 0)
					{
						return EXIT_FAILURE;
					}
					state = s_initial;
				}
			}
			else if (state < p_connect_git_)
			{
				BRANCH_S(p_co, STR_CNTGIT);
			}
			else if (state == p_connect_git_)
			{
				BRANCH_2('r', p_connect_git_r,
					 'u', p_connect_git_u);
			}
			else if (state <= k_connect_git_receive_pack)
			{
				BRANCH_S(p_connect_git_r, STR_RCVPAK);
				if (state == 1 + k_connect_git_receive_pack)
				{
					cmd = 'r';
					goto do_command;
				}
			}
			else if (state < p_connect_git_upload_)
			{
				BRANCH_S(p_connect_git_u, STR_UPLOAD);
			}
			else if (state == p_connect_git_upload_)
			{
				BRANCH_2('a', p_connect_git_upload_a,
					 'p', p_connect_git_upload_p);
			}
			else if (state <= k_connect_git_upload_archive_)
			{
				BRANCH_S(p_connect_git_upload_a, STR_ARCHVE);
				if (state == 1 + k_connect_git_upload_archive_)
				{
					cmd = 'a';
					goto do_command;
				}
			}
			else if (state <= k_connect_git_upload_pack_)
			{
				BRANCH_S(p_connect_git_upload_p, STR_PACKLN);
				if (state == 1 + k_connect_git_upload_pack_)
				{
					cmd = 'u';
					goto do_command;
				}
			}
			else
			{
				if (memchr(ptr, '\n', end - ptr) == NULL) break;
				WRITE_MSG(STDERR_FILENO, "Bad command");
				return EXIT_FAILURE;
			}
		}
	}
do_command:
	tot_siz = sizeof prefix + 3 + tgt_siz + pat_siz;
	if (UNLIKELY((tgt = tot_siz <= sizeof buf ? buf : (char *) sbrk(tot_siz)) == NULL))
	{
		WRITE_MSG(STDERR_FILENO, "Out of memory\n");
		return EXIT_FAILURE;
	}
	if (UNLIKELY(write(STDOUT_FILENO, &newline, 1) < 0))
	{
		return EXIT_FAILURE;
	}
	memcpy(tgt, url, tgt_siz);
	tgt[tgt_siz] = '\0';
	nurl = tgt + tgt_siz + 1;
	memcpy(nurl, prefix, sizeof prefix - 1);
	narg = nurl + (sizeof prefix - 1);
	*narg++ = cmd;
	*narg++ = '+';
	memcpy(narg, spc, pat_siz);
	for (spn = 0; (spn += strcspn(narg + spn, ":/")) < pat_siz; narg[spn] = '+')
		;
	narg[pat_siz] = '\0';

	new_argv[0] = (char *) target_progname;
	new_argv[1] = tgt;
	new_argv[2] = nurl;
	new_argv[3] = NULL;

	execv(target_progname, new_argv);

	{
		int err = errno;
		char *msg = strerror(err);

		WRITE_MSG(STDERR_FILENO, "cannot execute ");
		WRITE_MSG(STDERR_FILENO, target_progname);
		WRITE_MSG(STDERR_FILENO, ": ");
		write_all(STDERR_FILENO, msg, strlen(msg));
		write(STDERR_FILENO, &newline, 1);
		return err == ENOENT ? 127 : 126;
	}
}
