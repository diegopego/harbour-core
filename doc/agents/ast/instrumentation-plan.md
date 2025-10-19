# Compiler Instrumentation Plan

## Executive Summary
- **Recommendation**: extend and reuse the existing Harbour compiler pipeline rather than maintaining the branch’s standalone lexer. This keeps refactoring features anchored to compiler truth, avoids grammar drift, and lets `HB_PP_TRACEINFO` power both token and AST projections.
- **Immediate objective**: land minimal instrumentation into `include/hbpp.h`, `src/pp/ppcore.c`, `src/compiler/complex.c`, `src/compiler/harbour.y`, and `src/compiler/hbcomp.c`, then migrate the experimental tooling (`src/ast/`, `utils/hbast`, `utils/hbrename`, related tests/docs) into a separate toolkit branch before reintroducing consumer APIs.
- **Data flow**: `HB_PP_TRACEINFO` (added on this branch) becomes the canonical macro-expansion payload. Tokens harvested in `hb_comp_yylex` will emit `HB_AST_EVENT_TOKEN` events carrying trace handles; parser reductions in `harbour.y` will publish `HB_AST_EVENT_NODE_{ENTER,LEAVE}` with stable IDs tied back to those tokens.
- **Timeline alignment**: instrumentation hardening (week of 2025-10-20), tooling extraction (week of 2025-10-27), instrumentation briefs and implementation restart (week of 2025-11-03).

## Status Check: `HB_PP_TRACEINFO`
- `HB_PP_TRACEINFO`, `HB_PP_TRACE_EVENT`, and `hb_pp_setTraceCallback()` **were introduced on this branch** (`include/hbpp.h`, `src/pp/ppcore.c`, `src/harbour.def`). Upstream Harbour at commit `cfb7bdc22c3bb722ddecc3b6c1c1a310e03a66ca` does not expose these structures.
- Impact:
  - The preprocessor now captures macro invocation metadata (module, line/column, byte offsets, call stack, expansion ID).
  - When realigned, compiler instrumentation can hand these structures directly to tooling, eliminating the duplicate macro-trace reconstruction that the standalone lexer attempted.
  - Tooling consumers must treat `HB_PP_TRACEINFO` as reference-counted objects (see retain/release helpers in `src/pp/ppcore.c`) and avoid copying strings without cloning to prevent leaks.

## Strategy Assessment

### Option 1 – Keep the separate lexer
- **Pros**
  - Rapid experimentation: can iterate on token schemas without touching core.
  - Isolated failures: regressions limited to tooling build/test.
  - Easier to prototype non-compiler features (e.g., rope-based incrementality).
- **Cons**
  - Guaranteed drift from Harbour grammar and semantics; every upstream change requires manual syncing.
  - Duplicate macro expansion logic lacks parity with `HB_PP_TRACEINFO`, risking mismatched ranges and trace stacks.
  - Higher maintenance load (two lexers, two AST builders, two fixture suites).
  - Performance hit from running both the real preprocessor and the tooling clone.
  - Testing burden doubles: need to assert equivalence between compiler output and tooling output continuously.

### Option 2 – Extend / reuse Harbour core
- **Pros**
  - Perfect grammar alignment: tooling consumes exactly the tokens/AST the compiler uses.
  - Leverages existing error handling, dialect flags, and macro semantics.
  - Lower long-term maintenance: one source of truth, one set of fixtures.
  - Unlocks deeper semantics (scope info, optimization hints) without reverse-engineering.
  - Simplifies performance story—no second pass.
- **Cons**
  - Requires careful instrumentation to avoid destabilizing the compiler.
  - Short-term complexity: need to design guarded hooks, event buffering, and APIs.
  - Integration with existing build/test harnesses must respect Harbour’s portability guarantees.
  - Tooling experiments must accept compiler release cadence.

### Recommendation
Adopt **Option 2**. The Harbour mission statement demands compiler-backed refactorings; carrying a forked lexer contradicts that requirement and increases risk. Guard instrumentation behind feature toggles where necessary, but keep the data flow inside the core. Maintain the existing tooling repo for experimentation, fed exclusively by compiler events emitted via the plan below.

## Hook Point Map

| File | Function / Rule | Insertion site | Captured data | Emitted event / action | Status (2025-10-21) |
| --- | --- | --- | --- | --- | --- |
| `src/compiler/hbcomp.c` | `hb_comp_new` / `hb_comp_free` | After the lexer owns `pPP` / before teardown frees it | `PHB_PP_STATE` callback slots, AST trace heap | `hb_compAstTraceInit()` installs the PP callback; `hb_compAstTraceShutdown()` clears it and releases retained buffers | Done |
| `src/compiler/complex.c` | `hb_comp_yylex` (post fetch) | Immediately after `pToken = hb_pp_tokenGet( pLex->pPP );` | Full `HB_PP_TOKEN`, including `pTraceInfo` | `hb_compAstTracePublishToken( pComp, pToken )` copies payload and retains trace info | Done |
| `src/compiler/complex.c` | `hb_comp_yylex` (pre-return) | Right before returning `0`, `ENDERR`, or any token code | Parser return code, `pLex->iState`, latest token ID | `hb_compAstTracePublishBoundary( pComp, code, pLex->iState )` sequences boundaries | Done |
| `src/compiler/harbour.y` | `Function` rule actions | After each `hb_compFunctionAdd()` call | Newly created `HB_HFUNC` handle, start token ID | `hb_compAstTraceNodeEnter()` records node enter events with stable IDs | Done |
| `src/compiler/hbmain.c` | `hb_compFinalizeFunction` | After jump fixups and before returning | Active `HB_HFUNC`, last token ID | `hb_compAstTraceNodeLeave()` emits the matching leave event | Done |
| `src/compiler/harbour.y` | Statement / expression reductions | Within actions that allocate expressions or manage stacks | Newly created `PHB_EXPR`, node stacks, codeblock tokens | `HB_AST_TRACE_EXPR` plus `hb_compAstTraceNodeEnterStack/LeaveStack` cover expressions, control flow, and codeblocks | Done (core set; backlog tracks remaining reductions) |
| `src/compiler/complex.c` / `tests` | Trace toggle plumbing | CLI/env switch handling prior to compilation | Feature flag state, outstanding retain counts | `hb_compAstTraceSetEnabled()` clears buffers on toggle changes; cmocka asserts zero outstanding traceinfo | Done |
| `src/compiler/hbcomp.c` | `hb_compParserRun` (single-module path) | After the `hb_pp_tokenGet()` guard when `fSingleModule` is true | Eager tokens consumed when bypassing the parser | Defer instrumentation; evaluate once single-module fixtures depend on trace buffering | Pending |

### Implementation status – 2025-10-21 oversight update

- Lifecycle wiring in `hb_comp_new()`/`hb_comp_free()` now mirrors the PP callback lifecycle and retains/release counters.
- `hb_comp_yylex` publishes token and boundary events with stable sequencing and retained `HB_PP_TRACEINFO`.
- Parser instrumentation spans functions, classes, control-flow statements, codeblocks, and expression reductions via `HB_AST_TRACE_EXPR`.
- `hbtraceast.c` guards toggles, clears buffers on state flips, and tracks retain/release balance with cmocka coverage.
- `scripts/test-ast.sh` passes with instrumentation enabled and disabled; `hbmk2 -w3` sweep remains outstanding before merge.

## Instrumentation Pipeline

### Event Flow
1. **Preprocessor layer** (`hb_ppcore.c`):
   - On every macro application, `hb_pp_traceinfoNew()` creates an `HB_PP_TRACEINFO` record with expansion ID, call site module/line/byte offsets, and parent pointer.
   - When the new compiler trace callback is set, `hb_pp_Process()` and friends emit `HB_PP_TRACE_EVENT` snapshots containing source/result text and associated trace info.
2. **Compiler lexer** (`hb_comp_yylex`):
   - Wrap retrieved `HB_PP_TOKEN` objects in `HB_COMP_AST_TRACE_TOKEN` payloads: `{ id, sequence, type, value, source_range, traceInfo }`.
   - Retain the referenced `HB_PP_TRACEINFO` until instrumentation releases it (mirroring current retain/release helpers).
3. **Parser reductions** (`harbour.y` / `hbmain.c`):
   - Function headers call `hb_compAstTraceNodeEnter()` with the freshly allocated `HB_HFUNC`, while `hb_compFinalizeFunction()` emits the matching leave event.
   - Node events (`HB_COMP_AST_TRACE_NODE_EVENT`) carry stable IDs, associated token IDs, and duplicated symbol names for downstream correlation.
4. **AST tooling bridge** (new module to be reintroduced post-extraction):
   - Consumes the token and node streams, serializes into JSON/CBOR matching the schema already documented in `serialization-format.md`.
   - Macro traces remain linked via `HB_PP_TRACEINFO.nExpansionId` → `MacroExpansion.expansion_id`.

### `HB_PP_TRACEINFO` Payload
- Fields:
  - `nExpansionId` – monotonic ID per preprocessor state.
  - `pszMacroName`, `pszCallModule` – null-terminated strings.
  - `iCallLine`, `iCallColumn`, `iCallEndLine`, `iCallEndColumn` – 1-based coordinates.
  - `nCallOffset`, `nCallEndOffset` – byte offsets within the module.

### Token and Boundary Payloads
- `HB_COMP_AST_TRACE_TOKEN`
  - `id`, `sequence` – stable identifiers for token tracking and total ordering.
  - `type`, `markerIndex`, `spaces`, `length` – verbatim metadata copied from the originating `HB_PP_TOKEN`.
  - `value`, `module` – heap-duplicated strings that survive beyond the preprocessor arena.
  - `line`, `column`, `endColumn`, `offset`, `endOffset` – 1-based coordinates and byte offsets for downstream mapping.
  - `traceInfo` – retained `HB_PP_TRACEINFO` handle (released when instrumentation clears the event queue).
- `HB_COMP_AST_TRACE_BOUNDARY`
  - `sequence` – shares the global event counter so tokens/boundaries can be merged chronologically.
  - `tokenId` – references the most recent token emitted prior to the boundary.
  - `code`, `lexState` – parser return code and lexer state snapshot at the boundary.
- `HB_COMP_AST_TRACE_NODE_EVENT`
  - `id`, `sequence` – mirror token sequencing so enter/leave events can be matched reliably.
  - `kind`, `phase` – node classification (function, etc.) and whether the event marks entry or exit.
  - `tokenId` – token associated with the declaration site (enter) or the latest token seen when exiting.
  - `name`, `handle` – duplicated symbol name and underlying compiler handle (`HB_HFUNC *`) for consumers needing deep linkage.
  - `pParent` – pointer to parent expansion; retain/release managed by instrumentation sink.
- Example JSON projection consumed by tooling:
  ```json
  {
    "expansion_id": 42,
    "macro_name": "DBG",
    "call_module": "src/foo.prg",
    "range": {"start": {"line": 12, "column": 7, "offset": 254}, "end": {"line": 12, "column": 20, "offset": 267}},
    "parent_expansion_id": 39
  }
  ```

## Incremental Migration Plan
1. **Week of 2025-10-20 – Core trace foundation**
   - Implement callbacks in `hb_comp_new` / `hb_comp_free`.
   - Instrument `hb_comp_yylex` to enqueue tokens and macro traces; guard behind `HB_AST_TRACE_ENABLED`.
   - Validate with `hbmk2 -w3` and existing compiler regression suites.
2. **Week of 2025-10-27 – Tooling extraction**
   - Move `src/ast/`, CLI utilities, schemas, and tests into a dedicated tooling repository branch.
   - Update `doc/agents/ast/divergence-ledger.md` once extraction lands.
   - Ensure `scripts/test-ast.sh` and cmocka suites run against compiler-emitted events via a shim.
3. **Week of 2025-11-03 – Parser event bridge**
   - Introduce `hb_astPublishNode*` helpers and modify `harbour.y` actions incrementally (start with function declarations, then statements, then expressions).
   - Rebuild LALR (`harbour.yyc`) and rerun cmocka + compiler acceptance tests.
4. **Rollback strategy**
   - Each step is gated by feature toggles; if instrumentation destabilizes parsing, disable the callback (`HB_AST_TRACE_ENABLED=0`) and revert the affected commit without impacting the extracted tooling.

## Risks & Mitigations
- **Grammar drift**: If parser actions become unstable, enforce nightly diffs against upstream `harbour.y` and isolate instrumentation macros in a single header to simplify rebases.
- **Performance overhead**: Token event emission must be amortized; use ring buffers and lazy JSON serialization to avoid excessive allocations.
- **Trace retention leaks**: Mismanaged retain/release could leak memory. Add cmocka tests that compile macro-heavy fixtures while checking for outstanding `HB_PP_TRACEINFO` references.
- **Cross-version compatibility**: Downstream tools must cope with absent trace callbacks (older compilers). Provide capability negotiation and ensure `hb_pp_setTraceCallback` is optional.
- **Testing coverage gaps**: Expand fixtures to cover nested macros, inline functions, and dialect switches. Require `hbmk2 -w3`, the `tests/ast` cmocka harness, and `scripts/test-ast.sh` before merging instrumentation patches.
- **Run log** (2025-10-21): `scripts/test-ast.sh` succeeds with instrumentation toggled on/off; `hbmk2 -w3` still pending for this brief.

## Open Questions & Verification Plan
- **Stable token IDs**: Confirm the formula (`file_id`, original range, token kind, macro depth) fits within existing Harbour data types. Prototype helper in `src/compiler/complex.c` and write cmocka tests.
- **Expression coverage**: Identify which grammar reductions must emit node events to fully reconstruct `PHB_EXPR`. Instrument a subset, run `scripts/test-ast.sh`, and diff serialized ASTs against the legacy tooling output.
- **Trace callback lifetime**: Ensure the callback remains valid across module boundaries (single-module vs multi-module). Instrument `hb_compParserRun`’s single-module path and add regression tests.
- **Error handling**: Determine whether syntax errors should flush pending events or keep them for diagnostics. Simulate parse errors in tests and manually inspect event stream.
- **Thread safety**: Harbour’s compiler is largely single-threaded, but confirm no concurrent `hb_pp_state` usage before assuming callbacks are safe. Audit call sites and document constraints.

Once these questions are resolved, the instrumentation plan is ready for delegation to the Compiler Instrumentation Agent and the AST Tooling Agent.
