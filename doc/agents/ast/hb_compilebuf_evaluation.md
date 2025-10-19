# hb_compileBuf Golden Test Evaluation

## Objective
- Determine how to execute Harbour compilations from the test harness (cmocka) using `hb_compileBuf()` so that instrumentation traces (tokens, AST nodes, preprocessor events) can be validated against golden snapshots.
- Identify integration points that allow capturing `HB_COMP` trace buffers produced during the compilation of in-memory sources.

## Findings
- `hb_compileBuf()` is exposed only as a Harbour function (`HB_FUNC( HB_COMPILEBUF )` in `src/compiler/hbcmplib.c`). It delegates to the C entry point `hb_compMainExt()`, which creates and frees an internal `PHB_COMP` instance.
- `hb_compMainExt()` currently returns only serialization buffers (`pOutBuf`) and does not expose the `PHB_COMP` instance before it is freed. Instrumentation state (`hb_compAstTrace*`) becomes inaccessible once `hb_compMainExt()` returns.
- The internal compiler pipeline requires environment/CLI preprocessing (`hb_compChkEnvironment`, `hb_compChkCommandLine`, `hb_compInitPP`, `hb_compIdentifierOpen`), making a minimal reimplementation in tests non-trivial without reusing `hb_compMainExt()`.
- Existing cmocka tests interact directly with `hb_compAstTrace*` APIs by instantiating `hb_comp_new()` manually, but they do not run the full parse pipeline.

## Proposed approach
1. **API surface**: Extend `hb_compMainExt()` (or add a sibling helper) to accept an optional callback that receives the active `PHB_COMP` prior to teardown. The callback can snapshot token/AST data for golden comparison.
2. **Test harness**: Create a cmocka suite (`tests/ast/ast_compilebuf_tests.c`) that:
   - Defines inline Harbour sources (or loads fixtures into memory).
   - Invokes the new helper (wrapping `hb_compMainExt`) with `--ast-trace` enabled and `hb_compAstTraceSetEnabled()` asserted.
   - Serializes `hb_compAstTraceToken/Boundary/Node` arrays to JSON (reuse existing tooling helpers or add a minimal exporter).
   - Compares the serialized payload with committed golden files (one per fixture) using stable ordering.
3. **Golden generation**: Reuse the current `scripts/test-ast.sh` pipeline to regenerate snapshots on demand (e.g., `scripts/gen-compilebuf-goldens.sh` that runs the cmocka binary in "record" mode).

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
