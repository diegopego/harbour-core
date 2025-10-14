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

## Next Steps

1. Evoluir o lexer incremental para suportar reprocessamento por blocos e geração real de tokens.
2. Implementar persistência do `ExpansionTraceLog` e APIs de consulta cruzando tokens ↔ macros.
3. Construir o encoder CBOR e garantir paridade com o schema JSON (`hbast.schema.json`).
4. Iniciar o executável `hbast verify`, consumindo o schema e aplicando as regras de integridade descritas.

By investing in agents that speak a common language core, this Harbour fork can offer the same developer experience programmers expect from modern typed ecosystems—while staying true to Harbour’s heritage.
