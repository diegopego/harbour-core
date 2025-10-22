# Compiler Trace Dump Format (`--ast-trace-dump`)

Harbour exposes its AST/token instrumentation via the compiler itself. Running

```sh
harbour --ast-trace --ast-trace-dump=- source.prg
```

emits a JSON document containing the token stream, parser node events, lexer boundary markers, and preprocessor trace events captured during compilation. This file documents the shape of that payload so downstream tools can parse it without relying on the retired hbast prototype.

## Top-level structure

```json
{
  "tokens": [ /* TokenEvent[] */ ],
  "nodes": [ /* NodeEvent[] */ ],
  "boundaries": [ /* BoundaryEvent[] */ ],
  "preprocessor": [ /* MacroEvent[] */ ]
}
```

Each array is emitted in the order events occurred. Consumers should treat the stream as append-only and rely on the monotonic `sequence` field to correlate events. The shape is asserted by `tests/ast/ast_hbmk_ast_tests.c` (CLI dumps) and `tests/ast/ast_trace_tests.c` (macro ancestry).

> **Note (2025-10-25)**  
> The standalone hbast/hbrename tooling overlay was removed. Every field documented here is emitted directly by `hb_compAstTraceDumpJson()` inside the Harbour compiler; downstream utilities should consume this payload instead of depending on legacy modules.

## `TokenEvent`

```json
{
  "sequence": 42,
  "id": 17,
  "type": 16421,
  "marker": 0,
  "value": "FUNCTION",
  "module": "src/foo.prg",
  "line": 12,
  "column": 1,
  "endColumn": 9,
  "offset": 256,
  "endOffset": 264,
  "expansionId": 0,
  "expansionParentId": 0,
  "expansionDepth": 0
}
```

* `sequence`: strictly increasing number identifying the event order.
* `id`: stable token identifier emitted by the compiler (may repeat when the lexer reuses cached tokens).
* `type`: raw `HB_PP_TOKEN_*` type returned by the preprocessor.
* `marker`: marker index (non-zero when the token originates from macro markers).
* `value`: token lexeme (may be `null` for whitespace-only tokens).
* `module`: logical source module; `null` for tokens injected by the preprocessor.
* `line`, `column`, `endColumn`: 1-based coordinates in the module file.
* `offset`, `endOffset`: byte offsets relative to the start of the module file (`HB_SIZE_MAX` when unavailable).
* `expansionId`: `HB_PP_TRACEINFO::nExpansionId` for the macro expansion that produced the token (`0` when the token originates from primary source input).
* `expansionParentId`: the parent macro expansion ID, enabling reconstruction of nested macro calls without consulting compiler-internal pointers.
* `expansionDepth`: zero-based nesting depth derived from the expansion ancestry.

## `NodeEvent`

```json
{
  "sequence": 108,
  "id": 5,
  "kind": "FUNCTION",
  "phase": "ENTER",
  "tokenId": 17,
  "name": "DemoProc"
}
```

* `kind`: textual representation of `HB_COMP_AST_NODE_KIND` (e.g., `FUNCTION`, `FUNCTION_INIT`, `FUNCTION_EXIT`, `INLINE`, `CLASS`, `STATEMENT_IF`, `CODEBLOCK`, …).
* `phase`: `"ENTER"` or `"LEAVE"`.
* `tokenId`: compiler token ID associated with the node (0 when not available).
* `name`: optional symbol/function name captured during instrumentation.

Consumers reconstruct AST scopes by pairing ENTER/LEAVE events with the same `id`.

## `BoundaryEvent`

```json
{
  "sequence": 58,
  "tokenId": 23,
  "code": 123,
  "lexState": 4
}
```

Boundaries mark lexer state changes (end-of-expression, block delimiters, etc.). `code` corresponds to the token returned to the parser at that point; `lexState` reflects the lexer mode.

## `MacroEvent`

```json
{
  "sequence": 72,
  "ruleKind": "command",
  "macro": "DBG",
  "callModule": "src/foo.prg",
  "callLine": 15,
  "callColumn": 4,
  "callEndLine": 15,
  "callEndColumn": 12,
  "callOffset": 312,
  "callEndOffset": 320,
  "expansionId": 27,
  "expansionParentId": 13,
  "expansionDepth": 1,
  "source": "DBG(\"hello\")",
  "result": "?? \"hello\""
}
```

`expansionId`, `expansionParentId`, and `expansionDepth` mirror the ancestry stored in `HB_PP_TRACEINFO`. They provide a deterministic mapping for downstream tools without requiring pointer traversal inside the compiler.

## Binary representation

The compiler currently emits JSON only. Downstream tools requiring a compact binary format should serialise the dump themselves (e.g., using CBOR) until a canonical encoder is added to `hb_compAstTraceDumpJson()`.

## Compatibility notes

* Field names and structures reflect the trace state as of 2025‑10‑25. New fields will be appended (never removed/renamed) and should be ignored by parsers that do not understand them yet.
* All numeric fields use unsigned 64-bit (`HB_SIZE`) ranges.
* Strings are UTF-8 as produced by the compiler; consumers should not assume normalised casing or path formats.
