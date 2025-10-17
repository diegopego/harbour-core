# gpt should ignore this whole file draft.md

Where We Stand?
What are the next steps?

You are working on the Harbour Core repository and must broaden regression coverage for the ongoing language-refactoring branch.

Primary objectives:
1. Expand the automated test suite by authoring new CMocka-based tests and related fixtures.
2. Instrument the Harbour code—`src/pp` and any other relevant modules—whenever it improves verification, and treat the captured instrumentation output as the golden reference for the refactoring tools’ expected behaviour.
3. Compile and lint every Harbour `.prg` test input with `hbmk2 -w3` before any additional processing to guarantee the snippets are valid.
4. Generate `.ppo` artifacts whenever they strengthen assertions or core Harbour investigations, and store them alongside the fixtures.
5. Prefer using Harbour’s own compiler and linter to validate and exercise the `.prg` fixtures during test execution.


Harbour Tooling Roadmap

Stage 1 · Lexer & AST Baseline

✅ Current branch: token snapshots with real nodes, JSON/CBOR emitters, macro-aware rename probe.
Run locally: make -C tests/ast && tests/ast/snapshot && tests/ast/rename.
Stage 2 · Node Builder Integration

Implement full AST node construction (parser hookup, symbol IDs).
Extend tests/ast/* fixtures to cover nested procedures, includes, multi-module macros.
Add JSON/CBOR round-trip validation (hbast verify draft).
Stage 3 · Refactoring Core

Build rename engine over node IDs + macro expansions (detect conflicts, module boundaries).
Introduce C harness (e.g. Criterion) for rename scenarios: safe rename, blocked rename via macro, cross-file updates.
Provide CLI command (hbrename apply) and update tests to assert file diffs.
Stage 4 · Language Server Plumbing

Expose refactor commands via service API (HTTP/IPC stub).
Prototype VS Code extension calling the CLI/LSP shim; smoke-test rename + snapshot preview.
Add integration tests (Node/TypeScript) to simulate editor rename, verifying transformed files.
Stage 5 · CBOR/JSON Verification & Toolchain

Implement hbast verify to validate both formats against schema and structural invariants.
Automate artifacts checks in tests (fixtures hashed, JSON vs CBOR parity).
All steps remain local-friendly; run suites before the next commit/push to confirm stability.

Next ideas

Teach the macro tracer to expose MODULE_NAME/INLINE_HELPER entries—the tokens already appear from extrahelpers.ch, but the PP still tags them as the VALUE expansion. That would let us tighten the snapshot expectations.
Expand the builder to recognise additional statement kinds (e.g. parameter lists, default arguments) so downstream rename tooling has finer-grained nodes.