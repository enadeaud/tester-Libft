# tester-Libft 🐟 sardine

Tester pour la libft de 42 (sujet v19.3, parties 1, 2 et 3).

```bash
./test.sh                       # 1er lancement: ajoute l'alias `sardine` a ~/.zshrc et ~/.bashrc
sardine                         # depuis le dossier de ta libft
sardine ~/libft ft_split ft_itoa  # un chemin et/ou certaines fonctions
```

Sans chemin: dossier courant s'il contient `libft.h`, sinon le dossier parent du tester.

- Makefile (regles, flags, relink), fonctions externes (`nm`), norminette si installee.
- README.md (chapitre V): 1re ligne en italique, sections Description / Instructions / Resources, mention de l'IA.
- Compare avec la vraie fonction de la libc quand elle existe (y compris `strlcpy`/`strlcat`, glibc >= 2.38).
- Chaque fonction passe aussi sous valgrind (fuites et acces invalides), `ft_putnbr_fd` compris.
- Un crash ou une boucle infinie ne stoppe pas le tester: le cas fautif est note et les suivants continuent.
- Une fonction absente (ou un `libft.h` incomplet) n'empeche pas de compiler les autres tests.
- Les tests en erreur sont ecrits dans `errors/<nom_fonction>.txt` (entree, attendu, obtenu).
