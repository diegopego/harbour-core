## compiling
hbmk2 links against the runtime/static libraries, but in a “compiler only” build those archives (libhbextern.a, libhbrtl.a, …) haven’t been produced yet. Because of that the linker can’t find them and the compiler step stops.

You already succeeded in building the host harbour executable (bin/linux/gcc/harbour). With that tool available you can now build the libraries:

make HB_BUILD_PARTS=lib HB_PLATFORM=linux HB_COMPILER=gcc
(or just run make / make HB_BUILD_PARTS=all ... if you’d rather let the standard bootstrap sequence happen). Once the libraries exist, rerun the compiler build so hbmk2 can link cleanly:

make HB_BUILD_PARTS=compiler HB_PLATFORM=linux HB_COMPILER=gcc
If you only want the compiler and don’t care about the utilities right now, you can skip building hbmk2 by adding HB_WITH_UTILS=no:

make HB_BUILD_PARTS=compiler HB_PLATFORM=linux HB_COMPILER=gcc HB_WITH_UTILS=no
After these steps you’ll have the new libhbastlex.a inside lib/linux/gcc/, ready to link against.

## testing the stub

Um teste de fumaça em C está disponível em `tests/ast/smoke.c`. Ele instancia o lexer incremental, percorre o fluxo de tokens produzidos pelo pré-processador e imprime categoria, tipo bruto e intervalo calculado.

1. Certifique-se de ter construído as bibliotecas (`make HB_BUILD_PARTS=lib ...`).
2. Compile o teste com o Makefile auxiliar:

   ```sh
   make -C tests/ast HB_PLATFORM=linux HB_COMPILER=gcc
   ```

   Ajuste `HB_PLATFORM`/`HB_COMPILER` conforme os diretórios gerados em `lib/`.

3. Execute o binário:

   ```sh
   ./tests/ast/smoke
   ```

O fixture `tests/ast/demo.prg` inclui `tests/ast/helpers.ch`; os tokens provenientes de cada arquivo preservam o campo `module`, permitindo observar como o PP capturou linhas e arquivos originais.

Quando o lexer ganhar implementação real, esse binário poderá ser expandido para inspecionar os tokens emitidos e validar round-trips.

> Dica: `make ast-smoke HB_PLATFORM=linux HB_COMPILER=gcc` automatiza as três etapas (construir libs, compilar o teste e executar o binário).

### Estado atual do lexer

- `hb_astLexerNextToken()` agora delega ao pré-processador (`hb_pp_lexGet`) e categoriza cada token segundo `HB_AST_TOKEN_KIND`.
- Campos preenchidos:
  - `id.uHash`: índice sequencial no fluxo.
  - `uPPType`: tipo bruto (`HB_PP_TOKEN_*`), útil para depuração e compatibilidade.
  - `nLexemeLength`/`pszLexeme`: conteúdo textual retornado pelo PP.
  - `original`/`expanded`: coordenadas básicas (line/column/offset) atualizadas a partir do lexema; ainda ingênuas para casos de macro expansão.
  - `pszModule` e `original.start.nLine`: herdados do PP, preservando o módulo/arquivo e a linha original de cada token.
- `tests/ast/smoke` imprime o stream com tipo, categoria e intervalo calculado, servindo de base para inspeções manuais.
- `tests/ast/snapshot` executa asserts automatizados garantindo que tokens e rastros de macro (`pMacroOrigin`) produzidos pelo snapshot representam corretamente o grafo de expansão e valida o JSON emitido por `hb_astTokenStreamSerializeMacrosJson()`/`hb_astTokenStreamWriteMacrosJson()`.
- `hb_astTokenStreamSnapshot()` agora clona o histórico de tokens e expõe acessos constantes através de `hb_astTokenStreamCount()` e `hb_astTokenStreamToken()`, permitindo inspeção sem depender do pré-processador.
- `pMacroOrigin` aponta para estruturas imutáveis de expansão; os helpers `hb_astMacroTraceName()`, `hb_astMacroTraceCallModule()`, `hb_astMacroTraceCallRange()`, `hb_astMacroTraceDepth()`, `hb_astMacroTraceParent()` e `hb_astMacroTraceId()` permitem reconstruir a cadeia de macros, mapear pais e obter um identificador determinístico para serialização. `hb_astTokenStreamMacroTraceCount()`/`hb_astTokenStreamMacroTrace()` expõem a lista de expansões únicas por snapshot.

### Limitações conhecidas

- Os ranges dependem do lexema e do campo `spaces` emitido pelo pré-processador; macros, includes e normalizações mais agressivas ainda não refletem posições reais do arquivo original.
- Não há cache incremental: qualquer alteração exigirá reconstruir o contexto PP.
- Canais (`channel`) distinguem apenas `code`, `directive` e `trivia`; comentários ainda são mapeados como código.

Próximos passos sugeridos:

1. Instrumentar o PP para reportar offsets reais (pré e pós expansão) em vez de inferir via lexema.
2. Persistir tokens em cache por bloco e invalidar apenas regiões alteradas.
3. Serializar o grafo de macro (`ExpansionTraceLog`) no payload AST para que consumidores externos acessem os ranges de chamada sem depender do lexer em memória.
