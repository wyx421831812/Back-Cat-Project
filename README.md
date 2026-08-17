# BackPet · 可自定义情绪价值桌面组件

> TRAE 创意大赛参赛作品 · 灵感来源于 BongoCat

## 项目简介

BackPet 是一个 Windows 桌面互动组件，灵感来源于 BongoCat，结合了手机端小组件的便捷功能，为用户提供持续的情绪价值。它可以常驻桌面最上层，用可爱的互动陪伴你的工作和学习。

不同于传统桌面宠物，BackPet 支持多种功能组件自由切换，用户可以自定义外观、行为和功能模块，做到功能性与情感陪伴的结合。

## 技术栈

| 模块 | 技术 |
|---|---|
| **桌面端** | C++ 17 · Qt 6（兼容 Qt 5.15+）· qmake · 模块 core/gui/widgets/network |
| **桌面端渲染** | QPainter 软件 3D（透视投影 + 深度排序 + 径向光照） |
| **平台** | Windows（macOS/Linux 见路线图） |

## 项目结构

```
BackPet/
├── BackPet.pro                # qmake 工程文件
├── resources.qrc              # Qt 资源文件
├── assets/
│   ├── styles.qss             # 全局样式表
│   └── bongo/                 # BongoCat 图片资源
└── src/
    ├── main.cpp               # 程序入口
    ├── petwidget.h/cpp        # 主窗口 (无边框/透明/置顶/穿透/拖拽)
    ├── petcanvas.h/cpp        # 宠物画布 (QPainter 软件3D渲染)
    ├── appconfig.h/cpp        # 配置管理 (JSON 读写单例)
    ├── settingsdialog.h/cpp   # 设置对话框 (主题/颜色/背景/透明度)
    └── components/
        ├── componentbase.h    # 组件抽象基类
        ├── clockwidget.*      # 时钟组件
        ├── quotewidget.*      # 每日寄语组件
        ├── todowidget.*       # 待办清单组件
        └── bongocatwidget.*   # BongoCat 组件 (全局键盘钩子 + 按键图层)
```

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
- 7 种预设主题配色（黄紫/青紫/粉紫/翠绿/橙红/仙鸟/幻灵）
- 自定义颜色选择器
- 窗口透明度调节、点击穿透、始终置顶

### 5. 桌面集成
- 无边框透明窗口、系统托盘图标、右键菜单、可拖拽移动、窗口位置记忆

## 编译方法

### 使用 Qt Creator
1. 打开 `BackPet.pro`
2. 选择 Qt 6（或 Qt 5.15+）Kit
3. 点击构建运行

### 命令行编译
```bash
# 使用 qmake
qmake BackPet.pro
nmake           # Windows (MSVC)
mingw32-make    # Windows (MinGW)
```

## 配置文件

配置存储在用户配置目录：
- Windows：`%APPDATA%/BackPet/backpet_config.json`
- 待办数据：`%APPDATA%/BackPet/backpet_todos.json`

## 使用说明

1. 启动后宠物显示在桌面右下角
2. **左键拖拽** 移动宠物位置
3. **左键点击** 宠物触发互动
4. **右键** 打开菜单切换组件/情绪/设置
5. **双击托盘图标** 快速切换宠物/时钟

## 许可证

MIT License — TRAE 创意大赛参赛作品
