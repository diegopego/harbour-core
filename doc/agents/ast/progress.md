# AST Tooling Progress Log

## 2025-10-25 Overseer session (Codex)
- Confirmed branch head `4ae9ecd262` builds on the pivot that deleted the first-attempt AST stack (`4da14975e24335cf02db6e340d4b0ec2e8ae75ce`); instrumentation now lives solely in `src/compiler/`.
- Reviewed coordination docs and cmocka suites (`tests/ast/ast_trace_tests.c`, `ast_compilebuf_tests.c`, `ast_hbmk_ast_tests.c`, `ast_preprocessor_trace_test.c`, `ast_hbmk2_fixtures_test.c`) to verify coverage: tokens/boundaries/nodes/PP events, diagnostics counters, compile-buffer paths, CLI dumps, PP fixtures, and `hbmk2 -w3` sweeps.
- Refreshed `doc/agents/ast/draft.md`, `README-AST.md`, `instrumentation-plan.md`, and `serialization-format.md` to reflect the compiler-trace-only architecture and current objectives for each agent.
- Logged outstanding instrumentation backlog items (`INLINE`/`INIT`/`EXIT`, macro-generated statements, `fSingleModule` audit) and delegated follow-ups to the appropriate agents.

- Added `tests/ast/python/ast_refactor_cli.py` to consume `hb_compAstTraceDumpJson()` output and emit rename/extract/reference payloads for experimentation; validated against `tests/ast/fixture_demo` and the new `tests/ast/ref_project` samples via trace-backed runs.
- Authored `doc/agents/ast/lsp-refactoring-cli.md` detailing the VS Code rename/extract/reference data contract, CLI usage, schema dependencies, and edit materialisation helper (now pointing to `tests/ast/python/apply_workspace_edit.py`).
- Scoped rename to the containing function/procedure using token sequence analysis and introduced a reference finder that aggregates call sites across modules while filtering out definitions; noted remaining limitations (macro expansions, token classification) for follow-up alignment with the AST tooling agent.
- No cmocka or `hbmk2` sweeps executed (tooling-only prototype); manual CLI runs with `--trace tests/ast/fixtures/fixture_demo.ast.json` provided output verification.
- Added pytest coverage in `tests/ast/python/test_refactor_cli.py` (invoked via `tests/ast/python/test-python.sh`) and integrated it into `tests/ast/Makefile` so `scripts/test-ast.sh` exercises rename/extract/reference cases alongside existing cmocka binaries.
- Landed `tests/ast/python/apply_workspace_edit.py` to replay WorkspaceEdit payloads onto copies of `.prg` files, enabling diffs against Harbour sources without touching the originals.
- Captured CLI outputs as checked-in fixtures (`tests/ast/fixture_demo.rename.prg`, `tests/ast/fixture_demo.helper_scope.prg`, `tests/ast/fixture_demo.extract.prg`, `tests/ast/ref_project/*.prg`, `tests/ast/fixtures/ref_project_supportfunc.references.json`) so Python tests leave refactored `.prg` and reference snapshots available for manual inspection.
