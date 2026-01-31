// Attempt to passthrough mouse events to a layer under us.
// This 'almost' works.
#include "wPassthrough.h"

EM_JS(void, setup_mouse_event_forwarding, (), {
  const canvas = document.getElementById('canvas');
  Module.isImGuiCapturingMouse = false;
  Module.isForwardingEvents    = false;
  Module.mouseEventsQueue      = [];
  Module.lastTarget = null; // New variable to track the last target

  ['mousedown', 'mouseup', 'click', 'dblclick', 'contextmenu', 'wheel', 'mouseleave',
  'mousemove'].forEach(eventType = > {
    canvas.addEventListener(eventType, function(e){
      if (!Module.isImGuiCapturingMouse && !Module.isForwardingEvents) {
        // Temporarily disable pointer-events to get the element underneath
        const originalPointerEvents = canvas.style.pointerEvents;
        canvas.style.pointerEvents  = 'none';
        const actualTarget         = document.elementFromPoint(e.clientX, e.clientY);
        canvas.style.pointerEvents = originalPointerEvents;

        // --- Logic to handle mouseenter and mouseleave events ---
        // FIXME: Isn't working properly
        if (e.type == = 'mousemove') {
          // Check if the target has changed
          if (actualTarget != = Module.lastTarget) {
            // Dispatch a mouseleave event to the previous target
            if (Module.lastTarget) {
              const leaveEvent = new MouseEvent('mouseleave', { bubbles: true });
              // console.log("----mouse leave last target");
              Module.lastTarget.dispatchEvent(leaveEvent);
            }

            // Dispatch a mouseenter event to the new target
            if (actualTarget) {
              const enterEvent = new MouseEvent('mouseenter', { bubbles: true });
              // console.log("----mouse enter last target");
              actualTarget.dispatchEvent(enterEvent);
            }

            // Update the last target
            Module.lastTarget = actualTarget;
          }
        }

        // --- Your existing queueing and cursor logic ---
        const targetCursorStyle = actualTarget ? window.getComputedStyle(actualTarget).cursor : 'default';
        canvas.style.cursor     = targetCursorStyle;

        if (e.type == = 'mouseleave') {
          // Special handling for the canvas-level mouseleave
          const leaveEvent = new MouseEvent('mouseleave', { bubbles: true });
          document.getElementById('map').dispatchEvent(leaveEvent);
          Module.lastTarget = null; // Reset the last target
        } else {
          // Store the original event and the found target
          Module.mouseEventsQueue.push({ event: e, target: actualTarget });
        }
      } else {
        // When ImGui is capturing, reset the cursor and last target
        canvas.style.cursor = 'default';
        if (Module.lastTarget) {
          const leaveEvent = new MouseEvent('mouseleave', { bubbles: true });
          Module.lastTarget.dispatchEvent(leaveEvent);
          Module.lastTarget = null;
        }
      }
    }, { capture: true, passive: false });
  });
});

// A new function to update the flag and perform cleanup from C++
EM_JS(void, set_imgui_capture_flag, (int capturing), {
  // We use 1 for true, 0 for false as that's how EM_JS handles booleans
  const isCapturing = !!capturing;

  if (isCapturing && !Module.isImGuiCapturingMouse) {
    // This is the critical part: If we're transitioning from
    // "not capturing" to "capturing", we must clear the queue.
    Module.mouseEventsQueue = [];
  }

  Module.isImGuiCapturingMouse = isCapturing;
});

// The forwarding function that empties the queue.
EM_JS(void, forward_all_stored_mouse_events_to_leaflet, (), {
  const map = document.getElementById('map');

  //  Module.isForwardingEvents = true;

  // Loop through and dispatch every event in the queue.
  while (Module.mouseEventsQueue.length > 0) {
    const eventInfo = Module.mouseEventsQueue.shift(); // Get the oldest event
    const e         = eventInfo.event;
    const target    = eventInfo.target;
    if (e && target) {
      let clonedEvent;
      // Create a new event object to avoid side effects
      if (e.type == = 'wheel') {
        clonedEvent = new WheelEvent(e.type, {
          bubbles: true, cancelable: true, composed: e.composed, detail: e.detail, view: e.view,
          screenX: e.screenX, screenY: e.screenY, clientX: e.clientX, clientY: e.clientY,
          ctrlKey: e.ctrlKey, shiftKey: e.shiftKey, altKey: e.altKey, metaKey: e.metaKey,
          button: e.button, buttons: e.buttons, relatedTarget: e.relatedTarget,
          // These are crucial for WheelEvent:
          deltaX: e.deltaX, deltaY: e.deltaY, deltaZ: e.deltaZ, deltaMode: e.deltaMode
        });
      } else if (e instanceof PointerEvent) { // Check for PointerEvent next
        clonedEvent = new PointerEvent(e.type, {
          bubbles: e.bubbles, cancelable: e.cancelable, composed: e.composed, detail: e.detail, view: e.view,
          screenX: e.screenX, screenY: e.screenY, clientX: e.clientX, clientY: e.clientY,
          ctrlKey: e.ctrlKey, shiftKey: e.shiftKey, altKey: e.altKey, metaKey: e.metaKey,
          button: e.button, buttons: e.buttons, relatedTarget: e.relatedTarget,
          movementX: e.movementX, movementY: e.movementY,
          // PointerEvent specific properties:
          pointerId: e.pointerId, width: e.width, height: e.height, pressure: e.pressure,
          tangentialPressure: e.tangentialPressure, tiltX: e.tiltX, tiltY: e.tiltY,
          twist: e.twist, pointerType: e.pointerType, isPrimary: e.isPrimary
        });
      } else {
        clonedEvent = new MouseEvent(e.type, e);
      }

      if (clonedEvent) {
        // Dispatch the cloned event to the correct, original target.
        target.dispatchEvent(clonedEvent);
      }
    }
  }
  //  Module.isForwardingEvents = false;
});
