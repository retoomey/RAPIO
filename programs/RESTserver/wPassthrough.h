#pragma once

#include <emscripten.h>

/** Routines for passthrough of events to a layer under us.
 * This is so we can overlay the IMGUI over the leaflet layer
 * It 'mostly' works.  Hoping to improve upon it
 * @author Robert Toomey
 */
extern "C" {
/** Called  to create a mouse event queue */
extern void
setup_mouse_event_forwarding();

/** On imgui capture, clear the mouse queue */
extern void
set_imgui_capture_flag(int capturing);

/** Forward any mouse events imgui didn't want/handle */
extern void
forward_all_stored_mouse_events_to_leaflet();
}
