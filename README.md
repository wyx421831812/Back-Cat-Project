# BackPet - 可自定义情绪价值桌面组件

> TRAE 创意大赛参赛作品

## 项目简介

BackPet 是一个灵感来源于 BongoCat 的 Windows 桌面互动组件，结合了手机端小组件的便捷功能，为用户提供持续的情绪价值。

## 技术栈

- **语言**: C++ 17
- **框架**: Qt 6 (兼容 Qt 5.15+)
- **模块**: core, gui, widgets, network
- **平台**: Windows

## 项目结构

```
BackPet/
├── BackPet.pro              # qmake 项目文件
├── resources.qrc            # Qt 资源文件
├── assets/
│   └── styles.qss           # 全局样式表
├── src/
│   ├── main.cpp             # 程序入口
│   ├── petwidget.h/cpp      # 主窗口 (无边框/透明/置顶/穿透/拖拽)
│   ├── petcanvas.h/cpp      # 宠物绘制画布 (QPainter 绘制 + 动画)
│   ├── appconfig.h/cpp      # 配置管理 (JSON 读写)
│   ├── settingsdialog.h/cpp # 设置对话框 (主题/颜色/背景/透明度)
│   └── components/
│       ├── componentbase.h  # 组件基类
│       ├── clockwidget.h/cpp    # 时钟组件
│       ├── quotewidget.h/cpp    # 每日寄语组件
│       └── todowidget.h/cpp     # 待办清单组件
```

## 核心功能

### 1. 互动宠物
- QPainter 原生绘制可爱宠物角色
- 4 种情绪状态: 安静/开心鼓掌/犯困打盹/高兴呐喊
- 点击宠物随机触发互动
- 高兴呐喊时弹出"爱上雷神~"对话框

### 2. 多组件切换
- 🐱 互动宠物 (BongoCat 风格)
- 🕐 时钟日历
- 💬 每日寄语
- ✅ 待办清单

### 3. 高度自定义
- 5 种预设主题配色 (黄紫/青紫/粉紫/翠绿/橙红)
- 自定义颜色选择器
- 自定义背景主题 (透明/纯色/渐变/图片/毛玻璃)
- 窗口透明度调节
- 点击穿透模式
- 始终置顶开关

### 4. 桌面集成
- 无边框透明窗口
- 系统托盘图标
- 右键菜单
- 可拖拽移动
- 窗口位置记忆

## 编译方法

### 使用 Qt Creator
1. 打开 `BackPet.pro`
2. 选择 Qt 6 (或 Qt 5.15+) Kit
3. 点击构建运行

### 命令行编译
```bash
# 使用 qmake
qmake BackPet.pro
make        # Linux/macOS
nmake       # Windows (MSVC)
mingw32-make # Windows (MinGW)
```

### 使用 CMake (可选)
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## 配置文件

配置存储在用户配置目录:
- Windows: `%APPDATA%/BackPet/backpet_config.json`
- 待办数据: `%APPDATA%/BackPet/backpet_todos.json`

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

## 使用说明

1. 启动后宠物显示在桌面右下角
2. **左键拖拽** 移动宠物位置
3. **左键点击** 宠物触发互动
4. **右键** 打开菜单切换组件/情绪/设置
5. **双击托盘图标** 快速切换宠物/时钟

## 许可证

MIT License - TRAE 创意大赛参赛作品
