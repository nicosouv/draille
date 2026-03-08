Scaffold a new display screen end-to-end.

Arguments: the screen name and what data it should display (e.g. "HeartRate screen showing BPM and zone")

Steps:
1. Use the ui-designer agent to design the layout, data struct, and drawXxx() implementation
2. Add the new AppScreen enum value to `src/ui/UIManager.h`
3. Insert the screen into NAV_ORDER in `src/ui/UIManager.cpp` (or leave it as long-press only — specify)
4. Add the data struct to `src/display/DisplayManager.h`
5. Add drawXxx() declaration to DisplayManager.h and implementation to DisplayManager.cpp
6. Add the case to `_render()` in UIManager.cpp and a `_buildXxxData()` method
7. Update `_periodicRender()` if the screen needs animation

Keep each change minimal and consistent with existing screens.
