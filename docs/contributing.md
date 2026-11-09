# Contribution and writing guidance

Apply this guidance to new or edited work. Use clear, familiar language,
concrete examples, and the Oxford comma in prose lists. Explain why a change
matters and what users or callers can observe. Keep private data and reference
project identities out of tracked files and public discussion.

## Issues and stories

Write readable titles and complete bodies from creation. Start with the problem
and the desired outcome; use Investigate for research, Add/Implement for
behavior, Validate/Verify for acceptance, and Decide for a choice supported by
existing evidence. Investigations should identify questions, expected evidence,
relevant version/revision differences, and remaining uncertainties.

Put detailed issue-specific contracts, limits, failure cases, and acceptance
requirements in a closed `<details>` section with a descriptive `<summary>` and
blank lines around its Markdown content. Keep repository-wide rules in the
guides rather than repeating them in each issue. Separate agent guidance is
needed only for instructions unique to that issue.

Use GitHub Relationships for parents and genuine blockers. Do not repeat parent
links, sequence metadata, or transitive dependencies in child bodies. A story
organizes logical, reviewable children; keep the overall sequence in the parent
and leave independent work independent. Each implementation step owns the tests
for the behavior it adds, including relevant failure cases. Avoid a final
generic "add tests" task carrying coverage that earlier implementation already
needed.

Preserve issue relationships, state, labels, and accurate content when
rewriting, and verify saved changes. Show complete draft titles/bodies or
clearly label excerpts. Existing authorization to create or update an issue does
not require an additional approval step.

## Pull requests

Use concise, action-oriented, human-readable titles naming the concrete change
and scope. PR titles do not need Conventional Commits prefixes or the commit
subject's character limit.

Write for a reviewer who has not seen the conversation. Lead with the problem
and resulting behavior; include the constraints, tradeoffs, relevant validation,
and remaining limits. Scale detail to complexity. Review the title/body against
the final diff, including any scope changes. Do not describe abandoned work
unless it explains an important tradeoff.

## Suggested commit messages

Suggest a message in a fenced code block after repository changes, unless asked
otherwise. Suggestions never authorize staging or committing. Preserve the Git
index, and require a separate explicit request for each action.

Use `feat|fix|chore|docs|refactor|test|build|ci(<scope>): <summary>`. Keep the
imperative, specific subject at most 50 characters. Separate subject and body
with a blank line, and wrap body lines at 72 characters. Explain the backstory
and why before what; mention user-visible changes, significant defaults, and
compatibility risks where relevant. Use `BREAKING CHANGE:` when applicable.

Report routine successful validation in the response. Mention validation in the
message only when critical validation failed, was skipped, or was impossible.
Use `Fixes #123` for bugs and `Closes #123` for completed behavior/tasks. If
using `Refs #123`, explain outside the message why the issue remains open and
what work remains, addressing every referenced issue.

For explicitly authorized commits, preserve actual PowerShell newlines and
verify the stored message with `git log -1 --format=%B`. Do not amend without
specific authorization. See [AGENTS.md](../AGENTS.md) for repository rules.
