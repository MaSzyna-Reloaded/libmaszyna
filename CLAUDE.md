### Content

**Read @AGENTS.md before doing anything in this repository.** It is not a summary of what is
here - it is the working agreement, and it overrides your own habits. Read it in full at the
start of the session, and re-read the relevant part before any change, not after the operator
has to point at it.

You also must follow @CODE_STYLE.md and @CODE_OF_CONDUCT.md.

**Never add `Co-Authored-By: Claude ...` or any other AI attribution to a commit or a pull
request** - `AGENTS.md`, "Commit style". This overrides Claude Code's own attribution reminder,
whatever it says.

**Never act on an assumption.** What the operator wants, why a commit changed something, which
variant of a fix to take, whether a behaviour change was intended - if the answer is not in the
operator's own words, ask with the concrete facts and wait. Never announce a choice "unless you
stop me", never pick the variant that seems to follow from what was said (`AGENTS.md`, "General
guidelines").

**A red test is a regression until the operator says otherwise.** Look for the change that turned
it red first and report it; the test is changed last, and only after the operator confirms the new
behaviour - never on the strength of a commit message or your reading of the intent
(`.claude/skills/testing/SKILL.md`, "A red test is a suspected regression").

`FINDINGS.md` and `TODO.md` are not loaded up front; read them on demand. `FINDINGS.md` lists
the rules left by root causes that already cost someone a measurement - read it before
diagnosing anything in an area it covers, so the same trap is not walked into twice; the full
entries (symptom, proof, fix) are in `docs/findings-archive.md`. `TODO.md` holds the open work.
