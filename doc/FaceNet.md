# 通用NPU人脸识别示例

### 介绍

本示例指导开发者如何在OpenHarmony上利用展锐NPU人脸识别功能，包括人脸检测、特征提取和相似度计算等核心步骤。

### 效果预览
![main](figures/facenet_main.png)
### 使用说明

#### 人脸比对功能（Index.ets）

1. 安装编译生成的hap包，并打开应用。
2. 分别从界面上操作选择两张图片。
3. 点击开始比对，计算是否为同一人。
4. 相似度大于等于0.6时显示"是同一个人"，否则显示"不是同一个人"。

### 工程目录

```
FaceNetApp/
├── entry/                          # 主模块
│   ├── src/
│   │   ├── main/
│   │   │   ├── ets/                # ArkTS 源代码
│   │   │   │   ├── entryability/   # 应用入口
│   │   │   │   │   └── EntryAbility.ets
│   │   │   │   └── pages/          # 页面组件
│   │   │   │       ├── Index.ets           # 人脸比对页面
│   │   │   │       ├── IndexCamera.ets     # 相机实时识别页面
│   │   │   │       ├── CameraService.ts     # 相机服务
│   │   │   │       └── Logger.ts            # 日志工具
│   │   │   ├── cpp/                # C++ 源代码
│   │   │   │   ├── FaceNet.cpp              # FaceNet 模型实现
│   │   │   │   ├── UltraFace.cpp            # UltraFace 模型实现
│   │   │   │   ├── FaceRecognitionEngine.cpp # 人脸辨识引擎
│   │   │   │   ├── napi_init.cpp            # NAPI 接口初始化
│   │   │   │   ├── utils.cpp                # 工具函数
│   │   │   │   ├── CMakeLists.txt           # CMake 配置
│   │   │   │   └── include/                 # 头文件
│   │   │   │       ├── FaceNet.h
│   │   │   │       ├── UltraFace.h
│   │   │   │       ├── FaceRecognitionEngine.h
│   │   │   │       ├── Common.h
│   │   │   │       └── FaceDetect.h
│   │   │   └── resources/          # 资源文件
│   │   │       └── rawfile/        # 模型文件
│   │   │           ├── version-RFB-640_s8s8.unm  # UltraFace 模型
│   │   │           └── facenet_s8s8.unm          # FaceNet 模型
│   │   └── ohosTest/               # 测试代码
│   └── libs/                       # 第三方库
│       └── arm64-v8a/              # ARM64 架构库
│           ├── libopencv_*.so      # OpenCV 库
│           ├── libuniai.so         # UniAI 库
│           └── unisoc_NPU_backend.so  # 展锐NPU后端
├── oh-package.json5                # 项目依赖配置
└── build-profile.json5             # 构建配置
```

### 具体实现

本示例基于展锐NPU完成推理计算，通过ArkTS（前端层）与C++（核心算法层）协同实现功能。

1. **模型初始化**：在`FaceRecognitionEngine::Init()`中加载UltraFace和FaceNet模型到NPU。
2. **人脸检测**：使用UltraFace模型检测图片中的人脸区域。
3. **特征提取**：使用FaceNet模型提取人脸特征向量（128维）。 
4. **相似度计算**：通过余弦相似度计算两个人脸特征向量的相似度。

#### 主要NAPI接口

| 接口名 | 描述 |
|--------|------|
| `LoadModel(resourceManager: ResourceManager): void`| 初始化模型 |
| `ReleaseModel(): void` |  释放模型 |
| `DetectJpgFace(buffer: ArrayBuffer): FaceEmbeddings` |JPG格式图片中的人脸，返回人脸特征向量及裁剪后的人脸图片 |
| `DetectPixelFace(buffer: ArrayBuffer, width: number, height: number<FaceEmbeddings>` | 检测PixelMap格式图片中的人脸（异步），返回人脸特征向量及裁剪后的人脸图片 |
| `Comparedings(embedding1: number[], embedding2: number[]): number` | 比较两个人脸特征向量的相似度，返回0-1之间的相似度值 |

#### FaceEmbeddings 结构

```typescript
interface FaceEmbeddings {
  value: number[];      // 128维人脸特征向量
  image: {              // 裁剪后的人脸图片
    data: ArrayBuffer;  // 图片数据
    width: number;      // 图片宽度
    height: number;     // 图片高度
  };
}
```

#### 源码参考

源码参考：[main目录](entry/src/main/)下的文件。

### 相关权限

不涉及。

### 依赖

- OpenCV：用于图像处理和格式转换
- UniAI：展锐NPU推理框架
- 展锐NPU后端：提供NPU硬件加速能力

### 约束与限制

1. 本示例仅支持标准系统上运行，支持设备：wukong100、7885开发者手机(7885开发者手机需手动修改NPU设备节点/dev/vha0 权限为 666)

2. 本示例为Stage模型，支持API12版本SDK，SDK版本号(API Version 12 Release)，镜像版本号(5.0 Release)

3. 本示例需要使用DevEco Studio 版本号(6.0.0 Release)及以上版本才可编译运行

4. 需要设备支持展锐NPU硬件加速

5. 模型文件需要放置在`resources/rawfile/`目录下：
   - `version-RFB-640_s8s8.unm`（UltraFace人脸检测模型）
   - `facenet_s8s8.unm`（FaceNet人脸特征提取模型）

### 下载

如需单独下载本工程，执行如下命令：

```
git init
git config core.sparsecheckout true
echo code/AI/FaceNetApp/ > .git/info/sparse-checkout
git remote add origin https://gitcode.com/openharmony/applications_app_samples.git
git pull origin OpenHarmony_standard_p7885_rk3588_d3000m_20251124
```
