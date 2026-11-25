import os
os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
import time
import argparse
import numpy as np
import tensorflow as tf

from tensorflow.keras.datasets import mnist
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Input, Conv2D, MaxPooling2D, Flatten, Dense
from tensorflow.keras.utils import to_categorical

import hls4ml

parser = argparse.ArgumentParser(description="Train and benchmark the LeNet Python reference model.")
parser.add_argument(
    "--hls-compile",
    action="store_true",
    help="Also run hls_model.compile(). On Windows this can fail because hls4ml may call Linux-style generated scripts.",
)
args = parser.parse_args()


# ==========================================
# 阶段 1：准备数据与训练 Keras 模型
# ==========================================
print(">>> Stage 1: prepare MNIST data and train Keras LeNet model...")

# 读取 MNIST 数据集
# x_train: 60000 张 28×28 灰度图
# y_train: 对应标签
# x_test / y_test: 测试集
(x_train, y_train), (x_test, y_test) = mnist.load_data()

# 将输入从 [N, 28, 28] 扩展为 [N, 28, 28, 1]
# 并归一化到 [0, 1]
x_train = np.expand_dims(x_train, axis=-1).astype("float32") / 255.0

# 标签转为 one-hot 编码
y_train = to_categorical(y_train, 10)

# ==========================================
# 构建 LeNet-5 风格 CNN 模型
# ==========================================
model = Sequential()

model.add(Input(shape=(28, 28, 1), name="mnist_input"))

model.add(
    Conv2D(
        6,
        kernel_size=(5, 5),
        activation="relu",
        name="conv1",
    )
)

model.add(
    MaxPooling2D(
        pool_size=(2, 2),
        name="pool1",
    )
)

model.add(
    Conv2D(
        16,
        kernel_size=(5, 5),
        activation="relu",
        name="conv2",
    )
)

model.add(
    MaxPooling2D(
        pool_size=(2, 2),
        name="pool2",
    )
)

model.add(Flatten(name="flatten"))

model.add(
    Dense(
        120,
        activation="relu",
        name="fc1",
    )
)

model.add(
    Dense(
        84,
        activation="relu",
        name="fc2",
    )
)

# 最后一层不加 softmax，输出 10 个 raw score / logits
model.add(
    Dense(
        10,
        name="output",
    )
)

model.compile(
    optimizer="adam",
    loss=tf.keras.losses.CategoricalCrossentropy(from_logits=True),
    metrics=["accuracy"],
)

# 训练 2 个 epoch
model.fit(
    x_train,
    y_train,
    epochs=2,
    batch_size=128,
    validation_split=0.1,
    verbose=2,
)


# ==========================================
# 阶段 1.5：测试 Keras / GPU 单张图片前向推理时间
# ==========================================
print("\n>>> Stage 1.5: benchmark Keras batch=1 single-image inference...")

# 查看 TensorFlow 是否检测到 GPU
gpus = tf.config.list_physical_devices("GPU")
print("TensorFlow detected GPUs:", gpus)

if len(gpus) == 0:
    print("WARNING: TensorFlow did not detect a GPU. This benchmark is running on CPU.")
else:
    print("TensorFlow will use GPU for the batch=1 benchmark.")

# 测试集输入同样需要扩展通道维度并归一化
x_test_for_bench = np.expand_dims(x_test, axis=-1).astype("float32") / 255.0

# 只取一张测试图片，用于 batch=1 单张图片前向推理
x_one = x_test_for_bench[0:1]


@tf.function
def forward_once(x):
    """
    单张图片前向推理函数。

    这里使用 tf.function 包装，减少 Python 解释器开销。
    model(x, training=False) 表示只执行前向传播，不执行训练。
    """
    return model(x, training=False)


# ------------------------------------------------------------
# 1. GPU warmup
# ------------------------------------------------------------
# 第一次运行 TensorFlow/GPU 时，可能包含：
#   1. 图构建
#   2. CUDA kernel 初始化
#   3. cuDNN 算法选择
#   4. PTX JIT 编译
#
# 所以正式测速前要先 warmup，避免第一次运行影响结果。
# ------------------------------------------------------------
warmup = 100

print("Starting batch=1 warmup...")
for _ in range(warmup):
    y = forward_once(x_one)
    _ = y.numpy()  # 强制同步，确保 GPU 计算真正完成


# ------------------------------------------------------------
# 2. batch=1 单张图片正式测速
# ------------------------------------------------------------
# 这里重复执行 1000 次，然后取平均时间。
# 每一次都是 batch=1，即单张图片前向传播。
# ------------------------------------------------------------
repeat = 1000

print("Starting batch=1 timed benchmark...")

start = time.perf_counter()

for _ in range(repeat):
    y = forward_once(x_one)
    _ = y.numpy()  # TensorFlow GPU 默认异步执行，numpy() 用于强制同步

end = time.perf_counter()

# GPU/Keras 单张图片前向推理平均时间，单位 ms
gpu_single_ms = (end - start) * 1000.0 / repeat

# 打印该测试图片的预测结果
score = forward_once(x_one).numpy()
pred = np.argmax(score, axis=1)[0]

print("==========================================")
print("Keras batch=1 single-image inference benchmark")
print("repeat =", repeat)
print("average time = %.6f ms / image" % gpu_single_ms)
print("test label =", y_test[0])
print("pred       =", pred)
print("==========================================")


# ==========================================
# FPGA 与 GPU 单张图片前向推理时间对比
# ==========================================
# 这里使用的是你此前 Vivado/Vitis HLS 报告中的 FPGA 推理延迟：
#
#   myproject_axi latency = 23589 cycles
#   clock period = 10 ns
#
# 因此：
#   23589 cycles × 10 ns = 235890 ns = 0.235890 ms
#
# 注意：
#   这个 FPGA 时间是 HLS 估计的单张图片前向推理延迟，
#   不是训练时间，也不是 PS 端完整端到端时间。
# ==========================================
fpga_cycles = 12580
fpga_clock_ns = 10
fpga_single_ms = fpga_cycles * fpga_clock_ns / 1e6

# FPGA 相对于 GPU batch=1 的加速倍数
speedup = gpu_single_ms / fpga_single_ms

print("==========================================")
print("Single-image forward inference time comparison")
print("FPGA HLS estimated single-image time: %.6f ms" % fpga_single_ms)
print("Keras batch=1 single-image time: %.6f ms" % gpu_single_ms)
print("FPGA vs Keras batch=1 speedup: %.2f x" % speedup)
print("==========================================")


# ==========================================
# 阶段 2：配置 hls4ml 硬件映射参数
# ==========================================
print("\n>>> Stage 2: generate hls4ml/Vitis HLS project sources...")

# 根据 Keras 模型生成 hls4ml 配置
# granularity='name' 表示按层名生成配置，方便单独设置每层参数
config = hls4ml.utils.config_from_keras_model(
    model,
    granularity="name",
)

# Resource 策略偏向资源复用，降低资源占用
config["Model"]["Strategy"] = "Resource"

# 整体定点精度
# ap_fixed<16,6>：
#   总位宽 16 bit
#   整数位 6 bit，包含符号位
#   小数位 10 bit
config["Model"]["Precision"] = "ap_fixed<16,6>"

# ==========================================
# 设置各层 ReuseFactor
# ==========================================
# ReuseFactor 可以理解为乘加资源复用程度：
#   值越大，资源占用通常越低，但速度可能更慢；
#   值越小，并行度更高，但 DSP / LUT 资源占用可能更高。
#
# 这里保留你当前已经综合通过的一组配置。
# ==========================================
config["LayerName"]["conv1"]["ReuseFactor"] = 25
config["LayerName"]["conv2"]["ReuseFactor"] = 75
config["LayerName"]["fc1"]["ReuseFactor"] = 256
config["LayerName"]["fc2"]["ReuseFactor"] = 60

# 给每一层补齐 result 和 accum 的定点精度
for layer in config["LayerName"].keys():
    if "Precision" not in config["LayerName"][layer]:
        config["LayerName"][layer]["Precision"] = {}

    config["LayerName"][layer]["Precision"]["result"] = "ap_fixed<16,6>"
    config["LayerName"][layer]["Precision"]["accum"] = "ap_fixed<16,6>"


# ==========================================
# 阶段 2.5：生成 hls4ml / Vitis HLS 工程
# ==========================================
output_dir = "lenet5_hls_prj"

# 核心修改：使用 Vitis 后端和 io_stream 接口
hls_model = hls4ml.converters.convert_from_keras_model(
    model,
    hls_config=config,
    output_dir=output_dir,
    part="xczu7ev-ffvc1156-2-i",
    clock_period=5,
    io_type="io_stream",
    backend="Vitis",
)

# 编译 hls4ml 模型用于本地 C 仿真检查。Windows 下 hls4ml 可能会调用
# Linux shell 风格的脚本并失败，所以默认只生成工程源码。
if args.hls_compile:
    try:
        hls_model.compile()
    except Exception as exc:
        print("WARNING: hls_model.compile() failed on this host.")
        print("Reason:", exc)
        print("The Keras benchmark above is still valid.")
        print("The hls4ml project sources were still generated in:", output_dir)
        print("Use the checked-in HLS/lenet_accel project for FPGA builds.")
else:
    print("Skipping hls_model.compile(). Add --hls-compile if you want local C simulation.")

print("hls4ml project sources generated at:", output_dir)
print("Use Vitis HLS / Vivado HLS to run build_prj.tcl if you need to rebuild this reference project.")


# ==========================================
# 阶段 3：启动 Vitis HLS 综合
# ==========================================
# 如果服务器上安装了 Vitis HLS / Vivado HLS，可以打开下面代码。
# 当前服务器如果没有 Xilinx 工具，就保持注释，只生成 HLS 工程源码。
# ==========================================

# print("\n>>> 阶段 3：启动 Vitis HLS 综合 (跳过 C 仿真)...")
#
# hls_model.build(
#     csim=False,
#     synth=True,
#     vsynth=False,
# )
#
# print("✅ 综合完成！")
#
# hls4ml.report.read_vivado_report(output_dir)
