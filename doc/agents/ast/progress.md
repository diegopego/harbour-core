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
