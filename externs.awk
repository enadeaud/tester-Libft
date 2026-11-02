# entree: `nm libft.a`. sortie: "ft_fonction symbole_interdit chemin" pour chaque appel externe
# shortcut: les externes sont lues par fichier .o, deux ft_ dans le meme .c partagent leurs appels; suffit pour le "une fonction par fichier" de 42.
# non autorise par le sujet, en suivant les appels vers les autres ft_ (ex: ft_putnbr_fd -> ft_itoa -> malloc).
BEGIN {
	split("ft_calloc ft_strdup ft_substr ft_strjoin ft_strtrim ft_itoa ft_strmapi ft_lstnew", a)
	for (i in a) ok[a[i], "malloc"] = 1
	split("ft_split ft_lstmap", a)
	for (i in a) { ok[a[i], "malloc"] = 1; ok[a[i], "free"] = 1 }
	split("ft_lstdelone ft_lstclear", a)
	for (i in a) ok[a[i], "free"] = 1
	split("ft_putchar_fd ft_putstr_fd ft_putendl_fd ft_putnbr_fd", a)
	for (i in a) ok[a[i], "write"] = 1
	ignore["__stack_chk_fail"] = ignore["_GLOBAL_OFFSET_TABLE_"] = 1
}
/:$/ { obj = $0; next }
NF == 3 && $2 == "T" { def[$3] = obj }
NF == 2 && $1 == "U" { und[obj] = und[obj] " " $2 }
END {
	for (fn in def) {
		if (fn !~ /^ft_/) continue
		delete seen; delete via
		n = 1; queue[1] = def[fn]; seen[def[fn]] = 1; via[def[fn]] = fn
		for (q = 1; q <= n; q++) {
			o = queue[q]
			m = split(und[o], syms, " ")
			for (k = 1; k <= m; k++) {
				s = syms[k]
				if (s in def) {
					if (!(def[s] in seen)) { seen[def[s]] = 1; queue[++n] = def[s]; via[def[s]] = via[o] " -> " s }
				} else if (!(s in ignore) && !((fn, s) in ok) && !((fn, s, "printed") in done)) {
					done[fn, s, "printed"] = 1
					print fn, s, via[o] " -> " s
				}
			}
		}
	}
}
