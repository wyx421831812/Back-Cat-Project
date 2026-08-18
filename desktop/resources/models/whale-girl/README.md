# whale-girl · 鲸娘女仆（Live2D 模型包）

基于参考图生成的 **Q 版鲸鱼/海豚女仆娘** Live2D 模型工程包，格式与项目内
`standard` / `keyboard` / `gamepad` 三套模型完全一致，可直接被 BackPet
桌面宠物的 Live2D 渲染引擎加载。

- 角色：淡蓝色头发、海豚耳朵 + 鲸鱼尾巴、蓝色蕾丝女仆装与头饰，疲惫办公的表情
- 画风：Chibi kawaii 赛璐璐数字插画（原图由豆包 AI 生成，右下角带「豆包AI生成」水印）
- 画布：`whale-girl.1024/texture_00.png`（2656×1536，原始参考图）

## ⚠️ 当前状态：缺少 `.moc3`（需要 Cubism Editor）

Live2D 的 `.moc3` 是 Cubism Editor 导出的二进制蒙皮文件，无法用脚本从一张平面图直接生成。
本包已包含**除 moc3 外的全部配套文件**。用 Cubism Editor 完成骨架绑定后导出
`whale-girl.moc3` 放入本目录，模型即可被应用加载。

## 包内文件

| 文件 | 说明 |
| :-- | :-- |
| `cat.model3.json` | 模型清单（应用按此文件名自动发现模型，勿改名） |
| `whale-girl.moc3` | **待 Cubism Editor 导出**（当前不存在） |
| `whale-girl.1024/texture_00.png` | 纹理（原图） |
| `whale-girl.cdi3.json` | 参数/部件显示信息（即本包的参数与部件约定） |
| `live2d_expression0~2.exp3.json` | 表情：默认 / 疲惫 / 开心 |
| `exp_1.exp3.json` / `exp_2.exp3.json` | 表情：惊讶 / 生气 |
| `live2d_motion1.motion3.json` | 动作：待机（尾巴左右摇摆 + 头发晃动） |
| `live2d_motion2.motion3.json` | 动作：挥手 |
| `model.json` | 应用内模型 id/名称（`whale-girl` · 鲸娘 · 办公女仆） |
| `resources/cover.png` | 模型选择器缩略图（612×354） |
| `resources/background.png` | 背景图（612×354，模糊版） |

## 用 Cubism Editor 完成绑定（约 30–60 分钟）

> Cubism Editor 免费版即可：https://www.live2d.com/download/cubism/

1. **新建工程**：Cubism Editor → 新建 → 画布尺寸设为 2656×1536（与纹理一致）。
2. **导入纹理**：把 `whale-girl.1024/texture_00.png` 导入画布。
   - 建议先在任意绘图软件中把角色从背景中抠出并补透明通道，再作为模型纹理；
     若直接用整张场景图，桌子和键盘会随模型一起渲染（本项目宠物风格可接受）。
   - 建议同时去除右下角「豆包AI生成」水印。
3. **自动网格**：选中图层 → 自动网格（Auto Mesh）生成，之后按需在
   眼睛/嘴巴/发梢/尾巴处手动加变形器（Warp Deformer）。
4. **分部件**：按 `whale-girl.cdi3.json` 中的 Parts 命名建图层分组：
   `PartBody / PartHead / PartFace / PartEyeL / PartEyeR / PartBrowL / PartBrowR /
   PartMouth / PartHairFront / PartHairSide / PartHairBack / PartEarL / PartEarR /
   PartArmL / PartArmR / PartTail / PartFin / PartApron / PartHeaddress`。
5. **建参数**（名称必须与 `cdi3` 一致，应用渲染引擎按这些 id 驱动）：
   - 必选（引擎每帧驱动）：`ParamAngleX/Y/Z`、`ParamEyeBallX/Y`、`ParamEyeLOpen/ROpen`、
     `ParamBreath`、`ParamBodyAngleX/Y/Z`、`ParamMouseX/Y`、`ParamMouseLeftDown/RightDown`、
     `CatParamLeftHandDown/RightHandDown`、`ParamMouthOpenY`、`ParamMouthForm`
   - 建议：`ParamEyeLSmile/RSmile`、`ParamBrowL/RY`、`ParamBrowL/RAngle`、`ParamCheek`、
     `ParamHairFront/Side/Back`
   - 本模型特色：`ParamTailSway`（尾巴左右，建议范围 -30~30）、`ParamTailUpDown`（上下）、
     `ParamFinSway`（胸鳍）、`ParamEarTwitch`（耳朵抖动）、`Param3`（挥手 0~30）
   - 参数范围尽量与引擎假设一致：眼睛开闭 0~1、呼吸 0~1、角度类 ±30、手部按下 0~1。
6. **眨眼/嘴形组**：在模型设置中把 `ParamEyeLOpen`、`ParamEyeROpen` 加入 EyeBlink 组
   （LipSync 组可留空，与现有模型一致）。
7. **物理（可选）**：给尾巴/头发加 Physics 物理摆动（输出到
   `ParamTailSway`、`ParamHairFront/Side/Back`），导出后生成
   `whale-girl.physics3.json`，并在 `cat.model3.json` 的 FileReferences 中加一行
   `"Physics": "whale-girl.physics3.json"`。
8. **导出**：文件 → 导出为 moc3 → 命名 `whale-girl.moc3` 放入本目录；
   纹理若被编辑器重新打包，同步覆盖 `whale-girl.1024/texture_00.png`。

## 让应用加载它

模型管理器会扫描用户模型目录，**任意子目录含 `cat.model3.json` 即自动识别**：

```
%APPDATA%\BackPet\BackPet\bongo_models\whale-girl\
```

把本目录整个复制过去即可（应用启动后可在模型列表看到「鲸娘 · 办公女仆」）。
开发期也可直接把本目录放在 `desktop/resources/models/whale-girl/` 并在
`desktop/resources.qrc` 中追加文件条目（需重新编译）。

## 兼容性说明

- 入口文件名固定为 `cat.model3.json`（`bongomodelmanager.cpp` 的约定）。
- 渲染引擎（`live2d-view.html`）通过 `hasParam()` 探测参数存在性，缺失的参数会被安全跳过，
  因此参数范围/数量不强制，但按上表实现可获得完整动画效果（鼠标跟随、眨眼、呼吸、
  按键抬手、待机摇尾）。
- 动作文件为线性插值（flag=0），无需音频文件。
