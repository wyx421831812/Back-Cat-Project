# Back Cat Project · 开发路线图 (ROADMAP)

> 本文档详细描述 Back Cat Qt 正式版的分阶段实现目标。
> 每个阶段都是可独立运行的里程碑，方便渐进式开发与验证。

---

## 总览

| 阶段 | 代号 | 核心定位 | 预计周期 |
|---|---|---|---|
| **M1** | 🪟 「桌面壳子」 | 透明悬浮窗 + 键鼠监听跑通 | 2 ~ 3 周 |
| **M2** | 🐱 「会动的猫」 | Live2D 渲染 + 动作映射（BongoCat 核心玩法） | 3 ~ 4 周 |
| **M3** | 🧩 「组件系统」 | 模块化组件切换 + 设置面板 + 情绪互动 | 4 ~ 6 周 |
| **M4** | 🎨 「模型生态」 | 3D glTF / VRM / MMD 多模型格式支持 | 5 ~ 8 周 |
| **M5** | 🎥 「动捕虚拟形象」 | 摄像头/ARKit 面捕 → 虚拟形象 | 6 ~ 10 周 |
| **M6** | 🌟 「生态完善」 | 插件、i18n、自动更新、直播推流 | 持续迭代 |

---

## M1 · 桌面壳子

**目标**：一个透明、无边框、置顶、不抢焦点、可穿透点击的 Qt 窗口能跑起来，并能接收全局键盘/鼠标输入。

### 必须完成

| 编号 | 任务 | 技术要点 |
|---|---|---|
| 1.1 | CMake + Qt 6 工程搭建 | `find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Quick3D Widgets Multimedia Network LinguistTools)` |
| 1.2 | 跨平台透明无边框置顶窗口 | `QQuickWindow::setDefaultAlphaBuffer(true)` + `Qt::FramelessWindowHint  Qt::WindowStaysOnTopHint  Qt::Tool`；macOS 用 NSPanel 不抢焦点 |
| 1.3 | 鼠标点击穿透切换 | `SetWindowLongPtr`(Win) / `NSWindow setIgnoresMouseEvents`(macOS) / `XShapeCombineRectangles`(X11) 实现穿透模式 |
| 1.4 | 托盘菜单（Tray Icon） | `QSystemTrayIcon`：显示/隐藏、切换穿透模式、退出、打开设置（占位） |
| 1.5 | Windows 全局键鼠监听 | `SetWindowsHookEx(WH_KEYBOARD_LL)` + `WH_MOUSE_LL`；按左/右键、字母/数字 → 分类信号 |
| 1.6 | macOS 全局键鼠监听 | `CGEventTapCreate` + `kCGEventKeyDown`/`kCGEventLeftMouseDown`；处理 Accessibility 权限提示 |
| 1.7 | Linux X11 全局键鼠监听 | XRecord 扩展或 libinput；Wayland 可延后 |
| 1.8 | 手柄监听（可选 M1 内） | `QGamepadManager` 或 SDL2 GameController API；摇杆/按钮事件 |
| 1.9 | 基础信号槽数据通路 | `InputListener` 发出 `keyPressed(QString key, bool isLeft)` → 主窗口接收并打日志 |
| 1.10 | 配置持久化 V1 | `QSettings` 保存窗口位置 `pos`、大小 `size`、穿透 `clickThrough`、置顶 `alwaysOnTop`；启动自动恢复 |
| 1.11 | 拖拽调整位置 + 滚轮缩放 | 在非穿透模式下支持鼠标拖拽移动、滚轮缩放整体窗口 |

### 验收标准
- ✅ 三平台（Win/macOS/Linux X11）都能启动一个透明猫咪占位框
- ✅ 托盘菜单可用，穿透/非穿透切换生效
- ✅ 按任意键盘按键、鼠标点击，控制台输出对应事件（左/右、键名）
- ✅ 关闭后重启，窗口位置、大小、模式完全恢复

---

## M2 · 会动的猫

**目标**：正式引入 Live2D，把输入事件映射成猫咪动作，达到原版 BongoCat 的核心体验。

### 必须完成

| 编号 | 任务 | 技术要点 |
|---|---|---|
| 2.1 | Live2D Cubism SDK 集成 | 下载 Cubism 5.x Native SDK for C++；封装 `Live2DRenderer` 配合 `QOpenGLWindow` 或 QRhi |
| 2.2 | 内置模型 V1 | 至少 1 套 BongoCat 风格 Live2D 模型（左爪键、右爪键、空格、鼠标、表情） |
| 2.3 | 键鼠 → 参数映射表 | `config/mappings.json`：按键（按左/右手分区）→ 触发爪部动作；鼠标 → 对应爪抬起/按下 |
| 2.4 | 动作状态机 | 按键 → 动作 A → 持续 N ms → 回 idle；防止高频按键导致抖动（debounce + 平滑过渡） |
| 2.5 | 摇杆/方向键 → 头部偏转 | 手柄摇杆或方向键 → Live2D `ParamAngleX` / `ParamAngleY` 参数 |
| 2.6 | 表情与呼吸 idle | 空闲时偶发眨眼、嘴巴微动、呼吸效果，让猫「活」起来 |
| 2.7 | 模型资源路径管理 | `:/models/` 内置资源 + 用户目录（`%APPDATA%`/`~/.config`/`~/Library/Application Support`）自定义模型 |
| 2.8 | 窗口尺寸 → 模型自适应 | 模型按窗口宽高等比缩放，保持居中；`resizeEvent` 重新计算 |

### 验收标准
- ✅ 打字时左/右爪会随左/右手按键同步按下（类似 BongoCat 体验）
- ✅ 鼠标左/右键点击对应爪动作
- ✅ 空格/回车等长按键有独立动作
- ✅ 空闲 5 秒后猫会自然眨眼、微动
- ✅ 缩放窗口后模型始终按比例完整显示

---

## M3 · 组件系统 + 情绪价值

**目标**：从「单一 BongoCat 玩具」升级成「可切换的桌面组件平台」，并加入情绪互动。

### 必须完成

| 编号 | 任务 | 技术要点 |
|---|---|---|
| 3.1 | 组件抽象基类 | `class BaseWidget : public QObject`：统一生命周期 `onInit/onShow/onHide/onResize`、渲染接口、信号接口、配置读写 |
| 3.2 | 组件注册中心 + 热切换 | `ComponentManager` 提供 `switchTo(QString id)`；托盘菜单列出所有组件；切换平滑过渡动画 |
| 3.3 | ⏰ 时钟组件 | `ClockWidget`：默认显示大号数字时钟 + 日期；可选指针样式 |
| 3.4 | 🌤️ 天气组件 | 通过 QNetworkRequest 调用和风/OpenWeather API；显示当前温度、天气图标、24h 预报（可滚动） |
| 3.5 | ✅ 待办组件 | Todo 增删改查；本地 JSON 存储；到期提醒弹气泡 |
| 3.6 | 💬 每日寄语组件 | 内置语录库（分「治愈/励志/搞怪/情话」分类）；每日自动换一句；点击换一条 |
| 3.7 | 🫶 互动反馈系统 | 点击猫 → 随机反馈表情+音效；连续点击 10 次触发彩蛋；长时间不动推送问候 |
| 3.8 | 🎨 主题系统 | 亮色/暗色/跟随系统；自定义主题色主色 accent；背景透明度；背景图（图片/渐变） |
| 3.9 | ⚙️ 设置面板（主窗口） | 分页：`通用 / 外观 / 行为 / 模型 / 组件 / 快捷键 / 关于`；基于 QML `StackLayout` 或 QWidgets `QTabWidget` |
| 3.10 | 导入/导出配置 | `exportConfig(file)` / `importConfig(file)` → 单 JSON 文件，方便迁移/分享 |
| 3.11 | 多显示器支持 | 窗口可拖拽到任意屏幕；记住对应屏幕位置；屏幕断开后自动移回主屏 |

### 验收标准
- ✅ 托盘菜单可一键切换 5 种组件，切换带过渡动画
- ✅ 设置面板能改主题色、透明度、背景图，实时生效
- ✅ 配置可导出到 JSON、重新导入后完全一致
- ✅ 连点猫 10 次会触发特殊反应（比如生气/跳跃/撒花）

---

## M4 · 模型生态（2D/3D 融合）

**目标**：打破「只能用 Live2D 平面猫」的限制，支持 3D glTF/VRM、可选 MMD PMX，让用户玩到更多模型。

### 必须完成

| 编号 | 任务 | 技术要点 |
|---|---|---|
| 4.1 | 模型加载器工厂 | `ModelLoader::create(QString filePath)` → 根据扩展名返回对应 `IModel` 子类；统一 `render()/setParam()/playMotion()` 接口 |
| 4.2 | glTF 2.0 模型渲染 | `Qt Quick 3D` 直接 `Model { source: "cat.gltf" }`；封装 `GltfModel` 接入工厂 |
| 4.3 | VRM 0.x / 1.0 支持 | VRM = glTF + 扩展；VRM 1.0 直接用 glTF loader 加载 meta/humanoid；实现 VRM SpringBone（头发/尾巴摆动） |
| 4.4 | 模型选择 UI | 设置面板 → 「模型」页：内置模型 + 已导入模型列表；预览缩略图；一键切换 |
| 4.5 | 自定义模型导入 | 拖拽文件到窗口 → 识别格式 → 复制到用户目录 + 生成 metadata.json（作者、预览图、参数映射模板） |
| 4.6 | 参数映射编辑器 UI | 可视化编辑「键盘/鼠标事件 → 模型参数/动作」的映射；保存为 JSON 配置 |
| 4.7 | 动作/表情播放 | 模型内置 `.motion3.json`（Live2D）/ `.glb` 动画（glTF）/ `.vmd`（MMD）列表；右键菜单触发播放 |
| 4.8 | 三渲二 Toon Shader（可选） | Qt Quick 3D 自定义材质（CustomMaterial/QShader）实现 MMD 风格卡通描边着色 |
| 4.9 | MMD (PMX) 支持（可选） | 移植 libmmd 或自实现 PMX 解析 + 物理（Bullet 引擎简化版）；工作量大可延后 |

### 验收标准
- ✅ 至少内置一套 3D VRM 猫咪样例模型，拖入能正常显示并动起来
- ✅ 导入一个 glTF 文件（glTF-Sample-Models 任取一个），能渲染并可切换组件显示
- ✅ 参数映射编辑器可把 A 键绑定到某个 VRM BlendShape，按 A 键即时生效
- ✅ 拖拽 `cat.vrm` 到窗口后自动复制到用户目录、下次启动自动可切换

---

## M5 · 动作捕捉虚拟形象

**目标**：让 Back Cat 从「被动跟着键鼠动」进化为「用户动它也动」的 VTuber 级虚拟形象系统。

### 必须完成

| 编号 | 任务 | 技术要点 |
|---|---|---|
| 5.1 | 摄像头采集模块 | `QCamera` + `QVideoSink` 采集 640x480@30FPS；无摄像头时降级跳过 + 提示 |
| 5.2 | MediaPipe Face Landmarker 集成 | MediaPipe Tasks C++ API 推理 → 获取 468 landmarks + 52 BlendShape 系数 |
| 5.3 | Face → VRM/Live2D 映射 V1 | `jawOpen` → 嘴巴张合；`eyeBlinkLeft/Right` → 眨眼；`browOuterUp*` → 挑眉；`headRotation` → 头骨旋转 |
| 5.4 | Face → Live2D 参数映射 | 同样的 BlendShape 系数写入 `ParamMouthOpenY`/`ParamEyeLOpen`/`ParamAngleX` 等 |
| 5.5 | 平滑 + 防抖 | EMA/OneEuroFilter 平滑 landmarks 抖动，避免动画抽搐 |
| 5.6 | 嘴唇同步（Viseme） | BlendShape `mouthSmile/mouthFrown/mouthPucker` → VRM lip sync；可选：麦克风音量 → mouthOpen 辅助 |
| 5.7 | iPhone ARKit 接收端 | 监听 UDP 端口（iFacialMocap 协议 49983）→ 解析 52 blendshape → 复用上层管线（可在无真深感时作为高质量面捕方案） |
| 5.8 | 手势识别（可选 M5 内） | MediaPipe Hands 21 landmarks → 手部分类（握拳/张手/竖拇指/比耶等）→ 触发表情或动作 |
| 5.9 | 全身姿态（可选 M5 内） | MediaPipe Pose 33 landmarks → 上半身（肩/肘/腕）+ IK（FABRIK）解算 → 驱动 VRM 全身（头部+手臂+躯干） |
| 5.10 | 多源融合 + 延迟优化 | 摄像头面捕（~50ms）+ ARKit（~10ms）多源可选；动捕线程独立，渲染线程读共享数据，避免阻塞 |
| 5.11 | 校准界面 | 首次开启面捕引导用户摆正脸、眨眼张嘴做校准；保存个人默认参数阈值 |

### 验收标准
- ✅ 不接 iPhone、仅用普通笔记本摄像头，用户眨眼/张嘴/抬头低头，VRM 模型能同步且流畅（无明显抽搐）
- ✅ 接入 iFacialMocap App，面捕精度能达到 VSeeFace 同级效果（52 BlendShape 全部可用）
- ✅ 关掉摄像头，键鼠驱动依然可用；两种驱动方式可在设置面板一键切换 |
- ✅ 连续运行 10 分钟，内存与 CPU 稳定，无泄漏或崩溃 |

---

## M6 · 生态与高阶

**目标**：让项目从「个人玩具」成长为「可持续迭代的社区产品」。

| 编号 | 任务 | 技术要点 |
|---|---|---|
| 6.1 | 国际化 i18n | `QLinguist` + `.ts/.qm` 文件；支持 zh-CN/zh-TW/en-US/ja-JP/ko-KR/pt-BR；跟随系统语言，可手动切换 |
| 6.2 | 插件 SDK v1 | `IPlugin` 接口 + `PluginManager`：第三方组件、自定义模型格式、动捕数据源均可插件化；热加载（`QLibrary`） |
| 6.3 | 自动更新 | WinSparkle（Win）+ Sparkle（mac）+ AppImageUpdate（Linux）统一封装 `Updater`；对比 GitHub Releases latest |
| 6.4 | 单实例 + 二次启动唤起 | `QLocalServer/QLocalSocket`，第二次启动直接唤起主窗口，不重复开进程 |
| 6.5 | 快捷键系统 | `QHotkey` 或平台注册快捷键：全局切换穿透模式 `Ctrl+Alt+C`、显示/隐藏 `Ctrl+Alt+H`、唤醒设置 `Ctrl+Alt+S`、自定义 |
| 6.6 | 开机自启 | Windows 注册表 / macOS LaunchAgent / Linux `.desktop` autostart；UI 一键开关 |
| 6.7 | 直播推流集成（可选） | NDI SDK / Spout2（Win）/ Syphon（mac）发送模型画面给 OBS；或 OBS Virtual Camera 输出 |
| 6.8 | 模型社区对接 | 接入 GitHub `Awesome-BackCat-Models` 仓库列表；设置面板内直接浏览、一键下载、启用 |
| 6.9 | 崩溃日志与上报 | `Breakpad` 或 `drmingw` 捕获 dump；可选本地保存 vs 自动上传（隐私默认关） |
| 6.10 | Accessibility | 无障碍：键盘 Tab 可遍历设置面板；屏幕阅读器（NVDA/VoiceOver）支持；高对比度主题 |

---

## 跨平台策略

| 特性 | Windows | macOS | Linux X11 | Wayland |
|---|---|---|---|---|
| 透明悬浮窗 | ✅ M1 | ✅ M1（NSPanel） | ✅ M1 | ⚪ 可后续适配 |
| 全局键鼠 Hook | ✅ M1 | ✅ M1（需辅助功能权限） | ✅ M1 | ❌ 受限 |
| 点击穿透 | ✅ M1 | ✅ M1 | ✅ M1 | ❌ 受限 |
| 托盘菜单 | ✅ M1 | ✅ M1 | ✅ M1 | ✅ M1 |
| 手柄 | ✅ M1 | ✅ M1 | ✅ M1 | ✅ M1 |
| Live2D | ✅ M2 | ✅ M2 | ✅ M2 | ✅ M2 |
| VRM/glTF | ✅ M4 | ✅ M4 | ✅ M4 | ✅ M4 |
| 摄像头面捕 | ✅ M5 | ✅ M5 | ✅ M5 | ✅ M5 |
| ARKit 接收 | ✅ M5 | ✅ M5 | ✅ M5 | ✅ M5 |
| 开机自启 | ✅ M6 | ✅ M6 | ✅ M6 | ✅ M6 |
| 自动更新 | ✅ M6 | ✅ M6 | ✅ M6（AppImage） | ✅ M6 |

---

## 里程碑交付物

每个 M 里程碑结束后，应产出：

- [ ] **可运行安装包**：Win `.exe/msi`、mac `.dmg`（已公证）、Linux `.AppImage/.deb`
- [ ] **Release Notes**：中文 + 英文
- [ ] **演示 GIF/录屏**：放在 Release Description & README
- [ ] **API / 配置文档**：开发者与模型作者都能看懂怎么接
- [ ] **已知问题清单**：下一阶段要解决的坑

---

## 长远想法（Icebox，暂不排期）

- 🎭 多人联动：局域网两台电脑的猫咪互相做动作
- 🎮 与游戏联动：读取游戏内存/事件 → 猫咪跳舞/哭泣/欢呼
- 🎤 语音识别：唤醒词「猫猫」→ 触发问候、打开设置、念待办
- 🌙 AI 情绪陪伴：接入本地 LLM（llama.cpp）做轻量对话与情绪回应
- 📱 手机伴侣 App：手机划屏 → 电脑猫咪同步（WebSocket）

---

> 最后更新：2026-08-12
>
> 路线图是活的文档，会随开发进度持续调整。欢迎提 Issue / PR 一起完善！🚀
