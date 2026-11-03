/* Tester libft (sujet v19.3). Compare avec la libc quand la fonction existe. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#if __has_include(<valgrind/valgrind.h>)
# include <valgrind/valgrind.h>
#else
# define RUNNING_ON_VALGRIND 0
# define VALGRIND_COUNT_ERRORS 0
# define VALGRIND_PRINTF(...) 0
#endif

/* glibc < 2.38 (ex: Ubuntu 22.04) n'a pas strlcpy/strlcat: reference BSD a la place */
#if defined(__GLIBC__) && !__GLIBC_PREREQ(2, 38)
static size_t	strlcpy(char *dst, const char *src, size_t size)
{
	size_t	len = strlen(src);

	if (size)
	{
		size_t n = len < size - 1 ? len : size - 1;
		memcpy(dst, src, n);
		dst[n] = 0;
	}
	return (len);
}

static size_t	strlcat(char *dst, const char *src, size_t size)
{
	size_t	dl = strnlen(dst, size);

	if (dl == size)
		return (size + strlen(src));
	return (dl + strlcpy(dst + dl, src, size - dl));
}
#endif

/* prototypes du sujet: le tester compile meme si libft.h est incomplet */
typedef struct s_list
{
	void			*content;
	struct s_list	*next;
}	t_list;

int		ft_isalpha(int c);
int		ft_isdigit(int c);
int		ft_isalnum(int c);
int		ft_isascii(int c);
int		ft_isprint(int c);
size_t	ft_strlen(const char *s);
void	*ft_memset(void *s, int c, size_t n);
void	ft_bzero(void *s, size_t n);
void	*ft_memcpy(void *dst, const void *src, size_t n);
void	*ft_memmove(void *dst, const void *src, size_t n);
size_t	ft_strlcpy(char *dst, const char *src, size_t size);
size_t	ft_strlcat(char *dst, const char *src, size_t size);
int		ft_toupper(int c);
int		ft_tolower(int c);
char	*ft_strchr(const char *s, int c);
char	*ft_strrchr(const char *s, int c);
int		ft_strncmp(const char *s1, const char *s2, size_t n);
void	*ft_memchr(const void *s, int c, size_t n);
int		ft_memcmp(const void *s1, const void *s2, size_t n);
char	*ft_strnstr(const char *big, const char *little, size_t len);
int		ft_atoi(const char *nptr);
void	*ft_calloc(size_t nmemb, size_t size);
char	*ft_strdup(const char *s);
char	*ft_substr(char const *s, unsigned int start, size_t len);
char	*ft_strjoin(char const *s1, char const *s2);
char	*ft_strtrim(char const *s1, char const *set);
char	**ft_split(char const *s, char c);
char	*ft_itoa(int n);
char	*ft_strmapi(char const *s, char (*f)(unsigned int, char));
void	ft_striteri(char *s, void (*f)(unsigned int, char *));
void	ft_putchar_fd(char c, int fd);
void	ft_putstr_fd(char *s, int fd);
void	ft_putendl_fd(char *s, int fd);
void	ft_putnbr_fd(int n, int fd);
t_list	*ft_lstnew(void *content);
void	ft_lstadd_front(t_list **lst, t_list *new);
unsigned int	ft_lstsize(t_list *lst);
t_list	*ft_lstlast(t_list *lst);
void	ft_lstadd_back(t_list **lst, t_list *new);
void	ft_lstdelone(t_list *lst, void (*del)(void *));
void	ft_lstclear(t_list **lst, void (*del)(void *));
void	ft_lstiter(t_list *lst, void (*f)(void *));
t_list	*ft_lstmap(t_list *lst, void *(*f)(void *), void (*del)(void *));

#define FUNCS \
	X(ft_isalpha) X(ft_isdigit) X(ft_isalnum) X(ft_isascii) X(ft_isprint) \
	X(ft_strlen) X(ft_memset) X(ft_bzero) X(ft_memcpy) X(ft_memmove) \
	X(ft_strlcpy) X(ft_strlcat) X(ft_toupper) X(ft_tolower) X(ft_strchr) \
	X(ft_strrchr) X(ft_strncmp) X(ft_memchr) X(ft_memcmp) X(ft_strnstr) \
	X(ft_atoi) X(ft_calloc) X(ft_strdup) \
	X(ft_substr) X(ft_strjoin) X(ft_strtrim) X(ft_split) X(ft_itoa) \
	X(ft_strmapi) X(ft_striteri) X(ft_putchar_fd) X(ft_putstr_fd) \
	X(ft_putendl_fd) X(ft_putnbr_fd) \
	X(ft_lstnew) X(ft_lstadd_front) X(ft_lstsize) X(ft_lstlast) \
	X(ft_lstadd_back) X(ft_lstdelone) X(ft_lstclear) X(ft_lstiter) \
	X(ft_lstmap)

/* weak: une fonction absente donne NULL au lieu d'une erreur de link */
#define DO_PRAGMA(x) _Pragma(#x)
#define X(f) DO_PRAGMA(weak f)
FUNCS
#undef X

static const char	*g_fn;
static char			g_cur[2048];
static FILE			*g_log;
static sigjmp_buf	g_jmp;
static int			g_case;		/* index du cas en cours (1..n) */
static int			g_case_ko;
static int			g_failed;
static int			g_crashes;
static int			g_replay;	/* dernier cas qui a crashe: les cas <= sont deja logges */
static int			g_skip[256];
static int			g_nskip;

static void	logmsg(const char *fmt, ...)
{
	va_list	ap;
	char	path[256];

	if (getenv("TESTER_NOLOG") || (g_case && g_case <= g_replay))
		return ;
	if (!g_log)
	{
		snprintf(path, sizeof path, "errors/%s.txt", g_fn);
		g_log = fopen(path, "a");
		if (!g_log)
			return ;
		setvbuf(g_log, NULL, _IONBF, 0);
	}
	va_start(ap, fmt);
	vfprintf(g_log, fmt, ap);
	va_end(ap);
}

static char			g_detail[4096];
static int			g_active;	/* le cas courant a ete execute (pas saute) */

/* mode verbeux (sardine <fonction>-test): chaque entree testee est affichee sur stderr */
static int	verbose(void)
{
	return (getenv("TESTER_VERBOSE") && g_case > g_replay);
}

static void	detail(const char *fmt, ...)
{
	va_list	ap;
	size_t	l = strlen(g_detail);

	va_start(ap, fmt);
	vsnprintf(g_detail + l, sizeof g_detail - l, fmt, ap);
	va_end(ap);
}

static unsigned int	g_vg_errs;
static int			g_vg_tagged;

/* sous valgrind: ecrit dans son log l'entree du cas qui vient de provoquer une erreur */
static void	vg_tag(void)
{
	unsigned int	e;

	if (!RUNNING_ON_VALGRIND)
		return ;
	e = VALGRIND_COUNT_ERRORS;
	if (e > g_vg_errs && g_vg_tagged++ < 5)
		VALGRIND_PRINTF("[VALGRIND] erreur pendant le test: %s\n", g_cur);
	g_vg_errs = e;
}

static void	case_end(void)
{
	if (g_active)
		vg_tag();
	if (g_active && verbose())
	{
		fprintf(stderr, "      %s %s\n", g_case_ko ? "\e[31m✘\e[0m" : "\e[32m✔\e[0m", g_cur);
		if (g_case_ko)
			fprintf(stderr, "\e[2m%s\e[0m", g_detail);
	}
	g_detail[0] = 0;
	g_active = 0;
	g_failed += g_case_ko;
	g_case_ko = 0;
}

/* demarre un cas; renvoie 1 s'il a deja crashe et doit etre saute */
static int	case_begin(const char *fmt, ...)
{
	va_list	ap;

	case_end();
	g_case++;
	for (int i = 0; i < g_nskip; i++)
		if (g_skip[i] == g_case)
			return (1);
	va_start(ap, fmt);
	vsnprintf(g_cur, sizeof g_cur, fmt, ap);
	va_end(ap);
	g_active = 1;
	alarm(getenv("TESTER_NOLOG") ? 20 : 3);
	return (0);
}

/* chaque T() doit etre dans une boucle (ou ONCE) pour pouvoir sauter un cas qui a crashe */
#define ONCE for (int once_ = 1; once_; once_ = 0)
#define T(...) if (case_begin(__VA_ARGS__)) continue
#define CHECK(cond, ...) do { if (!(cond)) { g_case_ko = 1; \
	logmsg("[KO] %s   (%s:%d)\n     ", g_cur, __FILE__, __LINE__); \
	logmsg(__VA_ARGS__); logmsg("\n\n"); \
	if (verbose()) { detail("          "); detail(__VA_ARGS__); detail("\n"); } } } while (0)

static void	on_signal(int sig)
{
	if (sig == SIGALRM)
		logmsg("[TIMEOUT] boucle infinie ? pendant le test: %s\n\n", g_cur);
	else
		logmsg("[CRASH] %s pendant le test: %s\n\n", strsignal(sig), g_cur);
	vg_tag();
	if (verbose())
		fprintf(stderr, "      \e[31m💥 %s\e[0m %s\n", sig == SIGALRM ? "TIMEOUT" : strsignal(sig), g_cur);
	g_active = 0;
	g_detail[0] = 0;
	g_crashes++;
	g_replay = g_case;
	if (g_case == 0)
		g_nskip = 256;	/* crash hors d'un cas: on abandonne */
	else if (g_nskip < 256)
		g_skip[g_nskip++] = g_case;
	siglongjmp(g_jmp, 1);
}

/* ---------- malloc/free interceptes (-Wl,--wrap) pour le mode strict ---------- */

void		*__real_malloc(size_t n);
void		__real_free(void *p);
static int	g_track;	/* suivi actif pendant l'appel a la fonction testee */
static int	g_fail_at;	/* numero du malloc qui doit renvoyer NULL (0 = aucun) */
static int	g_nalloc;
static int	g_in_f;		/* malloc fait par une fonction du tester (f de lstmap): jamais en echec */
static struct { void *p; size_t n; }	g_live[8192];
static int	g_nlive;

void	*__wrap_malloc(size_t n)
{
	void	*p;

	if (g_track && !g_in_f && ++g_nalloc == g_fail_at)
		return (NULL);
	p = __real_malloc(n);
	if (g_track && p && g_nlive < 8192)
	{
		g_live[g_nlive].p = p;
		g_live[g_nlive++].n = n;
	}
	return (p);
}

void	__wrap_free(void *p)
{
	for (int i = 0; g_track && i < g_nlive; i++)
		if (g_live[i].p == p)
		{
			g_live[i] = g_live[--g_nlive];
			break ;
		}
	__real_free(p);
}

/* ---------- helpers d'affichage ---------- */

/* rend une chaine lisible (echappee, entre guillemets), buffers tournants */
static const char	*esc_n(const char *s, size_t n)
{
	static char	bufs[8][512];
	static int	k;
	char		*b;
	size_t		j;

	if (!s)
		return ("(null)");
	b = bufs[k++ % 8];
	j = 0;
	b[j++] = '"';
	for (size_t i = 0; i < n && j < 500; i++)
	{
		unsigned char c = s[i];
		if (c == '\n') j += sprintf(b + j, "\\n");
		else if (c == '\t') j += sprintf(b + j, "\\t");
		else if (c == '"') j += sprintf(b + j, "\\\"");
		else if (c == '\\') j += sprintf(b + j, "\\\\");
		else if (!isprint(c)) j += sprintf(b + j, "\\x%02x", c);
		else b[j++] = c;
	}
	b[j++] = '"';
	b[j] = 0;
	return (b);
}

static const char	*esc(const char *s)
{
	return (s ? esc_n(s, strlen(s)) : "(null)");
}

static int	sign(int x)
{
	return ((x > 0) - (x < 0));
}

/* sortie d'un *_fd capturee dans un fichier temporaire */
static FILE	*g_cap;

static int	cap_open(void)
{
	g_cap = tmpfile();
	return (fileno(g_cap));
}

static const char	*cap_read(size_t *len)
{
	static char	buf[8192];
	ssize_t		n;

	lseek(fileno(g_cap), 0, SEEK_SET);
	n = read(fileno(g_cap), buf, sizeof buf - 1);
	fclose(g_cap);
	if (n < 0)
		n = 0;
	buf[n] = 0;
	*len = n;
	return (buf);
}

static const char	*g_strs[] = {"", "a", "Hello World", "bonjour\tle\nmonde",
	"\x80\xff\x7f", "1234567890abcdefghijklmnopqrstuvwxyz", "  espaces  ", NULL};

/* ---------- Partie 1 ---------- */

#define CTYPE_TEST(ft, real, strict) \
	for (int c = -1; c <= 255; c++) { T(#ft "(%d)", c); \
		int e = strict ? (real(c) ? 1 : 0) : real(c); int g = ft(c); \
		CHECK(g == e, "attendu: %d   obtenu: %d", e, g); }

static void	t_ft_isalpha(void) { CTYPE_TEST(ft_isalpha, isalpha, 1) }
static void	t_ft_isdigit(void) { CTYPE_TEST(ft_isdigit, isdigit, 1) }
static void	t_ft_isalnum(void) { CTYPE_TEST(ft_isalnum, isalnum, 1) }
static void	t_ft_isascii(void) { CTYPE_TEST(ft_isascii, isascii, 1) }
static void	t_ft_isprint(void) { CTYPE_TEST(ft_isprint, isprint, 1) }
static void	t_ft_toupper(void) { CTYPE_TEST(ft_toupper, toupper, 0) }
static void	t_ft_tolower(void) { CTYPE_TEST(ft_tolower, tolower, 0) }

static void	t_ft_strlen(void)
{
	char	big[10000];

	for (int i = 0; g_strs[i]; i++)
	{
		T("ft_strlen(%s)", esc(g_strs[i]));
		CHECK(ft_strlen(g_strs[i]) == strlen(g_strs[i]), "attendu: %zu   obtenu: %zu",
			strlen(g_strs[i]), ft_strlen(g_strs[i]));
	}
	memset(big, 'x', sizeof big - 1);
	big[sizeof big - 1] = 0;
	ONCE {
		T("ft_strlen(chaine de 9999 'x')");
		CHECK(ft_strlen(big) == 9999, "attendu: 9999   obtenu: %zu", ft_strlen(big));
	}
}

static void	t_ft_memset(void)
{
	int		cs[] = {0, 'A', 255, -1, 256 + 'B', 127};
	size_t	ns[] = {0, 1, 5, 31, 64};

	for (size_t i = 0; i < sizeof cs / sizeof *cs; i++)
		for (size_t j = 0; j < sizeof ns / sizeof *ns; j++)
		{
			char a[80], b[80];
			memset(a, '.', 80);
			memset(b, '.', 80);
			T("ft_memset(buf + 3, %d, %zu)", cs[i], ns[j]);
			void *r = ft_memset(a + 3, cs[i], ns[j]);
			memset(b + 3, cs[i], ns[j]);
			CHECK(r == a + 3, "retour attendu: buf + 3   obtenu: %p (buf = %p)", r, (void *)a);
			CHECK(!memcmp(a, b, 80), "buffer attendu: %s\n     buffer obtenu : %s",
				esc_n(b, 80), esc_n(a, 80));
		}
}

static void	t_ft_bzero(void)
{
	size_t	ns[] = {0, 1, 5, 31, 64};

	for (size_t j = 0; j < sizeof ns / sizeof *ns; j++)
	{
		char a[80], b[80];
		memset(a, 'z', 80);
		memset(b, 'z', 80);
		T("ft_bzero(buf + 3, %zu)", ns[j]);
		ft_bzero(a + 3, ns[j]);
		bzero(b + 3, ns[j]);
		CHECK(!memcmp(a, b, 80), "buffer attendu: %s\n     buffer obtenu : %s",
			esc_n(b, 80), esc_n(a, 80));
	}
}

static void	t_ft_memcpy(void)
{
	const char	src[] = "abc\0def\xffghijklmnopqrstuvwxyz0123456789";
	size_t		ns[] = {0, 1, 4, 8, sizeof src};

	for (size_t j = 0; j < sizeof ns / sizeof *ns; j++)
	{
		char a[64], b[64];
		memset(a, '.', 64);
		memset(b, '.', 64);
		T("ft_memcpy(dst, %s, %zu)", esc_n(src, sizeof src), ns[j]);
		void *r = ft_memcpy(a, src, ns[j]);
		memcpy(b, src, ns[j]);
		CHECK(r == a, "retour attendu: dst (%p)   obtenu: %p", (void *)a, r);
		CHECK(!memcmp(a, b, 64), "dst attendu: %s\n     dst obtenu : %s",
			esc_n(b, 64), esc_n(a, 64));
	}
}

static void	t_ft_memmove(void)
{
	struct { int d; int s; size_t n; } c[] = {
		{0, 5, 10}, {5, 0, 10}, {0, 1, 20}, {1, 0, 20}, {3, 3, 5}, {2, 7, 0},
		{0, 10, 10}, {10, 0, 10}};

	for (size_t i = 0; i < sizeof c / sizeof *c; i++)
	{
		char a[40], b[40];
		for (int k = 0; k < 40; k++)
			a[k] = b[k] = 'A' + k % 26;
		T("ft_memmove(buf + %d, buf + %d, %zu)  (zones qui se chevauchent)", c[i].d, c[i].s, c[i].n);
		void *r = ft_memmove(a + c[i].d, a + c[i].s, c[i].n);
		memmove(b + c[i].d, b + c[i].s, c[i].n);
		CHECK(r == a + c[i].d, "retour attendu: buf + %d   obtenu: %p (buf = %p)", c[i].d, r, (void *)a);
		CHECK(!memcmp(a, b, 40), "buffer attendu: %s\n     buffer obtenu : %s",
			esc_n(b, 40), esc_n(a, 40));
	}
	ONCE
	{
		const char src[] = "non chevauchant";
		char a[32] = {0}, b[32] = {0};
		T("ft_memmove(dst, %s, %zu)", esc(src), sizeof src);
		ft_memmove(a, src, sizeof src);
		memmove(b, src, sizeof src);
		CHECK(!memcmp(a, b, 32), "attendu: %s   obtenu: %s", esc(b), esc(a));
	}
}

static void	t_ft_strlcpy(void)
{
	for (int i = 0; g_strs[i]; i++)
		for (size_t sz = 0; sz <= strlen(g_strs[i]) + 2 && sz < 40; sz++)
		{
			char a[48], b[48];
			memset(a, 'X', 48);
			memset(b, 'X', 48);
			T("ft_strlcpy(dst, %s, %zu)", esc(g_strs[i]), sz);
			size_t r = ft_strlcpy(a, g_strs[i], sz);
			size_t e = strlcpy(b, g_strs[i], sz);
			CHECK(r == e, "retour attendu: %zu   obtenu: %zu", e, r);
			CHECK(!memcmp(a, b, 48), "dst attendu: %s\n     dst obtenu : %s",
				esc_n(b, 48), esc_n(a, 48));
		}
}

static void	t_ft_strlcat(void)
{
	const char	*dsts[] = {"", "abc", "Hello ", "1234567890", NULL};

	for (int d = 0; dsts[d]; d++)
		for (int i = 0; g_strs[i]; i++)
			for (size_t sz = 0; sz < 24; sz++)
			{
				char a[64], b[64];
				memset(a, 'X', 64);
				memset(b, 'X', 64);
				strcpy(a, dsts[d]);
				strcpy(b, dsts[d]);
				T("ft_strlcat(dst = %s, %s, %zu)", esc(dsts[d]), esc(g_strs[i]), sz);
				size_t r = ft_strlcat(a, g_strs[i], sz);
				size_t e = strlcat(b, g_strs[i], sz);
				CHECK(r == e, "retour attendu: %zu   obtenu: %zu", e, r);
				CHECK(!memcmp(a, b, 64), "dst attendu: %s\n     dst obtenu : %s",
					esc_n(b, 40), esc_n(a, 40));
			}
	ONCE
	{
		char a[16], b[16];
		memset(a, 'X', 16);
		memset(b, 'X', 16);
		T("ft_strlcat(dst non termine \"XXXXXXXXXXXXXXXX\", \"abc\", 5)");
		size_t r = ft_strlcat(a, "abc", 5);
		size_t e = strlcat(b, "abc", 5);
		CHECK(r == e, "retour attendu: %zu   obtenu: %zu", e, r);
		CHECK(!memcmp(a, b, 16), "dst modifie alors qu'il ne devait pas l'etre: %s", esc_n(a, 16));
	}
}

static const int	g_chars[] = {'o', 'b', 'e', 'z', '\0', ' ', 'b' + 256, 'o' + 512, -1, 0x80};

static void	t_ft_strchr(void)
{
	const char	*s = "bonjour le monde \x80!";

	for (size_t i = 0; i < sizeof g_chars / sizeof *g_chars; i++)
	{
		T("ft_strchr(%s, %d)", esc(s), g_chars[i]);
		char *r = ft_strchr(s, g_chars[i]);
		char *e = strchr(s, g_chars[i]);
		CHECK(r == e, "attendu: %s   obtenu: %s",
			e ? esc(e) : "NULL", r ? esc(r) : "NULL");
	}
	ONCE {
		T("ft_strchr(\"\", 'a')");
		CHECK(ft_strchr("", 'a') == NULL, "attendu: NULL");
	}
}

static void	t_ft_strrchr(void)
{
	const char	*s = "bonjour le monde \x80!";

	for (size_t i = 0; i < sizeof g_chars / sizeof *g_chars; i++)
	{
		T("ft_strrchr(%s, %d)", esc(s), g_chars[i]);
		char *r = ft_strrchr(s, g_chars[i]);
		char *e = strrchr(s, g_chars[i]);
		CHECK(r == e, "attendu: %s   obtenu: %s",
			e ? esc(e) : "NULL", r ? esc(r) : "NULL");
	}
	ONCE {
		T("ft_strrchr(\"\", '\\0')");
		{
			const char *em = "";
			CHECK(ft_strrchr(em, 0) == em, "attendu: pointeur sur le '\\0' final");
		}
	}
}

static void	t_ft_strncmp(void)
{
	const char	*p[][2] = {{"abc", "abc"}, {"abc", "abd"}, {"abd", "abc"},
		{"abc", "ab"}, {"ab", "abc"}, {"", ""}, {"", "a"}, {"a", ""},
		{"\x80", "\x01"}, {"\x01", "\x80"}, {"\xff", "\x00"}, {"abc\0x", "abc\0y"},
		{"test\200", "test\0"}, {"Hello", "hello"}};
	size_t		ns[] = {0, 1, 2, 3, 4, 10, SIZE_MAX};

	for (size_t i = 0; i < sizeof p / sizeof *p; i++)
		for (size_t j = 0; j < sizeof ns / sizeof *ns; j++)
		{
			T("ft_strncmp(%s, %s, %zu)", esc(p[i][0]), esc(p[i][1]), ns[j]);
			int r = ft_strncmp(p[i][0], p[i][1], ns[j]);
			int e = strncmp(p[i][0], p[i][1], ns[j]);
			CHECK(sign(r) == sign(e), "signe attendu: %d   obtenu: %d (valeur %d)", sign(e), sign(r), r);
		}
}

static void	t_ft_memchr(void)
{
	const char	s[] = "abc\0def\xff\x80xyz";
	int			cs[] = {'a', 'd', '\0', 0xff, -1, 0x80, 'z', 'q', 'a' + 256};
	size_t		ns[] = {0, 1, 3, 5, sizeof s};

	for (size_t i = 0; i < sizeof cs / sizeof *cs; i++)
		for (size_t j = 0; j < sizeof ns / sizeof *ns; j++)
		{
			T("ft_memchr(%s, %d, %zu)", esc_n(s, sizeof s), cs[i], ns[j]);
			void *r = ft_memchr(s, cs[i], ns[j]);
			void *e = memchr(s, cs[i], ns[j]);
			CHECK(r == e, "attendu: %s   obtenu: %s",
				e ? "s + n" : "NULL", r ? "non NULL" : "NULL");
			if (r != e && e && r)
				logmsg("     (attendu s + %td, obtenu s + %td)\n\n", (char *)e - s, (char *)r - s);
		}
}

static void	t_ft_memcmp(void)
{
	struct { const char *a; const char *b; size_t n; } c[] = {
		{"abc", "abc", 3}, {"abc", "abd", 3}, {"abd", "abc", 3}, {"abc", "abd", 2},
		{"a\0b", "a\0c", 3}, {"a\0b", "a\0c", 2}, {"\x80", "\x01", 1}, {"\x01", "\xff", 1},
		{"\0\0\0x", "\0\0\0y", 4}, {"xyz", "abc", 0}, {"\xff\xfe", "\xff\xff", 2}};

	for (size_t i = 0; i < sizeof c / sizeof *c; i++)
	{
		T("ft_memcmp(%s, %s, %zu)", esc_n(c[i].a, c[i].n), esc_n(c[i].b, c[i].n), c[i].n);
		int r = ft_memcmp(c[i].a, c[i].b, c[i].n);
		int e = memcmp(c[i].a, c[i].b, c[i].n);
		CHECK(sign(r) == sign(e), "signe attendu: %d   obtenu: %d (valeur %d)", sign(e), sign(r), r);
	}
}

/* strnstr n'existe pas dans la glibc: reference BSD */
static char	*ref_strnstr(const char *h, const char *n, size_t len)
{
	size_t	nl = strlen(n);

	if (!nl)
		return ((char *)h);
	for (size_t i = 0; h[i] && i + nl <= len; i++)
		if (!strncmp(h + i, n, nl))
			return ((char *)h + i);
	return (NULL);
}

static void	t_ft_strnstr(void)
{
	const char	*h[] = {"lorem ipsum dolor sit amet", "aaabcabcd", "", "abc", NULL};
	const char	*n[] = {"ipsum", "dolor", "", "lorem", "amet", "abcd", "aabc", "abc",
		"abcabcd", "z", "lorem ipsum dolor sit amet!", NULL};
	size_t		ls[] = {0, 1, 3, 4, 5, 9, 10, 15, 26, 30, SIZE_MAX};

	for (int i = 0; h[i]; i++)
		for (int j = 0; n[j]; j++)
			for (size_t k = 0; k < sizeof ls / sizeof *ls; k++)
			{
				T("ft_strnstr(%s, %s, %zu)", esc(h[i]), esc(n[j]), ls[k]);
				char *r = ft_strnstr(h[i], n[j], ls[k]);
				char *e = ref_strnstr(h[i], n[j], ls[k]);
				CHECK(r == e, "attendu: %s   obtenu: %s", e ? esc(e) : "NULL", r ? esc(r) : "NULL");
			}
}

static void	t_ft_atoi(void)
{
	const char	*s[] = {"0", "42", "-42", "+42", "   42", "\t\n\v\f\r 42", "  -0",
		"--42", "+-42", "-+42", "++42", "42abc", "4 2", "abc", "", " ", "-",
		"+", "2147483647", "-2147483648", "0000000000042", "-00042", "\e42",
		" \x01 42", "1-2", "  +2147483646", "999999999", "-999999999", NULL};

	for (int i = 0; s[i]; i++)
	{
		T("ft_atoi(%s)", esc(s[i]));
		CHECK(ft_atoi(s[i]) == atoi(s[i]), "attendu: %d   obtenu: %d", atoi(s[i]), ft_atoi(s[i]));
	}
	srand(42);
	for (int i = 0; i < 500; i++)
	{
		char	buf[32];
		int		v = rand() - rand();
		snprintf(buf, sizeof buf, "%d", v);
		T("ft_atoi(%s)", esc(buf));
		CHECK(ft_atoi(buf) == v, "attendu: %d   obtenu: %d", v, ft_atoi(buf));
	}
}

static void	t_ft_calloc(void)
{
	size_t	c[][2] = {{10, sizeof(int)}, {1, 1}, {123, 7}, {0, 0}, {0, 5}, {5, 0}};

	for (size_t i = 0; i < sizeof c / sizeof *c; i++)
	{
		T("ft_calloc(%zu, %zu)", c[i][0], c[i][1]);
		unsigned char *p = ft_calloc(c[i][0], c[i][1]);
		CHECK(p != NULL, "attendu: un pointeur non NULL (liberable avec free)   obtenu: NULL");
		if (!p)
			continue ;
		size_t k = 0;
		while (k < c[i][0] * c[i][1] && p[k] == 0)
			k++;
		CHECK(k == c[i][0] * c[i][1], "memoire non mise a zero a l'index %zu (valeur %d)", k, p[k]);
		free(p);
	}
	ONCE {
		T("ft_calloc(SIZE_MAX, SIZE_MAX)  (depassement)");
		void *p = ft_calloc(SIZE_MAX, SIZE_MAX);
		CHECK(p == NULL, "attendu: NULL (comme calloc)   obtenu: %p", p);
		free(p);
	}
	ONCE {
		T("ft_calloc(SIZE_MAX / 2 + 2, 2)  (depassement: le produit deborde en 2)");
		void *p = ft_calloc(SIZE_MAX / 2 + 2, 2);
		CHECK(p == NULL, "attendu: NULL (comme calloc)   obtenu: %p", p);
		free(p);
	}
}

static void	t_ft_strdup(void)
{
	for (int i = 0; g_strs[i]; i++)
	{
		T("ft_strdup(%s)", esc(g_strs[i]));
		char *r = ft_strdup(g_strs[i]);
		CHECK(r && r != g_strs[i] && !strcmp(r, g_strs[i]), "attendu: copie %s   obtenu: %s",
			esc(g_strs[i]), esc(r));
		free(r);
	}
}

/* ---------- Partie 2 ---------- */

static void	t_ft_substr(void)
{
	struct { const char *s; unsigned int st; size_t len; const char *e; } c[] = {
		{"Hello World", 0, 5, "Hello"}, {"Hello World", 6, 5, "World"},
		{"Hello World", 6, 100, "World"}, {"Hello World", 3, SIZE_MAX, "lo World"},
		{"Hello World", 11, 5, ""}, {"Hello World", 42, 3, ""},
		{"Hello World", 0, 0, ""}, {"", 0, 5, ""}, {"", 1, 1, ""},
		{"Hello World", 10, 1, "d"}, {"Hello World", UINT_MAX, 5, ""}};

	for (size_t i = 0; i < sizeof c / sizeof *c; i++)
	{
		T("ft_substr(%s, %u, %zu)", esc(c[i].s), c[i].st, c[i].len);
		char *r = ft_substr(c[i].s, c[i].st, c[i].len);
		CHECK(r && !strcmp(r, c[i].e), "attendu: %s   obtenu: %s", esc(c[i].e), esc(r));
		free(r);
	}
}

static void	t_ft_strjoin(void)
{
	const char	*c[][3] = {{"abc", "def", "abcdef"}, {"", "", ""}, {"abc", "", "abc"},
		{"", "def", "def"}, {"Hello ", "World!", "Hello World!"}};

	for (size_t i = 0; i < sizeof c / sizeof *c; i++)
	{
		T("ft_strjoin(%s, %s)", esc(c[i][0]), esc(c[i][1]));
		char *r = ft_strjoin(c[i][0], c[i][1]);
		CHECK(r && !strcmp(r, c[i][2]), "attendu: %s   obtenu: %s", esc(c[i][2]), esc(r));
		free(r);
	}
}

static void	t_ft_strtrim(void)
{
	const char	*c[][3] = {{"  hello  ", " ", "hello"}, {"xxhixx", "x", "hi"},
		{"abc", "", "abc"}, {"aaaa", "a", ""}, {"", "abc", ""}, {"", "", ""},
		{" \t a b \t ", " \t", "a b"}, {"abcba", "ab", "c"}, {"hello", "xyz", "hello"},
		{"lorem ipsum", "lm", "orem ipsu"}, {"   xxx   xxx", " x", ""}};

	for (size_t i = 0; i < sizeof c / sizeof *c; i++)
	{
		T("ft_strtrim(%s, %s)", esc(c[i][0]), esc(c[i][1]));
		char *r = ft_strtrim(c[i][0], c[i][1]);
		CHECK(r && !strcmp(r, c[i][2]), "attendu: %s   obtenu: %s", esc(c[i][2]), esc(r));
		free(r);
	}
}

static void	t_ft_split(void)
{
	struct { const char *s; char c; const char *e[8]; } c[] = {
		{"  hello  world  ", ' ', {"hello", "world", NULL}},
		{"hello", ' ', {"hello", NULL}},
		{"", ' ', {NULL}},
		{"     ", ' ', {NULL}},
		{"a,b,,c,", ',', {"a", "b", "c", NULL}},
		{",,,a,,,", ',', {"a", NULL}},
		{"hello world", '\0', {"hello world", NULL}},
		{"lorem ipsum dolor sit amet", ' ', {"lorem", "ipsum", "dolor", "sit", "amet", NULL}},
		{"xxaxxbxx", 'x', {"a", "b", NULL}}};

	for (size_t i = 0; i < sizeof c / sizeof *c; i++)
	{
		char	got[512] = "", exp[512] = "";
		T("ft_split(%s, %s)", esc(c[i].s), esc_n(&c[i].c, 1));
		char **r = ft_split(c[i].s, c[i].c);
		for (int k = 0; c[i].e[k]; k++)
			snprintf(exp + strlen(exp), sizeof exp - strlen(exp), "%s, ", esc(c[i].e[k]));
		strcat(exp, "NULL");
		if (!r)
		{
			CHECK(0, "attendu: {%s}   obtenu: NULL", exp);
			continue ;
		}
		int ok = 1, k = 0;
		for (; r[k] && k < 16; k++)
		{
			snprintf(got + strlen(got), sizeof got - strlen(got), "%s, ", esc(r[k]));
			if (!c[i].e[k] || strcmp(r[k], c[i].e[k]))
				ok = 0;
		}
		strcat(got, r[k] ? "..." : "NULL");
		if (c[i].e[k] || r[k])
			ok = 0;
		CHECK(ok, "attendu: {%s}\n     obtenu : {%s}", exp, got);
		for (k = 0; r[k] && k < 16; k++)
			free(r[k]);
		free(r);
	}
}

static void	t_ft_itoa(void)
{
	int	v[] = {0, 1, -1, 9, 10, -10, 42, -42, 100, INT_MAX, INT_MIN, INT_MAX - 1,
		INT_MIN + 1, 1000000000, -1000000000};

	for (size_t i = 0; i < sizeof v / sizeof *v + 300; i++)
	{
		char	e[16];
		int		n = i < sizeof v / sizeof *v ? v[i] : rand() - rand();
		snprintf(e, sizeof e, "%d", n);
		T("ft_itoa(%d)", n);
		char *r = ft_itoa(n);
		CHECK(r && !strcmp(r, e), "attendu: %s   obtenu: %s", esc(e), esc(r));
		free(r);
	}
}

static char	map_f(unsigned int i, char c)
{
	return (c + i % 3);
}

static void	t_ft_strmapi(void)
{
	const char	*s[] = {"", "abc", "Hello World", "aaaaaaaa", NULL};

	for (int i = 0; s[i]; i++)
	{
		char e[64];
		size_t k;
		for (k = 0; s[i][k]; k++)
			e[k] = map_f(k, s[i][k]);
		e[k] = 0;
		T("ft_strmapi(%s, f)   avec f(i, c) = c + i %% 3", esc(s[i]));
		char *r = ft_strmapi(s[i], map_f);
		CHECK(r && r != s[i] && !strcmp(r, e), "attendu: %s   obtenu: %s", esc(e), esc(r));
		free(r);
	}
}

static unsigned int	g_iter_calls;

static void	iteri_f(unsigned int i, char *c)
{
	g_iter_calls++;
	if (i % 2 == 0)
		*c = toupper(*c);
}

static void	t_ft_striteri(void)
{
	const char	*s[][2] = {{"", ""}, {"abc", "AbC"}, {"hello world", "HeLlO WoRlD"}, {NULL, NULL}};

	for (int i = 0; s[i][0]; i++)
	{
		char buf[64];
		strcpy(buf, s[i][0]);
		g_iter_calls = 0;
		T("ft_striteri(%s, f)   avec f qui met en majuscule les index pairs", esc(s[i][0]));
		ft_striteri(buf, iteri_f);
		CHECK(!strcmp(buf, s[i][1]), "attendu: %s   obtenu: %s", esc(s[i][1]), esc(buf));
		CHECK(g_iter_calls == strlen(s[i][0]), "f appelee %u fois au lieu de %zu",
			g_iter_calls, strlen(s[i][0]));
	}
}

static void	t_ft_putchar_fd(void)
{
	for (int c = 0; c < 256; c++)
	{
		size_t	len;
		int		fd = cap_open();
		T("ft_putchar_fd(%d, fd)", c);
		ft_putchar_fd((char)c, fd);
		const char *o = cap_read(&len);
		CHECK(len == 1 && (unsigned char)o[0] == c, "attendu: 1 octet %s   obtenu: %zu octet(s) %s",
			esc_n((char[]){(char)c}, 1), len, esc_n(o, len));
	}
}

static void	t_ft_putstr_fd(void)
{
	for (int i = 0; g_strs[i]; i++)
	{
		size_t	len;
		int		fd = cap_open();
		T("ft_putstr_fd(%s, fd)", esc(g_strs[i]));
		ft_putstr_fd((char *)g_strs[i], fd);
		const char *o = cap_read(&len);
		CHECK(len == strlen(g_strs[i]) && !memcmp(o, g_strs[i], len),
			"attendu: %s   obtenu: %s", esc(g_strs[i]), esc_n(o, len));
	}
}

static void	t_ft_putendl_fd(void)
{
	for (int i = 0; g_strs[i]; i++)
	{
		char	e[128];
		size_t	len;
		int		fd = cap_open();
		snprintf(e, sizeof e, "%s\n", g_strs[i]);
		T("ft_putendl_fd(%s, fd)", esc(g_strs[i]));
		ft_putendl_fd((char *)g_strs[i], fd);
		const char *o = cap_read(&len);
		CHECK(len == strlen(e) && !memcmp(o, e, len), "attendu: %s   obtenu: %s", esc(e), esc_n(o, len));
	}
}

static void	t_ft_putnbr_fd(void)
{
	int	v[] = {0, 1, -1, 9, 10, -10, 42, -42, INT_MAX, INT_MIN, INT_MIN + 1, 1000000000};

	for (size_t i = 0; i < sizeof v / sizeof *v + 300; i++)
	{
		char	e[16];
		size_t	len;
		int		n = i < sizeof v / sizeof *v ? v[i] : rand() - rand();
		int		fd = cap_open();
		snprintf(e, sizeof e, "%d", n);
		T("ft_putnbr_fd(%d, fd)", n);
		ft_putnbr_fd(n, fd);
		const char *o = cap_read(&len);
		CHECK(len == strlen(e) && !memcmp(o, e, len), "attendu: %s   obtenu: %s", esc(e), esc_n(o, len));
	}
}

/* ---------- Partie 3 : listes ---------- */

static t_list	*node(void *content)
{
	t_list	*n = malloc(sizeof *n);

	n->content = content;
	n->next = NULL;
	return (n);
}

static t_list	*make_list(int n)
{
	t_list	*head = NULL;

	while (n > 0)
	{
		int *v = malloc(sizeof *v);
		*v = n--;
		t_list *x = node(v);
		x->next = head;
		head = x;
	}
	return (head);
}

static void	free_list(t_list *l)
{
	while (l)
	{
		t_list *n = l->next;
		free(l->content);
		free(l);
		l = n;
	}
}

static const char	*list_str(t_list *l)
{
	static char	b[512];
	int			k = 0;

	b[0] = 0;
	while (l && k++ < 20)
	{
		snprintf(b + strlen(b), sizeof b - strlen(b), "%d -> ", l->content ? *(int *)l->content : -999);
		l = l->next;
	}
	strcat(b, l ? "..." : "NULL");
	return (b);
}

static int	g_del;

static void	del_count(void *p)
{
	g_del++;
	free(p);
}

static void	t_ft_lstnew(void)
{
	int		x = 42;
	t_list	*n;

	ONCE {
		T("ft_lstnew(&x)");
		n = ft_lstnew(&x);
		CHECK(n && n->content == &x && n->next == NULL, "attendu: content = &x, next = NULL");
		free(n);
	}
	ONCE {
		T("ft_lstnew(NULL)");
		n = ft_lstnew(NULL);
		CHECK(n && n->content == NULL && n->next == NULL, "attendu: content = NULL, next = NULL");
		free(n);
	}
}

static void	t_ft_lstadd_front(void)
{
	t_list	*l = NULL;
	int		*a = malloc(sizeof *a), *b = malloc(sizeof *b);

	*a = 2;
	*b = 1;
	ONCE {
		T("ft_lstadd_front(&lst, new) sur une liste vide");
		ft_lstadd_front(&l, node(a));
		CHECK(l && l->content == a && !l->next, "attendu: 2 -> NULL   obtenu: %s", list_str(l));
	}
	ONCE {
		T("ft_lstadd_front(&lst, new) sur 2 -> NULL");
		ft_lstadd_front(&l, node(b));
		CHECK(l && l->content == b && l->next && l->next->content == a && !l->next->next,
			"attendu: 1 -> 2 -> NULL   obtenu: %s", list_str(l));
		free_list(l);
	}
}

static void	t_ft_lstsize(void)
{
	for (int n = 0; n < 5; n++)
	{
		t_list *l = make_list(n);
		T("ft_lstsize(liste de %d elements)", n);
		CHECK((long)ft_lstsize(l) == n, "attendu: %d   obtenu: %ld", n, (long)ft_lstsize(l));
		free_list(l);
	}
}

static void	t_ft_lstlast(void)
{
	ONCE {
		T("ft_lstlast(NULL)");
		CHECK(ft_lstlast(NULL) == NULL, "attendu: NULL");
		for (int n = 1; n < 5; n++)
		{
			t_list *l = make_list(n), *last = l;
			while (last->next)
				last = last->next;
			T("ft_lstlast(%s)", list_str(l));
			CHECK(ft_lstlast(l) == last, "attendu: le noeud %d", n);
			free_list(l);
		}
	}
}

static void	t_ft_lstadd_back(void)
{
	t_list	*l = NULL;
	int		*a = malloc(sizeof *a), *b = malloc(sizeof *b), *c = malloc(sizeof *c);

	*a = 1;
	*b = 2;
	*c = 3;
	ONCE {
		T("ft_lstadd_back(&lst, new) sur une liste vide");
		ft_lstadd_back(&l, node(a));
		CHECK(l && l->content == a && !l->next, "attendu: 1 -> NULL   obtenu: %s", list_str(l));
	}
	ONCE {
		T("ft_lstadd_back(&lst, new) sur 1 -> NULL puis 1 -> 2 -> NULL");
		ft_lstadd_back(&l, node(b));
		ft_lstadd_back(&l, node(c));
		CHECK(!strcmp(list_str(l), "1 -> 2 -> 3 -> NULL"), "attendu: 1 -> 2 -> 3 -> NULL   obtenu: %s",
			list_str(l));
		free_list(l);
	}
}

static void	t_ft_lstdelone(void)
{
	t_list	*l = make_list(2), *second = l->next;

	g_del = 0;
	ONCE {
		T("ft_lstdelone(1er noeud de 1 -> 2 -> NULL, del)");
		ft_lstdelone(l, del_count);
		CHECK(g_del == 1, "del appelee %d fois au lieu de 1", g_del);
		CHECK(*(int *)second->content == 2, "le noeud suivant a ete modifie/libere");
		free_list(second);
	}
}

static void	t_ft_lstclear(void)
{
	t_list	*l = make_list(4);

	g_del = 0;
	ONCE {
		T("ft_lstclear(&lst, del) sur 1 -> 2 -> 3 -> 4 -> NULL");
		ft_lstclear(&l, del_count);
		CHECK(g_del == 4, "del appelee %d fois au lieu de 4", g_del);
		CHECK(l == NULL, "*lst doit valoir NULL apres ft_lstclear");
		g_del = 0;
	}
	ONCE {
		T("ft_lstclear(&lst, del) sur une liste vide");
		ft_lstclear(&l, del_count);
		CHECK(g_del == 0 && l == NULL, "del appelee %d fois au lieu de 0", g_del);
	}
}

static void	iter_f(void *p)
{
	*(int *)p *= 10;
}

static void	t_ft_lstiter(void)
{
	t_list	*l = make_list(3);

	ONCE {
		T("ft_lstiter(1 -> 2 -> 3 -> NULL, f)   avec f qui multiplie par 10");
		ft_lstiter(l, iter_f);
		CHECK(!strcmp(list_str(l), "10 -> 20 -> 30 -> NULL"), "attendu: 10 -> 20 -> 30 -> NULL   obtenu: %s",
			list_str(l));
		free_list(l);
	}
	ONCE {
		T("ft_lstiter(NULL, f)");
		ft_lstiter(NULL, iter_f);
	}
}

static void	*map_double(void *p)
{
	int	*v;

	g_in_f = 1;
	v = malloc(sizeof *v);
	g_in_f = 0;
	if (v)
		*v = *(int *)p * 2;
	return (v);
}

static void	t_ft_lstmap(void)
{
	t_list	*l = make_list(4), *r;
	char	orig[512];

	g_del = 0;
	ONCE {
		T("ft_lstmap(1 -> 2 -> 3 -> 4 -> NULL, f, del)   avec f qui multiplie par 2");
		r = ft_lstmap(l, map_double, del_count);
		strcpy(orig, list_str(l));
		CHECK(!strcmp(list_str(r), "2 -> 4 -> 6 -> 8 -> NULL"), "attendu: 2 -> 4 -> 6 -> 8 -> NULL   obtenu: %s",
			list_str(r));
		CHECK(!strcmp(orig, "1 -> 2 -> 3 -> 4 -> NULL"), "la liste d'origine a ete modifiee: %s", orig);
		CHECK(r != l, "ft_lstmap doit creer une nouvelle liste");
		CHECK(g_del == 0, "del appelee %d fois alors qu'aucune allocation n'a echoue", g_del);
		free_list(l);
		if (r != l)
			free_list(r);
	}
	ONCE {
		T("ft_lstmap(NULL, f, del)");
		CHECK(ft_lstmap(NULL, map_double, del_count) == NULL, "attendu: NULL");
	}
}

#include "strict.c"

/* ---------- main ---------- */

static const struct { const char *name; void (*test)(void); void *fn; } g_tests[] = {
#define X(f) {#f, t_##f, (void *)&f},
	FUNCS
#undef X
};

int	main(int ac, char **av)
{
	if (ac == 2 && !strcmp(av[1], "--list"))
	{
		for (size_t i = 0; i < sizeof g_tests / sizeof *g_tests; i++)
			printf("%s\n", g_tests[i].name);
		return (0);
	}
	for (size_t i = 0; ac == 2 && i < sizeof g_tests / sizeof *g_tests; i++)
	{
		if (strcmp(av[1], g_tests[i].name))
			continue ;
		g_fn = g_tests[i].name;
		if (!g_tests[i].fn)
		{
			logmsg("[MISSING] %s n'existe pas dans libft.a\n", g_fn);
			return (2);
		}
		signal(SIGSEGV, on_signal);
		signal(SIGBUS, on_signal);
		signal(SIGABRT, on_signal);
		signal(SIGFPE, on_signal);
		signal(SIGALRM, on_signal);
		/* apres un crash on relance la fonction de test en sautant les cas fautifs */
		sigsetjmp(g_jmp, 1);
		g_case = 0;
		g_case_ko = 0;
		g_failed = 0;
		srand(42);
		g_track = 0;
		if (g_nskip < 256)
		{
			g_tests[i].test();
			if (getenv("TESTER_STRICT"))
				run_strict(g_fn);
		}
		case_end();
		alarm(0);
		if (g_log)
			fclose(g_log);
		printf("%d %d %d\n", g_case, g_failed + g_crashes, g_crashes);
		return (g_failed + g_crashes != 0);
	}
	fprintf(stderr, "usage: %s --list | ft_nom_fonction\n", av[0]);
	return (3);
}
