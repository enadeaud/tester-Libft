# tester-Libft 🐟 sardine

Tester pour la libft de 42 (sujet v19.3, parties 1, 2 et 3).

```bash
./test.sh                       # 1er lancement: ajoute l'alias `sardine` a ~/.zshrc et ~/.bashrc
sardine                         # depuis le dossier de ta libft
sardine split itoa              # seulement certaines fonctions (avec ou sans ft_)
sardine split-test              # affiche chaque entree envoyee a la fonction (✔/✘, attendu/obtenu)
sardine --strict                # tests plus durs (malloc qui echoue, taille exacte, fuzzing, 100000 noeuds)
sardine ~/libft ft_split        # avec un chemin
```

Sans chemin: dossier courant s'il contient `libft.h`, sinon le dossier parent du tester.

- Makefile (regles, flags, relink), norminette si installee.
- Fonctions externes verifiees par fonction (`nm`), en suivant les appels entre ft_: ex. `ft_putnbr_fd` -> `ft_itoa` -> `malloc` est interdit.
- README.md (chapitre V): 1re ligne en italique, sections Description / Instructions / Resources, mention de l'IA.
- Compare avec la vraie fonction de la libc quand elle existe (y compris `strlcpy`/`strlcat`, glibc >= 2.38).
- Chaque fonction passe aussi sous valgrind (fuites et acces invalides), `ft_putnbr_fd` compris.
- Un crash ou une boucle infinie ne stoppe pas le tester: le cas fautif est note et les suivants continuent.
- Une fonction absente (ou un `libft.h` incomplet) n'empeche pas de compiler les autres tests.
- `--strict`: chaque malloc mis en echec a tour de role (NULL attendu, sans fuite), taille exacte des allocations, fuzzing contre la libc avec des buffers de taille exacte, entrees de 1 Mo et listes de 100000 noeuds.
- Les tests en erreur sont ecrits dans `errors/<nom_fonction>.txt` (entree, attendu, obtenu).
