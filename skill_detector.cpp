#include "skill_detector.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;
namespace Endfield {

SkillDetector::SkillDetector(const Config& cfg) : cfg_(cfg) {}

void SkillDetector::setEnergyBar(const NormalizedRect& rect) {
    energyBarRect_ = rect;
}

void SkillDetector::addSkillIcon(const SkillIconRegion& icon) {
    icons_.push_back(icon);
}

// ----------------------------------------------------------
// 能量格数检测
//   思路：在能量条 ROI 里做 HSV 阈值提取黄色，
//         统计黄色区域的"连续段数"即为格数
// ----------------------------------------------------------
int SkillDetector::measureEnergySegments(const cv::Mat& frame) const {
    // 三个格子的中心点（归一化坐标）
    float centerX[3] = { 0.442f, 0.500f, 0.557f };
    float centerY    = 0.917f;

    int count = 0;
    for (int i = 0; i < 3; ++i) {
        int px = static_cast<int>(centerX[i] * frameW_);
        int py = static_cast<int>(centerY   * frameH_);

        if (px < 0 || px >= frame.cols || py < 0 || py >= frame.rows) continue;

        cv::Vec3b bgr = frame.at<cv::Vec3b>(py, px);
        cv::Mat pixel(1, 1, CV_8UC3, bgr);
        cv::Mat hsv;
        cv::cvtColor(pixel, hsv, cv::COLOR_BGR2HSV);
        cv::Vec3b h = hsv.at<cv::Vec3b>(0, 0);

      
        // 黄色判断
        if (h[0] >= 15 && h[0] <= 45 && h[1] > 150 && h[2] > 150)
            ++count;
    }
    return count;
}
// ----------------------------------------------------------
// 图标亮度检测
//   取图标区域 HSV 的 V 通道均值作为亮度
// ----------------------------------------------------------
void SkillDetector::measureIconGlows(const cv::Mat& frame,
                                     float outGlow[4]) const {
    for (int i = 0; i < 4; ++i) outGlow[i] = 0.f;

    for (const auto& icon : icons_) {
        int idx = icon.index - 1; // 转为 0-based
        if (idx < 0 || idx >= 4) continue;

        cv::Rect roi = icon.rect.toPixel(frameW_, frameH_);
        roi &= cv::Rect(0, 0, frame.cols, frame.rows);
        if (roi.area() <= 0) continue;

        cv::Mat region = frame(roi);
        cv::Mat hsv;
        cv::cvtColor(region, hsv, cv::COLOR_BGR2HSV);

        std::vector<cv::Mat> ch;
        cv::split(hsv, ch);
        cv::Scalar mean = cv::mean(ch[2]); // V 通道
        outGlow[idx] = static_cast<float>(mean[0]) / 255.f;
    }
}

// ----------------------------------------------------------
// 找最亮图标
//   条件：最亮值 > glowThreshold 且 与次亮的差 > glowDiffMin
// ----------------------------------------------------------
int SkillDetector::findBrightestIcon(const float glow[4]) const {
    return -1;
    /*
    int   maxIdx  = 0;
    float maxVal  = glow[0];
    float secVal  = -1.f;

    for (int i = 1; i < 4; ++i) {
        if (glow[i] > maxVal) {
            secVal = maxVal;
            maxVal = glow[i];
            maxIdx = i;
        } else if (glow[i] > secVal) {
            secVal = glow[i];
        }
    }

    if (maxVal < cfg_.glowThreshold)         return -1;
    if (maxVal - secVal < cfg_.glowDiffMin)  return -1;
    return maxIdx;
    */
}

// ----------------------------------------------------------
// 处理单帧
// ----------------------------------------------------------
// ----------------------------------------------------------
// ----------------------------------------------------------
bool SkillDetector::processFrame(const cv::Mat& frame, int frameIdx) {
    if (frame.empty()) return false;

    frameW_ = frame.cols;
    frameH_ = frame.rows;
    if (frameIdx < 0) frameIdx = currentFrame_;
    ++currentFrame_;

    // 冷却倒计时
    if (cooldownLeft_ > 0) { 
        --cooldownLeft_; 
        // 冷却期间仍然要更新亮度历史
        float glow[4];
        measureIconGlows(frame, glow);
        for (int i = 0; i < 4; ++i) {
            glowHistory_[i].push_back(glow[i]);
            if ((int)glowHistory_[i].size() > 12) glowHistory_[i].pop_front();
        }
        return false; 
    }

    int energy = measureEnergySegments(frame);
    
    // 每帧都记录亮度
    float glow[4];
    measureIconGlows(frame, glow);
    for (int i = 0; i < 4; ++i) {
        glowHistory_[i].push_back(glow[i]);
        if ((int)glowHistory_[i].size() > 8) glowHistory_[i].pop_front();
    }
    
    // 填充历史
    energyHistory_.push_back(energy);
    if ((int)energyHistory_.size() > cfg_.historyFrames)
        energyHistory_.pop_front();

    if ((int)energyHistory_.size() < 2) return false;

    // 取历史中的最大值作为"之前的格数"（过滤闪烁造成的误读）
    int prevEnergy = *std::max_element(energyHistory_.begin(), energyHistory_.end() - 1);
    int currEnergy = energyHistory_.back();

    // 判断能量格减少
    if (prevEnergy <= currEnergy || prevEnergy == 0) return false;

    // 当前读数必须稳定（连续2帧一致），排除闪烁瞬间
    if ((int)energyHistory_.size() >= 2) {
        int prev1 = energyHistory_[energyHistory_.size() - 2];
        if (prev1 != currEnergy) return false;
    }

    // 过滤非战斗状态的缓慢减少：
    // 要求前3帧能量都稳定在同一格数，突然才减少
    if ((int)energyHistory_.size() >= 3) {
        int olderEnergy = energyHistory_[energyHistory_.size() - 3];
        if (olderEnergy != prevEnergy) return false;
    }

    // 能量格减少了 → 找最近几帧中变化最突出的图标
    int brightIdx = -1;
    
    if ((int)glowHistory_[0].size() >= 6) {
        // 记录每个图标在所有帧偏移中的最大变化（绝对值）
        float maxAbsChange[4] = {0};
        int bestFrameOffset[4] = {0};
        
        // 检查最近3帧的帧间变化
        for (int frameOffset = 10; frameOffset >= 1; --frameOffset) {
            for (int i = 0; i < 4; ++i) {
                int size = glowHistory_[i].size();
                float prev = glowHistory_[i][size - frameOffset - 1];
                float curr = glowHistory_[i][size - frameOffset];
                float diff = curr - prev;
                
                // 记录每个图标的最大绝对值变化
                if (std::abs(diff) > maxAbsChange[i]) {
                    maxAbsChange[i] = std::abs(diff);
                    bestFrameOffset[i] = frameOffset;
                }
            }
        }
        
        // 找所有图标中变化最大的
        float globalMaxChange = 0;
        int globalMaxIcon = -1;
        
        for (int i = 0; i < 4; ++i) {
            if (maxAbsChange[i] > globalMaxChange) {
                globalMaxChange = maxAbsChange[i];
                globalMaxIcon = i;
            }
        }
        
        // 找第二大的变化（不同图标）
        float secMaxChange = 0;
        for (int i = 0; i < 4; ++i) {
            if (i != globalMaxIcon && maxAbsChange[i] > secMaxChange) {
                secMaxChange = maxAbsChange[i];
            }
        }
        
          // 判断条件：全局最大变化足够大，且明显高于其他图标
        if (globalMaxIcon >= 0 && 
            globalMaxChange > cfg_.glowChangeThreshold && 
            (globalMaxChange - secMaxChange) > cfg_.glowChangeMin) {
            brightIdx = globalMaxIcon;
        }
    }

    SkillEvent evt;
    evt.skillIndex   = brightIdx + 1;
    evt.frameIndex   = frameIdx;
    evt.timestamp    = frameIdx / cfg_.fps;
    evt.energyBefore = prevEnergy;
    evt.energyAfter  = currEnergy;
    for (int i = 0; i < 4; ++i) evt.iconGlow[i] = glow[i];

    events_.push_back(evt);
    fireCallbacks(evt);
    cooldownLeft_ = cfg_.cooldownFrames;

    std::cout << std::fixed << std::setprecision(2)
              << evt.timestamp << "s 战技" << evt.skillIndex << "\n";

    if (cfg_.saveSnapshots) saveSnapshot(frame, evt);
    return true;
}

// 批量处理视频
// ----------------------------------------------------------
void SkillDetector::processVideo(const std::string& videoPath) {
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        std::cerr << "[错误] 无法打开视频: " << videoPath << "\n";
        return;
    }

    double vidFps = cap.get(cv::CAP_PROP_FPS);
    if (vidFps > 0) cfg_.fps = vidFps;
    int total = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

    std::cout << "[信息] " << videoPath
              << "  帧率=" << cfg_.fps
              << "  总帧=" << total << "\n";

    cv::Mat frame;
    int idx = 0;
    while (cap.read(frame)) {
        processFrame(frame, idx++);

    }

    std::cout << "[完成] 共检测到 " << events_.size() << " 次战技释放\n";
}

// ----------------------------------------------------------
// 导出 CSV
// ----------------------------------------------------------
void SkillDetector::exportCSV(const std::string& path) const {
    std::ofstream f(path);
    f << "skillIndex,frameIndex,timestamp(s),energyBefore,energyAfter,"
         "glow1,glow2,glow3,glow4\n";
    for (const auto& e : events_) {
        f << e.skillIndex << ","
          << e.frameIndex << ","
          << std::fixed << std::setprecision(3) << e.timestamp << ","
          << e.energyBefore << ","
          << e.energyAfter  << ","
          << e.iconGlow[0]  << ","
          << e.iconGlow[1]  << ","
          << e.iconGlow[2]  << ","
          << e.iconGlow[3]  << "\n";
    }
    std::cout << "[输出] " << path << "\n";
}

// ----------------------------------------------------------
// 调试绘制
// ----------------------------------------------------------
cv::Mat SkillDetector::drawDebug(const cv::Mat& frame) const {
    cv::Mat out = frame.clone();

    // 绘制能量条
    cv::Rect er = energyBarRect_.toPixel(out.cols, out.rows);
    cv::rectangle(out, er, {0, 220, 255}, 2);
    cv::putText(out, "Energy", er.tl() + cv::Point(2, -4),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, {0, 220, 255}, 1);

    // 绘制图标区域
    for (const auto& icon : icons_) {
        cv::Rect r = icon.rect.toPixel(out.cols, out.rows);
        cv::rectangle(out, r, {0, 255, 100}, 2);
        cv::putText(out, "Skill" + std::to_string(icon.index),
                    r.tl() + cv::Point(2, -4),
                    cv::FONT_HERSHEY_SIMPLEX, 0.45, {0, 255, 100}, 1);
    }
    return out;
}

void SkillDetector::fireCallbacks(const SkillEvent& evt) {
    for (auto& cb : callbacks_) cb(evt);
}

void SkillDetector::saveSnapshot(const cv::Mat& frame,
                                 const SkillEvent& evt) const {
    fs::create_directories(cfg_.snapshotDir);
    std::ostringstream ss;
    ss << cfg_.snapshotDir << "skill" << evt.skillIndex
       << "_f" << evt.frameIndex << ".jpg";
    cv::imwrite(ss.str(), frame);
}

} // namespace Endfield
