#ifndef VIDEO_STREAM_H
#define VIDEO_STREAM_H

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <memory>
#include <vector>
#include <sys/types.h>
#include "../lib/http/httplib.h"

// Роздає H.264/MPEG-TS потік з ffmpeg усім клієнтам, підписаним на конкретну
// камеру: GET http://host:port/<cameraId>. Транспорт — звичайний chunked
// HTTP-response, той самий механізм, що і в handleEvents.cpp (SSE), тільки
// пишемо бінарні байти замість тексту. Ніяких нових залежностей понад
// httplib, який вже є в проекті.
//
// На кожну камеру запускається один процес ffmpeg незалежно від кількості
// глядачів — усі клієнти отримують те саме, що читається з stdout ffmpeg.
class VideoStream {
public:
    explicit VideoStream(int port = 9002);
    ~VideoStream();

    // Піднімає окремий httplib::Server у власному потоці (свій пул потоків,
    // щоб довгі перегляди відео не забирали воркерів у основного API).
    void start();
    void stop();

    // inputSource: шлях V4L2-пристрою ("/dev/video0") або URL ("rtsp://...").
    bool startCamera(const std::string& cameraId, const std::string& inputSource, bool isRtsp);
    void stopCamera(const std::string& cameraId);
    bool isCameraActive(const std::string& cameraId) const;

private:
    struct CameraPipeline {
        pid_t ffmpegPid = -1;
        int stdoutFd = -1;
        std::thread readerThread;
        std::atomic<bool> running{false};
    };

    void readerLoop(const std::string& cameraId);
    void broadcast(const std::string& cameraId, const char* data, size_t len);
    void handleViewer(const std::string& cameraId, httplib::Response& res);
    static std::vector<std::string> buildFfmpegArgs(const std::string& inputSource, bool isRtsp);

    int _port;
    std::unique_ptr<httplib::Server> _server;
    std::thread _serverThread;

    mutable std::mutex _mutex;
    std::map<std::string, std::shared_ptr<CameraPipeline>> _pipelines;
    std::map<std::string, std::vector<httplib::DataSink*>> _subscribers;
};

#endif