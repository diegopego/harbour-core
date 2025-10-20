# Phase 0 Divergence Ledger (2025-10-25)

The standalone AST tooling overlay (`src/ast/…`, `utils/hbast`, `utils/hbrename`, legacy cmocka suites/fixtures, associated docs) has been retired. Harbour’s compiler trace pipeline is now the single source of truth.

| Path / Area | Status | Notes |
| --- | --- | --- |
| `include/hbpp.h`, `src/pp/ppcore.c`, `src/compiler/*` trace hooks | **Keep** | Required for `HB_PP_TRACEINFO`-backed instrumentation. |
| `tests/ast/ast_trace_tests.c`, `ast_compilebuf_tests.c`, `ast_hbmk_ast_tests.c`, `scripts/test-ast.sh` | **Keep** | Exercise the compiler trace APIs; already scoped to core instrumentation. |
| Governance docs (`Agents.md`, `doc/agents/ast/draft.md`, `doc/agents/ast/progress.md`, `doc/agents/ast/hb_compilebuf_evaluation.md`) | **Keep** | Still drive planning and reporting. |
| `README-AST.md` | **Keep (updated)** | Documents how to consume compiler traces via `harbour --ast-trace --ast-trace-dump`. |
| `doc/agents/ast/serialization-format.md` | **Update** | Reframed to describe the compiler’s trace dump payload; reviews should ensure it stays in sync with `hb_compAstTraceDumpJson()`. |
| Legacy tooling artefacts (`src/ast/`, `utils/hbast`, `utils/hbrename`, tooling fixtures, hbast docs/schema) | **Removed** | No longer shipped with Harbour; consult git history if historical reference is needed. |
