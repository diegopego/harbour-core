# Compiler Instrumentation Plan

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
4. **Trace export**:
   - `hb_compAstTraceDumpJson()` renders the event stream as JSON. Downstream tools should parse this output (or call the APIs directly) instead of relying on a separate tooling layer.
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

### Harbour / Clipper Source Terminology Matrix

Use the following naming when documenting fixtures, tests, and instrumentation so we consistently distinguish preprocessor activity from compiler directives and runtime macro evaluation.

| Category (preferred term) | Harbour syntax / examples | Stage in toolchain | Notes / treatment |
| --- | --- | --- | --- |
| **Preprocessor directive** | `#define`, `#undef`, `#include`, `#if[n]def`, `#ifdef`, `#else`, `#error` | Runs in the Harbour preprocessor before tokens reach the parser | Produces replacement tokens or controls conditional compilation. Refer to these as **PP directives**. |
| **Preprocessor command / translate** | `#command`, `#translate`, `#xcommand`, `#xtranslate` | Preprocessor rewrite that expands to Harbour source prior to parsing | Commonly called “PP commands” in Clipper/Harbour docs. They are not runtime macros. |
| **Preprocessor pragma** | `#pragma`, including `#pragma -k*`, `#pragma __text`, `#pragma __stream`, `#pragma __endtext`, `#pragma /B-` | Preprocessor phase | Pragmas may toggle compiler switches or inject code templates. Refer to them as **PP pragmas**. |
| **Preprocessor text block** | `TEXT ... ENDTEXT`, `TEXT TO VAR ... ENDTEXT`, `TEXT INTO ... ENDTEXT` | Lowered to `#pragma __text/__stream/__cstream` before parsing | Treat these as **PP text blocks**; instrumentation should attribute expansions to the underlying pragmas. |
| **Runtime macro operator** | `&cSymbol`, `&( cExpr )`, `&("Func")()` | Evaluated by the VM at runtime | This is the traditional Clipper “macro”. When instrumentation observes these expressions, label them as **runtime macro operator** usage. |

By default, reserve the word **macro** for the runtime `&` operator or explicitly qualify it (e.g., “PP directive”, “PP command”) when referring to preprocessor constructs.

#### Canonical language reference

Harbour maintains source compatibility with CA-Clipper. When Harbour documentation does not spell out nomenclature or token classification, consult the original Clipper 5.3 Programming Guide (e.g. `clc53/doc/en/c53g01c.txt` from the `ng-hbdoc` archive). Treat that manual as the authoritative reference for naming conventions and statement categories when updating fixtures, tests, or docs.

