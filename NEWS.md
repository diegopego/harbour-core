<!-- changelog-baseline: harbour-core@611e0c45cc (feature/compiler-ast-dump) -->
<!-- Delta pointer. Everything after this commit is NOT yet described here.
     To catch up:  git log 611e0c45cc..HEAD   (see § Maintaining this file). -->

# NEWS — `feature/compiler-ast-dump`

**Audience: the Harbour programmer.** What this branch lets you *do*, what changes
in your day, and where it stops.

> **Why `NEWS.md` and not `CHANGELOG.md`.** The GNU convention this project already
> follows draws exactly the line that matters here: **`ChangeLog` is for developers**
> (every change, in detail) — **`NEWS` is for users** (what became possible). Harbour
> already ships [`ChangeLog.txt`](ChangeLog.txt); this is its missing counterpart.
>
> So: **this is not a changelog for contributors — for that, the changelog is git
> itself.** The commit history is complete, precise and dated; it is where the *how*
> lives (which function, which hook, which structure). This file answers the other
> question, the one git does not: *what can I now do, and where does it bite me?* If
> you find implementation detail here, that is a bug in this file.

---

## What this branch gives you, in one paragraph

Two things, and they are independent. **`-kt`** turns the `AS` annotations you
already write into **invariants the runtime enforces** — a violation fails loudly,
at the site. **`-x`** makes the compiler **export what it knows** while compiling, so
tools (a refactorer, a linter, an IDE) can act on facts instead of guessing with
regexes. Neither costs you anything if you don't ask for it: without the switch, your
compiled program is **identical, byte for byte**, to the one stock Harbour produces.

---

## 2026-07-09 → 07-12 — `-kt`: your `AS` annotations stop being decoration

You have been able to write this for years:

```harbour
FUNCTION Total( oConta AS CLASS Conta, nQtd AS NUMERIC )
   LOCAL nSaldo AS NUMERIC := oConta:Saldo()
   RETURN nSaldo * nQtd
```

…and the runtime **never checked any of it**. The annotation was a comment with
better syntax: parsed, used for a `-w3` warning, and dropped. Pass something that is
not a `Conta` and you found out three call frames later, if at all.

Compile with **`-kt`** and every annotation you wrote becomes a real check — on the
parameters when the function is entered, after each assignment to an annotated local,
and on the `RETURN` of a function you `DECLARE`d. A violation fails **at the site**,
naming it.

The `is-a` test runs against the **live object**, so a class your program assembles at
runtime passes just fine — the check knows things static analysis cannot.

**It reaches inside codeblocks too.** An annotated block parameter is checked on
**every `Eval`**:

```harbour
bSoma := {| oC AS CLASS Conta | oC:Saldo() }   // checked on every Eval, under -kt
```

*(Before this branch, `AS CLASS` on a block parameter was silently **thrown away** by
the parser. You could write it, and nothing whatsoever happened.)*

**Cost, honestly:** `-kt` emits real checks and they run. It is a development and CI
mode, not a free assertion. Your release build stays exactly as fast as it was.

### Two bugs fixed along the way (these hit stock Harbour too)

- **A compiler crash.** The compiler dereferenced a null class name and
  **segfaulted** — reachable from any module that knew about a class. Fixed; as a
  bonus, the `W0025` warning now names the real class instead of printing `'(null)'`.
- **A false warning.** `W0019` ("duplicate declaration") fired even when the second
  declaration merely **filled in a missing return type** — a completion, not a
  conflict. Under `-w3 -es2` that turned a legitimate build into a failure. It is
  silent now; a genuine conflict still warns.

## 2026-07-05 → 07-12 — `-x`: the compiler tells you what it knows

```
$ harbour app.prg -xout/          # writes out/app.ast.json while compiling
```

Every Harbour tool that ever tried to rename a variable safely hit the same wall:
**the source text lies.** By the time your code reaches the compiler, the
preprocessor has rewritten it — your `#command` became something else, one name got
pasted into a symbol, another got dumped into a string. A tool reading the `.prg`
with regexes cannot tell which is which, so it guesses, and eventually it corrupts
your code.

The compiler never had that problem: it *knows*. This switch makes it say so. While
it compiles, it writes a JSON file with the facts — each token with its **exact line
and column** (surviving the preprocessor's rewrites), every declaration with its real
scope, every variable use with its access mode, calls, message sends, and the
expression tree of each statement.

**Why this may matter even if you never write a tool:** it is what lets a refactoring
tool act on *facts* instead of on text — and refuse, honestly, when the fact isn't
there. The tool built on it is [hbrefactor](https://github.com/diegopego/hbrefactor).

### What each version added

The file carries a `"schema"` field; a consumer should check it.

| schema | what a tool can now know |
|---|---|
| `ast-1` | the token stream with exact positions — **surviving `#command`/`#translate` rewrites** |
| `ast-2` | **the preprocessor rules, and every application of them** — the words of a DSL are eaten by the pp and never reach the parser, so before this they existed *nowhere* |
| `ast-3` | **where a synthesized name came from**: which marker of which directive, and how (copied through, pasted into a symbol, or dumped into a string) |
| `ast-4` | the declared **types** (`AS`, `DECLARE`, `_HB_CLASS`, `_HB_MEMBER`), carried through as written |
| `ast-5` | **the directive seen from inside** — its match and result patterns, so a tool can name (and edit) what is *inside* a `#command` |
| `ast-6` | which value is the one being `RETURN`ed (before, a consumer had to guess) |
| `ast-7` | which annotations `-kt` is enforcing |
| `ast-8` | the same, **inside codeblocks** |
| `ast-9` | the exact byte position of a name **as you wrote it** — so a tool can insert an `AS CLASS` annotation without guessing where |
| `ast-10` | **declared inheritance** (`_HB_SUPER`): a directive can state that a class descends from another, and a tool can trust it |
| `ast-11` | a codeblock's own declared parameters; and the `Self` a directive generates for an `INLINE` block carries its class |
| `ast-12` | **whether a directive turns a name into code** (pastes it into a symbol, dumps it into a string) or merely lets it **pass through** — the difference between renaming your local and renaming a macro that manufactures function names |
| `ast-13` | **a directive that writes another directive** (`hbclass.ch` does it constantly) — the generated rule is linked back to whatever created it |
| `ast-14` | a marker that a rule matches but never uses is no longer indistinguishable from a word of the rule itself |
| `ast-15` | **which** word of the rule a token matched — needed because `#command` accepts its keywords **abbreviated** (from 4 letters on), so `GRAV` may be the rule's own keyword *or* `GRAVAR` cut short, and only the preprocessor knows which |
| `ast-16` | **a directive has a lifetime, and which family it belongs to.** `#xuncommand` (and friends) *switch a directive off* partway through a file — the preprocessor did that and told nobody, so a tool renaming the directive left the switch-off behind, pointing at a name that no longer existed, and the directive silently leaked past the point where you turned it off. Also: Harbour has **three** directive families (keywords abbreviable, exact, exact-and-case-sensitive) and this file used to describe the third as if it were the first — i.e. it claimed an exact rule accepts abbreviations |

`hbmk2` passes `-x` through per module, so it works with whatever project shape it
already accepts.

### Declarations your own directives can emit

If you build DSLs with `#command`, these let a tool understand the code your DSL
generates:

- `_HB_CLASS` / `_HB_MEMBER` — declare a class and its members;
- `_HB_SUPER` — declare that a class descends from another;
- `_HB_INLINESELF` — declare the class of the `Self` your directive generates for an
  inline block. **Informative only**: `-kt` never enforces it.

---

## Guarantees

- **Your build does not change unless you ask.** Without `-x`, nothing is exported;
  without `-kt`, no check is emitted. The compiled output is **identical, byte for
  byte**, to stock Harbour — verified by rebuilding the whole tree both ways and
  comparing.
- **Facts, never guesses.** Everything exported is something the compiler or the
  preprocessor had *already worked out* while doing its job. Where a fact did not
  exist yet, the compiler was taught to keep it — it was never inferred.

## Limits (honest)

- This is a **branch**, not upstream Harbour. To use it, you build this tree.
- The dump **schema is not frozen** (`ast-1` → `ast-16`). Check the `"schema"` field.
- After changing the compiler, rebuild **`harbour` *and* `hbmk2`** — `hbmk2` embeds
  the compiler, and a stale one keeps emitting the old schema without reporting any
  error.
- `-kt` is not free at runtime (see above).

## Maintaining this file

One entry per delivery, **written for the Harbour programmer** — never for the
contributor (that is what git is for). The HTML comment at the top is the **delta
pointer**: the last commit already described here. If the flow is skipped for a
while, nothing is lost — `git log <baseline>..HEAD` says exactly what is missing.
After writing, advance the pointer.

The consumer project keeps its own changelog with its own pointer, and the two are
maintained together (hbrefactor's `/update-manual` skill). **Every repository with a
new commit gets its own entry.**

*(Its counterpart is [`ChangeLog.txt`](ChangeLog.txt) — the Harbour project's
official, developer-facing technical log.)*
