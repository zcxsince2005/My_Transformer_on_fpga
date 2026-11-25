from PIL import Image
import numpy as np
import os

# ============================================================
# 配置区
# ============================================================
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
IMG_DIR = os.path.join(SCRIPT_DIR, "test_picture")
OUT_HEADER = os.path.join(SCRIPT_DIR, "mnist_test_imgs.h")

# 按文件名 0.png ~ 9.png 的顺序处理
IMAGE_FILES = [f"{i}.png" for i in range(10)]
LABELS = list(range(10))

# ap_fixed<16,6> 近似理解为 10 位小数
FRAC_BITS = 10
SCALE = 1 << FRAC_BITS


# ============================================================
# 工具函数：把图片处理成类似 MNIST 的 28x28 输入
# ============================================================
def preprocess_to_mnist_like(img_path: str) -> np.ndarray:
    """
    输入：任意一张黑字白底数字图
    输出：28x28 的 float32 数组，范围 0~1，风格尽量接近 MNIST（白字黑底）
    """
    # 1) 灰度读入
    img = Image.open(img_path).convert("L")
    arr = np.array(img)

    # 2) 提取前景（黑色笔画）
    #    这里假设背景偏白，笔画偏黑
    mask = arr < 200

    # 如果没找到前景，直接返回全零
    if not np.any(mask):
        return np.zeros((28, 28), dtype=np.float32)

    # 3) 裁剪出数字最小包围盒
    ys, xs = np.where(mask)
    y0, y1 = ys.min(), ys.max()
    x0, x1 = xs.min(), xs.max()
    digit = arr[y0:y1 + 1, x0:x1 + 1]

    # 4) 反相：黑字白底 -> 白字黑底
    digit = 255 - digit

    # 5) 弱背景压成黑色
    digit = np.where(digit > 30, digit, 0).astype(np.uint8)

    digit_img = Image.fromarray(digit, mode="L")

    # 6) 等比例缩放，最长边压到 20
    w, h = digit_img.size
    if w > h:
        new_w = 20
        new_h = max(1, int(round(h * 20 / w)))
    else:
        new_h = 20
        new_w = max(1, int(round(w * 20 / h)))

    digit_img = digit_img.resize((new_w, new_h), Image.Resampling.LANCZOS)

    # 7) 居中放到 28x28 画布
    canvas = Image.new("L", (28, 28), 0)
    paste_x = (28 - new_w) // 2
    paste_y = (28 - new_h) // 2
    canvas.paste(digit_img, (paste_x, paste_y))

    # 8) 归一化到 0~1
    out = np.array(canvas).astype(np.float32) / 255.0
    return out


# ============================================================
# 工具函数：float -> ap_fixed<16,6> 对应的 16bit 原始值
# ============================================================
def float_to_apfixed_16_6(x: float) -> int:
    """
    把 0~1 的浮点数转成 ap_fixed<16,6> 的原始整数表示。
    这里按 10 位小数近似：round(x * 2^10)
    """
    v = int(round(float(x) * SCALE))

    # 这里输入理论上不为负，也不会特别大
    if v < 0:
        v = 0
    if v > 0xFFFF:
        v = 0xFFFF

    return v


# ============================================================
# 工具函数：28x28 -> 392个u32
# ============================================================
def pack_28x28_to_u32(img28: np.ndarray) -> list[int]:
    """
    把 28x28 的 float 图像打平成 784 个 16bit，
    再两两打包成 392 个 32bit：
      low16  = pixel[i]
      high16 = pixel[i+1]
    """
    flat = img28.flatten()
    vals16 = [float_to_apfixed_16_6(v) for v in flat]

    packed = []
    for i in range(0, 784, 2):
        low = vals16[i] & 0xFFFF
        high = vals16[i + 1] & 0xFFFF
        packed.append((high << 16) | low)

    return packed


# ============================================================
# 主流程
# ============================================================
def main():
    all_imgs = []

    for fname in IMAGE_FILES:
        path = os.path.join(IMG_DIR, fname)
        if not os.path.exists(path):
            raise FileNotFoundError(f"找不到图片文件: {path}")

        img28 = preprocess_to_mnist_like(path)
        packed = pack_28x28_to_u32(img28)
        all_imgs.append(packed)

    with open(OUT_HEADER, "w", encoding="utf-8") as f:
        f.write("#ifndef MNIST_TEST_IMGS_H\n")
        f.write("#define MNIST_TEST_IMGS_H\n\n")
        f.write('#include "xil_types.h"\n\n')

        f.write("static const int test_labels[10] = {")
        f.write(",".join(str(x) for x in LABELS))
        f.write("};\n\n")

        f.write("static const u32 test_imgs[10][392] = {\n")
        for i, packed in enumerate(all_imgs):
            f.write("    {\n        ")
            for j, word in enumerate(packed):
                f.write(f"0x{word:08X}")
                if j != len(packed) - 1:
                    f.write(", ")
                if (j + 1) % 8 == 0 and j != len(packed) - 1:
                    f.write("\n        ")
            f.write("\n    }")
            if i != len(all_imgs) - 1:
                f.write(",")
            f.write("\n")
        f.write("};\n\n#endif\n")

    print(f"已生成头文件: {OUT_HEADER}")


if __name__ == "__main__":
    main()
