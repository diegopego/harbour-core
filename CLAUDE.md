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
- Commits só com autorização explícita do Diego **para AQUELE commit**;
  concluir/aprovar o trabalho não autoriza o commit. Um pedido por commit —
  não encadear. Sem push salvo pedido.
- Em sessão com o modelo Fable: delegar a subagentes **opus** para
  economizar tokens do Fable quando realmente compensar (trabalho
  mecânico bem especificado — varreduras, builds, baterias de teste);
  raciocínio central e código delicado ficam no Fable, que revisa o
  que os agentes entregam.
- Regra/preferência durável deste repo vai AQUI (versionado), não na memória
  privada do Claude (que não viaja com o repo); a memória fica para o que não
  pertence a um repo.

## Harbour (linguagem) — armadilhas ao escrever .prg

- **Não nomear variável formando keyword em uppercase**: Harbour é
  case-insensitive e lê identificadores em uppercase — `LOCAL nIL` vira a
  reservada `NIL` (`E0030 syntax error`). Evitar `nIL`, `cFor`, etc.
- **MEMVAR antes de PRIVATE/PUBLIC**: referenciar `PRIVATE`/`PUBLIC` sem uma
  declaração `MEMVAR` compile-time gera W0002 na criação e W0001 em cada uso —
  com `-es2` o build falha. Idioma: `MEMVAR xCfg` / `PRIVATE xCfg := 7`.
- **Comentário de linha `//` em .prg** (não `/* */`): um `*/` que apareça no
  conteúdo (ex.: `assert_*/`) fecha o bloco antes da hora e o resto vira
  código. Aplicar em código novo/editado, sem conversão em massa. (Em C o
  codestyle do projeto manda `/* */` e proíbe `//` — regra oposta.)
- **Verificar comportamento no fonte `src/` ANTES de afirmar** (não teorizar):
  ler/grep o arquivo relevante. Para depurar, preferir `hb_TraceLog()` +
  `hb_TraceFile()` (não `OutStd`/`QOut`/`?`, que passam pelo GT e enganam a
  medição). Gotcha: `Empty(" ")` é `.T.` — usar `Len(c) == 0` para "vazia".
