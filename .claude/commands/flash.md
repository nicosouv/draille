Build and flash the firmware to the connected T-Beam Supreme.

Steps:
1. Run `make ports` to check which serial port is detected
2. Run `make flash` (uses auto-detected port)

If the device is not found, remind the user to connect the board and check USB.
Show only errors from the build/flash output — suppress verbose progress lines.
