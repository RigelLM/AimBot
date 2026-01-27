#include <chrono>
#include <iostream>

#include <opencv2/opencv.hpp>

#include "aimbot/capture/DxgiDesktopDuplicationSource.h"
#include "aimbot/vision/HsvConfig.h"
#include "aimbot/vision/HsvMasker.h"
#include "aimbot/vision/ContourDetector.h"
#include "aimbot/viz/OverlayRenderer.h"

int main() {
    // Step1: Frame source (DXGI Desktop Duplication)
    DxgiDesktopDuplicationSource source;

    // Step2: HSV masker config (use your old thresholds)
    HsvConfig cfg;
    cfg.lower = cv::Scalar(82, 199, 118);
    cfg.upper = cv::Scalar(97, 255, 255);
    cfg.minArea = 1600;     // Step3: same as your old (r.width*r.height < 1600) filter
    cfg.morphOpen = 0;      // keep 0 for equivalent behavior
    cfg.morphClose = 0;

    HsvMasker masker(cfg);

    // Step3: contour detector
    ContourDetector detector(cfg);

    // Step4: overlay renderer
    OverlayStyle style;
    style.showConfidence = true;
    style.showArea = false;
    style.showIndex = false;
    OverlayRenderer overlay(style);

    cv::namedWindow("Screen Capture", cv::WINDOW_KEEPRATIO);
    cv::namedWindow("Mask", cv::WINDOW_KEEPRATIO);

    cv::Mat screen;
    cv::Mat mask;

    while (true) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // Grab a new frame (may return false if no new frame / timeout)
        if (!source.grab(screen) || screen.empty()) {
            int key = cv::waitKey(1);
            if (key == 27) break; // ESC
            continue;
        }

        // Step2: BGR -> HSV -> inRange => mask
        masker.run(screen, mask);

        // Step3: detect from mask
        auto dets = detector.detect(mask);

        // latency
        auto frame_end = std::chrono::high_resolution_clock::now();
        double latency_ms =
            std::chrono::duration<double, std::milli>(frame_end - frame_start).count();

        // Step4: draw overlay on the original screen frame
        overlay.drawDetections(screen, dets);
        overlay.drawLatency(screen, latency_ms);

        // Show windows
        cv::imshow("Mask", mask);
        cv::imshow("Screen Capture", screen);

        int key = cv::waitKey(1);
        if (key == 27) break; // ESC
    }

    cv::destroyAllWindows();
    return 0;
}
