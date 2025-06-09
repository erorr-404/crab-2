const char *page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta
      name="viewport"
      content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no"
    />
    <title>CRAB-2 Control Panel</title>
  </head>
  <body>
    <div class="panel">
      <div class="joy" id="left-joy">
        <div class="joy-ball" id="left-joy-ball"></div>
      </div>
      <div class="button-array">
          <div id="siren-button">📢</div>
        </div>
    </div>
    <div class="panel" id="center-panel">
      <img id="stream" src="/stream/image.png" />
    </div>
    <div class="panel" id="right-panel">
      <div class="joy" id="right-joy">
        <div class="joy-ball" id="right-joy-ball"></div>
      </div>
      <div class="button-array">
        <div id="light-button" class="">🔦</div>
        <div id="blinker-button" class="">🚨</div>
      </div>
    </div>
    <style>
      body {
        margin: 0;
        padding: 0;
        height: 100vh;
        overflow: hidden;
        display: grid;
        grid-template-columns: 1fr 2fr 1fr;
        grid-template-rows: 1fr;
        background-color: #26333b;
      }

      .panel {
        display: flex;
        flex-direction: column;
        justify-content: center;
        justify-content: flex-start;
        align-items: center;
        gap: 20px;
        padding-top: 20px;
      }

      .button-array {
        display: flex;
        flex-direction: row;
        justify-content: center;
        align-items: center;
        gap: 10px;
      }

      .button-array div {
        color: #fff;
        height: 50px;
        padding: 2px 10px;
        background-color: #1a232b;
        border-radius: 5px;
        display: flex;
        justify-content: center;
        align-items: center;
        font-size: 24px;
        fill: #ffffff;
        cursor: pointer;
        background-image: linear-gradient(
          to top,
          #1a232b 0%,
          rgb(35, 47, 58) 100%
        );
        box-shadow: 0 0 10px rgba(0, 0, 0, 0.7);
        border: #12181b solid 2px;
        transition: all 1s ease;
      }

      .button-array div.active {
        fill: #6cc9ff;
        cursor: pointer;
        background-image: linear-gradient(to top, #161e22 0%, #1f2a33 100%);
        box-shadow: inset 0 0 10px rgba(0, 0, 0, 0.7);
        transition: all 1s ease;
      }

      .joy {
        width: 175px;
        height: 175px;
        background-color: #1a232b;
        background-size: cover;
        align-self: center;
        justify-self: center;
        position: relative;
        box-shadow: inset 0 0 10px rgba(0, 0, 0, 0.7);
        border-radius: 50%;
      }

      .joy-ball {
        width: 50px;
        height: 50px;
        background: radial-gradient(circle, #3c5060, #243138);
        border-radius: 50%;
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        pointer-events: none;
        box-shadow: 0 0 10px rgba(0, 0, 0, 0.7);
      }

      .joy-ball::after {
        content: "";
        width: 10px;
        height: 10px;
        background-color: white;
        border-radius: 50%;
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
      }

      #left-joy {
        grid-column: 1 / 2;
      }

      #right-panel {
        grid-column: 3 / 4;
      }

      #stream {
        grid-column: 2 / 3;
        width: calc(100% - 4px);
        max-height: 90vh;
        object-fit: fill;
        border-radius: 10px;
        border: #12181b solid 2px;
        box-shadow: inset 0 0 10px rgba(0, 0, 0, 0.7);
        padding: 5px;
      }
    </style>
    <script>
      setInterval(() => {
        document.getElementById("stream").src =
          "/stream?" + new Date().getTime();
      }, 100);
      const leftJoy = document.getElementById("left-joy");
      const rightJoy = document.getElementById("right-joy");
      const leftJoyBall = document.getElementById("left-joy-ball");
      const rightJoyBall = document.getElementById("right-joy-ball");

      const lightButton = document.getElementById("light-button");
      const blinkerButton = document.getElementById("blinker-button");
      const sirenButton = document.getElementById("siren-button");
      const lightButtonState = { state: false };
      const blinkerButtonState = { state: false };
      const sirenButtonState = { state: false };

      lightButton.addEventListener("click", () => {
        lightButtonState.state = !lightButtonState.state;
        lightButton.classList.toggle("active", lightButtonState.state);
        const data = JSON.stringify({
          id: "!light",
          state: lightButtonState.state,
        });
        if (ws.readyState === WebSocket.OPEN) {
          ws.send(data);
          console.log("Light button state:", lightButtonState.state);
        }
      });

      blinkerButton.addEventListener("click", () => {
        blinkerButtonState.state = !blinkerButtonState.state;
        blinkerButton.classList.toggle("active", blinkerButtonState.state);
        const data = JSON.stringify({
          id: "!blinker",
          state: blinkerButtonState.state,
        });
        if (ws.readyState === WebSocket.OPEN) {
          ws.send(data);
          console.log("Blinker button state:", blinkerButtonState.state);
          
        }
      });

      sirenButton.addEventListener("click", () => {
        sirenButtonState.state = !sirenButtonState.state;
        sirenButton.classList.toggle("active", sirenButtonState.state);
        const data = JSON.stringify({
          id: "!siren",
          state: sirenButtonState.state,
        });
        if (ws.readyState === WebSocket.OPEN) {
          ws.send(data);
          console.log("Siren button state:", sirenButtonState.state);
        }
      });

      const ws = new WebSocket("ws://" + location.host + "/ws");
      ws.onclose = () => setTimeout(() => location.reload(), 3000); // Перепідключення при розриві

      function sendJoystickData(joystick, x, y) {
        if (ws.readyState === WebSocket.OPEN) {
          const data = JSON.stringify({ id: joystick.id, x, y });
          ws.send(data);
        }
      }

      function moveJoyBall(event, joystick, ball) {
        const rect = joystick.getBoundingClientRect();
        const touch = event.touches ? event.touches[0] : event;
        const x = touch.clientX - rect.left;
        const y = touch.clientY - rect.top;
        const radius = joystick.offsetWidth / 2;
        const ballRadius = ball.offsetWidth / 2;
        const dx = x - radius,
          dy = y - radius;
        const distance = Math.sqrt(dx * dx + dy * dy);

        if (distance > radius - ballRadius) {
          const angle = Math.atan2(dy, dx);
          ball.style.left = `${
            radius + (radius - ballRadius) * Math.cos(angle)
          }px`;
          ball.style.top = `${
            radius + (radius - ballRadius) * Math.sin(angle)
          }px`;
        } else {
          ball.style.left = `${x}px`;
          ball.style.top = `${y}px`;
        }

        const limitedX = Math.max(
          -100,
          Math.min(
            100,
            Math.round(((x - radius) / (radius - ballRadius)) * 100)
          )
        );
        const limitedY = -Math.max(
          -100,
          Math.min(
            100,
            Math.round(((y - radius) / (radius - ballRadius)) * 100)
          )
        );

        sendJoystickData(joystick, limitedX, limitedY);
      }

      function resetJoyBall(ball, joystick) {
        ball.style.left = "50%";
        ball.style.top = "50%";
        sendJoystickData(joystick, 0, 0);
      }

      [
        [leftJoy, leftJoyBall],
        [rightJoy, rightJoyBall],
      ].forEach(([joystick, ball]) => {
        joystick.addEventListener("mousedown", (event) => {
          moveJoyBall(event, joystick, ball);
          document.addEventListener("mousemove", moveHandler);
        });

        joystick.addEventListener("mouseup", () => {
          resetJoyBall(ball, joystick);
          document.removeEventListener("mousemove", moveHandler);
        });

        joystick.addEventListener("touchstart", (event) => {
          moveJoyBall(event, joystick, ball);
          document.addEventListener("touchmove", moveHandler);
        });

        joystick.addEventListener("touchend", () => {
          resetJoyBall(ball, joystick);
          document.removeEventListener("touchmove", moveHandler);
        });

        function moveHandler(event) {
          moveJoyBall(event, joystick, ball);
        }
      });
    </script>
  </body>
</html>
)rawliteral";