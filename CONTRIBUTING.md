# Contributing

## Guidelines

Every change should be reviewed and tested, either manually or automatically.
For this reason, contributions should be topic-focused and as small as possible.
They should be readable to reviewers, and the intent of the change should be
clear from the code, commit history, and pull request description.

Contributors should follow these rules:

* Keep patches as small as possible.
* Focus on a single topic per pull request.
* Do not change unrelated implementation details "by the way".
* Avoid unnecessary code reformatting.
* Review your own changes before requesting review.
* Use Draft Pull Requests for work in progress.
* Format the code according to the project style before requesting review
  (`make style-fix STYLE_FILE=<file>`, then `make style-check STYLE_FILE=<file>`).
* Explain non-trivial design or ownership decisions in the pull request
  description.

### AI Guidelines

AI coding agents can be useful tools, but they can also generate bloated,
incorrect, or incomprehensible code. They may also produce implementations
that do not fit the project architecture.

The general rule is: you are always responsible for the code you ship.

For this reason, contributors should follow these rules:

* AI is a helper, not a substitute for understanding.
* Never trust AI output blindly. Review and test the code.
* You must be able to explain the code you submit.
* If AI generates something unclear, rewrite it, simplify it, or refactor it manually.
* Do not use AI to make architecture decisions.
* Give AI agents the smallest and most precise tasks possible.
* Prefer planning, analysis, and review prompts over large open-ended code generation.
* Do not submit AI-generated changes that you do not fully understand.

### C++ / GDExtension Guidelines

This project uses C++ through Godot-CPP and GDExtension. The C++ layer should
follow Godot's object model, memory model, and API conventions as closely as
possible.

The goal is not to write isolated "pure C++" code and then attach it to Godot.
The goal is to extend Godot in a way that remains compatible with Godot's
lifetime rules, reference counting, editor integration, scripting API, resource
system, and rendering/server APIs.

Contributors should follow these rules:

* Prefer Godot types and containers when data crosses the Godot API boundary.
* Use `Ref<T>` for Godot `Resource` objects where ownership is shared or managed
  by Godot.
* Use `Object`, `Resource`, `Node`, and other Godot base classes according to
  their intended lifetime model.
* Avoid storing raw pointers unless the ownership and lifetime are explicit.
* Do not keep raw pointers to Godot objects in long-lived containers without a
  clear unregister or validation mechanism.
* Avoid unnecessary `new`, `delete`, `memnew`, and `memdelete`.
* Do not mix Godot-managed lifetime with manual C++ lifetime unless there is a
  clear reason and the ownership model is documented.
* Avoid building parallel pure C++ object graphs that duplicate Godot's object,
  resource, or scene model.
* Keep pure C++ helper code small, local, and implementation-specific.
* Do not expose pure C++ lifetime assumptions to GDScript or editor-facing APIs.
* Be careful with `RID` ownership and release order.
* Be careful with singleton registration, access, and shutdown order.
* Be careful with signals emitted during creation, reload, teardown, or cache
  invalidation.

If a change affects ownership, caching, reload logic, singleton lifetime,
resource lifetime, or `RID` management, the pull request description should
explain the intended lifetime model.

## Building and testing

libmaszyna is built into the library of libreloaded, the core of Rails Reloaded, which is the
`vendor/libreloaded` submodule. Everything goes through this repository's `Makefile`:

```sh
# libreloaded from the submodule (or a local checkout of it: make libreloaded-local LIBRELOADED_LOCAL=<path>)
make libreloaded-submodule

# the library with libmaszyna in it, into the test project demo/ (compile-release for a release build)
make compile-debug

# import the test project once after a build
make import

# the test scripts you wrote or changed, one by one (res://tests/<name>.gd); without TEST, all of them
make test TEST=<name>

# formatting and clang-tidy of the C++ files you touched
make style-fix STYLE_FILE=<file>
make style-check STYLE_FILE=<file>
```

A change libmaszyna needs in libreloaded is made in libreloaded's own repository, never in
`vendor/libreloaded`; build against your checkout of it with `make libreloaded-local`, and go back
to the submodule with `make libreloaded-submodule` once the change is pushed and the submodule moved
to it. See `AGENTS.md`, "libmaszyna and libreloaded".

## Request for changes/ Pull Requests
You first need to create a fork of this repository to commit your changes to it. Methods to fork a repository can be found in the [GitHub Documentation](https://docs.github.com/en/get-started/quickstart/fork-a-repo).

## Choose a base branch
Before starting development, you need to know which branch to base your modifications/additions on. When in doubt, use `main`.

| Type of change                | Branch prefix | Branches              |
| :------------------           |:---------:| ---------------------:|
| Development work              | `dev`     | `main`                |
| Bug fixes                     | `fix`     | `main`                |
| New features                  | `feature` | `main`                |

```sh
# Switch to the desired branch
git switch main

# Pull down any upstream changes
git pull

# Create a new branch to work on
git switch --create [prefix]/[feature-or-fix-name]
```

Commit your changes, then push the branch to your fork with `git push -u fork` and open a pull request on the base repository.

Your code style must comply with [our Code style](CODE_STYLE.md) and pass CI/CD checks in order for your contribution to be merged
