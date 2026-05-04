#include "skill_detector.h"
#include <opencv2/highgui.hpp>
#include <iostream>

// ============================================================
//  坐标配置（基于 2560x1600 截图量出，归一化）
//
//  如需重新校准：
//    运行 calibrate 模式，鼠标点击各区域角点，
//    记录打印出的归一化坐标填入下方
// ============================================================

// 能量条区域（黄色横条）
static Endfield::NormalizedRect ENERGY_BAR = {
    0.415407f,  // x
    0.912442f,  // y
    0.168466f,  // w（0.583873 - 0.415407）
    0.012673f   // h（0.925115 - 0.912442）
};

// 4个战技图标区域（右下角大圆圈）
// 注意：这里的坐标需要根据实际截图校准
static Endfield::SkillIconRegion SKILL_ICONS[4] = {
    { 1, { 0.802736f, 0.874424f, 0.038157f, 0.059908f } },
    { 2, { 0.853852f, 0.875576f, 0.035277f, 0.057604f } },
    { 3, { 0.902088f, 0.874424f, 0.036717f, 0.058756f } },
    { 4, { 0.951764f, 0.873272f, 0.037437f, 0.059908f } },
};

// ============================================================
//  校准工具：鼠标点击截图，打印归一化坐标
// ============================================================
static cv::Size g_imgSize;
static void onMouse(int event, int x, int y, int, void*) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        float nx = static_cast<float>(x) / g_imgSize.width;
        float ny = static_cast<float>(y) / g_imgSize.height;
        std::cout << "像素(" << x << ", " << y << ")  "
                  << "归一化(" << nx << ", " << ny << ")\n";
    }
}

void runCalibrate(const std::string& imgPath) {
    cv::Mat img = cv::imread(imgPath);
    if (img.empty()) {
        std::cerr << "[错误] 无法读取: " << imgPath << "\n";
        return;
    }
    g_imgSize = { img.cols, img.rows };
    std::cout << "=== 校准模式 ===  分辨率: "
              << img.cols << "x" << img.rows << "\n"
              << "左键点击各区域角点，按 ESC 退出\n";

    cv::namedWindow("calibrate", cv::WINDOW_NORMAL);
    cv::resizeWindow("calibrate", 1280, 800);
    cv::setMouseCallback("calibrate", onMouse, nullptr);
    cv::imshow("calibrate", img);
    while (cv::waitKey(0) != 27) {}
    cv::destroyAllWindows();
}

// ============================================================
//  调试预览：实时显示检测区域和结果
// ============================================================
void runDebug(const std::string& videoPath, Endfield::SkillDetector& det) {
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) { std::cerr << "[错误] 无法打开视频\n"; return; }

    cv::namedWindow("debug", cv::WINDOW_NORMAL);
    cv::resizeWindow("debug", 1280, 800);

    cv::Mat frame;
    int idx = 0;
    // 记录最近一次事件用于屏幕提示
    int  lastEventFrame = -999;
    int  lastSkillIdx   = 0;

    while (cap.read(frame)) {
        det.processFrame(frame, idx);

        const auto& evts = det.events();
        if (!evts.empty() && evts.back().frameIndex == idx) {
            lastEventFrame = idx;
            lastSkillIdx   = evts.back().skillIndex;
        }

        cv::Mat vis = det.drawDebug(frame);

        // 显示触发提示（持续30帧）
        if (idx - lastEventFrame < 30) {
            std::string label = ">>> 战技 " + std::to_string(lastSkillIdx) + " 释放！";
            cv::putText(vis, label, cv::Point(50, 80),
                        cv::FONT_HERSHEY_DUPLEX, 1.5,
                        cv::Scalar(0, 60, 255), 3);
        }

        cv::imshow("debug", vis);
        if (cv::waitKey(1) == 27) break;
        ++idx;
    }
    cv::destroyAllWindows();
}

// ============================================================
//  main
// ============================================================
int main(int argc, char* argv[]) {
    // 用法：
    //   skill_detector calibrate  screenshot.png   （校准模式）
    //   skill_detector debug      battle.mp4        （调试预览）
    //   skill_detector batch      battle.mp4        （批量处理）

    std::string mode   = argc > 1 ? argv[1] : "batch";
    std::string inFile = argc > 2 ? argv[2] : "battle.mp4";

    if (mode == "calibrate") {
        runCalibrate(inFile);
        return 0;
    }

    // 配置检测器
    Endfield::Config cfg;
    cfg.fps            = 60.0;
    cfg.glowThreshold  = 0.72f;  // 图标亮度阈值，可调
    cfg.glowDiffMin    = 0.08f;  // 最亮与次亮的最小差，可调
    cfg.cooldownFrames = 15;     // 
    cfg.saveSnapshots  = true;
    cfg.debugDraw      = true;
    cfg.historyFrames = 4;
    
    Endfield::SkillDetector detector(cfg);
    detector.setEnergyBar(ENERGY_BAR);
    for (auto& icon : SKILL_ICONS)
        detector.addSkillIcon(icon);

    // 注册回调
    detector.onSkillRelease([](const Endfield::SkillEvent& e) {
        std::cout << ">>> 战技" << e.skillIndex
                  << " @" << std::fixed << std::setprecision(2)
                  << e.timestamp << "s  "
                  << "能量格 " << e.energyBefore << "->" << e.energyAfter
                  << "\n";
    });

    if (mode == "debug") {
        runDebug(inFile, detector);
    } else {
        detector.processVideo(inFile);
        detector.exportCSV("skill_events.csv");
    }

    return 0;
}
