# 明日方舟终末地 · 技能释放检测器

## 依赖

- **OpenCV 4.x**（需要 core / imgproc / videoio / highgui）
- C++17 编译器（MSVC 2019+、GCC 9+、Clang 10+）
- CMake 3.16+

```bash
# Ubuntu
sudo apt install libopencv-dev cmake

# macOS (Homebrew)
brew install opencv cmake

# Windows: 从 opencv.org 下载预编译包，设置 OpenCV_DIR 环境变量
```

## 编译

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4
```

## 使用流程

### Step 1：截图校准 UI 坐标

在游戏中截一张 1080p 战斗截图，运行校准工具：

```bash
./skill_detector calibrate screenshot.png
```

- 鼠标点击能量条的**左上角**和**右下角**，记录输出的归一化坐标 (x, y)
- 用同样方式记录**技能图标**区域和**特效区域**
- 填入 `main.cpp` 的 `buildCharacterUIs()` 函数中

### Step 2：调试预览

```bash
./skill_detector debug battle.mp4
```

画面会显示三个检测框（黄=能量条，绿=技能图标，红=特效区域），
确认框位置正确后进行参数调整。

### Step 3：批量处理

```bash
./skill_detector batch battle.mp4
```

结果输出到 `skill_events.csv`，截图保存在 `./snapshots/`。

---

## 核心参数调整指南

| 参数 | 说明 | 调大 | 调小 |
|------|------|------|------|
| `energyDropThresh` | 能量下降幅度阈值（0~1） | 减少误报 | 减少漏报 |
| `energyFullThresh` | "能量满"判定阈值 | 更严格 | 更宽松 |
| `effectThresh` | 特效像素占比阈值 | 减少误报 | 减少漏报 |
| `pixelChangeTh` | 帧差灵敏度（0~255） | 只检测剧烈变化 | 连轻微特效也检测 |
| `historyFrames` | 能量历史窗口帧数 | 更平滑但有延迟 | 更灵敏 |

---

## 检测逻辑说明

```
每帧处理：
  ┌─────────────────────────────────────────┐
  │  measureEnergyBar()                     │
  │    HSV 阈值 → 统计黄色列数 → 得到比例   │
  ├─────────────────────────────────────────┤
  │  measureIconGlow()                      │
  │    V 通道均值 → 发光强度                 │
  ├─────────────────────────────────────────┤
  │  measureEffectScore()                   │
  │    帧差 → 超阈值像素比例               │
  └──────────────┬──────────────────────────┘
                 │
         checkRelease()
                 │
     能量大幅下降 && (之前接近满 || 有特效)
                 │
           触发 SkillEvent
```

---

## 进阶优化方向

1. **模板匹配**：存储技能"充能中"和"释放瞬间"的图标模板，用 `cv::matchTemplate` 做二次确认，提升精度
2. **光流法**：用 `cv::calcOpticalFlowFarneback` 检测特效区域的运动向量，比帧差更鲁棒
3. **多帧去抖**：检测到释放后冷却 N 帧，避免同一次释放被多次触发
4. **颜色自适应**：首次运行时自动采样能量条颜色，减少手动校准工作量
5. **GPU 加速**：将 HSV 转换和阈值操作移到 `cv::cuda` 命名空间，处理 4K60fps 视频

---

## HSV 颜色校准（能量条）

如果黄色能量条识别不准，用以下代码打印你截图中能量条区域的 HSV 均值：

```cpp
cv::Mat roi = img(cv::Rect(x, y, w, h));
cv::Mat hsv;
cv::cvtColor(roi, hsv, cv::COLOR_BGR2HSV);
cv::Scalar mean = cv::mean(hsv);
// 输出 H S V 均值，据此调整 ENERGY_BAR_COLOR() 的范围
std::cout << mean << "\n";
```
