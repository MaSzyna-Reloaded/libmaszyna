---
name: issues-and-commits
description: Group what was found by its root cause and file it as one GitHub issue and one commit per cause, with every issue, comment and edit shown to the operator as a draft before it is posted. Use before creating, editing or closing any GitHub issue, pull request or comment, before committing fixes of an audit or a search, and whenever several findings, places or files are about to be filed or fixed.
---

# Issues and commits

GitHub issues are public: people read them, and every one opened, edited or closed notifies the
watchers. An issue number is never reused - an issue opened by mistake stays as a gap in the
numbering even after it is closed or deleted. Nothing posted can be taken back.

## 1. Group by root cause first

Before a single issue or commit, list the findings and group them by **what causes them**, not by
where they show:

- One cause showing in many places - many files, sections, keys, importers - is **one** issue. Its
  body lists the places; the commit fixing them is **one** commit.
- An issue that already exists for the cause is **extended** (its title and body edited) - never a
  new issue beside it.
- A separate issue and a separate commit only for a **different cause**.

The test: would one sentence of the original's behaviour explain all of them? Then they are one.

Example (2026-11-07): the original's parser lowers every token (`cParser::getToken(ToLower =
true)`), the port's keeps the case. `LoadAccepted`, a track's type, a dynamic's driver and load
type, the scenery's keywords, a model's light names and the `.mat` keys all broke on capitals -
**one** cause. Opening #370-#375 and six commits beside #366 was wrong; they were closed as
duplicates and squashed into #366's commit, after the watchers had been notified of each.

## 2. Show the operator everything before it is posted

GitHub posts as the operator. Before `gh issue create`, `gh issue edit`, `gh issue close`,
`gh issue comment`, `gh pr create` or `gh pr edit`:

- show the grouping (which findings are one cause), and for each issue its title and body, as a
  draft;
- wait for the operator's yes. No issue is opened "to record it" on the way.

## 3. Commits and the pull request

- One cause, one commit: `(#N) Subject` - every place it fixes in that one commit
  (AGENTS.md, "Commit style").
- A different fix in the same pull request is its own commit with its own issue - created only
  after step 2.
- The pull request's body closes every issue it fixes, across repositories in full
  (`Closes MaSzyna-Reloaded/reports#N`); check `gh pr view --json closingIssuesReferences`.
