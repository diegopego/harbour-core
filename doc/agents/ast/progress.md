# AST Tooling Progress Log

## 2025-10-25 Overseer session (Codex)
- Confirmed branch head `4ae9ecd262` builds on the pivot that deleted the first-attempt AST stack (`4da14975e24335cf02db6e340d4b0ec2e8ae75ce`); instrumentation now lives solely in `src/compiler/`.
- Reviewed coordination docs and cmocka suites (`tests/ast/ast_trace_tests.c`, `ast_compilebuf_tests.c`, `ast_hbmk_ast_tests.c`, `ast_preprocessor_trace_test.c`, `ast_hbmk2_fixtures_test.c`) to verify coverage: tokens/boundaries/nodes/PP events, diagnostics counters, compile-buffer paths, CLI dumps, PP fixtures, and `hbmk2 -w3` sweeps.
- Refreshed `doc/agents/ast/draft.md`, `README-AST.md`, `instrumentation-plan.md`, and `serialization-format.md` to reflect the compiler-trace-only architecture and current objectives for each agent.
- Logged outstanding instrumentation backlog items (`INLINE`/`INIT`/`EXIT`, macro-generated statements, `fSingleModule` audit) and delegated follow-ups to the appropriate agents.

## 2025-10-25 LSP & Refactoring agent (Codex)
- Added `scripts/ast_refactor_cli.py` to consume `hb_compAstTraceDumpJson()` output and emit rename/extract `WorkspaceEdit` payloads for experimentation; validated against `tests/ast/fixture_demo` via trace-backed runs.
- Authored `doc/agents/ast/lsp-refactoring-cli.md` detailing the VS Code rename/extract data contract, CLI usage, schema dependencies, and edit materialisation helper (now pointing to `tests/python/apply_workspace_edit.py`).
- Noted current limitations (module-scoped rename, selection must intersect traced tokens, macro expansions skipped) and next-step alignment items with the AST tooling agent.
- No cmocka or `hbmk2` sweeps executed (tooling-only prototype); manual CLI runs with `--trace tests/ast/fixtures/fixture_demo.ast.json` provided output verification.
- Added pytest smoke coverage in `tests/python/test_refactor_cli.py` (invoked via `scripts/test-python.sh`) and integrated it into `tests/ast/Makefile` so `scripts/test-ast.sh` exercises rename/extract cases alongside existing cmocka binaries.
- Landed `tests/python/apply_workspace_edit.py` to replay WorkspaceEdit payloads onto copies of `.prg` files, enabling diffs against Harbour sources without touching the originals.
- Captured CLI outputs as checked-in fixtures (`tests/ast/fixture_demo.rename.prg`, `tests/ast/fixture_demo.extract.prg`) so Python tests leave refactored `.prg` snapshots available for manual inspection.
