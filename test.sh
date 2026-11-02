#!/bin/bash
# usage: sardine [chemin_vers_libft] [ft_fonction ...]
#   sans chemin: le dossier courant s'il contient libft.h, sinon le dossier parent du tester
SELF=$(realpath "$0")
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
[ -n "$ALIAS_ADDED" ] && echo "  ${Y}alias 'sardine' ajoute — ouvre un nouveau terminal ou fais: source ~/.zshrc${N}"
echo

section() { printf "\n  ${B}${C}%s${N}\n  ${D}%s${N}\n" "$1" "────────────────────────────────────────────────────────────"; }
check() { [ "$2" = 1 ] && printf "    %s %s\n" "$OKS" "$1" || printf "    %s %s\n" "$KOS" "$1"; }

[ -f "$LIBFT/libft.h" ] || { echo "  $KOS libft.h introuvable dans $LIBFT"; exit 1; }

section "Makefile & regles"
make -C "$LIBFT" fclean >/dev/null 2>&1
if ! make -C "$LIBFT" >/dev/null 2>"$DIR/.make.log"; then
	echo "    $KOS make a echoue:"; sed 's/^/      /' "$DIR/.make.log"; exit 1
fi
[ -f "$LIBFT/libft.a" ] || { echo "    $KOS libft.a absent apres make"; exit 1; }
before=$(stat -c %Y "$LIBFT/libft.a"); sleep 1
make -C "$LIBFT" >/dev/null 2>&1
check "pas de relink" "$([ "$before" = "$(stat -c %Y "$LIBFT/libft.a")" ] && echo 1)"
for rule in '\$\(NAME\)' all clean fclean re; do
	check "regle ${rule//\\/}" "$(grep -qE "^${rule}\s*:" "$LIBFT/Makefile" && echo 1)"
done
check "flags -Wall -Wextra -Werror" "$(grep -q -- -Wall "$LIBFT/Makefile" && grep -q -- -Wextra "$LIBFT/Makefile" && grep -q -- -Werror "$LIBFT/Makefile" && echo 1)"
grep -qE "^bonus\s*:" "$LIBFT/Makefile" && make -C "$LIBFT" bonus >/dev/null 2>&1
forbidden=$(nm -u "$LIBFT/libft.a" 2>/dev/null | awk '{print $2}' | sort -u \
	| grep -vE '^(ft_.*|malloc|free|write|__stack_chk_fail|_GLOBAL_OFFSET_TABLE_)$')
check "fonctions externes (malloc, free, write)${forbidden:+ — interdites: $(echo $forbidden)}" "$([ -z "$forbidden" ] && echo 1)"
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

cc -g -w tester.c -Wl,--whole-archive "$LIBFT/libft.a" -Wl,--no-whole-archive -o tester 2>"$DIR/.make.log" \
	|| { echo "    $KOS compilation du tester echouee:"; sed 's/^/      /' "$DIR/.make.log"; exit 1; }

rm -rf "$ERR"; mkdir -p "$ERR"
VG=$(command -v valgrind)
[ -z "$VG" ] && echo "    ${Y}! valgrind absent: pas de verification memoire${N}"
FUNCS=${*:-$(./tester --list)}
ok=0; nb=0

for f in $FUNCS; do
	case $f in
		ft_isalpha) section "Partie 1 — fonctions de la libc" ;;
		ft_substr) section "Partie 2 — fonctions supplementaires" ;;
		ft_lstnew) section "Partie 3 — listes chainees" ;;
	esac
	nb=$((nb + 1))
	out=$(timeout 120 ./tester "$f" 2>/dev/null); rc=$?
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
			{ echo "[VALGRIND] fuite memoire ou acces invalide:"; cat "$DIR/.vg.log"; echo; } >> "$ERR/$f.txt"
		else
			vg="${G}🐟 mem ok${N}"
		fi
	fi
	[ -f "$ERR/$f.txt" ] || ok=$((ok + 1))
	printf "    %-17s %s  %s\n" "$f" "$res" "$vg"
	if [ -f "$ERR/$f.txt" ]; then
		first=$(grep -m1 -A1 -E '^\[(KO|CRASH|TIMEOUT|MISSING|VALGRIND)' "$ERR/$f.txt" | tr -s ' ' | tr '\n' ' ' | cut -c1-90)
		printf "      ${D}↳ %s${N}\n" "$first"
	fi
done
rm -f "$DIR/.vg.log" "$DIR/.make.log"
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
