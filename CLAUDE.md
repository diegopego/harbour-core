# harbour-core (branch feature/compiler-ast-dump)

Fork do Harbour com o mecanismo de dump AST (`-x`, `src/compiler/compast.c`
+ ganchos de 1 linha gated por flag). Consumidor: ~/devel/hbrefactor.

## Regras de trabalho

- **Compile todo .prg de teste/fixture ANTES de usá-lo em qualquer teste**
  (`bin/linux/gcc/harbour arquivo.prg -n -q0` ou o projeto inteiro via
  hbmk2). Um fixture inválido produz diagnósticos enganosos — um erro de
  expansão de pp no fixture já foi confundido com regressão do mecanismo.
- Zero impacto sem `-x`: todo gancho novo no compilador/pp é gated
  (`fAst` / `fTrackPos`); prova = `.hrb` byte-idênticos com/sem `-x`.
- ATENÇÃO ao relink: `make` nem sempre relinca `bin/linux/gcc/harbour`
  quando só `libhbcplr.a`/`libhbpp.a` mudam — confira com
  `strings bin/linux/gcc/harbour | grep ast-` e, se preciso,
  `rm bin/linux/gcc/harbour && make`.
- Commits só com autorização explícita do Diego.
