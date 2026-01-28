#pragma once

#include <cmath>

// Single-axis PID controller with:
// - derivative low-pass filtering (dAlpha)
// - output saturation (outMin/outMax)
// - integral clamping (iMin/iMax)
// - simple anti-windup: do not integrate if saturated and error pushes further into saturation
struct PIDAxis {
    // PID gains
    double kp{ 0 }, ki{ 0 }, kd{ 0 };
    // Internal state
    double integral{ 0 }, prevError{ 0 }, prevDeriv{ 0 };

    // Output clamp (limits the command magnitude)
    double outMin{ -40 }, outMax{ 40 };
    // Integral clamp (prevents integral windup)
    double iMin{ -2000 }, iMax{ 2000 };
    // Derivative filter coefficient in [0,1):
    //   derivFiltered = dAlpha * prevDeriv + (1 - dAlpha) * derivRaw
    double dAlpha{ 0.85 };

    // Advance one PID step given current error e and time step dt (seconds).
    // Returns the saturated control output u.
    double step(double e, double dt) {
        if (dt <= 0) return 0.0;

        // Raw derivative of error
        double deriv = (e - prevError) / dt;
        // Low-pass filter derivative to reduce noise/jitter
        deriv = dAlpha * prevDeriv + (1.0 - dAlpha) * deriv;

        // Candidate integral update
        double integralCandidate = integral + e * dt;

        // Unsaturated control output
        double uUnsat = kp * e + ki * integralCandidate + kd * deriv;
        // Saturate output
        double u = (std::max)(outMin, (std::min)(uUnsat, outMax));

        // Anti-windup:
        // If saturated and the error would push further into saturation, skip integrating.
        bool saturated = (u != uUnsat);
        bool pushingFurther =
            (u == outMax && e > 0) ||
            (u == outMin && e < 0);

        if (!(saturated && pushingFurther)) {
            // Commit integral update with clamping
            integral = (std::max)(iMin, (std::min)(integralCandidate, iMax));
        }

        // Update state
        prevError = e;
        prevDeriv = deriv;
        return u;
    }

    // Reset controller memory/state.
    void reset() { integral = 0; prevError = 0; prevDeriv = 0; }
};

// Two-axis PID (X and Y share the same structure but independent state).
struct PID2D {
    PIDAxis x;
    PIDAxis y;
    void reset() { x.reset(); y.reset(); }
};

// Simple integer point in screen pixel coordinates.
struct CursorPoint {
    int x{ 0 };
    int y{ 0 };
};