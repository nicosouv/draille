Review the current git diff (staged + unstaged) for embedded firmware quality.

1. Run `git diff HEAD` to get all changes since last commit.
2. For each changed file in `src/`, apply the firmware-review agent checklist:
   - Memory: stack allocations, heap fragmentation, buffer sizes
   - Timing: blocking calls in loop(), millis() rollover
   - SPIFFS: unclosed file handles, buffer sizes (ISO8601 needs char[21])
   - TFT: FS.h include order, direct TFT calls bypassing sprite
   - GPS: const on getters (must NOT be const)
3. Also check for style regressions vs existing code (naming, structure).

List issues as: `[CATEGORY] file.cpp:line — description — suggested fix`
If no issues found, say "LGTM".
