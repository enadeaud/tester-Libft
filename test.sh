#!/bin/bash
# usage: sardine [chemin_vers_libft] [ft_fonction ...]
#   sans chemin: le dossier courant s'il contient libft.h, sinon le dossier parent du tester
SELF=$(realpath "$0")

# --strict (ou -s): tests plus durs (malloc qui echoue, taille exacte des allocations, fuzzing, grosses entrees)
ARGS=()
for a in "$@"; do
	case $a in --strict|-s|strict) export TESTER_STRICT=1 ;; *) ARGS+=("$a") ;; esac
done
set -- "${ARGS[@]}"
DIR=$(dirname "$SELF")

if [ -n "$1" ] && [ -d "$1" ]; then LIBFT=$(realpath "$1"); shift
elif [ -f "$PWD/libft.h" ]; then LIBFT=$PWD
else LIBFT=$(realpath "$DIR/..")
fi
cd "$DIR" || exit 1
ERR=$DIR/errors

G=$'\e[32m'; R=$'\e[31m'; Y=$'\e[33m'; C=$'\e[36m'; D=$'\e[2m'; B=$'\e[1m'; N=$'\e[0m'
OKS="${G}✔${N}"; KOS="${R}✘${N}"

# alias "sardine" ajoute une seule fois dans les rc du shell
for rc in "$HOME/.zshrc" "$HOME/.bashrc"; do
	if [ -f "$rc" ] && ! grep -q "alias sardine=" "$rc"; then
		echo "alias sardine='$SELF'" >> "$rc"
		ALIAS_ADDED=1
	fi
done

echo "${C}${B}"
cat <<'EOF'
        ><(((º>       ><(((º>
     ___   _   ___ ___ ___ _  _ ___
    / __| /_\ | _ \   \_ _| \| | __|
    \__ \/ _ \|   / |) | || .` | _|
    |___/_/ \_\_|_\___/___|_|\_|___|   libft tester
EOF
echo "${N}"
echo "  ${D}libft :${N} $LIBFT"
[ -n "$TESTER_STRICT" ] && echo "  ${R}${B}MODE STRICT${N} ${D}— malloc qui echoue, taille des allocations, fuzzing, grosses entrees${N}"
[ -n "$ALIAS_ADDED" ] && echo "  ${Y}alias 'sardine' ajoute — ouvre un nouveau terminal ou fais: source ~/.zshrc${N}"
echo

section() { printf "\n  ${B}${C}%s${N}\n  ${D}%s${N}\n" "$1" "────────────────────────────────────────────────────────────"; }
check() { [ "$2" = 1 ] && printf "    %s %s\n" "$OKS" "$1" || printf "    %s %s\n" "$KOS" "$1"; }

[ -f "$LIBFT/libft.h" ] || { echo "  $KOS libft.h introuvable dans $LIBFT"; exit 1; }

TARGETED=$#
[ $TARGETED = 0 ] && section "Makefile & regles"
make -C "$LIBFT" fclean >/dev/null 2>&1
if ! make -C "$LIBFT" >/dev/null 2>"$DIR/.make.log"; then
	echo "    $KOS make a echoue:"; sed 's/^/      /' "$DIR/.make.log"; exit 1
fi
[ -f "$LIBFT/libft.a" ] || { echo "    $KOS libft.a absent apres make"; exit 1; }
grep -qE "^bonus\s*:" "$LIBFT/Makefile" && make -C "$LIBFT" bonus >/dev/null 2>&1
nm "$LIBFT/libft.a" 2>/dev/null | awk -f "$DIR/externs.awk" > "$DIR/.externs"
# meme fonction definie dans 2 .o (ex: ft_toupper copie dans ft_tolower.c): on le signale et on linke quand meme
nm -A "$LIBFT/libft.a" 2>/dev/null | awk '$(NF-1) == "T" { split($1, a, ":"); o[$NF] = o[$NF] " " a[2]; n[$NF]++ }
	END { for (f in n) if (n[f] > 1) print f ":" o[f] }' | while read -r dup; do
	echo "    $KOS ${dup%%:*} defini plusieurs fois:${dup#*:} (une seule version sera testee)"
done
# fonctions ciblees: on saute les verifs Makefile / README pour aller vite
if [ $TARGETED = 0 ]; then
before=$(stat -c %Y "$LIBFT/libft.a"); sleep 1
make -C "$LIBFT" >/dev/null 2>&1
check "pas de relink" "$([ "$before" = "$(stat -c %Y "$LIBFT/libft.a")" ] && echo 1)"
for rule in '\$\(NAME\)' all clean fclean re; do
	check "regle ${rule//\\/}" "$(grep -qE "^${rule}\s*:" "$LIBFT/Makefile" && echo 1)"
done
check "fonctions externes autorisees par le sujet (detail par fonction plus bas)" "$([ -s "$DIR/.externs" ] || echo 1)"
check "flags -Wall -Wextra -Werror" "$(grep -q -- -Wall "$LIBFT/Makefile" && grep -q -- -Wextra "$LIBFT/Makefile" && grep -q -- -Werror "$LIBFT/Makefile" && echo 1)"
if command -v norminette >/dev/null; then
	check "norminette" "$(norminette "$LIBFT" 2>/dev/null | grep -q Error || echo 1)"
fi

section "README.md"
RM="$LIBFT/README.md"
if [ -f "$RM" ]; then
	check "README.md a la racine" 1
	first=$(head -n1 "$RM" | tr -d '\r')
	check "1re ligne en italique: *This project has been created as part of the 42 curriculum by <login>*" \
		"$(echo "$first" | grep -qE '^([*_])This project has been created as part of the 42 curriculum by [^*_]+\1[[:space:]]*$' && echo 1)"
	for s in Description Instructions Resources; do
		check "section \"$s\"" "$(grep -qiE "^#+[[:space:]]*$s" "$RM" && echo 1)"
	done
	check "usage de l'IA decrit (mot AI/IA dans le README)" "$(grep -qwiE 'AI|IA|ChatGPT|Claude|LLM' "$RM" && echo 1)"
else
	check "README.md a la racine" 0
fi
fi

cc -g -w tester.c -Wl,--allow-multiple-definition -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--whole-archive "$LIBFT/libft.a" -Wl,--no-whole-archive -o tester 2>"$DIR/.make.log" \
	|| { echo "    $KOS compilation du tester echouee:"; sed 's/^/      /' "$DIR/.make.log"; exit 1; }

VG=$(command -v valgrind)
[ -z "$VG" ] && echo "    ${Y}! valgrind absent: pas de verification memoire${N}"
ALL=$(./tester --list)
if [ $TARGETED = 0 ]; then
	FUNCS=$ALL
	rm -rf "$ERR"
else
	# accepte "split", "ft_split" ou "ft_split.c"; "split-test" affiche chaque entree testee
	FUNCS=""; VFUNCS=" "
	for a in "$@"; do
		case $a in *-test) a=${a%-test}; v=1 ;; *) v="" ;; esac
		a=${a%.c}; a=ft_${a#ft_}
		[ -n "$v" ] && VFUNCS="$VFUNCS$a "
		if echo "$ALL" | grep -qx "$a"; then FUNCS="$FUNCS $a"; rm -f "$ERR/$a.txt"
		else echo "  $KOS fonction inconnue: $a"; fi
	done
	[ -z "$FUNCS" ] && { echo "  fonctions disponibles:"; echo $ALL | fold -s -w 70 | sed 's/^/    /'; exit 1; }
	section "Fonctions ciblees"
fi
mkdir -p "$ERR"
ok=0; nb=0

for f in $FUNCS; do
	[ $TARGETED = 0 ] && case $f in
		ft_isalpha) section "Partie 1 — fonctions de la libc" ;;
		ft_substr) section "Partie 2 — fonctions supplementaires" ;;
		ft_lstnew) section "Partie 3 — listes chainees" ;;
	esac
	nb=$((nb + 1))
	if [[ "$VFUNCS" == *" $f "* ]]; then
		printf "    ${B}%s${N} ${D}— entrees testees:${N}\n" "$f"
		out=$(TESTER_VERBOSE=1 timeout 120 ./tester "$f"); rc=$?
	else
		out=$(timeout 120 ./tester "$f" 2>/dev/null); rc=$?
	fi
	read -r total failed crashes <<< "$out"
	vg=""
	case $rc in
		0|1)
			[ "$failed" = 0 ] && res="$OKS ${G}$(printf '%4s/%-4s' "$total" "$total")${N}" \
				|| res="$KOS ${R}$(printf '%4s/%-4s' "$((total - failed))" "$total")${N}"
			[ "$crashes" != 0 ] && res="$res ${R}💥 $crashes crash/timeout${N}" ;;
		2) res="${Y}?  absente de libft.a${N}" ;;
		124) res="$KOS ${R}timeout global${N}"; echo "[TIMEOUT] le tester a ete tue apres 120s" >> "$ERR/$f.txt" ;;
		*) res="$KOS ${R}crash du tester (code $rc)${N}"; echo "[CRASH] code de sortie $rc" >> "$ERR/$f.txt" ;;
	esac
	if [ -n "$VG" ] && { [ $rc = 0 ] || [ $rc = 1 ]; }; then
		# apres un crash les fuites du cas interrompu sont normales: on ne regarde que les acces invalides
		leak="--leak-check=full --show-leak-kinds=definite,indirect --errors-for-leak-kinds=definite,indirect"
		[ "$crashes" != 0 ] && leak="--leak-check=no"
		TESTER_NOLOG=1 timeout 300 valgrind -q $leak --error-exitcode=42 --log-file="$DIR/.vg.log" \
			./tester "$f" >/dev/null 2>&1
		if [ $? = 42 ]; then
			vg="${R}🐟 valgrind KO${N}"
			{
				# entrees fautives taggees par le tester (VALGRIND_PRINTF), sinon c'est une fuite
				tags=$(grep -o 'erreur pendant le test: .*' "$DIR/.vg.log" | sed 's/^erreur pendant le test: //')
				if [ -n "$tags" ]; then
					echo "$tags" | sed 's/^/[VALGRIND] acces memoire invalide pendant: /'
				else
					echo "[VALGRIND] fuite memoire:"
				fi
				echo; cat "$DIR/.vg.log"; echo
			} >> "$ERR/$f.txt"
		else
			vg="${G}🐟 mem ok${N}"
		fi
	fi
	bad=$(awk -v f="$f" '$1 == f { print $2 }' "$DIR/.externs" | tr '\n' ' ')
	if [ -n "$bad" ]; then
		res="$res ${R}⛔ interdit: ${bad% }${N}"
		awk -v f="$f" '$1 == f { $1 = ""; print "[INTERDIT] appel a" $0 }' "$DIR/.externs" >> "$ERR/$f.txt"
	fi
	[ -f "$ERR/$f.txt" ] || ok=$((ok + 1))
	printf "    %-17s %s  %s\n" "$f" "$res" "$vg"
	if [ -f "$ERR/$f.txt" ]; then
		first=$(grep -m1 -A1 -E '^\[(KO|CRASH|TIMEOUT|MISSING|VALGRIND|INTERDIT)' "$ERR/$f.txt" | tr -s ' ' | tr '\n' ' ' | cut -c1-90)
		printf "      ${D}↳ %s${N}\n" "$first"
	fi
done
rm -f "$DIR/.vg.log" "$DIR/.make.log" "$DIR/.externs"
make -C "$LIBFT" fclean >/dev/null 2>&1

section "Resultat"
width=40
fill=$((nb ? ok * width / nb : 0))
bar="${G}$(printf '█%.0s' $(seq 1 $fill))${R}$(printf '░%.0s' $(seq 1 $((width - fill))))${N}"
[ $fill = 0 ] && bar="${R}$(printf '░%.0s' $(seq 1 $width))${N}"
[ $fill = $width ] && bar="${G}$(printf '█%.0s' $(seq 1 $width))${N}"
printf "    %s  ${B}%d/%d${N} fonctions OK\n" "$bar" "$ok" "$nb"
if [ $ok = $nb ]; then
	echo "    ${G}${B}Tout est bon, la sardine est fiere de toi ><(((º>${N}"
else
	echo "    ${R}Details des erreurs:${N} $ERR/<fonction>.txt"
fi
echo
[ $ok = $nb ]
