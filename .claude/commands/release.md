Create a git tag and GitHub release for the current state of main.

## Steps

1. Run `git tag --sort=-v:refname | head -5` to show the last tags and determine the current version.

2. Ask the user: "Current version is X.Y.Z — release type? (major / minor / patch)"
   Wait for the user's answer before continuing.

3. Compute the next version from the answer:
   - major: increment X, reset Y and Z to 0
   - minor: increment Y, reset Z to 0
   - patch: increment Z only
   If no existing tag is found, start at v0.1.0 for minor, v0.0.1 for patch, v1.0.0 for major.

4. Ask the user: "Release notes for vX.Y.Z? (press Enter to auto-generate from git log)"
   - If the user provides text, use it as-is.
   - If empty, run `git log $(git describe --tags --abbrev=0)..HEAD --oneline` to list commits since last tag and use them as bullet points in the release notes.

5. Show a summary and ask for confirmation:
   ```
   Tag:     vX.Y.Z
   Notes:   <first line of notes>...
   Confirm? (y/n)
   ```
   Stop if the user answers anything other than y.

6. Build the firmware:
   ```bash
   pio run -e t-beam-supreme
   ```
   If the build fails, stop and show the errors. Do not create the release.

7. Run in order:
   ```bash
   git tag vX.Y.Z
   git push origin vX.Y.Z
   gh release create vX.Y.Z --title "vX.Y.Z" --notes "<release notes>" \
     .pio/build/t-beam-supreme/firmware.bin#firmware-vX.Y.Z.bin \
     .pio/build/t-beam-supreme/firmware.elf#firmware-vX.Y.Z.elf
   ```

8. Print the GitHub release URL returned by `gh release create`.

## Rules
- Tag format: vMAJOR.MINOR.PATCH (e.g. v1.2.3)
- Never force-push or overwrite an existing tag
- If `gh` is not authenticated, stop and tell the user to run `gh auth login`
- No emoji in tag or release title
