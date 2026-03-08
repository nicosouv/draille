Generate a git commit message for the current staged changes.

Run `git diff --staged` to see what's staged. If nothing is staged, run `git diff HEAD` instead.

Then produce a single commit message line following these rules:
- Format: `<type>: <short description>` (e.g. `feat: add GPS spinner animation`)
- Types: feat, fix, refactor, test, docs, chore, style
- Max 72 characters total
- English only
- No emoji
- No period at the end
- No "by Claude" or similar attribution
- Focus on *what changed and why*, not implementation details

Print only the commit message — nothing else, no explanation, no surrounding text.
