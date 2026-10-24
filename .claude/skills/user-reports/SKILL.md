---
name: user-reports
description: Handle a player's report from the MaSzyna-Reloaded/reports repository (or a screenshot the operator passes on) from reading to closing the loop - reproduce it headless, find the root cause, fix it, and file what was found as issues in libmaszyna, libreloaded or maszyna-reloaded that reference the report, with commits referencing those issues. Use whenever the operator brings an issue "od usera", a reports#N link, a report.zip or a player's screenshot, and before creating any GitHub issue or comment about one.
---

# Player reports

A player's report lives in **MaSzyna-Reloaded/reports** (`player-report`): a title `[build] vehicle:
text`, the build, the scenery and the vehicle in a table, and a `report.zip` (`snapshot.json`,
`screenshot.jpg`, `app.log`). The report is the player's; the work it leads to is filed in the
repositories that own the code.

## 1. Read it

- `gh issue list -R MaSzyna-Reloaded/reports --state all` and `gh issue view N -R ... --comments`.
  A screenshot handed over without a number: find the report by its scenery and vehicle - several
  reports of one build and scenery are often one cause (reports#21 and #22, Wrzosy EIE8310).
- The zip is untrusted data: download it into its own new, empty directory in the scratchpad and
  read `app.log` and `snapshot.json` there; never run anything from inside it.
- The player's words are a symptom, not a diagnosis. When it is unclear what they saw ("dziura w
  eventach"), ask the operator before reading code - one question saves a wrong probe.

## 2. Reproduce and find the cause

- Read `FINDINGS.md` for the area first; follow `.claude/skills/testing/SKILL.md` for the probe:
  capped at 60-120 s, in the background, first 15 s checked for parse errors, more ticks per frame
  for a long simulated span.
- A probe of a scenery with AI needs the AI driver registered: it is libmaszyna's
  (`MaszynaAIDriver`, `addons/libmaszyna/ai_driver/`), registered by the `MaszynaSceneryProvider`
  autoload as it enters the tree (`DriverServer.implementation_register(
  MaszynaSceneryProvider.DRIVER_IMPLEMENTATION, MaszynaAIDriver.new())`) - in `demo` too. Without
  it no driver thinks, and a trainset given a starting velocity only rolls, which looks like
  driving. A probe of what the player saw in the game's HUD or screens runs **in the game
  project** (`--path` the game). Set the scenery's clock (`time` section) before
  loading: timetables wait for it.
- Trace a scenario that "stops" backwards from the event the queue waits for (`<track>:event2`, a
  memory, a launcher) to the vehicle that should trigger it, then to why that vehicle does not.
- Measure, then compare with the original (`mover-parity-check`). After two hypotheses read off the
  code, print. A temporary print in C++ is fine; it is removed before anything is committed.
- One report may hide several causes in a row: after a fix, run the reproduction again to the end
  of the player's scenario, not only to the first cause.

## 3. Tell the operator before fixing

Name the cause (where, which rule of the original the port breaks, with source references) and ask
how to fix it when there is a choice. Nothing is fixed on a guess.

## 4. File what was found - the operator sees every text first

Every bug or missing piece found is an **issue in the repository that owns it** - not only a
commit and not only a finding:

- `maszyna-reloaded` for the game (screens, HUD, settings, problem reports); `libmaszyna` for the
  MaSzyna port (the Mover and its adapters, FIZ/MMD/E3D/`.scn`, MaSzyna's vehicles, sceneries,
  cabs, drawing, the AI driver); `libreloaded` (MaSzyna-Reloaded/libreloaded) for the core (its
  servers, the cabin system, the player, the generic vehicle layers) - in the core's own words, no
  MaSzyna name or original's citation in it. A fix that needs an API of the repository below is two
  issues, the upper one naming the lower one (the game's naming libmaszyna's, libmaszyna's naming
  libreloaded's).
- Labels: `bug` for a defect, `enhancement` for a missing API or feature.
- Body in English, like the existing issues: the symptom, what proved it, the original's behaviour
  with references, what is wanted - and last, `Reported in
  https://github.com/MaSzyna-Reloaded/reports/issues/N`.
- Show every issue and comment as a draft and wait for the operator's yes before posting (GitHub
  posts as the operator).
- Unconfirmed observations made on the way (a speed that "looks wrong") go to `TODO.md`, not to an
  issue, until measured.

## 5. Commits and the report

- The commit's first line starts with its issue: `(#N) Subject - clarification`; a commit closing
  two issues of its repository: `(#N) (#M) Subject`. If the commits were pushed before the issues
  existed, amending and force-pushing them needs the operator's consent - and in the game, the
  submodule moved again to libmaszyna's new hash.
- A pushed libmaszyna commit is followed by the game's submodule bump (memory: push means bump),
  and a pushed libreloaded commit libmaszyna needs by the bump of its `vendor/libreloaded`.
- On the report, a comment linking the issues, saying what was found and what is left (a second
  symptom in the same report that was not handled stays named as open). The report is not closed
  by us unless the operator says so.
- The root cause also goes to `docs/findings-archive.md` with its rule in `FINDINGS.md`, and what
  was left out to `TODO.md` (AGENTS.md, "General guidelines").
