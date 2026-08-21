#include "Http.h"
#include <iostream>
#include <string>
#include "../Server/Server.h"

// namespace {
//     bool isRtspSource(const std::string& path) {
//         return path.rfind("rtsp://", 0) == 0;
//     }
// }

// void Http::handleCameraControl(const httplib::Request& req, httplib::Response& res) {
//     Http::addCorsHeaders(res);

//     std::string action = req.get_param_value("action");

//     // Поточний стан — фронт викликає це один раз при завантаженні сторінки
//     // (або при відкритті /api/events), щоб не залежати від того, встиг він
//     // побачити попередні SSE-події чи ні.
//     if (action == "status") {
//         nlohmann::ordered_json payload{
//             {"camera", "camera1"},
//             {"selected_path", serverPtr->camera1.devicePath},
//             {"is_selected", serverPtr->camera1.isActive},
//             {"is_streaming", videoStream.isCameraActive("camera1")}
//         };
//         res.set_content(payload.dump(), "application/json");
//         return;
//     }

//     // Пошук камер
//     if (action == "search") {
//         serverPtr->searching_new_usb_cam();

//         nlohmann::ordered_json camerasJson = nlohmann::ordered_json::array();
//         for (const auto& cam : serverPtr->foundCameras) {
//             camerasJson.push_back({{"name", cam.deviceName}, {"path", cam.devicePath}});
//         }

//         res.set_content(camerasJson.dump(), "application/json");

//         // Синхронізуємо список камер з усіма відкритими сторінками дашборду
//         broadcast_event("cameras_found", camerasJson);
//         return;
//     }

//     // Вибір конкретної камери (фронт має прислати шлях)
//     if (action == "select") {
//         std::string selectedPath = req.get_param_value("path");
//         serverPtr->set_new_camera(selectedPath);
//         res.set_content("{\"status\":\"selected\"}", "application/json");

//         nlohmann::ordered_json payload{{"path", selectedPath}};
//         broadcast_event("camera_selected", payload);
//         return;
//     }

//     // Запуск відео (ffmpeg -> H.264/MPEG-TS -> WebSocket) для вибраної камери
//     if (action == "start") {
//         if (!serverPtr->camera1.isActive) {
//             res.status = 400;
//             res.set_content("{\"error\":\"No camera selected\"}", "application/json");
//             return;
//         }

//         bool started = videoStream.startCamera(
//             "camera1",
//             serverPtr->camera1.devicePath,
//             isRtspSource(serverPtr->camera1.devicePath));

//         if (!started) {
//             res.status = 500;
//             res.set_content("{\"error\":\"Failed to start stream\"}", "application/json");
//             return;
//         }

//         nlohmann::ordered_json payload{{"camera", "camera1"}, {"status", "started"}};
//         res.set_content(payload.dump(), "application/json");

//         // Усі відкриті сторінки одразу бачать, що камера запущена
//         broadcast_event("camera_state", payload);
//         return;
//     }

//     if (action == "stop") {
//         videoStream.stopCamera("camera1");
//         serverPtr->camera1.isActive = false; // Позначаємо, що камера не активна

//         nlohmann::ordered_json payload{{"camera", "camera1"}, {"status", "stopped"}};
//         res.set_content(payload.dump(), "application/json");

//         broadcast_event("camera_state", payload);
//         return;
//     }

//     res.status = 400;
//     res.set_content("{\"error\":\"Unknown action\"}", "application/json");
// }