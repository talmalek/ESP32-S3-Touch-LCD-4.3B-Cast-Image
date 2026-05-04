#include "web_server.h"
#include "frame_ui.h"
#include "../lvgl_v8_port.h"
#include <esp_display_panel.hpp>
extern esp_panel::board::Board *board;
#include "storage_manager.h"

static WebServer* server = nullptr;
static bool server_running = false;
static char server_ip[16] = "";
static uint16_t server_port = 80;

static const char* HTML_PAGE = R"raw(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Digital Picture Frame</title>
    <link href="https://cdnjs.cloudflare.com/ajax/libs/cropperjs/1.6.1/cropper.min.css" rel="stylesheet">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/cropperjs/1.6.1/cropper.min.js"></script>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            color: #fff;
            padding: 20px;
        }
        .container {
            background: rgba(30, 30, 60, 0.95);
            border-radius: 24px;
            padding: 40px;
            width: 100%;
            max-width: 600px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.4);
            border: 2px solid #4ecdc4;
        }
        h1 {
            color: #4ecdc4;
            text-align: center;
            margin-bottom: 8px;
            font-size: 28px;
        }
        .subtitle {
            text-align: center;
            color: #8888a0;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .upload-area {
            display: block;
            border: 3px dashed #4ecdc4;
            border-radius: 16px;
            padding: 50px 20px;
            text-align: center;
            transition: all 0.3s ease;
            cursor: pointer;
            margin-bottom: 20px;
            background: rgba(78, 205, 196, 0.05);
        }
        .upload-area:hover {
            background: rgba(78, 205, 196, 0.15);
        }
        input[type="file"] { display: none; }
        .upload-icon { font-size: 56px; margin-bottom: 16px; }
        .upload-text { color: #a0a0b0; font-size: 16px; }
        .upload-hint { color: #666680; font-size: 13px; margin-top: 8px; }
        
        .btn {
            width: 100%;
            padding: 16px;
            background: linear-gradient(135deg, #4ecdc4 0%, #44a08d 100%);
            border: none;
            border-radius: 12px;
            color: #1a1a2e;
            font-size: 17px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            margin-top: 10px;
        }
        .btn:hover { transform: translateY(-2px); box-shadow: 0 8px 20px rgba(78, 205, 196, 0.35); }
        .btn:disabled { background: #3d3d5c; color: #666; cursor: not-allowed; transform: none; box-shadow: none; }
        
        .status {
            margin-top: 20px;
            padding: 16px;
            border-radius: 12px;
            text-align: center;
            display: none;
            font-size: 14px;
        }
        .status.show { display: block; }
        .status.success { background: rgba(78, 205, 196, 0.2); color: #4ecdc4; border: 1px solid #4ecdc4; }
        .status.error { background: rgba(255, 99, 71, 0.2); color: #ff6347; border: 1px solid #ff6347; }
        
        .preview { margin-top: 20px; display: none; }
        .preview.show { display: block; }
        .img-container { max-width: 100%; max-height: 400px; }
        .img-container img { display: block; max-width: 100%; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Digital Picture Frame</h1>
        <p class="subtitle">Upload an image to display on the 800x480 screen</p>
        
        <label class="upload-area" id="uploadArea">
            <input type="file" id="fileInput" accept="image/jpeg,image/png,image/gif,image/bmp,image/webp">
            <div class="upload-icon">+</div>
            <div class="upload-text">Tap to select image</div>
            <div class="upload-hint">JPG, PNG, GIF, BMP supported</div>
        </label>
        
        <div class="preview" id="preview">
            <div class="img-container">
                <img id="previewImg" src="">
            </div>
        </div>
        
        <button class="btn" id="uploadBtn" style="display: none;">Upload Image</button>

        <div style="margin: 30px 0; border-top: 1px solid #3d3d5c;"></div>
        
        <p class="subtitle">Or choose a solid background color</p>
        <div style="display: flex; gap: 10px; align-items: center; margin-bottom: 20px;">
            <input type="color" id="solidColor" value="#1a1a2e" style="flex: 0 0 80px; height: 50px; cursor: pointer; background: none; border: 2px solid #4ecdc4; border-radius: 12px; padding: 4px;">
            <button class="btn" id="applyColorBtn" style="margin-top: 0; flex: 1;">Apply Solid Color</button>
        </div>
        
        <div class="status" id="status"></div>
    </div>

    <script>
        const fileInput = document.getElementById('fileInput');
        const uploadBtn = document.getElementById('uploadBtn');
        const status = document.getElementById('status');
        const preview = document.getElementById('preview');
        const previewImg = document.getElementById('previewImg');
        const uploadArea = document.getElementById('uploadArea');
        
        let cropper = null;
        let selectedFile = null;
        
        fileInput.addEventListener('change', e => {
            const file = e.target.files[0];
            if (!file) return;
            if (!file.type.match(/image.*/)) {
                showStatus('Please select a valid image', false);
                return;
            }
            selectedFile = file;
            
            uploadArea.style.display = 'none';
            uploadBtn.style.display = 'block';
            uploadBtn.disabled = false;
            
            previewImg.src = URL.createObjectURL(file);
            preview.classList.add('show');
            
            if (cropper) cropper.destroy();
            
            previewImg.onload = () => {
                cropper = new Cropper(previewImg, {
                    aspectRatio: 800 / 480,
                    viewMode: 1,
                    autoCropArea: 1,
                });
            };
            showStatus('Drag the frame to crop the image', true);
        });
        
        uploadBtn.addEventListener('click', async () => {
            if (!cropper) return;
            uploadBtn.disabled = true;
            uploadBtn.textContent = 'Processing...';
            
            try {
                const outWidth = 800;
                const outHeight = 480;

        function convertToRGB565(imgData) {
            const rgb565 = new Uint8Array(800 * 480 * 2);
            for (let i = 0, j = 0; i < imgData.length; i += 4, j += 2) {
                const r = imgData[i] >> 3;
                const g = imgData[i+1] >> 2;
                const b = imgData[i+2] >> 3;
                const rgb = (r << 11) | (g << 5) | b;
                rgb565[j] = rgb & 0xFF;
                rgb565[j+1] = (rgb >> 8) & 0xFF;
            }
            return rgb565;
        }

        async function uploadBinary(uint8Array) {
            const blob = new Blob([uint8Array], { type: 'application/octet-stream' });
            const formData = new FormData();
            formData.append('image', blob, 'image.bin');
            
            const resp = await fetch('/upload', { method: 'POST', body: formData });
            return await resp.json();
        }

        uploadBtn.addEventListener('click', async () => {
            if (!cropper) return;
            uploadBtn.disabled = true;
            uploadBtn.textContent = 'Processing...';
            
            try {
                const canvas = cropper.getCroppedCanvas({ width: 800, height: 480, fillColor: '#000000' });
                const imgData = canvas.getContext('2d').getImageData(0, 0, 800, 480).data;
                const rgb565 = convertToRGB565(imgData);
                
                uploadBtn.textContent = 'Uploading...';
                const result = await uploadBinary(rgb565);
                
                if (result.success) {
                    showStatus('Image uploaded! Display updated.', true);
                    setTimeout(() => {
                        uploadBtn.style.display = 'none';
                        preview.classList.remove('show');
                        uploadArea.style.display = 'block';
                        if (cropper) cropper.destroy();
                        cropper = null;
                        fileInput.value = '';
                    }, 2000);
                } else {
                    showStatus('Error: ' + result.error, false);
                }
            } catch(e) {
                showStatus('Error: ' + e.message, false);
            } finally {
                uploadBtn.disabled = false;
                uploadBtn.textContent = 'Upload Image';
            }
        });

        const solidColorInput = document.getElementById('solidColor');
        const applyColorBtn = document.getElementById('applyColorBtn');

        applyColorBtn.addEventListener('click', async () => {
            applyColorBtn.disabled = true;
            const originalText = applyColorBtn.textContent;
            applyColorBtn.textContent = 'Applying...';
            
            try {
                const canvas = document.createElement('canvas');
                canvas.width = 800;
                canvas.height = 480;
                const ctx = canvas.getContext('2d');
                ctx.fillStyle = solidColorInput.value;
                ctx.fillRect(0, 0, 800, 480);
                
                const imgData = ctx.getImageData(0, 0, 800, 480).data;
                const rgb565 = convertToRGB565(imgData);
                
                const result = await uploadBinary(rgb565);
                if (result.success) {
                    showStatus('Solid color background applied!', true);
                } else {
                    showStatus('Failed to apply color', false);
                }
            } catch (e) {
                showStatus('Error: ' + e.message, false);
            } finally {
                applyColorBtn.disabled = false;
                applyColorBtn.textContent = originalText;
            }
        });
        
        function showStatus(msg, success) {
            status.textContent = msg;
            status.className = 'status show ' + (success ? 'success' : 'error');
            setTimeout(() => status.className = 'status', 5000);
        }
    </script>
</body>
</html>)raw";

static void handleRoot() {
    server->send(200, "text/html", HTML_PAGE);
}

static File uploadFile;

static void handleUpload() {
    HTTPUpload& upload = server->upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Upload start: %s\n", upload.filename.c_str());
        
        // MUST stop LVGL before touching LittleFS - LVGL DMA transfers corrupt flash writes
        if (board && board->getBacklight()) {
            board->getBacklight()->off();
        }
        lvgl_port_stop(); // Stop DMA/SPI activity from LVGL task
        
        if (LittleFS.exists("/image.bin")) {
            LittleFS.remove("/image.bin");
        }
        uploadFile = LittleFS.open("/image.bin", FILE_WRITE);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
        }
        Serial.printf("Upload done: %d bytes\n", upload.totalSize);
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        if (uploadFile) {
            uploadFile.close();
            LittleFS.remove("/image.bin");
        }
        lvgl_port_resume();
        if (board && board->getBacklight()) board->getBacklight()->on();
    }
}

static void handleNotFound() {
    server->send(404, "text/plain", "Not Found");
}

void ImageWebServer::start() {
    if (server_running) return;
    
    server = new WebServer(server_port);
    server->on("/", HTTP_GET, handleRoot);
    
    server->on("/upload", HTTP_POST, []() {
        // File is written and closed. Now safe to queue the load.
        // We stay in 'stopped' mode (black screen) until loadStoredImage finishes.
        FrameUI::queueImageLoad();
        server->send(200, "application/json", "{\"success\":true,\"message\":\"Image uploaded successfully\"}");
    }, handleUpload);
    
    server->onNotFound(handleNotFound);
    server->begin();
    server_running = true;
    
    strlcpy(server_ip, WiFi.localIP().toString().c_str(), sizeof(server_ip));
    Serial.printf("Web server: http://%s:%d\n", server_ip, server_port);
}

void ImageWebServer::stop() {
    if (server) { server->stop(); delete server; server = nullptr; }
    server_running = false;
}

void ImageWebServer::loop() {
    if (server_running && server) {
        server->handleClient();
    }
}

bool ImageWebServer::isRunning() { return server_running; }
const char* ImageWebServer::getServerIP() { return server_ip; }
uint16_t ImageWebServer::getServerPort() { return server_port; }