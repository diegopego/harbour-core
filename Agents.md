# Harbour Agents

This fork exists to accelerate Harbour into a modern, tooling-first language platform. The **Agents** initiative defines the automation, services, and developer tools that will grow around the compiler and runtime so that Harbour can compete with ecosystems such as TypeScript or C#. Each agent represents a focused capability built on a shared language core.

## Objectives

- Deliver a reusable abstract syntax tree (AST) pipeline that is accurate, stable, and easy to consume.
- Expose language intelligence (symbol graph, type metadata, diagnostics) to external tools.
- Provide first-class support for test-driven development, coverage tracking, and static analysis.
- Enable powerful refactoring and navigation workflows that feel native to modern IDEs.
- Maintain backwards compatibility where practical while opening a path to gradual typing and new language features.

## Guiding Principles

- **Evolve, do not fork away**: keep Harbour source compatibility high, gating breaking changes behind opt-in switches.
- **Open APIs first**: every agent builds on documented interfaces so the community can extend or replace them.
- **Data flow over ad hoc parsing**: all intelligence is driven by a canonical AST and semantic model.
- **Incremental tooling**: support partial files, background compilation, and live feedback loops.
- **Quality through automation**: CI, tests, and coverage are part of the minimal viable deliverable for each agent.

## Shared Language Core

1. **Parser modernisation**
   - Stabilise the existing parser and emit a structured AST (JSON + binary interchange).
   - Preserve macro and preprocessor information to support precise refactoring and formatting.
   - Record symbol ownership, scopes, and type hints for downstream consumers.

2. **Semantic services**
   - Build a semantic graph that augments the AST with inferred types, dependencies, and side effects.
   - Introduce a capability registry so agents can request data (e.g. symbol lookup, type resolution, doc comments) without duplicating work.

3. **Incremental build pipeline**
   - Cache AST and semantic artefacts keyed by file hashes and compiler options.
   - Provide change notifications to tooling layers so editors can update diagnostics in real time.

### AST Generation Architecture

To viabilise refatorações confiáveis e análises estáticas robustas, o pipeline de AST segue estas etapas coordenadas:

1. **Pré-processamento consciente de macros**
   - Lexer incremental gera tokens com origem (`source range`) preservada antes e depois da expansão.
   - Macro expansions são anotadas no grafo para permitir navegação reversa e evitar falsos positivos em renames.

2. **Construção de AST normalizada**
   - Cada nó recebe um identificador determinístico (`stable id`) derivado do caminho + offset + tipo de nó.
   - Nós carregam metadados essenciais: intervalo textual, comentários adjacentes, tipo inferido (quando disponível), informações de visibilidade/escopo.
   - Estrutura de dados em memória otimizada para leitura (árvores compartilhadas entre threads) e serialização para JSON/CBOR.

3. **Binder semântico**
   - Resolver vincula identificadores a símbolos com suporte a namespaces, usos dinâmicos e variáveis públicas/privadas.
   - Um grafo de escopos captura herança, closures, macros e acessos a variáveis globais.
   - Regras de inferência coletam pistas de tipo (assinaturas, uso em runtime functions, convenções) para alimentar análise posterior.

4. **Índices e consultas**
   - Índice de referências (`cross-reference index`) armazena todas as leituras, escritas e importações com o `stable id` do nó alvo.
   - Serviços de consulta expõem APIs para rename seguro, busca por símbolo, detecção de dead code e validação de fluxo de dados.
   - Renames globais exigem confirmação de ausência de colisões via simulação (`what-if analysis`) antes de aplicar alterações.

5. **Persistência incremental**
   - Artefatos AST/semântica são cacheados em disco com delta updates, permitindo reuso entre builds e agentes.
   - Uma `Change Journal` compacta registra efeitos colaterais de cada arquivo para facilitar invalidation pontual.

6. **Garantia de qualidade**
   - Conjunto de fixtures cobre dialetos, macros complexas, metaprogramação e APIs nativas.
   - Testes de renomeação verificam se todas as ocorrências reais são tratadas e que não há falsos positivos.
   - Ferramenta `hbast verify` valida consistência do AST gerado contra regras de integridade e meta-modelo.
7. **Documentação técnica**
   - Especificações detalhadas em `doc/agents/ast/incremental-lexer.md` e `doc/agents/ast/serialization-format.md`.
   - Schema inicial publicado em `doc/agents/ast/hbast.schema.json` e plano do verificador em `doc/agents/ast/hbast-verify.md`.
   - Protótipo de código disponível em `src/ast/lexer/hbast_lexer.c` e `include/ast/lexer/hbast_lexer.h`.

### Implementação Atual (Jun/2024)

- O lexer incremental mantém um histórico de tokens persistente (`HB_AST_TOKEN_ENTRY`) com lexema/módulo clonados, permitindo consumo assíncrono sem depender do estado do PP.
- `hb_pp_patternReplace()` passou a fabricar `HB_PP_TRACEINFO` com nome da macro, módulo e intervalo da chamada; `HB_PP_TOKEN` mantém esse traço e refcounts para reaproveitar dados entre expansões.
- `hb_astTokenStreamSnapshot()` continua a fornecer cópias profundas e agora replica os rastros de macro (`HB_AST_MACRO_TRACE_INFO`). A API pública expõe helpers (`hb_astMacroTraceName()`, `hb_astMacroTraceCallModule()`, `hb_astMacroTraceCallRange()`, `hb_astMacroTraceDepth()`, `hb_astMacroTraceParent()`, `hb_astMacroTraceId()`) e iteradores (`hb_astTokenStreamMacroTraceCount()`, `hb_astTokenStreamMacroTrace()`) para navegar na pilha de expansões e produzir IDs estáveis para serialização.
- `pMacroOrigin` dentro de `HB_AST_TOKEN` é estável, carregando profundidade e ranges do ponto de chamada; `tests/ast/smoke` imprime essa informação para validação rápida.
- Documentação e fixtures alinhados: `README-AST.MD` descreve as novas APIs, `doc/agents/ast/incremental-lexer.md` esclarece o campo `origin`, `doc/agents/ast/serialization-format.md` cobre o payload e o smoke continua a servir como verificação de regressão.
- Serialização auxiliar disponível: `hb_astTokenStreamSerializeMacrosJson()` e `hb_astTokenStreamWriteMacrosJson()` exportam o grafo de macros (`macros.expansions`) em JSON, prontos para ser embutidos no payload final.
- CLI `hbast` disponível em `utils/hbast/` gerando dumps JSON (`tokens` + `macros.expansions`) a partir de arquivos `.prg`.
- Testes cobrindo o pipeline: `tests/ast/smoke` imprime o fluxo para depuração e `tests/ast/snapshot` valida programaticamente o snapshot, o grafo de macros e a serialização JSON.

#### Artefatos relevantes

- Código-base: `src/ast/lexer/hbast_lexer.c` concentra as estruturas `HB_AST_TOKEN_ENTRY`, `HB_AST_TOKEN_STREAM_ENTRY` e macros como `HB_AST_LEXER_HISTORY_GROWTH`, além das rotinas internas `hb_astLexerHistoryReset()` e `hb_astLexerHistoryStore()` que calibram o cache. O módulo `src/ast/lexer/hbast_json.c` gera o JSON de `macros.expansions`.
- API pública: `include/ast/lexer/hbast_lexer.h` exporta `hb_astTokenStreamSnapshot()`, `hb_astTokenStreamCount()` e `hb_astTokenStreamToken()`; revisar antes de evoluções de assinatura.
- Fixtures: `tests/ast/ast_smoke_test.c`, `tests/ast/fixture_demo.prg` e `tests/ast/fixture_helpers.ch` validam o fluxo atual via `make -C tests/ast`.
- Build alvo: `src/ast/lexer/Makefile` gera `libhbastlex.a`, consumida por `tests/ast/Makefile`.
- Documentação: `doc/agents/ast/incremental-lexer.md` e `doc/agents/ast/serialization-format.md` detalham as próximas etapas do pipeline, enquanto `doc/agents/ast/hbast-verify.md` descreve o verificador planejado.

## Primary Agents

| Agent | Goals | Notes |
| ----- | ----- | ----- |
| `ast-gen` | Produce canonical AST dumps, tooling SDK, and schema docs. | Foundation for every other agent; must support round-tripping and diagnostics. |
| `lsp-core` | Offer Language Server Protocol services (hover, completion, rename, formatting hooks). | Ships as a standalone binary with plugin hooks for editors. |
| `static-check` | Execute rule-based analysis (linting, security, dependency hygiene). | Runs standalone or as part of CI; configurable severities. |
| `tdd-suite` | Streamline project scaffolding, test discovery, and execution. | Integrates with `hbunit` (or successor) and exports JUnit-compatible reports. |
| `cov-report` | Collect coverage (statement, branch, expression) and publish HTML + CLI summaries. | Uses compiler instrumentation flags; supports merge across multiple runs. |
| `refactor-pro` | Provide safe transforms (rename, extract procedure, inline, dead-code removal). | Depends on rich semantic data; includes preview and undo metadata. |

## Roadmap

### Phase 1 – Foundations (0-6 months)

- Finalise AST schema, including comment and macro retention.
- Provide CLI tooling (`hbast`) to dump, validate, and diff ASTs.
- Publish developer documentation and SDK for embedding AST services.
- Establish test suites to validate AST fidelity against legacy Harbour codebases.

### Phase 2 – Interactive Tooling (6-12 months)

- Release `lsp-core` with hover, go-to-definition, diagnostics, and basic completions.
- Introduce project configuration files describing module layout, dependencies, and dialect flags.
- Implement incremental compilation cache and file-watching daemon.
- Add baseline static checks focused on errors currently detected only at compile time.

### Phase 3 – Quality Automation (12-18 months)

- Launch `tdd-suite` with scaffolding commands, snapshot testing support, and CI adapters.
- Integrate coverage instrumentation into the build system; generate HTML and text reports.
- Expand static analysis with pattern-based rules and data-flow checks.
- Ensure CI workflows execute tests, analysis, and coverage on every PR.

### Phase 4 – Advanced Refactoring (18-24 months)

- Deliver `refactor-pro` rename and extract procedure with safety guarantees.
- Support cross-file refactors (move symbol, organise imports, dead code pruning).
- Add code actions to `lsp-core` surfacing refactoring options in editors.
- Investigate gradual typing strategies (type annotations, inference, and validation).

## Collaboration Model

- **Repositories**: keep core agents inside this fork unless a subproject needs an independent cadence.
- **Issue labels**: use `agent:<name>` to prioritise and triage work.
- **Design reviews**: require lightweight design docs for schema or protocol changes, recorded in `doc/agents/`.
- **Quality gates**: no agent ships without automated tests, coverage targets, and documentation updates.
- **Community feedback**: schedule regular feedback sessions with Harbour users to validate tooling ergonomics.

### Pipeline `make test-ast`

- `make test-ast` é agora o ponto único de automação para o projeto de extensão AST. Ele simplesmente delega para `scripts/test-ast.sh`, que recompila `libhbastlex` (`src/ast/lexer/`), reconstrói a CLI `hbast` (`utils/hbast/`) e aciona `tests/ast/Makefile` (`make tests`) para recompilar e rodar os binários auxiliares.
- A receita executa em sequência `tests/ast/smoke`, `tests/ast/rename`, `tests/ast/snapshot` e `tests/ast/builder-test`, garantindo que o lexer, o construtor de nós e o pipeline de serialização sigam consistentes após cada alteração.
- Ao final é impresso o caminho da CLI recompilada (`./bin/<plat>/<compiler>/hbast`) para quem desejar serializar fixtures ou rodar provas manuais.
- Sugestão: exporte `HB_PLATFORM`/`HB_COMPILER` conforme o ambiente local antes de rodar `make test-ast` para evitar recompilações redundantes; utilize `make -j` se desejar paralelizar compilação nos subprojetos.

## Next Steps

1. Instrumentar o pré-processador para produzir coordenadas originais/expandidas precisas e diferenciar trivia (comentários, espaços) sem heurísticas.
2. Integrar o grafo de macro (`macros.expansions`) ao payload completo do `hbast` (JSON/CBOR), reaproveitando o helper disponível.
3. Revisar o motor de rename/extract para consumir os rastros de macro, bloqueando cenários inseguros e emitindo relatórios de colisão.
4. Introduzir cache incremental por bloco de tokens, reaproveitando rastros existentes em edições locais.
5. Construir o encoder CBOR mantendo paridade com o schema JSON (`hbast.schema.json`) e iniciar o comando `hbast verify`. *Dependência:* aguarda a serialização de macro traces no payload completo para garantir ranges consistentes.*
6. Iniciar o builder de AST semântico reutilizando o fluxo de tokens categorizado. *Status:* pendente; será alimentado pelos tokens enriquecidos com grafo de macros e offsets confiáveis.


By investing in agents that speak a common language core, this Harbour fork can offer the same developer experience programmers expect from modern typed ecosystems—while staying true to Harbour’s heritage.
