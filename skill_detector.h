#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <functional>
#include <deque>

// ============================================================
//  《明日方舟终末地》战技释放检测器
//  逻辑：
//    1. 共用黄色能量条格数减少 → 判断"有战技释放"
//    2. 4个战技图标亮度对比   → 判断"释放了哪个战技"
// ============================================================

namespace Endfield {

// ----------------------------------------------------------
// 归一化坐标（自适应任意分辨率）
// ----------------------------------------------------------
struct NormalizedRect {
    float x, y, w, h;
    cv::Rect toPixel(int W, int H) const {
        return cv::Rect(
            static_cast<int>(x * W),
            static_cast<int>(y * H),
            static_cast<int>(w * W),
            static_cast<int>(h * H)
        );
    }
};

// ----------------------------------------------------------
// 战技图标槽
// ----------------------------------------------------------
struct SkillIconRegion {
    int            index; // 1~4
    NormalizedRect rect;  // 图标外接矩形区域
};

// ----------------------------------------------------------
// 战技释放事件
// ----------------------------------------------------------
struct SkillEvent {
    int    skillIndex;    // 哪个战技触发（1~4）
    int    frameIndex;
    double timestamp;     // 秒
    int    energyBefore;  // 释放前能量格数
    int    energyAfter;   // 释放后能量格数
    float  iconGlow[4];   // 4个图标亮度（调试用）
};

using SkillCallback = std::function<void(const SkillEvent&)>;

// ----------------------------------------------------------
// 配置
// ----------------------------------------------------------
struct Config {
    double fps            = 60.0;
    int    historyFrames  = 4;     // 能量格历史窗口
<<<<<<< HEAD
    float  glowChangeThreshold = 0.10f; // 亮度变化阈值（前后帧差值）
    float  glowChangeMin       = 0.05f; // 变化量与次大变化量的最小差
=======
    float  glowThreshold  = 0.72f; // 图标"亮"的亮度阈值
    float  glowDiffMin    = 0.08f; // 最亮与次亮的最小差，防误判
>>>>>>> origin/main
    int    cooldownFrames = 20;    // 释放后冷却帧数（防重复触发）
    bool   debugDraw      = false;
    bool   saveSnapshots  = false;
    std::string snapshotDir = "./snapshots/";
};

// 能量条黄色 HSV 范围
inline cv::Scalar ENERGY_HSV_LOW()  { return cv::Scalar(18, 150, 180); }
inline cv::Scalar ENERGY_HSV_HIGH() { return cv::Scalar(40, 255, 255); }

// ----------------------------------------------------------
// 检测器
// ----------------------------------------------------------
class SkillDetector {
public:
    explicit SkillDetector(const Config& cfg = {});

    void setEnergyBar(const NormalizedRect& rect);
    void addSkillIcon(const SkillIconRegion& icon);
    void onSkillRelease(SkillCallback cb) { callbacks_.push_back(cb); }

    // 处理单帧（BGR 格式）
    bool processFrame(const cv::Mat& frame, int frameIdx = -1);

    // 批量处理视频
    void processVideo(const std::string& videoPath);

    const std::vector<SkillEvent>& events() const { return events_; }
    void exportCSV(const std::string& path) const;
    cv::Mat drawDebug(const cv::Mat& frame) const;

private:
    Config cfg_;
    int frameW_ = 2560, frameH_ = 1600;
    int currentFrame_ = 0;
    int cooldownLeft_ = 0;

    NormalizedRect              energyBarRect_;
    std::vector<SkillIconRegion> icons_;
    std::vector<SkillCallback>  callbacks_;
    std::vector<SkillEvent>     events_;
    std::deque<int>             energyHistory_;

    // 测量能量条当前格数（通过统计黄色像素列的间隔）
    int  measureEnergySegments(const cv::Mat& frame) const;

    // 测量4个图标的亮度
    void measureIconGlows(const cv::Mat& frame, float outGlow[4]) const;

    // 找到最亮的图标索引（0~3），若无明显最亮返回 -1
    int  findBrightestIcon(const float glow[4]) const;

    void fireCallbacks(const SkillEvent& evt);
    void saveSnapshot(const cv::Mat& frame, const SkillEvent& evt) const;
<<<<<<< HEAD
    
    std::deque<float> glowHistory_[4];  // 每个图标的亮度历史
=======
>>>>>>> origin/main
};

} // namespace Endfield
