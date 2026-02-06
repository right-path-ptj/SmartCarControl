import cv2
import torch
from torchvision import models, transforms
import torch.nn.functional as F
from PIL import Image
from picamera2 import Picamera2
import time
import serial

# ===============================
# FPS 설정
# ===============================
CAMERA_FPS = 15
TARGET_FPS = 15
FRAME_TIME = 1.0 / TARGET_FPS

# ===============================
# device 설정
# ===============================
device = torch.device("cpu")
print("Model loading...")

# ===============================
# 모델 로드
# ===============================
model = models.mobilenet_v3_small(weights=None)
num_ftrs = model.classifier[3].in_features
model.classifier[3] = torch.nn.Linear(num_ftrs, 3)

model.load_state_dict(
    torch.load(
        "/home/rwam/Desktop/rain_model3C_scripted.pth",
        map_location=device
    )
)
model.eval()
print("Model load completed!")

# ===============================
# 전처리
# ===============================
preprocess = transforms.Compose([
    transforms.Resize((224, 224)),
    transforms.ToTensor(),
    transforms.Normalize(
        [0.485, 0.456, 0.406],
        [0.229, 0.224, 0.225]
    )
])

# ===============================
# 카메라 설정 (FPS 제한)
# ===============================
picam2 = Picamera2()
video_config = picam2.create_preview_configuration(
    main={"format": "XBGR8888", "size": (640, 480)},
    controls={"FrameRate": CAMERA_FPS}
)
picam2.configure(video_config)
picam2.start()

# ===============================
# UART 설정
# ===============================
uart = serial.Serial(
    port="/dev/ttyAMA0",
    baudrate=9600,
    timeout=1
)
time.sleep(2)

# ===============================
# 클래스 정보
# ===============================
class_names = ['clear', 'light_rain', 'heavy_rain']
class_colors = {
    'clear': (0, 255, 0),
    'light_rain': (0, 255, 255),
    'heavy_rain': (0, 0, 255)
}

# ===============================
# 상태 변수
# ===============================
last_inference_time = 0.0
class_idx = 0
probability = 0.0

print("Start main loop")

# ===============================
# 메인 루프
# ===============================
while True:
    loop_start = time.time()

    # ----- 프레임 캡처 -----
    frame = picam2.capture_array()

    # 180도 회전
    frame = cv2.rotate(frame, cv2.ROTATE_180)

    # BGRA → RGB
    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGRA2RGB)

    # PIL 변환
    pil_image = Image.fromarray(frame_rgb)

    current_time = time.time()

    # ===============================
    # 1초마다 딥러닝 추론
    # ===============================
    if current_time - last_inference_time >= 1.0:
        # 추론용 이미지는 다운샘플
        infer_image = pil_image.resize((224, 224))

        input_tensor = preprocess(infer_image).unsqueeze(0).to(device)

        with torch.no_grad():
            outputs = model(input_tensor)
            probs = F.softmax(outputs, dim=1)
            top_p, top_class = probs.topk(1, dim=1)

            probability = top_p.item()
            class_idx = top_class.item()

        print(
            f"STATE={class_idx} | "
            f"{class_names[class_idx]} "
            f"({probability*100:.1f}%)"
        )

        # UART 전송
        uart.write(str(class_idx).encode())

        last_inference_time = current_time

    # ===============================
    # 화면 표시 (이전 추론 결과 유지)
    # ===============================
    class_name = class_names[class_idx]
    label_text = f"{class_name} ({probability*100:.1f}%)"
    color = class_colors[class_name]

    cv2.putText(
        frame_rgb,
        label_text,
        (20, 50),
        cv2.FONT_HERSHEY_SIMPLEX,
        1,
        color,
        2
    )

    cv2.imshow("Rain Detector (3-Class)", frame_rgb)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

    # ===============================
    # FPS 제한 (루프 안정화)
    # ===============================
    elapsed = time.time() - loop_start
    sleep_time = FRAME_TIME - elapsed
    if sleep_time > 0:
        time.sleep(sleep_time)

# ===============================
# 종료 처리
# ===============================
cv2.destroyAllWindows()
picam2.stop()
uart.close()
