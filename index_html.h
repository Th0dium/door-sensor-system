const char index_html_stamode[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Maker Nhatduy</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { padding-bottom: 70px; background: #f5f5f5; font-family: Arial, sans-serif; }
        .bottom-nav { position: fixed; bottom: 0; width: 100%; z-index: 1000; background: #fff; box-shadow: 0 -2px 5px rgba(0,0,0,0.1); }
        .nav-link { color: #333; font-weight: 500; }
        .nav-link.active { background: #007bff; color: white !important; }
        .topHeader { display: flex; align-items: center; justify-content: center; background: linear-gradient(to right, #f1ebea, #ebe4ce); color: #dd1111; padding: 10px; border-radius: 0; margin-bottom: 20px; }
        .topHeader-logo { height: 25px; margin-right: 10px; }
        .topHeader-title { font-size: 1rem; font-weight: bold; }
        footer { text-align: center; padding: 20px 0; margin-top: 40px; border-top: 1px solid #ccc; color: #777; }
        .control-panel { margin-bottom: 30px; }
        .form-range { width: 100%; }
        .timer-display { max-width: 300px; }
        .control-panel-container { display: flex; align-items: center; justify-content: space-between; }
        .control-panel-content { flex-grow: 1; }
        .side-image { height: 100px; margin-left: 20px; }
        .card { border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        .table { margin-bottom: 0; }
        .table th, .table td { text-align: center; }
        canvas { max-width: 100%; }
    </style>
</head>
<body>
    <nav class="nav nav-pills nav-justified bottom-nav bg-light border-top">
        <a class="nav-link active" href="#tab1" data-bs-toggle="tab">Cấu Hình</a>
        <a class="nav-link" href="#tab2" data-bs-toggle="tab">Lịch Sử</a>
        <a class="nav-link" href="#tab3" data-bs-toggle="tab">Email</a>
    </nav>

    <div class="tab-content container mt-3">
        <div class="tab-pane fade show active" id="tab1">
            <div class="container my-5">
                <div class="topHeader">
                    <img src="https://pos.nvncdn.com/f2fe44-24897/store/20180126_gVLn1I1Irv2dz2XjhYDIshMM.png" alt="Logo" class="topHeader-logo">
                    <div class="topHeader-title">Hệ Thống Chống Trộm</div>
                </div>
                <div class="card control-panel mb-4">
                    <div class="card-body control-panel-container">
                        <div class="control-panel-content">
                            <div class="mb-3">
                                <label class="form-label">Cảnh Báo</label>
                                <div class="form-check form-switch d-flex align-items-center">
                                    <input class="form-check-input" type="checkbox" id="alertModeSwitch" style="width: 3em; height: 1.5em;">
                                    <label class="form-check-label ms-3" for="alertModeSwitch" id="alertModeLabel">Tắt</label>
                                </div>
                            </div>
                        </div>
                        <img src="https://media.istockphoto.com/id/1301407091/vi/vec-to/t%C3%AAn-tr%E1%BB%99m-%C4%91%E1%BB%99t-nh%E1%BA%ADp-v%C3%A0o-nh%C3%A2n-v%E1%BA%ADt-vector-m%C3%A0u-ph%E1%BA%B3ng-an-to%C3%A0n-v%C3%B4-di%E1%BB%87n-safecracker-k%C3%A9t-s%E1%BA%AFt.jpg?s=170667a&w=0&k=20&c=pFq97tF967IdKMjHkozrxFTrhB-Ixa5odI1v6Tpx6WE=" alt="Side Image" class="side-image">
                    </div>
                </div>
                <div class="card mb-4">
                    <div class="card-body">
                        <div id="alertContainer" style="display: none;">
                            <h5 class="mb-3">Khung Giờ Cảnh Báo</h5>
                            <div class="input-group mb-3" style="max-width: 250px;">
                                <input type="number" id="startHour" class="form-control" placeholder="Giờ bật" min="0" max="23">
                                <input type="number" id="startMinute" class="form-control" placeholder="Phút bật" min="0" max="59">
                                <button type="button" class="btn btn-primary" onclick="addTime()">Thêm</button>
                            </div>
                            <div class="input-group mb-3" style="max-width: 187px;">
                                <input type="number" id="endHour" class="form-control" placeholder="Giờ tắt" min="0" max="23">
                                <input type="number" id="endMinute" class="form-control" placeholder="Phút tắt" min="0" max="59">
                            </div>
                            <div id="timersContainer"></div>
                        </div>
                        <div class="mb-3">
                            <label for="volume" class="form-label">Âm Lượng</label>
                            <div class="d-flex align-items-center">
                                <input type="range" class="form-range" id="volume" min="0" max="100" step="1" value="50" oninput="updateVolumeValue(this.value)">
                                <span class="ms-3" id="volumeValue">50%</span>
                            </div>
                        </div>
                        <div class="mb-3">
                            <label for="soundSelect" class="form-label" id="soundLabel">Chọn âm báo</label>
                            <select class="form-select" id="soundSelect" onchange="updateSelectedSound()"></select>
                        </div>
                    </div>
                </div>
                <div class="d-grid mb-4">
                    <button type="button" class="btn btn-primary" onclick="sendConfigToESP32()">Gửi Cấu Hình</button>
                </div>
            </div>
            <div id="configSuccessAlert" class="alert alert-success text-center" style="display: none;">
                Cấu hình đã được gửi thành công!
            </div>
            <footer>by Nhatduy</footer>
        </div>

        <div class="tab-pane fade" id="tab2">
            <div class="container my-5">
                <h3 class="mb-4">Lịch Sử</h3>
                <div class="card mb-4">
                    <div class="card-body">
                        <h5>Biểu Đồ Thống Kê Cửa Mở Theo Giờ</h5>
                        <canvas id="historyChart" height="150"></canvas>
                        <h5 class="mt-4">Lịch Sử Đóng Mở Cửa</h5>
                        <button type="button" class="btn btn-danger mb-3" onclick="clearHistory()">Xóa Lịch Sử</button>
                        <table class="table table-bordered">
                            <thead>
                                <tr>
                                    <th>Thời Gian</th>
                                    <th>Trạng Thái</th>
                                    <th>Chế Độ Cảnh Báo</th>
                                </tr>
                            </thead>
                            <tbody id="historyTable">
                                <!-- Dữ liệu lịch sử sẽ được thêm bằng JS -->
                            </tbody>
                        </table>
                    </div>
                </div>
            </div>
            <footer>by Nhatduy</footer>
        </div>

        <div class="tab-pane fade" id="tab3">
            <div class="container my-5">
                <h3 class="mb-4">Cấu Hình Email</h3>
                <div class="card mb-4">
                    <div class="card-body">
                        <div class="mb-3">
                            <label for="senderEmail" class="form-label">Email gửi</label>
                            <input type="email" class="form-control" id="senderEmail" placeholder="example@gmail.com">
                        </div>
                        <div class="mb-3">
                            <label for="senderPassword" class="form-label">Mật khẩu ứng dụng</label>
                            <input type="password" class="form-control" id="senderPassword" placeholder="Mật khẩu ứng dụng (16 ký tự)">
                        </div>
                        <div class="mb-3">
                            <label for="recipientEmail" class="form-label">Email nhận</label>
                            <input type="email" class="form-control" id="recipientEmail" placeholder="your-email@gmail.com">
                        </div>
                        <div class="mb-3">
                            <label for="alertContent" class="form-label">Nội dung cảnh báo</label>
                            <textarea class="form-control" id="alertContent" rows="3" placeholder="Nhập nội dung cảnh báo"></textarea>
                        </div>
                        <div class="d-grid">
                            <button type="button" class="btn btn-primary" onclick="sendEmailConfigToESP32()">Gửi Cấu Hình Email</button>
                        </div>
                    </div>
                </div>
                <div id="emailSuccessAlert" class="alert alert-success text-center" style="display: none;">
                    Cấu hình email đã được gửi thành công!
                </div>
            </div>
            <footer>by Nhatduy</footer>
        </div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js"></script>
    <script>
        let ws = null;
        let timers = [];
        let selectedAlertSound = 'none';
        let selectedNormalSound = 'none';
        let historyChart = null;

        function connectWebSocket() {
            ws = new WebSocket('ws://' + location.host + '/ws');
            ws.onopen = function() {
                console.log('WebSocket connected');
            };
            ws.onmessage = function(event) {
                console.log('Received from WebSocket:', event.data);
                const message = JSON.parse(event.data);
                if (message.type === 'config') {
                    updateConfigUI(message.data);
                } else if (message.type === 'history') {
                    updateHistoryUI(message.data);
                } else if (message.type === 'email') {
                    updateEmailUI(message.data);
                }
            };
            ws.onclose = function() {
                console.log('WebSocket disconnected, reconnecting...');
                setTimeout(connectWebSocket, 1000);
            };
            ws.onerror = function(error) {
                console.error('WebSocket error:', error);
            };
        }

        function updateVolumeValue(value) {
            document.getElementById('volumeValue').textContent = value + '%';
        }

        function switchAlertMode(isAlert) {
            const alertContainer = document.getElementById('alertContainer');
            const soundLabel = document.getElementById('soundLabel');
            const soundSelect = document.getElementById('soundSelect');
            soundSelect.innerHTML = '<option value="none">Không có âm báo</option>';

            if (isAlert) {
                alertContainer.style.display = 'block';
                soundLabel.textContent = 'Chọn âm báo cảnh báo';
                const alertSounds = [
                    { value: "alert1", text: "Âm báo 1" },
                    { value: "alert2", text: "Âm báo 2" },
                    { value: "alert3", text: "Âm báo 3" }
                ];
                alertSounds.forEach(sound => {
                    soundSelect.innerHTML += `<option value="${sound.value}">${sound.text}</option>`;
                });
                soundSelect.value = selectedAlertSound;
            } else {
                alertContainer.style.display = 'none';
                soundLabel.textContent = 'Chọn âm báo bình thường';
                const normalSounds = [
                    { value: "normal1", text: "Âm báo 1" },
                    { value: "normal2", text: "Âm báo 2" },
                    { value: "normal3", text: "Âm báo 3" }
                ];
                normalSounds.forEach(sound => {
                    soundSelect.innerHTML += `<option value="${sound.value}">${sound.text}</option>`;
                });
                soundSelect.value = selectedNormalSound;
            }
        }

        document.getElementById('alertModeSwitch').addEventListener('change', function() {
            const isAlert = this.checked;
            document.getElementById('alertModeLabel').textContent = isAlert ? 'Bật' : 'Tắt';
            switchAlertMode(isAlert);
        });

        function updateSelectedSound() {
            const isAlert = document.getElementById('alertModeSwitch').checked;
            const soundSelect = document.getElementById('soundSelect');
            if (isAlert) {
                selectedAlertSound = soundSelect.value;
            } else {
                selectedNormalSound = soundSelect.value;
            }
        }

        function renderTimers() {
            const timersContainer = document.getElementById('timersContainer');
            timersContainer.innerHTML = '';
            timers.forEach((timer, index) => {
                timersContainer.innerHTML += `
                    <div class="input-group timer-display mb-2">
                        <input type="text" class="form-control" value="Bật: ${String(timer.startHour).padStart(2, '0')}:${String(timer.startMinute).padStart(2, '0')} - Tắt: ${String(timer.endHour).padStart(2, '0')}:${String(timer.endMinute).padStart(2, '0')}" readonly>
                        <button class="btn btn-danger" onclick="removeTimer(${index})">Xóa</button>
                    </div>`;
            });
        }

        function addTime() {
            const startHour = parseInt(document.getElementById('startHour').value);
            const startMinute = parseInt(document.getElementById('startMinute').value);
            const endHour = parseInt(document.getElementById('endHour').value);
            const endMinute = parseInt(document.getElementById('endMinute').value);
            if (isNaN(startHour) || isNaN(startMinute) || isNaN(endHour) || isNaN(endMinute) ||
                startHour < 0 || startHour > 23 || startMinute < 0 || startMinute > 59 ||
                endHour < 0 || endHour > 23 || endMinute < 0 || endMinute > 59) {
                alert('Giờ hoặc phút không hợp lệ.');
                return;
            }
            if (startHour === endHour && startMinute === endMinute) {
                alert('Giờ bật và giờ tắt không được trùng nhau.');
                return;
            }
            if (timers.some(t => t.startHour === startHour && t.startMinute === startMinute && t.endHour === endHour && t.endMinute === endMinute)) {
                alert('Khoảng thời gian này đã tồn tại.');
                return;
            }
            timers.push({ startHour, startMinute, endHour, endMinute });
            renderTimers();
            document.getElementById('startHour').value = '';
            document.getElementById('startMinute').value = '';
            document.getElementById('endHour').value = '';
            document.getElementById('endMinute').value = '';
        }

        function removeTimer(index) {
            timers.splice(index, 1);
            renderTimers();
        }

        function sendConfigToESP32() {
            const config = {
                alertMode: document.getElementById('alertModeSwitch').checked,
                timers: timers,
                timerCount: timers.length,
                volume: parseInt(document.getElementById('volume').value),
                alertSound: selectedAlertSound,
                normalSound: selectedNormalSound
            };
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ type: "config", data: config }));
                document.getElementById('configSuccessAlert').style.display = 'block';
                setTimeout(() => document.getElementById('configSuccessAlert').style.display = 'none', 3000);
            } else {
                console.error("WebSocket chưa kết nối!");
            }
        }

        function updateConfigUI(data) {
            document.getElementById('alertModeSwitch').checked = data.alertMode;
            switchAlertMode(data.alertMode);
            document.getElementById('alertModeLabel').textContent = data.alertMode ? 'Bật' : 'Tắt';
            timers = data.timers || [];
            renderTimers();
            document.getElementById('volume').value = data.volume;
            updateVolumeValue(data.volume);
            selectedAlertSound = data.alertSound || 'none';
            selectedNormalSound = data.normalSound || 'none';
            document.getElementById('soundSelect').value = data.alertMode ? selectedAlertSound : selectedNormalSound;
        }

        function updateHistoryUI(history) {
            const historyTable = document.getElementById('historyTable');
            historyTable.innerHTML = '';

            for (let i = history.length - 1; i >= 0; i--) {
                const event = history[i];
                const row = document.createElement('tr');
                row.innerHTML = `
                    <td>${event.time}</td>
                    <td>${event.status}</td>
                    <td>${event.alertMode ? 'Bật' : 'Tắt'}</td>
                `;
                historyTable.appendChild(row);
            }

            const hourlyOpenCounts = Array(24).fill(0);
            history.forEach(event => {
                if (event.status === 'Mở') {
                    const hour = parseInt(event.time.split(':')[0]);
                    hourlyOpenCounts[hour]++;
                }
            });

            const timeLabels = Array.from({ length: 24 }, (_, i) => `${i}h`);
            renderHistoryChart(timeLabels, hourlyOpenCounts);
        }

        function renderHistoryChart(timeLabels, hourlyOpenCounts) {
            const ctx = document.getElementById('historyChart').getContext('2d');
            if (historyChart) {
                historyChart.destroy();
            }
            historyChart = new Chart(ctx, {
                type: 'bar',
                data: {
                    labels: timeLabels,
                    datasets: [
                        {
                            label: 'Số lần mở cửa',
                            data: hourlyOpenCounts,
                            backgroundColor: 'rgba(54, 162, 235, 0.7)',
                            borderColor: 'rgba(54, 162, 235, 1)',
                            borderWidth: 1
                        }
                    ]
                },
                options: {
                    scales: {
                        y: {
                            beginAtZero: true,
                            title: {
                                display: true,
                                text: 'Số lần mở'
                            }
                        },
                        x: {
                            title: {
                                display: true,
                                text: 'Giờ trong ngày'
                            }
                        }
                    },
                    plugins: {
                        legend: {
                            display: true,
                            position: 'top'
                        }
                    }
                }
            });
        }

        function clearHistory() {
            if (confirm('Bạn có chắc chắn muốn xóa toàn bộ lịch sử không?')) {
                if (ws && ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify({ type: "clear_history" }));
                    setTimeout(() => {
                        alert('Lịch sử đã được xóa thành công!');
                        location.reload(); // Tải lại trang để hiển thị lịch sử rỗng
                    }, 500); // Đợi 500ms để đảm bảo server xử lý xong
                } else {
                    console.error("WebSocket chưa kết nối!");
                }
            }
        }

        document.querySelector('a[href="#tab2"]').addEventListener('shown.bs.tab', function() {
            // Không cần yêu cầu dữ liệu thủ công vì WebSocket tự động cập nhật
        });

        function sendEmailConfigToESP32() {
            const emailConfig = {
                senderEmail: document.getElementById('senderEmail').value,
                senderPassword: document.getElementById('senderPassword').value,
                recipientEmail: document.getElementById('recipientEmail').value,
                alertContent: document.getElementById('alertContent').value
            };
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ type: "email", data: emailConfig }));
                document.getElementById('emailSuccessAlert').style.display = 'block';
                setTimeout(() => document.getElementById('emailSuccessAlert').style.display = 'none', 3000);
            } else {
                console.error("WebSocket chưa kết nối!");
            }
        }

        function updateEmailUI(config) {
            document.getElementById('senderEmail').value = config.senderEmail || '';
            document.getElementById('senderPassword').value = config.senderPassword || '';
            document.getElementById('recipientEmail').value = config.recipientEmail || '';
            document.getElementById('alertContent').value = config.alertContent || '';
        }

        connectWebSocket();
    </script>
</body>
</html>
)rawliteral";

const char index_html_apmode[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, width=device-width, initial-scale=1.0, minimum-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Cài đặt WiFi</title>
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
            font-family: Arial, sans-serif;
        }
        body {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: flex-start;
            height: 100vh;
            text-align: center;
            padding-top: 20px;
        }
        .container {
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.2);
            border: 2px solid #007bff;
            width: 100%;
            max-width: 400px;
            text-align: center;
        }
        .container h2 {
            margin-bottom: 15px;
            color: #007bff;
        }
        .input-group {
            margin-bottom: 10px;
            text-align: left;
        }
        .input-group label {
            font-weight: bold;
        }
        .input-group input {
            width: 100%;
            padding: 10px;
            margin-top: 5px;
            border: 1px solid #ccc;
            border-radius: 5px;
        }
        .btn {
            background: #007bff;
            color: white;
            padding: 10px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            width: 100%;
            margin-top: 10px;
            font-size: 16px;
        }
        .btn:hover {
            background: #0056b3;
        }
    </style>
</head>
<body>
    <div class="container">
        <h2>Cài đặt WiFi</h2>
        <div class="input-group">
            <label for="wifi-name">Tên WiFi:</label>
            <input type="text" id="wifi-name" placeholder="Nhập tên WiFi">
        </div>
        <div class="input-group">
            <label for="wifi-password">Mật khẩu:</label>
            <input type="text" id="wifi-password" placeholder="Nhập mật khẩu">
        </div>
        <button class="btn" id="btnSubmit">Cài đặt</button>
    </div>
    <script>
      const ssid = document.getElementById("wifi-name");
      const pass = document.getElementById("wifi-password");
      var xhttp = new XMLHttpRequest();
      xhttp.open("GET", "/data_before", true);
      xhttp.send();
      xhttp.onreadystatechange = function() {
        if (xhttp.readyState == 4 && xhttp.status == 200) {
          const obj = JSON.parse(this.responseText);
          ssid.value = obj.ssid;
          pass.value = obj.pass;
        }
      }
      var xhttp2 = new XMLHttpRequest();
      const btnSubmit = document.getElementById("btnSubmit"); 
      btnSubmit.addEventListener('click', () => { 
          data = {
            ssid: ssid.value,
            pass: pass.value,
          }
          xhttp2.open("POST", "/post_data", true);
          xhttp2.send(JSON.stringify(data));
          xhttp2.onreadystatechange = function() {
            if (xhttp2.readyState == 4 && xhttp2.status == 200) {
              alert("Cài đặt thành công");
            } 
          }
      });
    </script>
</body>
</html>
)rawliteral";