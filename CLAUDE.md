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
  `rm bin/linux/gcc/harbour && make`. **Vale TAMBÉM para `hbmk2`**: ele
  compila .prg com o compilador EMBUTIDO (linka `libhbcplr`), então um
  hbmk2 velho emite dumps do schema antigo mesmo com o `harbour` novo —
  já custou um diagnóstico (dump ast-1 sem ppRules via hbmk2 enquanto o
  harbour direto emitia ast-2).
- Commits só com autorização explícita do Diego.
- Em sessão com o modelo Fable: delegar a subagentes **opus** para
  economizar tokens do Fable quando realmente compensar (trabalho
  mecânico bem especificado — varreduras, builds, baterias de teste);
  raciocínio central e código delicado ficam no Fable, que revisa o
  que os agentes entregam.
