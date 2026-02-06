const express = require("express");
const app = express();
const PORT = 3000;

app.use(express.json());

// ===== 자동차 제어 상태 =====
let controlState = {
  speed: 0,
  steering: 0,
  speed1: 0,
  speed2: 0,
  off: 0,
  auto: 0,
  water: 0,
};

// ===== ESP32 연결 상태 확인용 변수 =====
let lastEspAccess = 0;

// ===== 프론트 → 자동차 제어 =====
app.post("/api/control", (req, res) => {
  const body = req.body;

  if (body.speed !== undefined) {
    controlState.speed = Math.max(-100, Math.min(100, body.speed));
  }
  if (body.steering !== undefined) {
    controlState.steering = Math.max(-100, Math.min(100, body.steering));
  }

  if (body.speed1 !== undefined) controlState.speed1 = body.speed1;
  if (body.speed2 !== undefined) controlState.speed2 = body.speed2;
  if (body.off !== undefined) controlState.off = body.off;
  if (body.auto !== undefined) controlState.auto = body.auto;
  if (body.water !== undefined) controlState.water = body.water;

  res.sendStatus(200);
});

// ===== ESP32가 가져가는 데이터 (Heartbeat 역할) =====
app.get("/api/control", (req, res) => {
  lastEspAccess = Date.now();
  res.json(controlState);
});

// ===== 프론트엔드용 연결 상태 확인 API =====
app.get("/api/status", (req, res) => {
  const now = Date.now();
  const diff = now - lastEspAccess;
  const isConnected = diff < 2000;
  res.json({ connected: isConnected, diff: diff });
});

// ===== 프론트엔드 HTML (UI + Simulation + Status) =====
app.get("/", (req, res) => {
  res.send(`
<!DOCTYPE html>
<html>
<head>
  <title>RC Ultimate Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700&display=swap" rel="stylesheet">
  <style>
    /* ===== 전체 레이아웃 ===== */
    body {
      background-color: #121212;
      color: #e0e0e0;
      font-family: 'Orbitron', sans-serif;
      margin: 0; padding: 0;
      user-select: none; overflow: hidden;
      height: 100vh; display: flex; flex-direction: column;
    }

    .header {
      padding: 10px 0;
      background: #0a0a0a;
      border-bottom: 1px solid #333;
      display: flex; flex-direction: column; align-items: center; justify-content: center;
      z-index: 100;
    }

    h2 {
      color: #00d2ff; text-shadow: 0 0 10px rgba(0, 210, 255, 0.7);
      margin: 0; font-size: 20px;
    }

    /* 연결 상태 표시기 */
    .connection-indicator {
      margin-top: 5px; font-size: 12px; padding: 4px 12px;
      border-radius: 12px; background: #111; border: 1px solid #333;
      display: flex; align-items: center; gap: 8px;
    }
    .status-dot { width: 8px; height: 8px; border-radius: 50%; background-color: #555; transition: all 0.3s; }
    .status-text { color: #888; font-weight: bold; transition: all 0.3s; }

    .status-online .status-dot { background-color: #00ff00; box-shadow: 0 0 10px #00ff00; }
    .status-online .status-text { color: #00ff00; }
    .status-offline .status-dot { background-color: #ff0000; box-shadow: 0 0 10px #ff0000; }
    .status-offline .status-text { color: #ff0000; }

    /* 메인 컨테이너 */
    .main-container { display: flex; flex: 1; height: 100%; overflow: hidden; }

    /* 좌측 패널 (컨트롤러) */
    .left-panel {
      flex: 1; display: flex; flex-direction: column;
      align-items: center; justify-content: center;
      border-right: 2px solid #333; padding: 10px; position: relative; z-index: 50; background: #121212;
    }

    /* 우측 패널 (시뮬레이션) */
    .right-panel {
      flex: 1; background: #050505; position: relative; overflow: hidden;
      display: flex; justify-content: center; align-items: center;
      perspective: 800px; /* 원근감 강화 */
    }

    @media (max-width: 768px) {
      .main-container { flex-direction: column-reverse; }
      .left-panel { border-right: none; border-top: 2px solid #333; flex: 1.2; }
      .right-panel { flex: 0.8; }
    }

    /* 계기판 */
    .dashboard-panel { display: flex; gap: 15px; margin-bottom: 20px; }
    .gauge-box {
      background: #1e1e1e; border: 1px solid #333; border-radius: 8px;
      padding: 8px; width: 80px; text-align: center;
    }
    .gauge-label { font-size: 10px; color: #888; }
    .gauge-value { font-size: 20px; font-weight: bold; color: #fff; }
    .val-active { color: #00ff9d; text-shadow: 0 0 10px #00ff9d; }

    /* 조이스틱 */
    #joystick-zone {
      width: 180px; height: 180px;
      background: radial-gradient(circle at center, #2a2a2a 0%, #111 100%);
      border: 3px solid #444; border-radius: 50%;
      position: relative; margin-bottom: 20px;
      touch-action: none;
    }
    #stick {
      width: 60px; height: 60px;
      background: radial-gradient(circle at 30% 30%, #555, #000);
      border: 2px solid #666; border-radius: 50%;
      position: absolute; top: 60px; left: 60px;
    }
    #stick::after {
      content: ''; width: 15px; height: 15px; background: #00d2ff;
      border-radius: 50%; position: absolute; top: 20px; left: 20px;
      box-shadow: 0 0 8px #00d2ff; opacity: 0.8;
    }

    /* 버튼 */
    .btn-container { display: flex; flex-wrap: wrap; justify-content: center; gap: 8px; max-width: 350px; }
    .toggle-btn {
      width: 70px; height: 45px; border: 1px solid #444; border-radius: 6px;
      font-family: 'Orbitron', sans-serif; font-size: 11px; font-weight: bold;
      color: #aaa; background: linear-gradient(180deg, #333, #222); cursor: pointer;
    }
    .toggle-btn:active { transform: translateY(2px); }
    .active { border-color: #00d2ff; color: #fff; background: linear-gradient(180deg, #005f73, #003344); }
    .active-red { border-color: #ff3333; color: #fff; background: linear-gradient(180deg, #800000, #440000); }
    .active-water { border-color: #4d4dff; color: #fff; background: linear-gradient(180deg, #000080, #000044); }

    /* ===== ★ 개선된 시뮬레이션 도로 ★ ===== */
    .sim-road {
      position: absolute; width: 200%; height: 200%; 
      background-color: #1a1a1a;
      
      /* 중앙 점선과 양옆 차선 */
      background-image: 
        /* 중앙 점선 (노란색) */
        linear-gradient(180deg, transparent 50%, #cca300 50%),
        /* 좌측 실선 (흰색) */
        linear-gradient(90deg, transparent 0%, transparent 29%, #555 29%, #fff 30%, #fff 32%, #555 33%, transparent 33%),
        /* 우측 실선 (흰색) */
        linear-gradient(90deg, transparent 0%, transparent 67%, #555 67%, #fff 68%, #fff 70%, #555 71%, transparent 71%);
      
      background-size: 20px 120px, 100% 100%, 100% 100%; /* 점선의 길이 조정 */
      background-position: center top;
      background-repeat: repeat-y, no-repeat, no-repeat;
      
      /* 원근감 효과: 3D 회전 */
      transform: rotateX(60deg) translateY(-20%);
      transform-origin: 50% 50%;
      top: -30%; left: -50%;
      box-shadow: inset 0 0 100px #000;
    }

    .sim-car {
      width: 80px; height: 140px; background: #222; border-radius: 10px;
      position: absolute; box-shadow: 0 20px 40px rgba(0,0,0,0.8); z-index: 10;
      transition: transform 0.1s;
    }
    .sim-car::after { content: ''; position: absolute; top: 30px; left: 10px; right: 10px; bottom: 30px; background: #111; border: 1px solid #444; border-radius: 5px; }
    
    .wheel { width: 14px; height: 28px; background: #000; border: 1px solid #333; position: absolute; border-radius: 4px; }
    .fl { top: 15px; left: -14px; transition: transform 0.1s; } .fr { top: 15px; right: -14px; transition: transform 0.1s; }
    .rl { bottom: 15px; left: -14px; } .rr { bottom: 15px; right: -14px; }

    .headlight { width: 20px; height: 10px; background: #555; position: absolute; top: -5px; border-radius: 5px 5px 0 0; }
    .hl-left { left: 5px; } .hl-right { right: 5px; }
    
    /* 헤드라이트 빔 효과 강화 */
    .beam { 
      position: absolute; top: -200px; width: 60px; height: 200px; 
      background: linear-gradient(to top, rgba(255,255,255,0), rgba(200,255,255,0.1) 40%, rgba(255,255,255,0.4)); 
      opacity: 0; transition: opacity 0.2s; pointer-events: none; mix-blend-mode: screen;
    }
    .b-left { left: -10px; transform: rotate(-8deg); transform-origin: bottom center; } 
    .b-right { right: -10px; transform: rotate(8deg); transform-origin: bottom center; }

    .taillight { width: 20px; height: 5px; background: #300; position: absolute; bottom: 0; box-shadow: 0 0 5px #500; transition: all 0.2s; }
    .tl-left { left: 5px; } .tl-right { right: 5px; }

    .lights-on .headlight { background: #fff; box-shadow: 0 0 20px #fff; } .lights-on .beam { opacity: 1; }
    .brake-on .taillight { background: #ff0000; box-shadow: 0 0 25px #ff0000; }
  </style>
</head>
<body>

  <div class="header">
    <h2>RC COMMAND CENTER</h2>
    <div id="connection-status" class="connection-indicator status-offline">
      <div class="status-dot"></div>
      <div class="status-text" id="status-text">ESP32: OFFLINE</div>
    </div>
  </div>

  <div class="main-container">
    <div class="left-panel">
      <div class="dashboard-panel">
        <div class="gauge-box"><div class="gauge-label">SPEED</div><div id="disp-speed" class="gauge-value">0</div></div>
        <div class="gauge-box"><div class="gauge-label">STEER</div><div id="disp-steer" class="gauge-value">0</div></div>
      </div>
      <div id="joystick-zone"><div id="stick"></div></div>
      <div class="btn-container">
        <button id="btn-speed1" class="toggle-btn" onclick="toggleBtn('speed1')">ACTIVE 1</button>
        <button id="btn-speed2" class="toggle-btn" onclick="toggleBtn('speed2')">ACTIVE 2</button>
        <button id="btn-auto"   class="toggle-btn" onclick="toggleBtn('auto')">SMART MODE</button>
        <button id="btn-off"    class="toggle-btn" onclick="toggleBtn('off')" style="color:#ff8888">STOP</button>
        <div style="flex-basis: 100%; height: 0;"></div>
        <button id="btn-water"  class="toggle-btn" onclick="toggleBtn('water')" style="width: 120px;">WASHER</button>
      </div>
    </div>

    <div class="right-panel">
      <div id="sim-road" class="sim-road"></div>
      <div id="sim-car" class="sim-car brake-on">
        <div class="wheel fl" id="wheel-fl"></div> <div class="wheel fr" id="wheel-fr"></div>
        <div class="wheel rl"></div> <div class="wheel rr"></div>
        <div class="headlight hl-left"><div class="beam b-left"></div></div>
        <div class="headlight hl-right"><div class="beam b-right"></div></div>
        <div class="taillight tl-left"></div> <div class="taillight tl-right"></div>
      </div>
    </div>
  </div>

  <script>
    // ===== 상태 변수 =====
    let joyState = { speed: 0, steering: 0 };
    let btnState = { speed1: 0, speed2: 0, off: 0, auto: 0, water: 0 };
    const exclusiveGroup = ['speed1', 'speed2', 'auto', 'off'];

    function toggleBtn(key) {
      if (key === 'water') {
        btnState.water = btnState.water === 0 ? 1 : 0;
      } else {
        const wasOn = btnState[key] === 1;
        exclusiveGroup.forEach(k => btnState[k] = 0);
        if (!wasOn) btnState[key] = 1;
      }
      updateButtons();
      sendControl();
    }

    function updateButtons() {
      ['speed1', 'speed2', 'auto', 'off', 'water'].forEach(key => {
        const btn = document.getElementById('btn-' + key);
        btn.classList.remove('active', 'active-red', 'active-water');
        if (btnState[key] === 1) {
          if (key === 'off') btn.classList.add('active-red');
          else if (key === 'water') btn.classList.add('active-water');
          else btn.classList.add('active');
        }
      });
    }

    function sendControl() {
      fetch('/api/control', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ...joyState, ...btnState })
      }).catch(e => console.error(e));
    }

    // ===== 연결 상태 확인 =====
    const statusDiv = document.getElementById("connection-status");
    const statusText = document.getElementById("status-text");

    setInterval(() => {
      fetch('/api/status')
        .then(res => res.json())
        .then(data => {
          if (data.connected) {
            statusDiv.className = "connection-indicator status-online";
            statusText.innerText = "ESP32: ONLINE";
          } else {
            statusDiv.className = "connection-indicator status-offline";
            statusText.innerText = "ESP32: OFFLINE";
          }
        })
        .catch(err => {
          statusDiv.className = "connection-indicator status-offline";
          statusText.innerText = "SERVER ERROR";
        });
    }, 1000);

    // ===== 조이스틱 및 시뮬레이션 =====
    const zone = document.getElementById("joystick-zone");
    const stick = document.getElementById("stick");
    const dispSpeed = document.getElementById("disp-speed");
    const dispSteer = document.getElementById("disp-steer");
    const simRoad = document.getElementById("sim-road");
    const simCar = document.getElementById("sim-car");
    const wheelFL = document.getElementById("wheel-fl");
    const wheelFR = document.getElementById("wheel-fr");

    const maxDist = 60; 
    let dragging = false;

    zone.addEventListener("mousedown", start);
    zone.addEventListener("touchstart", start, {passive: false});
    document.addEventListener("mouseup", end);
    document.addEventListener("touchend", end);
    document.addEventListener("mousemove", move);
    document.addEventListener("touchmove", move, {passive: false});

    function start(e) { dragging = true; stick.style.transition = 'none'; if(e.type==='touchstart') e.preventDefault(); }
    
    function end() {
      if(!dragging) return;
      dragging = false;
      stick.style.transition = '0.2s'; stick.style.top = "60px"; stick.style.left = "60px";
      joyState.speed = 0; joyState.steering = 0;
      updateUI(); sendControl();
    }

    function move(e) {
      if(!dragging) return;
      if(e.type==='touchmove') e.preventDefault();
      
      const rect = zone.getBoundingClientRect();
      const cx = e.touches ? e.touches[0].clientX : e.clientX;
      const cy = e.touches ? e.touches[0].clientY : e.clientY;
      
      let dx = cx - (rect.left + 90); let dy = cy - (rect.top + 90);
      const dist = Math.sqrt(dx*dx + dy*dy);
      
      if(dist > maxDist) {
        const ang = Math.atan2(dy, dx);
        dx = Math.cos(ang) * maxDist; dy = Math.sin(ang) * maxDist;
      }
      
      stick.style.left = (60 + dx) + "px"; stick.style.top = (60 + dy) + "px";
      joyState.speed = Math.round(-(dy/maxDist)*100);
      joyState.steering = Math.round((dx/maxDist)*100);
      updateUI(); sendControl();
    }

    function updateUI() {
      dispSpeed.innerText = joyState.speed;
      dispSteer.innerText = joyState.steering;
      dispSpeed.className = joyState.speed !== 0 ? 'gauge-value val-active' : 'gauge-value';
    }

    // ===== ★ 시뮬레이션 애니메이션 루프 (수정됨) ★ =====
    let roadPositionY = 0;
    
    function animateLoop() {
      const speed = joyState.speed;
      const steer = joyState.steering;

      // 1. 도로 이동 속도 대폭 증가 (0.3 -> 3.0)
      // 속도가 양수면 도로가 아래로(+), 음수면 위로(-) 흘러야 전진/후진 느낌이 남
      roadPositionY += speed * 3.0; 
      simRoad.style.backgroundPositionY = roadPositionY + "px";

      // 2. 바퀴 회전
      const wheelAngle = steer * 0.4; 
      wheelFL.style.transform = \`rotate(\${wheelAngle}deg)\`;
      wheelFR.style.transform = \`rotate(\${wheelAngle}deg)\`;

      // 3. 차체 움직임 및 떨림(Vibration) 효과
      const carRot = steer * 0.15; 
      const carX = steer * 0.3;
      
      // 속도가 빠를 때 차를 살짝 떨게 만듦 (랜덤 1px ~ 2px)
      let shakeX = 0;
      let shakeY = 0;
      if (Math.abs(speed) > 30) {
         shakeX = (Math.random() - 0.5) * 2;
         shakeY = (Math.random() - 0.5) * 2;
      }

      simCar.style.transform = \`translateX(\${carX + shakeX}px) translateY(\${shakeY}px) rotate(\${carRot}deg)\`;

      // 4. 라이트 효과
      if (speed > 5) {
        simCar.classList.add('lights-on'); simCar.classList.remove('brake-on');
      } else if (speed < -5) {
        // 후진 시 브레이크등 끔 (흰색 후진등이 없으니 라이트만 끄기)
        simCar.classList.remove('lights-on'); simCar.classList.remove('brake-on'); 
      } else {
        // 정지 시 브레이크등
        simCar.classList.remove('lights-on'); simCar.classList.add('brake-on');
      }
      
      requestAnimationFrame(animateLoop);
    }
    animateLoop();

  </script>
</body>
</html>
  `);
});

app.listen(PORT, () => {
  console.log("Server running on http://localhost:" + PORT);
});
