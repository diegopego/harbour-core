# Phase 0 Divergence Ledger

Definitions:
- **Keep**: Change should remain in Harbour core because it is required for compiler-backed instrumentation or shared governance.
- **Isolate**: Change should live outside the core tree (separate tooling distribution or experimental branch); drop from core when realigning.
- **Drop**: Change should be removed entirely rather than migrated (unused prototype or redundant asset).

| Path | Category | Recommendation | Dependencies / Notes |
| --- | --- | --- | --- |
| .gitignore | Build hygiene | Isolate | Ignores artefacts produced by `tests/ast` and `tests/tooling`; only needed if tooling stays in-tree. |
| Agents.md | Governance doc | Keep | Defines oversight model referenced by all sessions. |
| Makefile | Build wiring | Isolate | `test-ast` target invokes tooling harness (`scripts/test-ast.sh`). |
| README-AST.MD | Tooling doc | Isolate | Describes standalone AST toolkit. |
| doc/agents/ast/draft.md | Governance scratchpad | Keep | Overseer uses it for session notes. |
| doc/agents/ast/hbast-verify.md | Tooling doc | Isolate | Specification for `hbast verify` CLI. |
| doc/agents/ast/hbast.schema.json | Tooling schema | Isolate | JSON schema consumed by AST tooling. |
| doc/agents/ast/incremental-lexer.md | Tooling design | Isolate | Prototype lexer design, tied to `src/ast/lexer/`. |
| doc/agents/ast/progress.md | Governance log | Keep | Official progress record for oversight. |
| doc/agents/ast/serialization-format.md | Tooling design | Isolate | Serialization spec for tooling payloads. |
| include/Makefile | Build wiring | Isolate | Installs `ast/lexer/hbast_*.h`; only required if tooling headers ship with core. |
| include/ast/hbast_builder.h | Tooling header | Isolate | Depends on `src/ast/lexer/hbast_builder.c`. |
| include/ast/lexer/hbast_lexer.h | Tooling header | Isolate | Depends on `src/ast/lexer/hbast_lexer.c`. |
| include/hbpp.h | Core header | Keep | Adds trace structures/APIs used by `src/pp/ppcore.c`. |
| scripts/test-ast.sh | Tooling script | Isolate | Runs tooling tests; invoked by `Makefile:test-ast`. |
| src/Makefile | Build wiring | Isolate | Adds `ast` subtree to compiler build. |
| src/ast/Makefile | Tooling build | Isolate | Builds AST support library. |
| src/ast/lexer/Makefile | Tooling build | Isolate | Builds lexer/builder objects. |
| src/ast/lexer/hbast_builder.c | Tooling source | Isolate | Standalone builder impl; depends on tooling headers/tests. |
| src/ast/lexer/hbast_json.c | Tooling source | Isolate | Serialization helpers for tooling AST. |
| src/ast/lexer/hbast_lexer.c | Tooling source | Isolate | Incremental lexer prototype; uses Harbour PP but not core pipeline. |
| src/harbour.def | Export list | Keep | Exposes `hb_pp_setTraceCallback` for instrumentation consumers. |
| src/pp/ppcore.c | Core source | Keep | Implements macro trace bookkeeping, token metadata, and callback plumbing. |
| tests/ast/Makefile | Tooling tests | Isolate | Builds AST-specific cmocka suites. |
| tests/ast/ast_builder_test.c | Tooling tests | Isolate | Exercises `src/ast/lexer/hbast_builder.c`. |
| tests/ast/ast_preprocessor_trace_test.c | Tooling tests | Isolate | Depends on tooling lexer and trace fixtures. |
| tests/ast/ast_rename_test.c | Tooling tests | Isolate | Uses tooling builder and fixtures. |
| tests/ast/ast_smoke_test.c | Tooling tests | Isolate | Relies on `src/ast/` modules. |
| tests/ast/ast_snapshot_test.c | Tooling tests | Isolate | Consumes tooling JSON fixtures. |
| tests/ast/fixture_demo.hbast.json | Tooling fixture | Isolate | Consumed by tooling snapshot tests. |
| tests/ast/fixture_demo.prg | Tooling fixture | Isolate | Fixture for tooling tests. |
| tests/ast/fixture_extrahelpers.ch | Tooling fixture | Isolate | Included by tooling tests. |
| tests/ast/fixture_helpers.ch | Tooling fixture | Isolate | Included by tooling tests. |
| tests/ast/preprocessor/fixtures/command_trace.ppo | Tooling fixture | Isolate | Paired with tooling trace tests. |
| tests/ast/preprocessor/fixtures/command_trace.prg | Tooling fixture | Isolate | Paired with tooling trace tests. |
| tests/ast/preprocessor/fixtures/command_trace.trace.json | Tooling fixture | Isolate | Paired with tooling trace tests. |
| tests/ast/preprocessor/fixtures/macro_trace.ppo | Tooling fixture | Isolate | Paired with tooling trace tests. |
| tests/ast/preprocessor/fixtures/macro_trace.prg | Tooling fixture | Isolate | Paired with tooling trace tests. |
| tests/ast/preprocessor/fixtures/macro_trace.trace.json | Tooling fixture | Isolate | Paired with tooling trace tests. |
| tests/tooling/cmocka/README.md | Tooling doc | Isolate | Describes tooling cmocka harness. |
| tests/tooling/cmocka/test.c | Tooling harness | Isolate | Launches tooling suites. |
| utils/Makefile | Build wiring | Isolate | Adds `hbast` and `hbrename` utilities. |
| utils/hbast/Makefile | Tooling build | Isolate | Builds AST CLI. |
| utils/hbast/hbast.c | Tooling utility | Isolate | CLI for tooling AST export. |
| utils/hbrename/Makefile | Tooling build | Isolate | Builds rename prototype. |
| utils/hbrename/hbrename.c | Tooling utility | Isolate | Prototype rename CLI, depends on tooling builder. |
