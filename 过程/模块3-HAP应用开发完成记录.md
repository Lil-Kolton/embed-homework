# 模块3 — HAP 应用开发完成记录

> 作者：Kolton
> 日期：2026-06-11

---

## 一、做了什么？

将智慧台灯 HAP 应用从独立的 JS 项目目录复制到主构建树，并成功构建出 HAP 安装包。

---

## 二、修改了哪些文件？

### 2.1 新建目录和文件

**路径**：`bearpi-hm_micro_small/applications/BearPi/BearPi-HM_Micro/samples/Micro_E53_SmartLamp/`

```
Micro_E53_SmartLamp/
├── BUILD.gn                    ← 新建：GN 构建配置
├── cert/                       ← 新建：签名证书目录
│   └── com.huawei.launcher_HarmonyAppProvision_release.p7b
├── entry/src/main/
│   ├── config.json             ← 从 JS 项目复制
│   ├── js/default/             ← 从 JS 项目复制（包含我写的台灯页面）
│   │   ├── app.js
│   │   ├── pages/
│   │   │   ├── index/          ← 首页（场景选择）
│   │   │   ├── ludeng/         ← 智慧台灯页面 ⭐ 重点修改
│   │   │   ├── nongye/         ← 农业页面
│   │   │   ├── yangan/         ← 烟感页面
│   │   │   ├── jinggai/        ← 井盖页面
│   │   │   └── hongwai/        ← 红外页面
│   │   └── common/             ← 图片资源
│   └── resources/              ← 字符串资源
```

### 2.2 BUILD.gn 内容

```gn
import("//build/lite/config/hap_pack.gni")

hap_pack("Micro_E53_SmartLamp_hap") {
  mode = "hap"
  json_path = "entry/src/main/config.json"
  assets_path = "entry/src/main/js"
  resources_path = "entry/src/main/resources"
  force = "true"
  hap_name = "Micro_E53_SmartLamp"
  cert_profile = "cert/com.huawei.launcher_HarmonyAppProvision_release.p7b"
  privatekey = "HOS Application Provision Release"
}
```

---

## 三、智慧台灯页面功能（ludeng）

### 3.1 界面结构（ludeng.hml）

| 区域 | 内容 |
|------|------|
| 顶部导航栏 | 返回按钮 + 标题"智慧台灯" |
| 灯光显示区 | 圆形图标，开灯亮金色，关灯暗灰 |
| 数据卡片 | 两张卡片：环境光照(Lux)、当前亮度(%) |
| 亮度调节区 | 滑动条，0-100%，拖动实时调光 |
| 场景模式 | 4个按钮：阅读(100%)、休闲(60%)、夜灯(10%)、自定义 |
| 定时关灯 | 3个按钮：10分钟、30分钟、60分钟 + 倒计时显示 |
| 底部电源按钮 | 圆形开关按钮，开灯青绿色发光，关灯灰色 |

### 3.2 样式配色（ludeng.css）

| 变量 | 颜色 | 用途 |
|------|------|------|
| primary-100 | #00D2BE | 主色（青绿色） |
| accent-100 | #FFD700 | 强调色（金色，模拟台灯暖光） |
| bg-100 | #1C1C1C | 主背景（深色） |
| bg-200 | #2b2b2b | 卡片背景 |
| text-100 | #FFFFFF | 主文字 |

### 3.3 逻辑功能（ludeng.js）

| 功能 | 命令码 | 数据 | 说明 |
|------|--------|------|------|
| 初始化设备 | cmd=0 | - | 进入页面时调用 |
| 读取数据 | cmd=2 | - | 每2秒轮询，获取光照+亮度 |
| 开关灯 | cmd=3 | "ON"/"OFF" | 底部电源按钮 |
| 调节亮度 | cmd=4 | "0"-"100" | 滑动条拖动 |
| 场景模式 | cmd=4 | 预设值 | 阅读=100, 休闲=60, 夜灯=10 |
| 定时关灯 | cmd=3 | "OFF" | 倒计时结束后自动关灯 |
| 退出页面 | cmd=1 | - | 清理定时器，关闭设备 |

---

## 四、构建方法

### 方法1：使用 hap_pack.py 直接构建

```bash
cd /home/bearpi/project/bearpi-hm_micro_small
java -jar developtools/packing_tool/jar/hmos_app_packing_tool.jar \
  --mode hap \
  --json-path applications/BearPi/BearPi-HM_Micro/samples/Micro_E53_SmartLamp/entry/src/main/config.json \
  --assets-path applications/BearPi/BearPi-HM_Micro/samples/Micro_E53_SmartLamp/entry/src/main/js \
  --resources-path applications/BearPi/BearPi-HM_Micro/samples/Micro_E53_SmartLamp/entry/src/main/resources \
  --out-path out/Micro_E53_SmartLamp.hap \
  --force true
```

### 方法2：使用 GN 构建系统（需要 hb 工具）

```bash
hb build -t notest --tee -f --build-target Micro_E53_SmartLamp_hap
```

### 构建结果

| 项目 | 内容 |
|------|------|
| HAP 文件名 | Micro_E53_SmartLamp.hap |
| 文件大小 | 163KB |
| 输出路径 | out/Micro_E53_SmartLamp.hap |
| 备份位置 | applications/.../hap_example/Micro_E53_SmartLamp.hap |

---

## 五、安装方法

### 1. 烧录固件

使用 STM32CubeProgrammer 烧录 `OHOS_Image.stm32`

### 2. 传输 HAP 文件

将 `Micro_E53_SmartLamp.hap` 和 `bm` 工具复制到开发板（TF卡或USB）

### 3. 安装 HAP

```bash
./bm set -s disable
./bm set -d enable
./bm install -p Micro_E53_SmartLamp.hap
```

### 4. 运行应用

在开发板屏幕上点击"智慧台灯"图标

---

## 六、与模块1、2的联动

| JS 命令 | 驱动命令 | 硬件操作 |
|---------|----------|----------|
| cmd=0 (init) | E53_SC1_Start | 初始化 BH1750 + PWM + 按键 |
| cmd=1 (close) | E53_SC1_Stop | 释放资源 |
| cmd=2 (read) | E53_SC1_Read | 读取光照值，返回 JSON |
| cmd=3 (deng) | E53_SC1_SetLight | ON/OFF 控制 LED |
| cmd=4 (brightness) | E53_SC1_SetBrightness | PWM 调光 0-100% |

**JSON 返回格式**：`{"Lux":123.45,"LED":"ON","Brightness":80}`

---

## 七、注意事项

1. **证书问题**：当前使用 launcher 的证书，可能需要替换为专用证书
2. **hb 工具**：VM 上未安装 hb，使用 hap_pack.py 直接构建
3. **图片资源**：当前复用 Micro_E53 的图标，可后续替换为台灯专用图标
4. **其他页面**：保留了 nongye、yangan、jinggai、hongwai 页面，可按需删除
