Build the firmware and show only errors and warnings.

Run: `make build 2>&1 | grep -E "error:|warning:|undefined" | head -40`

If the build succeeds with no errors, say "Build OK" and nothing else.
If there are errors, list them concisely and suggest the most likely fix for each.
