#include "VideoStream.h"

#include <iostream>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

VideoStream::VideoStream(int port) : _port(port) {}

VideoStream::~VideoStream() {
    stop();
}

std::vector<std::string> VideoStream::buildFfmpegArgs(const std::string& inputSource, bool isRtsp) {
    std::vector<std::string> args = {"ffmpeg", "-hide_banner", "-loglevel", "error"};

    if (isRtsp) {
        args.insert(args.end(), {"-rtsp_transport", "tcp", "-i", inputSource});
    } else {
        // USB (V4L2) камера. Формат/роздільна здатність підганяються під
        // конкретну камеру — це те, що раніше передавалось у input_uvc.so.
        args.insert(args.end(), {
            "-f", "v4l2",
            "-input_format", "mjpeg",
            "-video_size", "1280x720",
            "-framerate", "30",
            "-i", inputSource
        });
    }

    args.insert(args.end(), {
        "-c:v", "libx264",
        "-preset", "ultrafast",
        "-tune", "zerolatency",
        "-b:v", "1500k",
        "-g", "30",
        "-f", "mpegts",
        "-" // MPEG-TS у stdout
    });

    return args;
}

bool VideoStream::startCamera(const std::string& cameraId, const std::string& inputSource, bool isRtsp) {
    std::lock_guard<std::mutex> lock(_mutex);

    auto it = _pipelines.find(cameraId);
    if (it != _pipelines.end() && it->second->running) {
        return true; // вже транслюється — нічого не робимо
    }

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        std::cerr << "VideoStream: pipe() failed: " << std::strerror(errno) << std::endl;
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "VideoStream: fork() failed: " << std::strerror(errno) << std::endl;
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (pid == 0) {
        // Дочірній процес: stdout -> канал, stderr глушимо
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);

        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        close(pipefd[1]);

        auto argsStr = buildFfmpegArgs(inputSource, isRtsp);
        std::vector<char*> argv;
        argv.reserve(argsStr.size() + 1);
        for (auto& a : argsStr) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);

        execvp("ffmpeg", argv.data());
        _exit(127); // сюди потрапляємо лише якщо execvp не зміг запустити ffmpeg
    }

    // Батьківський процес
    close(pipefd[1]);

    auto pipeline = std::make_shared<CameraPipeline>();
    pipeline->ffmpegPid = pid;
    pipeline->stdoutFd = pipefd[0];
    pipeline->running = true;

    _pipelines[cameraId] = pipeline;
    pipeline->readerThread = std::thread(&VideoStream::readerLoop, this, cameraId);

    return true;
}

void VideoStream::readerLoop(const std::string& cameraId) {
    std::shared_ptr<CameraPipeline> pipeline;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _pipelines.find(cameraId);
        if (it == _pipelines.end()) return;
        pipeline = it->second;
    }

    constexpr size_t CHUNK = 32 * 1024;
    std::vector<char> buf(CHUNK);

    while (pipeline->running) {
        ssize_t n = read(pipeline->stdoutFd, buf.data(), buf.size());
        if (n <= 0) break; // ffmpeg завершився або канал закрито
        broadcast(cameraId, buf.data(), static_cast<size_t>(n));
    }

    pipeline->running = false;
}

void VideoStream::broadcast(const std::string& cameraId, const char* data, size_t len) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _subscribers.find(cameraId);
    if (it == _subscribers.end()) return;

    // Прибираємо підписників, чиї з'єднання вже закриті
    auto& subs = it->second;
    subs.erase(std::remove_if(subs.begin(), subs.end(),
                               [](httplib::DataSink* s) { return !s || !s->is_writable(); }),
               subs.end());

    for (auto* sink : subs) {
        sink->write(data, len);
    }
}

void VideoStream::stopCamera(const std::string& cameraId) {
    std::shared_ptr<CameraPipeline> pipeline;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _pipelines.find(cameraId);
        if (it == _pipelines.end()) return;
        pipeline = it->second;
        _pipelines.erase(it);
    }

    pipeline->running = false;

    // Спочатку гасимо ffmpeg і чекаємо його завершення — тоді pipe закриється
    // сам, і читаючий потік коректно вийде за EOF, а не за обірваний fd.
    if (pipeline->ffmpegPid > 0) {
        kill(pipeline->ffmpegPid, SIGTERM);
        int status = 0;
        waitpid(pipeline->ffmpegPid, &status, 0);
    }

    if (pipeline->readerThread.joinable()) {
        pipeline->readerThread.join();
    }

    if (pipeline->stdoutFd >= 0) {
        close(pipeline->stdoutFd);
    }
}

bool VideoStream::isCameraActive(const std::string& cameraId) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _pipelines.find(cameraId);
    return it != _pipelines.end() && it->second->running;
}

// Тримає HTTP-з'єднання відкритим і реєструє sink у _subscribers[cameraId].
// Дані пишуться асинхронно з readerLoop() (потоку читання ffmpeg) через
// broadcast() — той самий трюк, що і в handleEvents.cpp для SSE.
void VideoStream::handleViewer(const std::string& cameraId, httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Cache-Control", "no-cache");

    res.set_chunked_content_provider(
        "video/mp2t",
        [this, cameraId](size_t /*offset*/, httplib::DataSink& sink) {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _subscribers[cameraId].push_back(&sink);
            }

            while (sink.is_writable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

            {
                std::lock_guard<std::mutex> lock(_mutex);
                auto& subs = _subscribers[cameraId];
                subs.erase(std::remove(subs.begin(), subs.end(), &sink), subs.end());
            }
            return true;
        });
}

void VideoStream::start() {
    _serverThread = std::thread([this]() {
        _server = std::make_unique<httplib::Server>();
        // Окремий пул потоків — щоб довгі перегляди відео не забирали
        // воркерів у основного API/SSE-сервера (порт 1616).
        _server->new_task_queue = [] { return new httplib::ThreadPool(32); };

        _server->Get(R"(/(.+))", [this](const httplib::Request& req, httplib::Response& res) {
            std::string cameraId = req.matches[1].str();
            handleViewer(cameraId, res);
        });

        std::cout << "Video stream (HTTP, MPEG-TS): http://<ip>:" << _port << "/<cameraId>" << std::endl;

        if (!_server->listen("0.0.0.0", _port)) {
            std::cerr << "VideoStream: failed to start on port " << _port << std::endl;
        }
    });
}

void VideoStream::stop() {
    std::vector<std::string> ids;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& kv : _pipelines) ids.push_back(kv.first);
    }
    for (auto& id : ids) stopCamera(id);

    if (_server) {
        _server->stop();
    }
    if (_serverThread.joinable()) {
        _serverThread.join();
    }
}