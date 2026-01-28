// End-to-end demo: desktop capture -> HSV mask -> contour detections -> overlay -> cursor assist PID.
//
// Hotkeys:
//   Q   : enable assist (track + optional dwell click)
//   E   : disable assist (stop tracking, clear target, unlock)
//   ESC : quit
//
// Locking strategy:
// - Once a target is selected ("locked"), we try to keep tracking the *same* target by
//   matching detections whose center stays within LOCK_MATCH_RADIUS of the previous locked center.
// - We do NOT switch targets immediately if the locked target is missing for a frame.
//   Instead we count consecutive missing frames; only after LOST_FRAMES_TO_UNLOCK frames
//   do we drop the lock and choose a new target.
//
// Coordinate notes:
// - If you capture a ROI instead of full screen, you must provide CAPTURE_OFFSET_X/Y which
//   maps image coordinates -> screen coordinates (screen = image + offset).
#include "aimbot/gui/ImGuiApp.h"

int main() {
    return aimbot::gui::ImGuiApp{}.run();
}
