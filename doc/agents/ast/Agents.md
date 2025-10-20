# Harbour Refactoring Agents

## Mission

Develop **compiler‑backed refactoring** for **Harbour** (`.prg`, `.ch`) via a **VS Code--compatible LSP**.
Use the **Harbour compiler** as the single source of truth (parsing, semantics, AST).

### Design Principles
-   **Build on, not over.** Extend existing logic; modify/replace only for measurable gains (maintainability, modularity, testability). No duplication.
-   **Preserve upstream parity.** Continuously rebase onto Harbour main.
-   **Refactor responsibly.** Change internals only if it improves maintainability, testing/reproducibility, or tooling decoupling **without** breaking correctness.
-   **Advance tests/docs** every milestone.
-   The **Overseer** may pause/pivot any track that risks upstream parity.

## Oversight Model

### Overseer Session (Codex Supervision)

- Owns **roadmap alignment**, **repository hygiene**, and **risk management**.
- Coordinates all subordinate implementation sessions.
- Maintains the master record in `hb_refactoring_tools_overseer.md` and formally approves each completed milestone before allowing further development.
- The user serves as the communication relay between the Overseer and all implementation sessions, transmitting instructions and reports in both directions.

### Coordinated Sessions (Implementation Agents)

- Operate under focused briefs, executing **scoped development tasks**.
- Run **cmocka** test suites and submit **test evidence with every diff review** before session closure.
- Any `.prg` or `.ch` file must pass strict validation via `hbmk2 -w3` **before commit**.
- Log deltas/blockers in `doc/agents/ast/progress.md`.

### Feedback Loop
Maintain a **continuous, auditable exchange** between implementation sessions and the Overseer, ensuring compiler parity, progress transparency, and safe iteration across commits.

* Every delegation logs **plan deltas**, **blockers**, and **follow-ups** in:

  * `doc/agents/ast/progress.md`
  * `doc/agents/ast/draft.md`
* The **Overseer session** will **pause or pivot** any track that threatens **compiler parity** with upstream Harbour.

## Agent Roles & Responsibilities
- **Overseer**
  - Clarify priorities per phase, ensure work preserves compiler behavior and advances refactoring goals.
  - Approve instrumentation points in `complex.c`, `harbour.y`, `hbmain.c`, `hbcomp.c`, (and related).
  - Provide suggested commit messages and leave actual commits to the user; never invoke `git commit` during Overseer sessions.
- **Compiler Instrumentation Agent**
  - Embed token/AST event emitters inside the existing compiler (token hooks in `complex.c`, parser actions in `harbour.y`).
  - Document schemas for downstream consumers.
- **AST Tooling Agent**
  - Steward the compiler-trace consumer surface: keep `README-AST.md`, `doc/agents/ast/*`, and `tests/ast/` fixtures aligned with the compiler-backed flow.
  - Guide out-of-tree adapters that parse the JSON dump; no in-tree standalone tooling ships after the retirement of `src/ast/` and `utils/hbrename`.
- **Testing & Verification Agent**
  - Expand cmocka coverage, manage fixtures, and ensure every `.prg` test compiles via `hbmk2 -w3`.
  - Track macro trace expectations and `.ppo` artefacts when they strengthen assertions.
- **LSP & Refactoring Agent**
  - Build CLI/API bridges, scaffold the LSP server, and prototype editor workflows (rename, extract).
  - Coordinate with AST tooling to guarantee refactorings run off compiler-derived truth.

## Current Focus
- **Overseer** – keep documentation in `doc/agents/ast/*` aligned with the compiler-trace-only architecture, log session outcomes in `progress.md`, and sequence the backlog for inline/init/exit instrumentation plus the `fSingleModule` audit.
- **Compiler Instrumentation Agent** – extend node coverage for `INLINE`/`INIT`/`EXIT`, tag macro-generated statements, document the single-module review, and update the instrumentation plan + compile-buffer notes when hooks evolve.
- **AST Tooling Agent** – maintain fixtures/serialization docs in sync with `hb_compAstTraceDumpJson()`, draft a lightweight consumer example (CLI or adapter), and coordinate schema changes with downstream LSP work.
- **Testing & Verification Agent** – run the cmocka suite (`tests/ast/*.c`), `hbmk2 -w3` sweeps, and `scripts/test-ast.sh`; add coverage when new node kinds land and preserve regen instructions for PP trace fixtures.
- **LSP & Refactoring Agent** – prototype a CLI that consumes `hb_compAstTraceDumpJson()` for rename/extract experiments, capture VS Code data-contract expectations, and align with tooling agent on schema stability.

## Execution Workflow
- **Checkpoint planning**: Overseer reviews outstanding work, updates roadmap entries, and confirms baseline status (`git status`, diff against `cfb7bdc2`).
- **Delegation packets**: For each substantive task, create a session brief detailing scope, files, tests, and success criteria.
- **Implementation**: Agents modify code within guarded flags or scratch branches when experimenting; permanent changes align with the agreed plan.
- **Validation**: Run the `tests/ast` cmocka harness, `scripts/test-ast.sh`, and any new suites. Failures block progress until resolved.
- **Review & Commit**: Overseer inspects diffs, records outcomes in `doc/agents/ast/progress.md`, and shares suggested commit messages; the user applies commits.

## Documentation & Logging
- `draft.md`: living scratchpad for session snapshots, decisions, and next actions.
- `doc/agents/ast/progress.md`: authoritative log of instrumentation status, AST/tooling gaps, and roadmap adjustments.
- `doc/agents/ast/instrumentation-plan.md`: evolving design for compiler event capture.
- Additional specs (e.g., LSP, refactoring backlog) live under `doc/agents/`.

## Roadmap Alignment
- **Phase 1 – Baseline alignment**: refresh roles (this document), audit repository state, and map compiler vs tooling AST flows.
- **Phase 2 – Instrumentation design**: document hook points and prove non-disruptive event capture.
- **Phase 3 – Token event stream**: ship compiler-backed token APIs with regression tests.
- **Phase 4 – AST projection**: traverse `PHB_EXPR` trees into tooling structures, validating against legacy expectations.
- **Phase 5 – Tooling interface & CLI**: surface snapshot requests through stable APIs and updated docs.
- **Phase 6 – Refactoring features**: iterate on rename/extract capabilities backed by rigorous tests and documentation.
- **Phase 8 – VSCode/LSP scaffolding**: prototype LSP server pinned to compiler-derived data.

## Quality Gates
- No code lands without passing cmocka suites relevant to the touched modules.
- Macro-heavy fixtures must document `.ppo` artefacts when helpful.
- Commits describe scope and link to roadmap checkpoints.
