<!-- changelog-baseline: harbour-core@71c0363c1f (feature/compiler-ast-dump) -->
<!-- Delta pointer. Everything after this commit is NOT yet described here.
     To catch up:  git log 30352b5d56..HEAD   (see § Maintaining this file). -->

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

## 2026-08-08 — the dump carries the module's identity, so tools stop parsing binaries

A refactoring tool has one closing duty: *prove* the edit changed nothing it did
not mean to change. The facts for that proof — which symbols a module has, and
whether each function's compiled code is still the same — lived only inside the
`.hrb` file, so any tool wanting them had to ship its own reader of that binary
format. A private reader of someone else's format drifts silently; the dump's
schema refuses loudly when it moves. So the dump now states those facts itself:

```jsonc
"symbols": [   // the symbol table, in table order
  { "name": "MAIN",   "scope": 517,  "link": "func" },
  { "name": "OUTSTD", "scope": 8192, "link": "extern" } ],
"functions": [
  { "name": "MAIN", "pcodeSize": 34,
    "pcodeHash": "641ef8115be05f41",       // the compiled bytes, exactly
    "pcodeNormHash": "7d10bed9943a2871",   // the same bytes, immune to
    ... } ]                                // symbol renumbering
```

`scope` is the compiler's full 16-bit scope — richer than the `.hrb` itself,
whose format truncates the upper half. Equal `pcodeHash` means the function's
compiled code is byte-for-byte the same.

The second hash answers a subtler question. Compiled code refers to variables,
fields and functions by their *position* in the module's symbol table — so
adding one symbol renumbers the table, and the raw bytes of functions nobody
touched change with it, while their behaviour does not. `pcodeNormHash` is
computed with each of those position references replaced by the symbol's name:
it stays identical across renumbering and moves on any real change. Measured
both ways: a symbol inserted mid-table leaves untouched functions with the raw
hash different and the normalized hash identical; a one-line edit changes both.

As with everything on this branch: without `-x`, none of this runs, and the
compiled program is byte-identical to stock Harbour's.

A tool that keeps a dump around has one question before trusting it again: *do the
sources still match this?* Until now the only way to answer was the file timestamp,
and the timestamp lies in two ordinary situations — an edit made within the same
second as the compile is invisible (incremental builds compare with about one second
of resolution), and a dump you deleted leaves no trace at all, because what the build
watches is the `.c`.

So the dump now records its own provenance: every file the compiler read to produce it
— the `.prg` and its includes, transitive ones included — each with its size and a
content hash, plus the `-D` defines in force.

```jsonc
"provenance": {
  "sum": "fnv1a64",
  "files": [ { "path": "m.prg", "size": 115, "sum": "45a9ac2775bbd192" },
             { "path": "a.ch",  "size":  56, "sum": "abd3ede22738a271" } ],
  "defines": [ { "name": "LIGA_C" } ] }
```

The list is the compiler's own — the same one `-gd` reports — so a header pulled in by
another header is in it, and one skipped by a false `#ifdef` is not.

And you do not have to compare it yourself. Two new switches answer about files and
compile nothing:

```
$ harbour --ast-fresh work/*.ast.json      # silence: every dump still matches
$ echo $?
0
$ harbour --ast-fresh work/*.ast.json      # after an edit whose mtime was restored
work/m1.ast.json	m1.prg: changed
$ echo $?
1
```

Only stale dumps are printed, one per line as `<dump><TAB><why>`; silence means all of
them still hold, and the exit carries the yes/no for callers that want only that. There
is also `--filesum <file>…`, which prints `<sum> <size> <path>` for files you want to
check without having a dump.

Measured: 207 files, 2.1 MB, verified in 4 ms. On the same state, an incremental build
recompiles nothing — which is precisely the case this exists for.

Two notes for whoever reads the diff. The hash is a local FNV-1a 64 rather than
`hb_md5file()`: md5 keeps its computation and its `HB_FUNC` wrapper in one object, so
linking it would pull `hb_parc`, `hb_retclen` and the rest of the runtime into a
compiler that today links none of it. And include tracking, which used to be collected
only for `-gd`, is now also collected for `-x` — without that the dump shipped an empty
provenance, which is worse than none, because it looks like an answer.

## 2026-08-07 — `-x`: a token says which directive wrote it

A name a rule writes plainly in its result - the `nAcc` of `#xcommand CMD_SOMA <v> =>
nAcc += <v>` - reaches the compiler from another file, and a column in another file is
not a column in this one. So every use that name produced came out with a line and
nothing else: a tool could tell you the statement, never the word.

Measured on `tbrowse.prg` in this tree, that is **40.3% of all sites**. In Harbour it is
not a corner case; it is how real code is written.

Such a token now carries `app`, the index of the rule application that produced it, and
so does the recorded site. The place a reader wants is the **application** - the
`CMD_SOMA` the programmer typed - which `ppApplications` already published with line,
column and length, and which is what an editor would have to open to change anything.

The index is a fact carried from the expansion, not "the application on the same line":
two directives on one line would make that a guess.

**Unchanged when the switch is off.** The stamp is guarded by the same position tracking
`-x` turns on. Compiling this tree with both compilers, no new switches: 889/889 `.hrb`
byte-identical, 0 divergent.

## 2026-08-06 — `-x`: a recorded use now carries the token it was written as

Every variable use, function call and message send the dump records already told you which
line it was on. It could not tell you **which word**, and on a line where the same name
appears more than once that is the whole question:

```harbour
nTotal := 0 + Eval( {| x | nTotal += x }, 1 ) + nTotal
```

Three words, three columns. A consumer had a line and four records, and no way to pair
them up. Pairing them **by counting** — the second record must be the second word — looks
reasonable and is wrong: records come out in the order the compiler walks the expression,
words come in the order you typed them, and for an assignment the target is walked last.

Now each of the three site channels carries the position of the token the parser actually
built that node from, and it is carried, not reconstructed: the scanner stamps every
symbol it hands over with the index of the token that starts it, the parser keeps the
stamps on its location stack in step with the semantic values, and the rule action reads
it back. Nothing counts and nothing searches the token stream for a matching name.

Two consequences worth naming:

- **a statement continued with `;`** reports the line the name is written on. The line
  the compiler was standing on when it recorded — the last physical line of the statement
  — is still there and still means the same thing; the written line appears beside it
  only when the two differ, so its mere presence tells a consumer "this is not where the
  other field says".
- **an optimisation no longer erases a word you wrote.** `var := var + expr` is rewritten
  to `var += expr`, and the node for the middle `var` is released before code generation
  ever sees it. The pcode is right; the record of your source was missing an occurrence.
  It is recorded now, before the rewrite discards it.

**What stays absent, deliberately.** A name written in an `.ch` — anything a
`#command`/`#xcommand` produced — has no column, because a column in another file is not
a column in this one. Measured on `tbrowse.prg` in this tree, that is 40% of all sites:
in Harbour it is not a corner case, it is how real code is written. Absent is the honest
answer; the next step is to publish which directive application a site came from, so a
tool can point at the line you actually wrote.

## 2026-07-27 — `-x`: the regions conditional compilation skipped

A branch the preprocessor skips used to leave no trace anywhere — the `.ppo` shows blank
lines, the `.ppt` says nothing, the dump never mentioned it. The preprocessor does read
those lines and then frees them without recording anything.

That silence has a cost for any tool acting on the dump: it renames what compiled,
verifies what compiled, and reports success — while the *other* configuration is left
calling a name that no longer exists. The verification was never wrong; it had no way to
state the scope it covered.

The dump now records what had already been computed: the region (file, line range, and
the name the innermost `#if[n]def` tested) and the identifiers inside it. **Report only**
— that text never became a symbol, so a word spelled like one is not known to *be* one.
Proving otherwise would mean compiling the other branch, which is a different program.

## 2026-07-22 — `-x`: three facts the dump used to throw away

Almost everything the preprocessor synthesizes already records where it came from — a
marker's content cloned into a result, two words pasted together, a marker turned into a
string. Three things did not, and each of them is something a tool must be able to *report*
about your code without ever editing it.

**A value built out of its own position.** `__LINE__` and `__FILE__` are the preprocessor's
two dynamic defines: what they expand to is not text sitting somewhere, it is a value the
preprocessor invents at expansion time out of the line it is on and the file it is reading.
That literal used to reach the compiler looking exactly like a number you typed. A tool
walking your program saw `247` with no way to know `__LINE__` had produced it — the only
tie back to the origin was to match things up **by line number**, which is precisely the
thing that moves the moment anything edits the file.

Now the value keeps a link back to the rule that expanded it, and that link does not depend
on the line. A tool that shifts your lines — extracting a function, inlining a variable —
can see that a value here follows the position, and tell you. It must not freeze the old
number: after the edit the *new* number is the correct one. What was missing was the ability
to say so.

**Which axis that value follows.** `__LINE__` follows the line; `__FILE__` follows the file.
They are otherwise the same kind of value and arrive the same way, so a tool that only knew
"this is position-built" would have to warn about both when editing lines — and the warning
about `__FILE__` would be wrong. The dump now says which axis each one reads.

**A string that re-expands at run time.** `"modelo &cLayout do relatorio"` is not just text:
at run time Harbour expands `&cLayout` to the value of the memvar it names. A tool renaming
that memvar changes what the string does. Such a string now carries the list of memvar names
it will expand, so a tool matches the name it is renaming against that **list** — rather than
going hunting for it inside the text of the string, which is guessing dressed up as
searching.

The honest limit, said where it bites: that list is the names **as written**, extracted with
the compiler's own rule for finding them. The compiler then checks each name's scope, and
only a memvar or an undeclared name really becomes a run-time expansion — under `-kd` a
declared local is taken apart at compile time instead, so there the list can name one string
too many. In a default build there is no divergence, since `&<local>` is a compile error to
begin with. And the failure mode is **one warning too many, never an edit**: nothing in this
channel authorizes touching a string. Under `-kM` the field is absent, exactly as the
compiler's own decision is.

None of this changes the code your program compiles to: it is bookkeeping done only while
the dump is being written. Without `-x`, nothing at all is different.

## 2026-07-13 — `-x`: a `TEXT … ENDTEXT` line is data, and now it says *where it came from*

Inside a `TEXT … ENDTEXT` block your source stops being code. Every raw line leaves the
preprocessor as a **string**, exactly as you typed it, spaces and all:

```harbour
LOCAL cSaldo := "1.234,00"

TEXT
Relatorio mensal
cSaldo apurado no periodo
ENDTEXT
```

becomes, for the compiler:

```harbour
QOut( "   Relatorio mensal" )
QOut( "   cSaldo apurado no periodo" )
```

That second line is the interesting one. The word `cSaldo` in it is **not your variable** —
it is text that merely *looks* like it. And that is exactly how it should be treated: no
tool has any business editing it, because nothing can prove what it means.

The hard part is not deciding to leave it alone. It is **being able to tell**. Those strings
used to reach the dump with **no position at all** — no line, no column, no origin — even
though the preprocessor had just read them from a concrete line of your file. A tool could
not line them up against your source, and could not tell this fabricated line from a string
you actually wrote that happens to read the same.

That difference decides whether a tool should speak. A string **you wrote** whose contents
equal a name can be a call by name — `&()`, `__mvGet` — so a tool may reasonably point at it
and let you judge. A stream-block line has no such mechanism: it is printed data, and a word
in it matching one of your variables is a **coincidence**, not an occurrence. There is
nothing to report, and reporting it would be noise.

Now the line carries its source position like any other token, and it is **sealed as
stream-produced**. So a tool stays silent about it because it was *told* the string is data —
not because it guessed from the shape of the token, and not by matching names inside your
text. Either way it is never edited.

What does *not* change: the code your program compiles to. This is position bookkeeping,
recorded only while the dump is being produced. Without `-x` nothing at all is different.

The line-by-line form (the Cl*pper `TEXT … ENDTEXT`) is exact — every line reports its own
line. The forms that glue the whole block into a single string report the closing line, since
that is what the string honestly is.

## 2026-07-13 — `hbmk2 --hbproject`: ask what a project is made of, and get an answer

Write a tool that has to work on a Harbour project — an IDE, a linter, a refactorer, a
build dashboard — and the first thing you need is the most basic thing there is: **which
files does this project actually consist of?** Until now there was no way to *ask*. You
could only **watch hbmk2 build** and scrape the compiler command line out of `-traceonly`.

That line was never meant to answer this question, and it doesn't:

- it lists the sources that need **(re)compiling** — not the sources of the target. Turn on
  incremental mode with an up-to-date target and it is **not printed at all**;
- so the tool has to force a full rebuild just to find out what the project contains;
- and the line is written for a human to read, so you get to re-implement quoting rules to
  take it apart again.

Now you ask:

```
$ hbmk2 myapp.hbp --hbproject
{"targetname":"myapp.hbp","targettype":"hbexe",
 "sources":["src/main.prg","src/util.prg"],
 "incpaths":["/opt/harbour/include","./include"],
 "prgflags":["-n2","-w3","-es2","-i/opt/harbour/include","-i./include"]}
```

One JSON block, everything already **resolved** — your `.hbp`, `.hbc` and `.hbm` files, the
`-i` paths, the `${macros}`, the `{filters}`: hbmk2 has expanded all of it, exactly as it
would for a real build. `sources` carries the `.hbx` inputs alongside the `.prg` ones, the
same way the compiler receives them. `prgflags` is the **complete** set of options the
Harbour compiler gets for that target — so a tool can compile a module the way hbmk2 would,
not an approximation of it.

For a project with several targets (a container, or `-target=`), add `=nested` and you get
one block per target, each on its own line:

```
$ hbmk2 all.hbp --hbproject=nested
```

**It does not build anything.** The question is answered and hbmk2 exits — which also means
it answers for a project that does not currently compile, and that the answer never depends
on whatever happens to be sitting in your work directory.

**`--hbinfo` is untouched.** It describes the *build* (platform, compiler, target type); this
describes what the target is *made of*. They are separate options with separate output, so
nothing that reads `--hbinfo` today notices this at all.

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

### 2026-07-13 — the dump now tells you the truth about its own version

The `"schema"` field is the one thing this file tells you to check before you consume
a dump — and it was **lying**. The rule-lifetime channel (the `"un"` directives,
`undoes`, `removed`, and the family prefix on `kind`) shipped while the dump still
declared the *previous* version. A tool that trusted the field would have concluded
those fields were not there.

The dump now declares the version it actually emits. **If you gate on `"schema"`, gate
on a minimum, never on a list of known versions** — the schema is additive (each one
is the previous plus a channel), and a list silently rejects every dump we ship after
you wrote it. We learned this the hard way: fixing the number above made the tool that
consumes this branch reject every module it was handed, because it carried exactly
such a list.

### 2026-07-13 — `-x` stops getting quadratic on a big module

Dumping a module used to cost **more than proportionally** to its size: what drove the
cost was the number of preprocessor expansions in it, and each one was re-answering a
question the compiler had already answered. Double the expansions, quadruple the time.
It is **linear** now — the compiler asks the question once per module instead of once
per word.

**What that is worth, honestly.** On real Harbour code the gain is noticeable but not
dramatic — dumping a whole project the way a tool consuming `-x` does, roughly a
**third faster**. The dramatic case is pathological: a module dense in expansions,
where the old cost curve turned a fraction of a second into more than a minute.
Whether your code has such a module is something only your code can say — time
`harbour -x<dir>/` on your biggest one and see. That number is worth more than ours:
it is about your code, on your machine.

Nothing you read changed: the dump a given source produces is **byte for byte the one
the previous build produced**, verified across the whole corpus of the consumer that
uses this branch.

What is left is **linear and dominated by writing the dump out** — a large module
writes a large file. `hbmk2 -inc` re-dumps only the modules you touched.

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
