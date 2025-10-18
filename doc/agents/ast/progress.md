# AST Tooling Progress Log

## 2025-10-18 – Oversight Baseline Review

- **Repository status**: `git status -sb` shows local edits to `Agents.md` (roles rewrite) and an existing modification in `draft.md`. No other worktree changes detected; outstanding content in `draft.md` must be preserved when committing.
- **Compiler token/AST flow snapshot**:
  - `hbmain.c` initialises the runtime, loads compiler configuration, and hands execution to `hb_compMain()` in `hbcomp.c`.
  - `hbcomp.c` orchestrates command parsing, preprocessor setup, and compilation units; it drives the scanner via `hb_compParserDo()`, wiring the lexer (`hb_pp_lex*` routines in `complex.c`) to the parser.
  - `complex.c` owns the token pipeline: it integrates the Harbour preprocessor, produces `HB_PP_TOKEN` records, tracks source modules, and feeds tokens to the parser while managing include stacks and macro expansion.
  - `harbour.y` defines the grammar and semantic actions that build `PHB_EXPR` trees; its generated counterpart `harbour.yyc` contains the C implementation (`hb_yyparse`, reduction handlers) consumed at build time.
  - Semantic data is layered through `ppcore.c`/`function.c` helpers that attach scope and symbol metadata to `PHB_EXPR` nodes before code generation.
- **New branch additions (ast-3rd-experiment)**:
  - `src/ast/lexer/hbast_lexer.c` implements an incremental lexer that mirrors the preprocessor, capturing macro traces (`HB_PP_TRACEINFO`), source ranges, and token history snapshots.
  - `src/ast/lexer/hbast_builder.c` provides a standalone builder to assemble node and symbol graphs, backed by dynamic arrays and JSON/CBOR serialization helpers in `src/ast/lexer/hbast_json.c`.
  - Tooling docs under `doc/agents/ast/` describe serialization schemas, incremental lexer design, and verification strategy (`hbast-verify.md`, `incremental-lexer.md`, etc.).
  - Tests in `tests/ast/` exercise the prototype lexer/builder via cmocka suites and snapshot fixtures, plus shell helpers in `scripts/test-ast.sh`.
- **Gaps between compiler AST and tooling AST**:
  - Tooling currently replays preprocessor output independently; the compiler proper (`complex.c`, `harbour.y`) is not yet instrumented to emit the same token stream, causing duplication and potential drift (ranges, macro depth, trivia channels).
  - `PHB_EXPR` trees built during parsing are not exposed to the tooling builder; the JSON/CBOR output is generated from a parallel structure that lacks guaranteed parity with compiler semantics (symbol resolution, codegen flags, hidden nodes).
  - Macro trace fidelity depends on reconstructing `HB_PP_TRACEINFO` snapshots outside the compiler, with no guarantees that parser-time transformations (e.g., implicit statements, aliasing) are reflected.
  - No current bridge exists from the compiler’s semantic passes (scope resolution, optimisations) to the tooling layer, leaving refactoring features blind to compiler-only insights.
- **Immediate follow-ups**:
  - Finalise instrumentation plan for hooking `complex.c` token lifecycle and `harbour.y` reductions without regressing compiler behaviour.
  - Align tooling builder schemas with real `PHB_EXPR` node kinds to avoid divergence as grammar evolves.
  - Document outstanding change in `draft.md` before committing to keep historical context intact.

## 2025-10-18 – Oversight Instrumentation Kickoff

- **Scope survey**: Reviewed divergence against `cfb7bdc22c3bb722ddecc3b6c1c1a310e03a66ca`; branch introduces macro trace plumbing in `src/pp/ppcore.c` and standalone tooling in `src/ast/lexer/`.
- **State of instrumentation**: Compiler remains uninvolved with the tooling prototypes—`HB_PP_TRACEINFO` data is extended but not yet streamed to consumers, and `PHB_EXPR` trees stay private to parser actions.
- **Prioritised actions**:
  - Author `doc/agents/ast/instrumentation-plan.md` capturing concrete hook points (token emission in `complex.c`, reduction callbacks in `harbour.y`, aggregation in `hbcomp.c`).
  - Prepare delegation brief for the Compiler Instrumentation Agent to surface stable token IDs and macro traces from the real preprocessor into the AST pipeline.
  - Prepare delegation brief for the AST Tooling Agent to map `PHB_EXPR` kinds onto the JSON/CBOR schema, identifying missing coverage in `hbast_builder`.
- **Risks & guards**: `ppcore.c` modifications touch critical preprocessor paths; mandate `hbmk2 -w3` and cmocka suites before any commit integrating new hooks.
- **Follow-ups**: Update `doc/agents/ast/draft.md` with delegation outlines and testing matrix revisions ahead of implementation sessions.

## 2025-10-18 – Phase 0 Branch Assessment Kickoff

- **Mandate**: Before pushing instrumentation work, determine whether `ast-3rd-experiment` should be retained as-is, split, or rewritten to align with Harbour core goals.
- **Branch divergence snapshot**:
  - Core touchpoints: `.gitignore`, top-level `Makefile`, `include/Makefile`, `include/hbpp.h`, `src/Makefile`, `src/harbour.def`, `src/pp/ppcore.c`, `utils/Makefile`.
  - Tooling overlay: new docs, schemas, `src/ast/` subsystem, CLI utilities (`utils/hbast`, `utils/hbrename`), dedicated tests/fixtures under `tests/ast/` and cmocka harness.
- **Evaluation tracks**:
  1. Validate whether core changes (preprocessor trace plumbing, build wiring) are production-ready or should be side-loaded via optional modules.
  2. Decide which tooling assets live outside the Harbour core tree (e.g., separate repo/tooling pack) vs what must integrate tightly.
  3. Identify any prototype code that should be dropped or rewritten before alignment.
- **Next actions**:
  - Catalogue each divergent file with rationale (keep, isolate, drop) and dependencies.
  - Draft alignment proposal documenting boundary between core compiler and external tooling deliverables.
  - Use the proposal to drive subsequent delegation briefs for implementation agents.
  - Documented governance rule in `Agents.md` requiring Overseer sessions to only suggest commit messages (no direct commits).
  - Divergence ledger captured in `doc/agents/ast/divergence-ledger.md`.
  - Decision criteria recorded; alignment memo now includes timeline and sequencing milestones (2025-10-20 core fixes, 2025-10-27 tooling extraction, 2025-11-03 instrumentation restart) in `doc/agents/ast/draft.md`.
  - Instrumentation plan with hook mapping and migration strategy published in `doc/agents/ast/instrumentation-plan.md`.
  - Verification matrix outlining mandatory suites/fixtures/pass-fail criteria documented in `doc/agents/ast/draft.md`.
