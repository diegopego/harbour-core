# hb_compileBuf Golden Test Evaluation

## Objective
- Determine how to execute Harbour compilations from the test harness (cmocka) using `hb_compileBuf()` so that instrumentation traces (tokens, AST nodes, preprocessor events) can be validated against golden snapshots.
- Identify integration points that allow capturing `HB_COMP` trace buffers produced during the compilation of in-memory sources.

## Findings
- `hb_compileBuf()` is exposed only as a Harbour function (`HB_FUNC( HB_COMPILEBUF )` in `src/compiler/hbcmplib.c`). It delegates to the C entry points `hb_compMainExt()` / `hb_compMainExtModule()`, which create and free an internal `PHB_COMP` instance.
- `hb_compMainExtModule()` layers the virtual-module-name support on top of the existing callback-enabled API so in-memory compilations can surface stable filenames without breaking older callers, and `--ast-trace-dump` / `HB_AST_TRACE_DUMP` let the compiler emit JSON traces directly to files or stdout.
- The internal compiler pipeline requires environment/CLI preprocessing (`hb_compChkEnvironment`, `hb_compChkCommandLine`, `hb_compInitPP`, `hb_compIdentifierOpen`), making a minimal reimplementation in tests non-trivial without reusing `hb_compMainExt()`.
- Existing cmocka tests interact directly with `hb_compAstTrace*` APIs by instantiating `hb_comp_new()` manually, but they do not run the full parse pipeline.

## Proposed approach
1. **API surface**: `hb_compMainExt()` exposes an optional finish callback (`HB_COMP_FINISH_FUNC`) invoked immediately before compiler teardown, while `hb_compMainExtModule()` adds the virtual module-name parameter for buffer compilations.
2. **Test harness**: Create a cmocka suite (`tests/ast/ast_compilebuf_tests.c`) that:
   - Defines inline Harbour sources (or loads fixtures into memory).
   - Invokes the new helper (wrapping `hb_compMainExt`) with `--ast-trace` enabled and `hb_compAstTraceSetEnabled()` asserted.
   - Serializes `hb_compAstTraceToken/Boundary/Node` arrays to JSON (reuse existing tooling helpers or add a minimal exporter).
   - Compares the serialized payload with committed golden files (one per fixture) using stable ordering.
3. **Golden generation**: Reuse the current `scripts/test-ast.sh` pipeline to regenerate snapshots on demand (e.g., `scripts/gen-compilebuf-goldens.sh` that runs the cmocka binary in "record" mode).

## Current status
- The finish callback in `hb_compMainExtModule()` is exercised by `tests/ast/compilebuf-tests`, which compiles an in-memory stub (`FUNCTION Demo()`) with `--ast-trace` enabled, supplies a virtual module name, and asserts that token events are captured alongside the persisted `compilebuf_fixture.c` output.
- The new CLI surface is covered by `tests/ast/hbmk-ast-tests`, invoking the compiler on real `.prg` fixtures with `--ast-trace` and `--ast-trace-dump` while comparing the emitted JSON to committed fixtures.
- The harness currently validates token sequencing; AST node capture remains optional until parser hooks guarantee coverage for buffer-based compilations.
- Next step is to extend the harness to serialize trace buffers (tokens/boundaries/nodes) into deterministically ordered snapshots and compare them against golden fixtures.

## Snapshot regeneration workflow (2025-10-25)
1. **Refresh CLI golden dumps** – run the compiler against each fixture from the repository root so `-iinclude` resolves the shared headers:
   ```
   bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/fixture_demo.ast.json tests/ast/fixture_demo.prg
   bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/fixture_blocks.ast.json tests/ast/fixture_blocks.prg
   bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/fixture_ppdirectives.ast.json tests/ast/fixture_ppdirectives.prg
   bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/fixture_statements.ast.json tests/ast/fixture_statements.prg
   bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/fixture_expressions.ast.json tests/ast/fixture_expressions.prg
   bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/fixture_includes.ast.json tests/ast/fixture_includes.prg
   bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/fixture_compat_clipper.ast.json tests/ast/fixture_compat_clipper.prg
   bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/fixture_compat_harbour.ast.json tests/ast/fixture_compat_harbour.prg
   ```
2. **Verify harness parity** – execute the hbmk harness (covers both default and `-m` modes) and the compile-buffer tests to confirm runtime capture continues to match the refreshed snapshots:
   ```
   tests/ast/hbmk-ast-tests
   ./tests/ast/compilebuf-tests
   ```
3. **Enforce warning-free fixtures** – keep the matrix clean under `hbmk2 -w3`:
   ```
   bin/linux/gcc/hbmk2 -w3 tests/ast/fixture_demo.prg
   bin/linux/gcc/hbmk2 -w3 tests/ast/fixture_blocks.prg
   bin/linux/gcc/hbmk2 -w3 tests/ast/fixture_ppdirectives.prg
   bin/linux/gcc/hbmk2 -w3 tests/ast/fixture_statements.prg
   bin/linux/gcc/hbmk2 -w3 tests/ast/fixture_expressions.prg
   bin/linux/gcc/hbmk2 -w3 tests/ast/fixture_includes.prg
   bin/linux/gcc/hbmk2 -w3 tests/ast/fixture_compat_clipper.prg
   bin/linux/gcc/hbmk2 -w3 tests/ast/fixture_compat_harbour.prg
   ```
4. **Clean temp artefacts** – remove any generated `.c` files or stray stdout captures (`fixture_*.c`, `compilebuf_fixture.c`, etc.) before handing off so subsequent sessions start from a tidy tree.
5. **Package the trace pack** – mirror the distribution step with `zip -j tests/ast/trace-pack/core-trace-pack-2025-10-25.zip tests/ast/fixture_*.prg tests/ast/fixture_*.ch tests/ast/fixtures/fixture_*.ast.json`.

## Open questions
- Should the callback own releasing retained `HB_PP_TRACEINFO` references, or should `hb_compAstTraceShutdown()` remain responsible after snapshotting?
- Where should JSON serialization live? Options include:
  - Embedding a lightweight serializer in the cmocka test (no new runtime dependencies).
  - Reusing tooling code (`src/ast/`), which may introduce circular dependencies when tooling is extracted.
- Do we need to support both success and failure compilations? (e.g., syntax errors should produce deterministic traces for diagnostics).

## Next steps
1. Prototype the callback-based extension to `hb_compMainExt()` and verify instrumentation data is observable before teardown.
2. Decide on snapshot format (JSON vs. binary) and storage location (`tests/ast/golden/compilebuf/*.json`).
3. Implement the cmocka runner and integrate with `tests/ast/Makefile`, mirroring the `hbmk2-fixtures` target.
4. Document regeneration instructions in `doc/agents/ast/progress.md` and ensure `hb_compileBuf` coverage enters the verification matrix.
