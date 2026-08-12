# Back Cat Project 🐱

> 一款基于 **Qt 6 + C++** 打造的跨平台桌面互动宠物 / 情绪价值组件
> 灵感来源于 Bongo Cat，目标是成为更轻、更强、更好玩的桌面伙伴。

![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-blue)
![Qt](https://img.shields.io/badge/Qt-6.5+-41CD52?logo=qt)

---

## ✨ 项目愿景

Back Cat 不只是一只「敲键盘时会动的猫」，而是一款 **可自定义、可扩展、具备情绪价值** 的桌面常驻组件系统：

- 🖱️ **键鼠/手柄驱动**：打字、点击、推摇杆时，猫咪同步做出对应动作（基础玩法）
- 🧩 **模块化组件**：宠物、时钟、天气、待办、每日寄语……自由切换，像手机桌面小组件一样方便
- 💖 **情绪互动**：点击、抚摸、投喂都会触发不同反应，定时推送治愈语录
- 🎨 **深度自定义**：模型、皮肤、配色、大小、位置、透明度……想怎么改就怎么改
- 🚀 **轻量高效**：Qt 原生开发，内存 ~50MB，CPU 几乎忽略不计，24h 挂机不卡机
- 🔮 **未来可期**：3D 模型支持、VRM/MMD 模型接入、摄像头面部动捕、全身动作捕捉……

---

## 📁 仓库说明

本仓库目前包含两部分：

| 目录 | 说明 | 状态 |
|---|---|---|
| `src/` + `index.html` + `vite.config.js` | **Vue 3 + Vite 预览页**：展示产品概念、特性、交互 Demo（当前 `mood-pet-widget.html` 已有可运行的原型） | ✅ 预览可用 |
| `_shared/` | 共享资源（模型素材、配置参考等） | 🚧 准备中 |
| `qt-app/`（即将创建） | **Qt 6 + C++ 正式版源码**：真正的跨平台桌面应用 | 🚀 开发中 |

> 👉 如果你想先看效果，直接运行 `pnpm install && pnpm dev` 打开预览页即可。

---

## 🏗️ Qt 正式版架构（目标）

```
Back-Cat-Project (Qt 6 + C++)
├── app/                     # 应用入口
│   └── main.cpp             # QGuiApplication 初始化
├── core/                    # 核心层
│   ├── WindowManager.*      # 窗口管理（透明、置顶、无边框、点击穿透、托盘）
│   ├── InputListener.*      # 全局键鼠/手柄监听（平台原生 API）
│   ├── ConfigStorage.*      # 配置持久化（QSettings + JSON）
│   └── PlatformHooks.*      # 平台相关：macOS NSPanel / Windows DWM / Linux X11
├── components/              # 组件系统（热插拔）
│   ├── BaseWidget.*         # 抽象基类：统一生命周期、信号槽、渲染接口
│   ├── CatWidget.*          # 宠物组件（2D/3D 模型渲染）
│   ├── ClockWidget.*        # 时钟组件
│   ├── WeatherWidget.*      # 天气组件
│   ├── TodoWidget.*         # 待办组件
│   └── QuoteWidget.*        # 每日寄语组件
├── renderer/                # 渲染系统
│   ├── Renderer2D.*         # QML/Scene Graph 2D 渲染
│   ├── Renderer3D.*         # Qt Quick 3D 渲染（glTF/VRM）
│   ├── Live2DRenderer.*     # Live2D Cubism SDK 封装
│   └── MmdRenderer.*        # MMD (PMX) 渲染器（可选）
├── model/                   # 模型管理
│   ├── ModelLoader.*        # 模型加载器工厂
│   ├── Live2DModel.*        # Live2D 模型封装
│   ├── GltfModel.*          # glTF 2.0 / VRM 模型封装
│   └── PmxModel.*           # PMX MMD 模型封装（可选）
├── capture/                 # 动作捕捉系统（中长期目标）
│   ├── FacePipeline.*       # MediaPipe Face → BlendShape 管线
│   ├── ARKitReceiver.*      # iPhone ARKit BlendShape 接收端（UDP）
│   ├── PosePipeline.*       # MediaPipe Pose / Hands 全身+手势管线
│   └── MocapFusion.*        # 多源融合 + IK 解算
├── ui/                      # 设置界面
│   ├── PreferenceDialog.*   # 主设置面板（外观、行为、模型、组件、快捷键…）
│   └── pages/*              # 分页式设置
├── plugins/                 # 插件接口（预留）
│   └── IPlugin.*            # 插件抽象基类
├── resources/
│   ├── models/              # 内置模型
│   │   ├── cat-default/     # 默认 Live2D 猫
│   │   └── cat-3d/          # 默认 3D 猫（glTF/VRM）
│   ├── icons/
│   ├── translations/        # i18n（zh_CN/en_US/ja_JP…）
│   └── qml/                 # QML 界面资源
└── tests/                   # 单元测试（Qt Test）
```

---

## 🧰 技术栈

### 前端/渲染层
- **Qt Quick 3D**：3D 模型（glTF 2.0 / VRM）渲染、骨骼动画、BlendShape
- **Qt Quick / QML**：2D UI、组件面板、设置界面
- **Live2D Cubism SDK C++**：Live2D 模型加载与驱动
- **Qt Multimedia + OpenCV**：摄像头采集（面捕用）

### 后端/原生层
- **C++20 / Qt 6.5+** 核心框架
- **全局输入监听**：
  - Windows：`SetWindowsHookEx` (WH_KEYBOARD_LL / WH_MOUSE_LL)
  - macOS：`CGEventTapCreate` + `IOHIDManager`
  - Linux：X11 `XSelectInput` / XRecord / libinput
- **手柄监听**：SDL2 GameController API 或 `QGamepadManager`

### 动捕与 AI
- **MediaPipe Tasks C++**：Face Landmarker / Pose / Hands（摄像头动捕）
- **VRM SpringBone**：自实现物理摆动（头发、衣物、尾巴）
- **IK 解算**：FABRIK / CCD 算法（全身动捕骨骼逆向）

### 工程工具
- **构建系统**：CMake 3.20+（配合 Qt CMake API）
- **打包部署**：
  - Windows：`windeployqt` + NSIS / MSIX
  - macOS：`macdeployqt` + DMG（公证 notarization）
  - Linux：`linuxdeployqt` / AppImage / DEB / RPM
- **自动更新**：WinSparkle（Win）/ Sparkle（mac）/ AppImageUpdate（Linux）或自实现
- **代码规范**：`clang-format` + `clang-tidy`

---

## 🖥️ 跨平台支持

| 平台 | 最低版本 | 说明 |
|---|---|---|
| **Windows** | 10 1809+ | WebView2 不依赖；完全 Qt 原生；输入 Hook + XInput 手柄 |
| **macOS** | 12 (Monterey)+ | NSPanel 悬浮窗、CGEventTap、ARkit 面捕需真深感设备 |
| **Linux** | X11 (Ubuntu 20.04+/Debian 11+/Fedora 34+) | Wayland 支持视需求；X11 优先 |

---

## 🧭 核心特性（已规划）

### Phase 1 — 基础桌面宠物
- [x] 透明无边框置顶窗口
- [x] 点击穿透模式切换
- [ ] 键鼠动作同步（打字/按键 → 左爪/右爪动作）
- [ ] 手柄摇杆/按键 → 猫咪动作同步
- [ ] 拖拽调整位置，滚轮缩放
- [ ] 系统托盘菜单
- [ ] 配置持久化（自动保存位置/大小/模式）

### Phase 2 — 组件化与情绪价值
- [ ] 模块化组件基类（可热插拔切换）
- [ ] ⏰ 时钟组件
- [ ] 🌤️ 天气组件
- [ ] ✅ 待办事项组件
- [ ] 💬 每日寄语组件
- [ ] 🫶 点击 / 抚摸 / 投喂 → 多种互动反馈
- [ ] 🎨 主题色 / 透明度 / 背景图自定义

### Phase 3 — 模型生态
- [ ] Live2D 模型渲染 + 参数映射
- [ ] 内置猫咪 Live2D 模型
- [ ] 自定义模型导入（拖拽 + JSON 配置）
- [ ] 模型市场 / 分享社区链接
- [ ] glTF 2.0 3D 模型支持（Qt Quick 3D）
- [ ] VRM 1.0 模型支持（VTuber 虚拟形象）
- [ ] **MMD（PMX）模型支持**（可选）

### Phase 4 — 动作捕捉虚拟形象
- [ ] MediaPipe Face 摄像头面捕 → Live2D / VRM BlendShape
- [ ] 头部姿态（位置+旋转）→ 模型头骨
- [ ] iPhone ARKit 52 BlendShape 接收（支持 iFacialMocap 协议）
- [ ] 嘴唇同步（Viseme）
- [ ] MediaPipe Hands 手势识别 → 模型手部动作
- [ ] MediaPipe Pose 全身动捕 + IK 解算
- [ ] Vive / SteamVR 设备追踪（可选）

### Phase 5 — 生态与高阶
- [ ] 多语言（zh-CN/en-US/ja-JP/ko-KR…）
- [ ] 插件系统（第三方组件 / 模型扩展）
- [ ] 配置导入 / 导出 / 分享
- [ ] 多显示器支持
- [ ] 直播推流（NDI / Virtual Camera / OBS 源）
- [ ] 自动更新

> 👉 详细实现计划请查看：[ROADMAP.md](./ROADMAP.md)

---

## 🚀 预览版快速开始（Vue 原型）

```bash
# 克隆仓库
git clone https://github.com/wyx421831812/Back-Cat-Project.git
cd Back-Cat-Project

# 安装依赖（推荐 pnpm，也可用 npm）
pnpm install

# 启动预览（默认 http://localhost:5173）
pnpm dev

# 查看独立组件 Demo
# 浏览器直接打开 mood-pet-widget.html 即可
```

---

## 🤝 参考与灵感

- **[Bongo-Cat-Mver](https://github.com/MMmmmoko/Bongo-Cat-Mver)**：最初的灵感来源，原版 Windows Bongo Cat
- **[ayangweb/BongoCat](https://github.com/ayangweb/BongoCat)**：Tauri + Live2D 的优秀跨平台实现（本项目的架构对比参考）
- **[VSeeFace](https://www.vseeface.icu/)**：行业级 VTuber 工具，动捕 + VRM 参考
- **MediaPipe**：Google 开源 AI 视觉框架
- **VRM Consortium**：VRM 标准与生态

---

## 📄 License

本项目代码采用 **MIT License** 开源，具体内置模型素材请遵循各自作者的授权协议。

---

<div align="center">
  <i>Made with 💖 for every lonely coder out there.</i><br/>
  <b>编程很苦，猫咪很甜。</b>
</div>
