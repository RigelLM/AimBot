# Cursor Assist (DXGI + OpenCV + PID)

面向**辅助残障用户使用电脑**的实验性项目：通过 **屏幕捕捉 → 颜色/边缘线索提取 → 目标检测 → 目标锁定 → PID 控制鼠标**，实现对屏幕目标的“跟踪/辅助点击”等交互能力。

> 这个 repo 目前实现的是一条实时 CV 控制闭环：  
> **DXGI Desktop Duplication** 捕捉桌面帧 → **HSV 分割** 生成 mask → **轮廓检测** 得到候选目标 → **锁定策略** 维持同一目标 → **PID** 输出鼠标相对位移（可选 dwell click）

---

## 功能特性

- ✅ **Windows 桌面捕捉（DXGI Desktop Duplication / D3D11）**：低延迟获取屏幕帧
- ✅ **HSV 掩膜 + 形态学（可选）**：快速分割目标颜色区域
- ✅ **轮廓检测（findContours）**：输出候选目标 `Detection`（bbox/center/area/confidence）
- ✅ **目标锁定策略（Lock & Track）**  
  - 匹配半径内持续跟踪同一目标  
  - 目标短暂消失不会立刻切换，累计丢失帧数超过阈值才解锁并重新选择
- ✅ **鼠标辅助控制（PID 2D）**：线程循环按 tick 频率输出 `(dx, dy)` 相对移动
- ✅ **可视化 Overlay**：绘制检测框/中心点、延迟等信息
- ✅ **JSON 配置加载**：`config/app.json` 可覆盖默认参数（缺省字段保留默认）

---

## 快速开始

### Demo

程序入口在 `main.cpp`，目前提供端到端 demo：

- `Q`：开启 assist（开始锁定/跟踪 + 可选 dwell click）
- `E`：关闭 assist（停止跟踪、清空目标、重置锁定状态）
- `ESC`：退出

默认读取配置：
- `config/app.json`（读取失败会回退到默认配置并在 stderr 打印原因）

---

## 依赖与环境

### 平台
- **Windows 10/11 (x64)**  
  使用 **DXGI Desktop Duplication + D3D11** 做屏幕捕捉，因此目前仅支持 Windows。

### 工具链
- **CMake >= 3.20**
- **C++17 编译器（MSVC 推荐）**
- Visual Studio（推荐使用 CMake Presets 里的配置）
  - 预设使用 `generator = "Visual Studio 17 2022"`，并指定 `toolset = v142`（即 VS2019 的 MSVC 工具集）
  - 也可手动改为你本机可用的 toolset/VS 版本

### 第三方依赖
- **OpenCV（本repo使用的是官方的windows prebuilt版本）**
  - 工程默认假设 OpenCV 位于：`vendor/opencv/build`
  - 根 CMakeLists 里写死了：
    ```cmake
    set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/vendor/opencv/build")
    find_package(OpenCV REQUIRED CONFIG)
    ```
  - Windows 下会在构建后自动拷贝 OpenCV DLL 到可执行文件目录：
    - 默认拷贝目录：`vendor/opencv/build/x64/vc16/bin`
    - 如果你用的是别的 OpenCV 路径/版本，需要同步修改 `OPENCV_BIN_DIR`

### 额外说明
- Windows 下 `aimbot_core` 会链接：
  - `d3d11`, `dxgi`
- 运行时会自动拷贝 `config/` 到输出目录：`<exe_dir>/config`
---

## 构建

本项目使用 CMake，根工程会构建：
- `aimbot_core`（静态库：capture/vision/viz/cursor/lock 等核心模块）
- `aimbot`（主程序：`src/app/main.cpp`）

同时支持可选构建 `examples/` 目录下的实验性小可执行文件（默认 ON）。

### 方式 A：使用 CMake Presets（推荐）

仓库提供 `CMakePresets.json`，默认配置为：
- configure preset: `vs2026-v142-x64`
  - binaryDir: `build_vs`
  - 配置类型：`Debug;Release;Dist`

**Configure：**
```bash
cmake --preset vs2026-v142-x64
```

**Build：**

```bash
cmake --build --preset build-debug
# 或
cmake --build --preset build-release
# 或
cmake --build --preset build-dist
```

构建产物一般在：

* `build_vs/<Config>/`（Visual Studio 多配置生成器的常见输出结构）

### 方式 B：不使用 Presets（手动）

如果你希望自己指定生成器 / toolset / OpenCV_DIR：

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -T v142 -DOpenCV_DIR="path/to/opencv/build"
cmake --build build --config Debug
```

---

### Examples

根 CMakeLists 提供选项：

* `BUILD_EXAMPLES`（默认 `ON`）

关闭 examples 编译：

```bash
cmake -S . -B build -DBUILD_EXAMPLES=OFF
cmake --build build --config Debug
```

`examples/CMakeLists.txt` 会自动扫描 `examples/*.cpp`：

* 每个 `*.cpp` 都会生成一个同名可执行文件（`NAME_WE`），并链接 `aimbot_core`
* 因此目前两个 demo 的 target/可执行文件名就是：

  * `CaptureDetectionMain`
  * `CursorControllerMain`

运行方式（以 Debug 为例，具体输出路径取决于生成器/IDE）：

* `build_vs/examples/Debug/CaptureDetectionMain.exe`
* `build_vs/examples/Debug/CursorControllerMain.exe`

#### CaptureDetectionMain

功能：DXGI 抓屏 → HSV Mask → Contour 检测 → Overlay 可视化
按键：`ESC` 退出

#### CursorControllerMain

功能：PID 鼠标控制 demo（画圆轨迹的 target）
按键：

* `Q` 开始
* `E` 停止并清空 target
* `ESC` 退出
---

## 配置文件说明（config/app.json）

配置结构由 `AppConfig.h` 定义、`AppConfigJson.h` 负责反序列化。一个示例：

```json
{
  "capture": { "offset_x": 0, "offset_y": 0 },
  "hsv": {
    "lower": [82, 199, 118],
    "upper": [97, 255, 255],
    "minArea": 1600,
    "morphOpen": 0,
    "morphClose": 0
  },
  "lock": {
    "match_radius": 60.0,
    "lost_frames_to_unlock": 5
  },
  "cursor": {
    "tick_hz": 240,
    "deadzone_px": 2,
    "auto_click": false,
    "click_dist_px": 18.0,
    "dwell_ms": 120,
    "cooldown_ms": 400,
    "min_sleep_us": 500,
    "pid": {
      "x": { "kp": 0.25, "ki": 0.00, "kd": 0.06, "outMin": -40, "outMax": 40, "iMin": -2000, "iMax": 2000, "dAlpha": 0.85 },
      "y": { "kp": 0.25, "ki": 0.00, "kd": 0.06, "outMin": -40, "outMax": 40, "iMin": -2000, "iMax": 2000, "dAlpha": 0.85 }
    }
  },
  "overlay": {
    "showConfidence": true,
    "showArea": false,
    "showIndex": false,
    "latencyPos": [10, 30],
    "fpsPos": [10, 60]
  }
}
```

### ROI / 坐标映射

当前 demo 在 `main.cpp` 里写死了：

```cpp
const int CAPTURE_OFFSET_X = 0;
const int CAPTURE_OFFSET_Y = 0;
```

如果未来只捕捉屏幕 ROI（非全屏），需要保证坐标转换一致：

* **图像坐标 → 屏幕坐标**：`screen = image + offset`
* **鼠标屏幕坐标 → 图像坐标**：`image = mouse - offset`

---

## 模块结构

### capture/

* `DxgiDesktopDuplicationSource`：通过 DXGI 复制桌面帧，输出 CPU 上可用的 `cv::Mat(BGR)`

### vision/

* `HsvMasker`：BGR→HSV→inRange，输出二值 mask（可选开/闭运算）
* `ContourDetector`：mask 上找轮廓，按面积阈值过滤，输出 `Detection`

### lock/

* `TargetLock`：锁定状态机（丢失帧累计解锁；锁定时只做匹配，不乱切目标）
* `LockMatcher`：用 `match_radius` 将当前检测与历史锁定目标做关联
* `NearestToAimSelector`：解锁时的默认选目标策略（选离 aimPoint 最近的 detection）

### cursor/

* `CursorAssistController`：线程循环读取 cursor、计算误差、PID 输出移动（可选 dwell click）
* `PIDAxis / PID2D`：带导数低通、输出限幅、积分限幅、简单 anti-windup

### viz/

* `OverlayRenderer`：绘制 bbox/中心点/文字/延迟等调试信息

### app & config/

* `LoadAppConfigOrDefault`：加载 JSON 配置，失败回退默认值
* `JsonLoader`：读文件 + parse JSON

---

## 常见问题

* **捕捉画面黑屏/卡住**

  * 部分全屏独占应用可能无法被 DXGI duplication 正常捕捉（建议窗口化/无边框）
  * 检查是否有权限/安全软件拦截（尤其是注入/屏幕录制相关策略）

* **检测不到目标**

  * 先开 `Mask` 窗口确认 HSV 阈值是否覆盖目标颜色
  * 适当调大 `morphClose` 填洞，或调大 `minArea` 过滤噪点

* **鼠标抖动/过冲**

  * 增大 `deadzone_px`
  * 降低 `kp`、提高 `kd`，或提高 `dAlpha`（更强导数滤波）
  * 收紧 `outMin/outMax` 限制单 tick 最大移动

---

## 下一步计划

* [ ] ROI 捕捉与配置化（用 `capture.offset_x/y` 统一管理坐标）
* [ ] 更稳健的目标关联：加入 bbox IOU / 速度预测 / 卡尔曼滤波
* [ ] GUI 调参面板（HSV/PID/锁定参数实时调试）
* [ ] 更丰富的选择策略：优先级、稳定性评分、多目标管理

---

## 免责声明与用途边界

本项目以**辅助交互/可访问性研究**为目标（如帮助行动不便者更易完成鼠标操作）。
请在合法合规的场景中使用与扩展本项目代码，并遵守你所在地区的法律法规与软件服务条款。

---

## License

本项目使用 **MIT License**。

Copyright (c) 2026 Rigel

详见仓库根目录的 `LICENSE` 文件。