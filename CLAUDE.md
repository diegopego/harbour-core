# harbour-core (branch feature/compiler-ast-dump)

Harbour fork carrying the AST dump mechanism (`-x`, `src/compiler/compast.c`
plus one-line hooks gated by a flag). Consumer: ~/devel/hbrefactor.

**Everything in this repository is written in ENGLISH** — code, comments,
documentation, and commit messages. It is the upstream Harbour project: a
contributor anywhere in the world has to be able to read it, and this branch is
meant to be upstreamable (see CHANGELOG.md). The working language with Diego is
Portuguese; what lands in this tree is not.

## Working rules

- **Compile every test/fixture .prg BEFORE using it in any test**
  (`bin/linux/gcc/harbour file.prg -n -q0`, or the whole project through
  hbmk2). An invalid fixture produces misleading diagnostics — a pp expansion
  error in a fixture was once mistaken for a regression in the mechanism.
- Zero impact without `-x`: every new hook in the compiler/pp is gated
  (`fAst` / `fTrackPos`); the proof is byte-identical `.hrb` with and without
  `-x`.
- MIND THE RELINK: `make` does not always relink `bin/linux/gcc/harbour` when
  only `libhbcplr.a`/`libhbpp.a` changed — check with
  `strings bin/linux/gcc/harbour | grep ast-` and, if needed,
  `rm bin/linux/gcc/harbour && make`. **This applies to `hbmk2` as well**: it
  compiles .prg with the compiler it EMBEDS (it links `libhbcplr`), so a stale
  hbmk2 emits dumps of the old schema even with a fresh `harbour` — this already
  cost one diagnosis (ast-1 dumps without ppRules through hbmk2 while the direct
  harbour emitted ast-2).
- Commits only with Diego's explicit authorization **for THAT commit**;
  finishing or approving the work does not authorize the commit. One request per
  commit — do not chain them. No push unless asked.
- **Fable only** (Diego, 2026-07-07; revokes the earlier delegation rule): no
  opus/sonnet subagents — solving capacity is worth more than token savings.
- A durable rule or preference of this repo goes HERE (versioned), not into
  Claude's private memory (which does not travel with the repo).

## Harbour (the language) — traps when writing .prg

- **Never name a variable that spells a keyword in uppercase**: Harbour is
  case-insensitive and reads identifiers in uppercase — `LOCAL nIL` becomes the
  reserved `NIL` (`E0030 syntax error`). Avoid `nIL`, `cFor`, and friends.
- **MEMVAR before PRIVATE/PUBLIC**: referencing a `PRIVATE`/`PUBLIC` without a
  compile-time `MEMVAR` declaration yields W0002 on creation and W0001 on every
  use — under `-es2` the build fails. Idiom: `MEMVAR xCfg` / `PRIVATE xCfg := 7`.
- **Line comments `//` in .prg** (not `/* */`): a `*/` appearing in the content
  (e.g. `assert_*/`) closes the block early and the rest turns into code. Apply
  to new/edited code, no mass conversion. (In C the project codestyle mandates
  `/* */` and forbids `//` — the opposite rule.)
- **Check the behaviour in `src/` BEFORE asserting it** (do not theorize):
  read/grep the relevant file. To debug, prefer `hb_TraceLog()` +
  `hb_TraceFile()` (not `OutStd`/`QOut`/`?`, which go through the GT and skew the
  measurement). Gotcha: `Empty(" ")` is `.T.` — use `Len(c) == 0` for "empty".
