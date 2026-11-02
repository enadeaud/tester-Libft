/* Mode strict (sardine --strict): inclus par tester.c.
 * - malloc qui echoue: chaque malloc de la fonction est mis en echec a tour de role,
 *   elle doit renvoyer NULL sans fuite ni crash
 * - taille exacte des allocations (pas d'octet en trop)
 * - fuzzing contre la libc avec des buffers de taille exacte (valgrind voit les debordements)
 * - grosses entrees (1 Mo, listes de 100000 noeuds) */

static void	track_start(int fail_at)
{
	g_nalloc = 0;
	g_nlive = 0;
	g_fail_at = fail_at;
	g_track = 1;
}

static void	track_stop(void)
{
	g_track = 0;
}

static long	alloc_size(void *p)
{
	for (int i = 0; i < g_nlive; i++)
		if (g_live[i].p == p)
			return ((long)g_live[i].n);
	return (-1);
}

/* copie dans un buffer malloc de exactement n octets (sans '\0' ajoute) */
static char	*exact(const char *s, size_t n)
{
	char	*p = malloc(n ? n : 1);

	memcpy(p, s, n);
	return (p);
}

static char	*rand_str(char *buf, size_t max, const char *alphabet)
{
	size_t	n = rand() % max;
	size_t	al = strlen(alphabet);

	for (size_t i = 0; i < n; i++)
		buf[i] = alphabet[rand() % al];
	buf[n] = 0;
	return (buf);
}

static void	free_split(char **r)
{
	for (int k = 0; r && r[k]; k++)
		free(r[k]);
	free(r);
}

static void	free_str(void *p)
{
	free(p);
}

static void	free_lst(void *p)
{
	free_list(p);
}

static void	free_split_v(void *p)
{
	free_split(p);
}

/* met en echec chaque malloc de call() a tour de role */
static void	fail_inject(const char *desc, void *(*call)(void), void (*cleanup)(void *))
{
	void	*r;
	int		n;

	track_start(0);
	r = call();
	n = g_nalloc;
	track_stop();
	if (r)
		cleanup(r);
	for (int k = 1; k <= n; k++)
	{
		T("%s   avec le malloc n°%d/%d qui renvoie NULL", desc, k, n);
		track_start(k);
		r = call();
		track_stop();
		int leaks = g_nlive;
		CHECK(r == NULL, "attendu: NULL quand malloc echoue   obtenu: %p", r);
		CHECK(r != NULL || leaks == 0, "fuite: %d bloc(s) non libere(s) apres l'echec de malloc", leaks);
		if (r)
			cleanup(r);
		else
			while (g_nlive)
				__real_free(g_live[--g_nlive].p);
	}
}

/* la chaine renvoyee doit etre allouee avec exactement strlen + 1 octets */
#define EXACT_SIZE(desc, expr) ONCE { \
	T("%s   (taille de l'allocation)", desc); \
	track_start(0); char *r_ = (expr); track_stop(); \
	long sz_ = alloc_size(r_); \
	CHECK(r_ && sz_ == (long)strlen(r_) + 1, "attendu: malloc(%zu)   obtenu: malloc(%ld) pour %s", \
		r_ ? strlen(r_) + 1 : 0, sz_, esc(r_)); \
	free(r_); }

/* ---------- partie 1 ---------- */

static void	st_mem(void)
{
	for (int it = 0; it < 2000; it++)
	{
		char	a[600], b[600], src[300];
		size_t	n = rand() % 300, off = rand() % 200;
		int		c = rand() % 512 - 128;

		for (size_t k = 0; k < sizeof src; k++)
			src[k] = rand();
		for (size_t k = 0; k < sizeof a; k++)
			a[k] = b[k] = rand();
		char *s = exact(src, n);
		if (!strcmp(g_fn, "ft_memset"))
		{
			T("fuzz ft_memset(buf + %zu, %d, %zu)", off, c, n);
			ft_memset(a + off, c, n);
			memset(b + off, c, n);
		}
		else if (!strcmp(g_fn, "ft_bzero"))
		{
			T("fuzz ft_bzero(buf + %zu, %zu)", off, n);
			ft_bzero(a + off, n);
			bzero(b + off, n);
		}
		else if (!strcmp(g_fn, "ft_memcpy"))
		{
			T("fuzz ft_memcpy(buf + %zu, src (malloc de %zu octets exactement), %zu)", off, n, n);
			ft_memcpy(a + off, s, n);
			memcpy(b + off, s, n);
		}
		else
		{
			size_t o2 = rand() % 200;
			T("fuzz ft_memmove(buf + %zu, buf + %zu, %zu)", off, o2, n);
			ft_memmove(a + off, a + o2, n);
			memmove(b + off, b + o2, n);
		}
		free(s);
		CHECK(!memcmp(a, b, sizeof a), "buffer different de la libc");
	}
	if (!strcmp(g_fn, "ft_memmove") || !strcmp(g_fn, "ft_memcpy"))
		ONCE
		{
			size_t	n = 1 << 20;
			char	*a = malloc(2 * n), *b = malloc(2 * n);
			for (size_t k = 0; k < 2 * n; k++)
				a[k] = b[k] = k * 7;
			T("%s sur 1 Mo%s", g_fn, !strcmp(g_fn, "ft_memmove") ? " qui se chevauche" : "");
			if (!strcmp(g_fn, "ft_memmove"))
			{
				ft_memmove(a + 3, a, n);
				memmove(b + 3, b, n);
			}
			else
			{
				ft_memcpy(a + n, a, n);
				memcpy(b + n, b, n);
			}
			CHECK(!memcmp(a, b, 2 * n), "resultat different de la libc");
			free(a);
			free(b);
		}
}

static void	st_memchr_cmp(void)
{
	for (int it = 0; it < 2000; it++)
	{
		char	x[64], y[64];
		size_t	n = rand() % 64;

		for (size_t k = 0; k < n; k++)
		{
			x[k] = "ab\0\x80\xff"[rand() % 5];
			y[k] = rand() % 4 ? x[k] : "ab\0\x80\xff"[rand() % 5];
		}
		char *a = exact(x, n), *b = exact(y, n);
		if (!strcmp(g_fn, "ft_memchr"))
		{
			int c = "ab\0\x80\xff"[rand() % 5] + 256 * (rand() % 2);
			T("fuzz ft_memchr(%s (sans '\\0' final), %d, %zu)", esc_n(a, n), c, n);
			CHECK(ft_memchr(a, c, n) == memchr(a, c, n), "resultat different de memchr");
		}
		else
		{
			T("fuzz ft_memcmp(%s, %s, %zu)", esc_n(a, n), esc_n(b, n), n);
			CHECK(sign(ft_memcmp(a, b, n)) == sign(memcmp(a, b, n)), "signe attendu %d obtenu %d",
				sign(memcmp(a, b, n)), sign(ft_memcmp(a, b, n)));
		}
		free(a);
		free(b);
	}
}

static void	st_str(void)
{
	for (int it = 0; it < 2000; it++)
	{
		char	x[64], y[64];

		rand_str(x, 40, "abc \x80\xff");
		rand_str(y, 40, "abc \x80\xff");
		if (rand() % 3 == 0)
			strcpy(y, x);
		char *a = exact(x, strlen(x) + 1), *b = exact(y, strlen(y) + 1);
		if (!strcmp(g_fn, "ft_strlen"))
		{
			T("fuzz ft_strlen(%s)", esc(a));
			CHECK(ft_strlen(a) == strlen(a), "attendu %zu obtenu %zu", strlen(a), ft_strlen(a));
		}
		else if (!strcmp(g_fn, "ft_strchr") || !strcmp(g_fn, "ft_strrchr"))
		{
			int c = "abc \x80\xff"[rand() % 7] + 256 * (rand() % 3);
			int r = !strcmp(g_fn, "ft_strrchr");
			T("fuzz %s(%s, %d)", g_fn, esc(a), c);
			char *got = r ? ft_strrchr(a, c) : ft_strchr(a, c);
			char *exp = r ? strrchr(a, c) : strchr(a, c);
			CHECK(got == exp, "attendu %s obtenu %s", exp ? esc(exp) : "NULL", got ? esc(got) : "NULL");
		}
		else if (!strcmp(g_fn, "ft_strncmp"))
		{
			size_t n = rand() % 3 ? rand() % 50 : SIZE_MAX;
			T("fuzz ft_strncmp(%s, %s, %zu)", esc(a), esc(b), n);
			CHECK(sign(ft_strncmp(a, b, n)) == sign(strncmp(a, b, n)), "signe attendu %d obtenu %d",
				sign(strncmp(a, b, n)), sign(ft_strncmp(a, b, n)));
		}
		else if (!strcmp(g_fn, "ft_strlcpy") || !strcmp(g_fn, "ft_strlcat"))
		{
			char d1[128], d2[128];
			size_t sz = rand() % 70;
			int cat = !strcmp(g_fn, "ft_strlcat");
			memset(d1, 'X', sizeof d1);
			strcpy(d1, cat ? b : "");
			memcpy(d2, d1, sizeof d1);
			T("fuzz %s(dst = %s, %s, %zu)", g_fn, esc(d1), esc(a), sz);
			size_t r = cat ? ft_strlcat(d1, a, sz) : ft_strlcpy(d1, a, sz);
			size_t e = cat ? strlcat(d2, a, sz) : strlcpy(d2, a, sz);
			CHECK(r == e && !memcmp(d1, d2, sizeof d1), "attendu %zu %s   obtenu %zu %s",
				e, esc(d2), r, esc(d1));
		}
		else if (!strcmp(g_fn, "ft_atoi"))
		{
			char s[32];
			rand_str(s, 12, " \t\n+-0123456789");
			int digits = 0;
			for (char *p = s; *p; p++)
				digits += isdigit(*p);
			if (digits > 9)
			{
				free(a);
				free(b);
				continue ;
			}
			T("fuzz ft_atoi(%s)", esc(s));
			CHECK(ft_atoi(s) == atoi(s), "attendu %d obtenu %d", atoi(s), ft_atoi(s));
		}
		free(a);
		free(b);
	}
	if (!strcmp(g_fn, "ft_strlen"))
		ONCE
		{
			size_t	n = 1 << 20;
			char	*s = malloc(n + 1);
			memset(s, 'a', n);
			s[n] = 0;
			T("ft_strlen(chaine de 1 Mo)");
			CHECK(ft_strlen(s) == n, "attendu %zu obtenu %zu", n, ft_strlen(s));
			free(s);
		}
}

static void	st_strnstr(void)
{
	for (int it = 0; it < 3000; it++)
	{
		char	h[64], nd[8];

		rand_str(h, 30, "ab");
		rand_str(nd, 4, "ab");
		size_t hl = strlen(h), len = rand() % (hl + 2);
		/* haystack de len octets sans '\0' quand len <= strlen: rien ne doit etre lu apres */
		char *a = len <= hl ? exact(h, len) : exact(h, hl + 1);
		size_t lim = len <= hl ? len : hl;
		char tmp[64];
		memcpy(tmp, h, lim);
		tmp[lim] = 0;
		T("fuzz ft_strnstr(%s (buffer de %zu octets), %s, %zu)", esc(tmp), len <= hl ? len : hl + 1, esc(nd), len);
		char *r = ft_strnstr(a, nd, len);
		char *e = ref_strnstr(tmp, nd, len);
		CHECK((r ? r - a : -1) == (e ? e - tmp : -1), "attendu: index %td   obtenu: index %td",
			e ? e - tmp : -1, r ? r - a : -1);
		free(a);
	}
}

static void	*c_calloc(void) { return (ft_calloc(7, 3)); }
static void	*c_strdup(void) { return (ft_strdup("sardine")); }
static void	*c_substr(void) { return (ft_substr("Hello World", 3, 5)); }
static void	*c_strjoin(void) { return (ft_strjoin("sar", "dine")); }
static void	*c_strtrim(void) { return (ft_strtrim("  sardine  ", " ")); }
static void	*c_split(void) { return (ft_split("  une  sardine dans la  boite ", ' ')); }
static void	*c_itoa(void) { return (ft_itoa(-2147483648)); }
static void	*c_strmapi(void) { return (ft_strmapi("sardine", map_f)); }
static void	*c_lstnew(void) { return (ft_lstnew(NULL)); }
static t_list	*g_map_in;
static void	*c_lstmap(void) { return (ft_lstmap(g_map_in, map_double, del_count)); }

static void	st_calloc(void)
{
	ONCE
	{
		T("ft_calloc(1 << 20, 1) entierement a zero");
		unsigned char *p = ft_calloc(1 << 20, 1);
		size_t k = 0;
		while (p && k < (1 << 20) && !p[k])
			k++;
		CHECK(p && k == 1 << 20, "octet non nul a l'index %zu", k);
		free(p);
	}
	ONCE
	{
		T("ft_calloc(7, 3)   (taille de l'allocation)");
		track_start(0);
		void *p = ft_calloc(7, 3);
		track_stop();
		CHECK(alloc_size(p) == 21, "attendu: malloc(21)   obtenu: malloc(%ld)", alloc_size(p));
		free(p);
	}
	fail_inject("ft_calloc(7, 3)", c_calloc, free_str);
}

static void	st_strdup(void)
{
	EXACT_SIZE("ft_strdup(\"sardine\")", ft_strdup("sardine"));
	EXACT_SIZE("ft_strdup(\"\")", ft_strdup(""));
	fail_inject("ft_strdup(\"sardine\")", c_strdup, free_str);
}

/* ---------- partie 2 ---------- */

static void	st_substr(void)
{
	EXACT_SIZE("ft_substr(\"Hello World\", 6, 100)", ft_substr("Hello World", 6, 100));
	EXACT_SIZE("ft_substr(\"Hello World\", 42, 5)", ft_substr("Hello World", 42, 5));
	EXACT_SIZE("ft_substr(\"Hello World\", 0, SIZE_MAX)", ft_substr("Hello World", 0, SIZE_MAX));
	EXACT_SIZE("ft_substr(\"Hello World\", 3, 2)", ft_substr("Hello World", 3, 2));
	ONCE
	{
		char *s = exact("abc", 4);
		T("ft_substr(\"abc\", 1, 1000)  (ne doit pas lire apres le '\\0')");
		char *r = ft_substr(s, 1, 1000);
		CHECK(r && !strcmp(r, "bc"), "attendu \"bc\" obtenu %s", esc(r));
		free(r);
		free(s);
	}
	fail_inject("ft_substr(\"Hello World\", 3, 5)", c_substr, free_str);
}

static void	st_strjoin(void)
{
	EXACT_SIZE("ft_strjoin(\"sar\", \"dine\")", ft_strjoin("sar", "dine"));
	EXACT_SIZE("ft_strjoin(\"\", \"\")", ft_strjoin("", ""));
	fail_inject("ft_strjoin(\"sar\", \"dine\")", c_strjoin, free_str);
}

static void	st_strtrim(void)
{
	EXACT_SIZE("ft_strtrim(\"  sardine  \", \" \")", ft_strtrim("  sardine  ", " "));
	EXACT_SIZE("ft_strtrim(\"xxxx\", \"x\")", ft_strtrim("xxxx", "x"));
	for (int it = 0; it < 500; it++)
	{
		char s[40], set[4];
		rand_str(s, 30, "ab c");
		rand_str(set, 3, "ab ");
		size_t b = 0, e = strlen(s);
		while (s[b] && strchr(set, s[b]))
			b++;
		while (e > b && strchr(set, s[e - 1]))
			e--;
		char exp[40];
		memcpy(exp, s + b, e - b);
		exp[e - b] = 0;
		T("fuzz ft_strtrim(%s, %s)", esc(s), esc(set));
		char *r = ft_strtrim(s, set);
		CHECK(r && !strcmp(r, exp), "attendu %s obtenu %s", esc(exp), esc(r));
		free(r);
	}
	fail_inject("ft_strtrim(\"  sardine  \", \" \")", c_strtrim, free_str);
}

static void	st_split(void)
{
	ONCE
	{
		T("ft_split(\"  une  sardine dans la  boite \", ' ')   (taille des allocations)");
		track_start(0);
		char **r = ft_split("  une  sardine dans la  boite ", ' ');
		track_stop();
		int ok = r != NULL, k = 0;
		for (; r && r[k]; k++)
			ok &= alloc_size(r[k]) == (long)strlen(r[k]) + 1;
		CHECK(ok && alloc_size(r) == (long)((k + 1) * sizeof(char *)),
			"tableau: attendu malloc(%zu) obtenu malloc(%ld), ou une chaine allouee trop grande",
			(k + 1) * sizeof(char *), alloc_size(r));
		free_split(r);
	}
	for (int it = 0; it < 500; it++)
	{
		char s[60], exp[512] = "", got[512] = "";
		rand_str(s, 50, "ab  ");
		for (char *p = s; *p;)
		{
			while (*p == ' ')
				p++;
			char *st = p;
			while (*p && *p != ' ')
				p++;
			if (p > st)
				snprintf(exp + strlen(exp), sizeof exp - strlen(exp), "%.*s|", (int)(p - st), st);
		}
		T("fuzz ft_split(%s, ' ')", esc(s));
		char **r = ft_split(s, ' ');
		for (int k = 0; r && r[k]; k++)
			snprintf(got + strlen(got), sizeof got - strlen(got), "%s|", r[k]);
		CHECK(r && !strcmp(exp, got), "attendu %s   obtenu %s", exp, r ? got : "NULL");
		free_split(r);
	}
	ONCE
	{
		size_t n = 20000;
		char *s = malloc(n + 1);
		for (size_t k = 0; k < n; k++)
			s[k] = k % 2 ? ',' : 'x';
		s[n] = 0;
		T("ft_split(\"x,x,x,...\" de 20000 caracteres, ',')");
		char **r = ft_split(s, ',');
		size_t k = 0;
		while (r && r[k])
			k++;
		CHECK(k == n / 2, "attendu %zu mots obtenu %zu", n / 2, k);
		free_split(r);
		free(s);
	}
	fail_inject("ft_split(\"  une  sardine dans la  boite \", ' ')", c_split, free_split_v);
}

static void	st_itoa(void)
{
	EXACT_SIZE("ft_itoa(INT_MIN)", ft_itoa(INT_MIN));
	EXACT_SIZE("ft_itoa(0)", ft_itoa(0));
	EXACT_SIZE("ft_itoa(-5)", ft_itoa(-5));
	EXACT_SIZE("ft_itoa(1000000)", ft_itoa(1000000));
	fail_inject("ft_itoa(INT_MIN)", c_itoa, free_str);
}

static void	st_strmapi(void)
{
	EXACT_SIZE("ft_strmapi(\"sardine\", f)", ft_strmapi("sardine", map_f));
	fail_inject("ft_strmapi(\"sardine\", f)", c_strmapi, free_str);
}

static void	st_put(void)
{
	if (!strcmp(g_fn, "ft_putstr_fd") || !strcmp(g_fn, "ft_putendl_fd"))
	ONCE
	{
		size_t	n = 100000, len;
		char	*s = malloc(n + 1);
		for (size_t k = 0; k < n; k++)
			s[k] = 'a' + k % 26;
		s[n] = 0;
		int fd = cap_open();
		T("%s(chaine de 100000 caracteres, fd)", g_fn);
		if (!strcmp(g_fn, "ft_putstr_fd"))
			ft_putstr_fd(s, fd);
		else
			ft_putendl_fd(s, fd);
		lseek(fd, 0, SEEK_END);
		len = lseek(fd, 0, SEEK_CUR);
		cap_read(&n);
		CHECK(len == strlen(s) + !strcmp(g_fn, "ft_putendl_fd"), "attendu %zu octets ecrits obtenu %zu",
			strlen(s) + !strcmp(g_fn, "ft_putendl_fd"), len);
		free(s);
	}
	ONCE
	{
		T("%s sur un fd invalide (-1): ne doit pas crasher", g_fn);
		if (!strcmp(g_fn, "ft_putstr_fd"))
			ft_putstr_fd("x", -1);
		else if (!strcmp(g_fn, "ft_putendl_fd"))
			ft_putendl_fd("x", -1);
		else if (!strcmp(g_fn, "ft_putnbr_fd"))
			ft_putnbr_fd(INT_MIN, -1);
		else
			ft_putchar_fd('x', -1);
	}
}

/* ---------- partie 3 ---------- */

static void	st_lstnew(void)
{
	fail_inject("ft_lstnew(NULL)", c_lstnew, free_str);
}

static void	st_lstbig(void)
{
	int		n = 100000;
	t_list	*l = make_list(n);

	if (!strcmp(g_fn, "ft_lstsize"))
		ONCE
		{
			T("ft_lstsize(liste de %d noeuds)", n);
			CHECK((long)ft_lstsize(l) == n, "attendu %d obtenu %ld", n, (long)ft_lstsize(l));
		}
	if (!strcmp(g_fn, "ft_lstlast"))
		ONCE
		{
			T("ft_lstlast(liste de %d noeuds)", n);
			CHECK(ft_lstlast(l) && *(int *)ft_lstlast(l)->content == n, "mauvais dernier noeud");
		}
	if (!strcmp(g_fn, "ft_lstiter"))
		ONCE
		{
			T("ft_lstiter(liste de %d noeuds, f)", n);
			ft_lstiter(l, iter_f);
			t_list *last = l;
			while (last->next)
				last = last->next;
			CHECK(*(int *)l->content == 10 && *(int *)last->content == n * 10, "f n'a pas ete appliquee a tous les noeuds");
		}
	if (!strcmp(g_fn, "ft_lstadd_back"))
		ONCE
		{
			T("ft_lstadd_back sur une liste de %d noeuds", n);
			t_list *x = node(malloc(sizeof(int)));
			*(int *)x->content = -1;
			ft_lstadd_back(&l, x);
			t_list *last = l;
			while (last->next)
				last = last->next;
			CHECK(last == x, "le noeud n'est pas a la fin");
		}
	if (!strcmp(g_fn, "ft_lstclear"))
		ONCE
		{
			g_del = 0;
			T("ft_lstclear(liste de %d noeuds, del)", n);
			ft_lstclear(&l, del_count);
			CHECK(g_del == n && l == NULL, "del appelee %d fois au lieu de %d", g_del, n);
		}
	free_list(l);
}

static void	st_lstmap(void)
{
	g_map_in = make_list(5);
	ONCE
	{
		g_del = 0;
		T("ft_lstmap(liste de 5)   del ne doit pas etre appelee si tout va bien");
		t_list *r = ft_lstmap(g_map_in, map_double, del_count);
		CHECK(g_del == 0, "del appelee %d fois", g_del);
		free_list(r);
	}
	fail_inject("ft_lstmap(1 -> 2 -> 3 -> 4 -> 5, f, del)", c_lstmap, free_lst);
	free_list(g_map_in);
	g_map_in = make_list(10000);
	ONCE
	{
		T("ft_lstmap(liste de 10000 noeuds, f, del)");
		t_list *r = ft_lstmap(g_map_in, map_double, del_count);
		CHECK(r && (long)ft_lstsize(r) == 10000, "liste resultat incorrecte");
		free_list(r);
	}
	free_list(g_map_in);
}

static const struct { const char *name; void (*test)(void); } g_strict[] = {
	{"ft_memset", st_mem}, {"ft_bzero", st_mem}, {"ft_memcpy", st_mem}, {"ft_memmove", st_mem},
	{"ft_memchr", st_memchr_cmp}, {"ft_memcmp", st_memchr_cmp},
	{"ft_strlen", st_str}, {"ft_strchr", st_str}, {"ft_strrchr", st_str}, {"ft_strncmp", st_str},
	{"ft_strlcpy", st_str}, {"ft_strlcat", st_str}, {"ft_atoi", st_str},
	{"ft_strnstr", st_strnstr}, {"ft_calloc", st_calloc}, {"ft_strdup", st_strdup},
	{"ft_substr", st_substr}, {"ft_strjoin", st_strjoin}, {"ft_strtrim", st_strtrim},
	{"ft_split", st_split}, {"ft_itoa", st_itoa}, {"ft_strmapi", st_strmapi},
	{"ft_putchar_fd", st_put}, {"ft_putstr_fd", st_put}, {"ft_putendl_fd", st_put},
	{"ft_putnbr_fd", st_put}, {"ft_lstnew", st_lstnew}, {"ft_lstsize", st_lstbig},
	{"ft_lstlast", st_lstbig}, {"ft_lstiter", st_lstbig}, {"ft_lstadd_back", st_lstbig},
	{"ft_lstclear", st_lstbig}, {"ft_lstmap", st_lstmap},
};

static void	run_strict(const char *name)
{
	for (size_t i = 0; i < sizeof g_strict / sizeof *g_strict; i++)
		if (!strcmp(g_strict[i].name, name))
			g_strict[i].test();
}
