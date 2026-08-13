# BackPet · 可自定义情绪价值桌面组件

> TRAE 创意大赛参赛作品 · 灵感来源于 BongoCat

## 项目简介

BackPet 是一个 Windows 桌面互动组件，灵感来源于 BongoCat，结合了手机端小组件的便捷功能，为用户提供持续的情绪价值。它可以常驻桌面最上层，用可爱的互动陪伴你的工作和学习。

不同于传统桌面宠物，BackPet 支持多种功能组件自由切换，用户可以自定义外观、行为和功能模块，做到功能性与情感陪伴的结合。

仓库按端拆分为两个独立子目录，互不依赖：

- [`desktop/`](desktop) — Qt 6 / C++ 桌面端（真正的桌宠应用）
- [`web/`](web) — Vue 3 / Vite 官网与交互演示站（浏览器中展示项目理念与预览）

> 两端相互独立、不共享运行时代码，可分别独立构建与运行。

---

## 技术栈

| 模块 | 位置 | 技术 |
|---|---|---|
| **桌面端** | `desktop/` | C++ 17 · Qt 6（兼容 Qt 5.15+）· qmake · 模块 core/gui/widgets/network |
| **桌面端渲染** | `desktop/` | QPainter 软件 3D（透视投影 + 深度排序 + 径向光照） |
| **官网/演示站** | `web/` | Vue 3 · Vite 5 |
| **平台** | — | Windows（macOS/Linux 见路线图） |

---

## 项目结构

```
BackPet/
├── desktop/                       # ── Qt 桌面端 ──
│   ├── BackPet.pro                # qmake 工程文件
│   ├── resources.qrc              # Qt 资源文件
│   ├── assets/
│   │   └── styles.qss             # 全局样式表
│   └── src/
│       ├── main.cpp               # 程序入口
│       ├── petwidget.h/cpp        # 主窗口 (无边框/透明/置顶/穿透/拖拽)
│       ├── petcanvas.h/cpp        # 宠物画布 (QPainter 软件3D渲染)
│       ├── appconfig.h/cpp        # 配置管理 (JSON 读写单例)
│       ├── settingsdialog.h/cpp   # 设置对话框 (主题/颜色/背景/透明度)
│       └── components/
│           ├── componentbase.h    # 组件抽象基类
│           ├── clockwidget.*      # 时钟组件
│           ├── quotewidget.*      # 每日寄语组件
│           ├── todowidget.*       # 待办清单组件
│           └── bongocatwidget.*   # BongoCat 组件 (全局键盘钩子 + 按键图层)
│
├── web/                           # ── Vue 官网/演示站 ──
│   ├── index.html                 # Vite 入口 HTML
│   ├── package.json               # Vite + Vue 3 依赖
│   ├── vite.config.js             # Vite 配置
│   ├── mood-pet-widget.html       # 单文件独立演示版 (含 echarts)
│   ├── _shared/js/echarts.min.js  # 共享 JS 库 (mood-pet-widget.html 引用)
│   └── src/
│       ├── main.js                # Vue 入口
│       ├── App.vue                # 根组件 (home / demo 双视图)
│       ├── assets/global.css      # 全局样式
│       └── components/*.vue       # Hero/Overview/Feature/Widget/... 展示组件
│
├── README.md
├── ROADMAP.md                     # M1–M6 里程碑路线图
└── .gitignore                     # 忽略 node_modules / Qt 构建产物等
```

> Qt 工程以 `.cpp/.h` 区分，Vue 工程以 `.vue/.js` 区分，现已分别归入 `desktop/` 与 `web/`，不再混在同一 `src/` 下。

---

## 核心功能（当前已实现）

### 1. 软件 3D 互动宠物
- QPainter 原生绘制，球体/椭球体组合 + 透视投影 + 深度排序 + 径向光照
- 5 种内置模型类型：猫咪 / 小熊 / 兔子 / 仙鸟 / 幻灵
- 4 种情绪状态：安静 / 开心鼓掌 / 犯困打盹 / 高兴呐喊
- 点击宠物随机触发互动，高兴呐喊时弹出对话框

### 2. BongoCat 键盘联动
- Windows 全局键盘钩子（`SetWindowsHookEx`）
- 按键时叠加对应按键图层，还原 BongoCat 玩法
- 支持 A–Z（部分）/ 1–5 / Space / Shift / Ctrl / Alt / Return

### 3. 多组件切换
- 🐱 互动宠物（3D 球体渲染）
- 🐱 BongoCat 键盘联动
- 🕐 时钟日历
- 💬 每日寄语
- ✅ 待办清单

### 4. 高度自定义
- 5 种预设主题配色（黄紫/青紫/粉紫/翠绿/橙红）
- 自定义颜色选择器
- 自定义背景主题（透明/纯色/渐变/图片/毛玻璃）
- 窗口透明度调节、点击穿透、始终置顶

### 5. 桌面集成
- 无边框透明窗口、系统托盘图标、右键菜单、可拖拽移动、窗口位置记忆

---

## 当前项目架构分析

当前架构以 **Qt Widget + QPainter 软渲染** 为核心，分为以下几层：

```
┌─────────────────────────────────────────────┐
│  PetWidget (主窗口 / 无边框透明置顶穿透)        │
│  ├─ QSystemTrayIcon (托盘 + 右键菜单)          │
│  └─ QStackedWidget (组件栈, 互斥切换)          │
├─────────────────────────────────────────────┤
│  组件层 ComponentBase                          │
│  ├─ PetCanvas        (软件3D宠物, 30fps)       │
│  ├─ BongoCatWidget   (键盘钩子 + 按键图层)     │
│  ├─ ClockWidget      (时钟)                    │
│  ├─ QuoteWidget      (寄语)                    │
│  └─ TodoWidget       (待办)                    │
├─────────────────────────────────────────────┤
│  渲染层 (PetCanvas)                            │
│  · 球体组合  · 透视投影  · 深度排序  · 径向光照  │
│  · 5 种硬编码模型  · QTimer 驱动动画            │
├─────────────────────────────────────────────┤
│  输入层                                        │
│  · Qt 鼠标事件 (拖拽/点击)                      │
│  · Win32 SetWindowsHookEx (仅 BongoCat)        │
├─────────────────────────────────────────────┤
│  配置层 AppConfig (单例 + JSON 持久化)          │
└─────────────────────────────────────────────┘
```

### 当前架构特征

| 维度 | 现状 | 说明 |
|---|---|---|
| **渲染方式** | CPU 软渲染 | QPainter 绘制球体，无 GPU 加速，30fps |
| **模型来源** | 硬编码 | 5 种模型写死在 C++ 里，无法导入外部模型 |
| **模型格式** | 无 | 不支持 Live2D / glTF / VRM / PMX |
| **输入驱动** | 键鼠事件 | 仅 BongoCat 用全局钩子，宠物本体靠点击 |
| **平台** | Windows only | 直接调用 `windows.h` / `SetWindowsHookEx` |
| **组件切换** | QStackedWidget | 互斥栈切换，无过渡动画 |
| **官网** | 独立 Vue 站 | 与桌面端无运行时共享，仅作展示 |

### 当前架构的瓶颈

1. **软渲染性能**：QPainter 在 CPU 上合成球体，模型复杂度和帧率受限，无法支撑高精度模型。
2. **模型封闭**：模型硬编码，用户无法导入/切换 Live2D 或 3D 模型，生态不可扩展。
3. **输入单一**：无面捕/动捕通路，宠物只能被动响应点击与按键。
4. **平台耦合**：输入层直接依赖 Win32 API，跨平台困难。
5. **无性能分级**：没有面向「打游戏/办公/直播」等不同负载场景的资源调度策略。

---

## 未来项目架构规划（三阶段）

针对当前瓶颈，BackPet 未来架构按 **三个阶段** 演进。每个阶段是可独立交付的里程碑，前一阶段为后一阶段奠基。

```
 一阶段                二阶段                  三阶段
 Live2D + 多组件  ──▶  3D 模型 + 性能优化  ──▶  虚拟形象 + 直播稳定
 (丝滑可控)            (低配置流畅)            (直播+游戏+形象)
```

### 一阶段 · Live2D 模型 + 多组件丝滑可控

**目标**：引入 Live2D Cubism SDK，实现键鼠→模型的丝滑参数驱动，补齐多组件可控体验，满足核心功能需求。

| 模块 | 升级内容 |
|---|---|
| **渲染层** | 新增 `Live2DRenderer`（Cubism 5.x Native SDK + QOpenGLWidget/QRhi），与现有 QPainter 软渲染并存，可切换 |
| **模型层** | Live2D `.moc3` 模型加载；内置至少 1 套 BongoCat 风格模型；支持用户目录自定义模型 |
| **输入层** | 抽象 `InputListener`（跨平台键鼠监听接口）；Win 用 `SetWindowsHookEx`、macOS 用 `CGEventTap`、Linux 用 XRecord |
| **映射层** | `mappings.json` 配置「键鼠事件 → Live2D 参数」（左右爪按键、鼠标抬起、`ParamAngleX/Y` 头部偏转） |
| **状态机** | 动作状态机 + debounce + 平滑过渡，防止高频按键抖动 |
| **组件层** | 升级 `ComponentManager` 热切换 + 过渡动画；保留 Clock/Quote/Todo/BongoCat |
| **idle** | 空闲眨眼/嘴巴微动/呼吸，让模型「活」起来 |

**验收**：打字时左右爪随左右手按键同步按下；空格/回车有独立动作；空闲 5s 自然眨眼；缩放窗口模型等比居中。

### 二阶段 · 3D 模型配置 + 性能优化

**目标**：打破「只能用 2D 平面模型」限制，支持 3D glTF/VRM 真实模型；将渲染从 CPU 软渲染迁移到 GPU 硬渲染，降低电脑配置需求，保证打游戏/办公场景下流畅。

| 模块 | 升级内容 |
|---|---|
| **渲染层** | 引入 **Qt Quick 3D / QRhi**（Vulkan / D3D11 / Metal 后端），GPU 硬件加速；QPainter 软渲染降级为兜底方案 |
| **模型层** | `ModelLoader` 工厂统一接口 `render()/setParam()/playMotion()`；支持 **glTF 2.0** + **VRM 0.x/1.0**（含 SpringBone 摆动）；MMD PMX 可选 |
| **模型管理** | 模型选择 UI（内置 + 已导入列表 + 缩略图预览）；拖拽导入自动生成 metadata；参数映射可视化编辑器 |
| **性能优化** | · GPU 渲染替代 CPU 软渲染，大幅降低 CPU 占用<br>· 帧率自适应（空闲降帧、交互升帧）<br>· 资源懒加载 + 纹理压缩 + 内存池<br>· 渲染线程与逻辑线程分离 |
| **场景适配** | 检测前台进程（游戏/办公），动态切换渲染质量档位，避免抢占用游戏/办公的 CPU/GPU 资源 |

**验收**：内置一套 VRM 猫咪样例模型可正常显示并动起来；导入 glTF 文件可渲染切换；按键绑定 VRM BlendShape 即时生效；游戏/办公场景下 CPU 占用显著下降、不掉帧。

### 三阶段 · 虚拟形象 + 直播性能稳定

**目标**：从「被动跟键鼠动」进化为 VTuber 级虚拟形象系统，支持面捕驱动；保证 **直播 + 游戏 + 虚拟形象** 三者同开时在正常电脑配置下稳定运行，并明确最低/推荐显卡规格。

| 模块 | 升级内容 |
|---|---|
| **面捕层** | · `QCamera` + MediaPipe Face Landmarker（468 landmarks + 52 BlendShape）<br>· iPhone ARKit 接收端（iFacialMocap UDP 49983 协议）<br>· 可选手势（Hands）/ 全身姿态（Pose + IK） |
| **驱动层** | BlendShape → VRM / Live2D 参数双通路；EMA/OneEuroFilter 平滑防抖；嘴唇同步 Viseme |
| **多源融合** | 面捕（~50ms）+ ARKit（~10ms）多源可选；动捕线程独立，渲染线程读共享数据，避免阻塞 |
| **推流集成** | NDI SDK / Spout2（Win）/ Syphon（mac）发送模型画面给 OBS；或 OBS Virtual Camera 输出 |
| **性能保障** | · 动捕推理 + 模型渲染 + 推流三线程隔离<br>· 推理降分辨率/半精度；GPU 任务优先级调度<br>· 内存上限保护 + 连续运行稳定性（无泄漏/无崩溃） |
| **校准** | 首次开启面捕引导摆正脸/眨眼/张嘴校准，保存个人阈值 |

**验收**：仅用笔记本摄像头即可让 VRM 模型同步眨眼/张嘴/抬头，无明显抽搐；接入 iFacialMocap 达到 VSeeFace 同级 52 BlendShape；关闭摄像头键鼠驱动仍可用；直播+游戏+形象同开连续 10 分钟稳定。

#### 三阶段硬件规格建议

| 场景 | 最低显卡 | 推荐显卡 | 说明 |
|---|---|---|---|
| **一阶段**（Live2D） | Intel HD 620 / 同级核显 | GTX 1050 / RX 560 及以上 | Live2D 负载低，核显即可 |
| **二阶段**（3D 模型 + 游戏/办公同开） | GTX 1050 Ti / RX 570 | GTX 1650 / RX 6500 XT 及以上 | 需与游戏/办公共享 GPU，留余量 |
| **三阶段**（直播 + 游戏 + 虚拟形象） | GTX 1660 / RX 5600 XT | RTX 3060 / RX 6600 及以上 | 面捕推理 + 渲染 + 推流 + 游戏并发，需硬件编码与 AI 算力 |

> 显卡规格为开发目标参考值，最终以实测为准并随版本更新调整。

---

## 当前架构 vs 未来架构 对比

| 维度 | 当前（已实现） | 一阶段 | 二阶段 | 三阶段 |
|---|---|---|---|---|
| **渲染** | QPainter CPU 软渲染 | + Live2D OpenGL | Qt Quick 3D / QRhi GPU | GPU + 面捕推理管线 |
| **模型** | 5 种硬编码球体 | Live2D `.moc3` | + glTF / VRM | + VRM BlendShape 驱动 |
| **输入** | 点击 + Win 键盘钩子 | 跨平台 InputListener | + 参数映射编辑器 | + MediaPipe / ARKit 面捕 |
| **平台** | Windows only | + macOS / Linux | 三平台 | 三平台 |
| **组件切换** | QStackedWidget | + 过渡动画 | 沿用 | 沿用 |
| **性能策略** | 无 | 平滑过渡 | 帧率自适应 + 进程感知 | 三线程隔离 + 推流 |
| **显卡要求** | 核显即可 | 核显可 | 独显起步 | 中端独显起步 |
| **定位** | 桌面玩具 | 丝滑可控组件 | 低配置 3D 伴侣 | VTuber 直播虚拟形象 |

### 演进关键点

1. **渲染从 CPU → GPU**：一阶段引入 OpenGL（Live2D），二阶段全面迁移到 QRhi，是降低配置需求的根本。
2. **模型从封闭 → 开放**：从硬编码到工厂 + 文件加载，是生态可扩展的前提。
3. **输入从被动 → 主动**：从点击/按键到面捕/动捕，是从玩具到虚拟形象的跨越。
4. **性能从无 → 分级调度**：进程感知 + 线程隔离 + 帧率自适应，是同开三场景稳定的关键。

> 更细粒度的任务拆解（M1–M6 里程碑）见 [ROADMAP.md](ROADMAP.md)。本 README 的三阶段规划与 ROADMAP 互补：README 侧重架构演进与目标，ROADMAP 侧重逐任务实现路径。

---

## 编译方法

> 桌面端与网页端分别在各自子目录内独立构建。

### Qt 桌面端（`desktop/`）

#### 使用 Qt Creator
1. 打开 `desktop/BackPet.pro`
2. 选择 Qt 6（或 Qt 5.15+）Kit
3. 点击构建运行

#### 命令行编译
```bash
cd desktop
# 使用 qmake
qmake BackPet.pro
make            # Linux/macOS
nmake           # Windows (MSVC)
mingw32-make    # Windows (MinGW)
```

#### 使用 CMake（可选）
```bash
cd desktop
mkdir build && cd build
cmake ..
cmake --build .
```

### Vue 官网/演示站（`web/`）
```bash
cd web
npm install     # 首次需安装依赖（node_modules 已加入 .gitignore，不会进入仓库）
npm run dev     # 本地开发 (http://localhost:5173)
npm run build   # 生产构建
npm run preview # 预览构建产物
```

> 单文件独立演示版可直接在浏览器打开 `web/mood-pet-widget.html`（无需构建）。

---

## 配置文件

配置存储在用户配置目录：
- Windows：`%APPDATA%/BackPet/backpet_config.json`
- 待办数据：`%APPDATA%/BackPet/backpet_todos.json`

```json
{
    "themeName": "yellowPurple",
    "primaryColor": "#fbbf24",
    "secondaryColor": "#8b5cf6",
    "accentColor": "#6366f1",
    "windowOpacity": 100,
    "alwaysOnTop": true,
    "clickThrough": false,
    "currentComponentIndex": 0
}
```

---

## 使用说明

1. 启动后宠物显示在桌面右下角
2. **左键拖拽** 移动宠物位置
3. **左键点击** 宠物触发互动
4. **右键** 打开菜单切换组件/情绪/设置
5. **双击托盘图标** 快速切换宠物/时钟

---

## 许可证

MIT License — TRAE 创意大赛参赛作品
