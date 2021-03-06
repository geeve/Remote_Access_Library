#include "ServerDriver.h"
#include "Shapes.h"
#include "IServerDriver.h"
#include "Configs.h"
#include "turbojpeg.h"
#include "NetworkStructs.h"
#include "Logging.h"
#include "ScreenCapture.h"

#include "SCCommon.h"
#include "WS_Lite.h"

#include <mutex>

namespace SL {
    namespace RAT {

        class ServerDriverImpl {

        public:
            std::shared_ptr<Server_Config> Config_;
            IServerDriver * IServerDriver_;

            std::mutex mutex;
            std::vector<std::shared_ptr<WS_LITE::IWSocket>> Clients;

            WS_LITE::WSListener h;

            void onKeyUp(const std::shared_ptr<WS_LITE::IWSocket>& socket, const unsigned char* data, size_t len) {
                switch (len) {
                case(sizeof(char)):
                    IServerDriver_->onKeyUp(socket, *reinterpret_cast<const char*>(data));
                    break;
                case(sizeof(wchar_t)):
                    IServerDriver_->onKeyUp(socket, *reinterpret_cast<const wchar_t*>(data));
                    break;
                case(sizeof(Input_Lite::SpecialKeyCodes)):
                    IServerDriver_->onKeyUp(socket, *reinterpret_cast<const Input_Lite::SpecialKeyCodes*>(data));
                    break;
                default:
                    return socket->close(1000, "Received invalid onKeyUp Event");
                }
            }
            void onKeyDown(const std::shared_ptr<WS_LITE::IWSocket>& socket, const unsigned char* data, size_t len) {
                switch (len) {
                case(sizeof(char)):
                    IServerDriver_->onKeyDown(socket, *reinterpret_cast<const char*>(data));
                    break;
                case(sizeof(wchar_t)):
                    IServerDriver_->onKeyDown(socket, *reinterpret_cast<const wchar_t*>(data));
                    break;
                case(sizeof(Input_Lite::SpecialKeyCodes)):
                    IServerDriver_->onKeyDown(socket, *reinterpret_cast<const Input_Lite::SpecialKeyCodes*>(data));
                    break;
                default:
                    return socket->close(1000, "Received invalid onKeyDown Event");
                }
            }
            void onMouseUp(const std::shared_ptr<WS_LITE::IWSocket>& socket, const unsigned char* data, size_t len) {
                if (len == sizeof(Input_Lite::MouseButtons)) {
                    return IServerDriver_->onMouseUp(socket, *reinterpret_cast<const Input_Lite::MouseButtons*>(data));
                }
                socket->close(1000, "Received invalid onMouseUp Event");
            }

            void onMouseDown(const std::shared_ptr<WS_LITE::IWSocket>& socket, const unsigned char* data, size_t len) {
                if (len == sizeof(Input_Lite::MouseButtons)) {
                    return IServerDriver_->onMouseDown(socket, *reinterpret_cast<const Input_Lite::MouseButtons*>(data));
                }
                socket->close(1000, "Received invalid onMouseDown Event");
            }
            void onMousePosition(const std::shared_ptr<WS_LITE::IWSocket>& socket, const unsigned char* data, size_t len) {
                if (len == sizeof(Point)) {
                    return IServerDriver_->onMousePosition(socket, *reinterpret_cast<const Point*>(data));
                }
                socket->close(1000, "Received invalid onMouseDown Event");
            }
            void onMouseScroll(const std::shared_ptr<WS_LITE::IWSocket>& socket, const unsigned char* data, size_t len) {
                if (len == sizeof(int)) {
                    return IServerDriver_->onMouseScroll(socket, *reinterpret_cast<const int*>(data));
                }
                socket->close(1000, "Received invalid onMouseScroll Event");
            }
            void onClipboardChanged(const std::shared_ptr<WS_LITE::IWSocket>& socket, const unsigned char* data, size_t len) {
                if (len < 1024 * 100) { //100K max
                    std::string str(reinterpret_cast<const char*>(data), len);
                    return IServerDriver_->onClipboardChanged(str);
                }
            }
            ServerDriverImpl(IServerDriver * r, std::shared_ptr<Server_Config> config) : Config_(config), IServerDriver_(r) {

                h = WS_LITE::CreateContext(WS_LITE::ThreadCount(1))
                    .CreateListener(config->WebSocketTLSLPort)
                    .onConnection([&](const std::shared_ptr<WS_LITE::IWSocket>& socket, const std::unordered_map<std::string, std::string>& header) {
                    if (Config_->MaxNumConnections > 0 && Clients.size() + 1 > static_cast<size_t>(Config_->MaxNumConnections)) {
                        socket->close(1000, "Closing due to max number of connections!");
                    }
                    else {
                        IServerDriver_->onConnection(socket);
                        std::lock_guard<std::mutex> lock(mutex);
                        Clients.push_back(socket);
                    }
                }).onDisconnection([&](const std::shared_ptr<WS_LITE::IWSocket>& socket, unsigned short code, const std::string& msg) {
                    SL_RAT_LOG(Logging_Levels::INFO_log_level, "onDisconnection  ");
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        Clients.erase(std::remove_if(Clients.begin(), Clients.end(), [&](auto& s) { return s == socket;  }));
                    }
                    IServerDriver_->onDisconnection(socket, code, msg);
                }).onMessage([&](const std::shared_ptr<WS_LITE::IWSocket>& socket, const WS_LITE::WSMessage& message) {

                    auto p = PACKET_TYPES::INVALID;
                    assert(message.len >= sizeof(p));

                    p = *reinterpret_cast<const PACKET_TYPES*>(message.data);
                    auto datastart = message.data + sizeof(p);
                    auto datasize = message.len - sizeof(p);

                    switch (p) {
                    case PACKET_TYPES::ONKEYDOWN:
                        onKeyDown(socket, datastart, datasize);
                        break;
                    case PACKET_TYPES::ONKEYUP:
                        onKeyUp(socket, datastart, datasize);
                        break;
                    case PACKET_TYPES::ONMOUSEUP:
                        onMouseUp(socket, datastart, datasize);
                        break;
                    case PACKET_TYPES::ONMOUSEDOWN:
                        onMouseDown(socket, datastart, datasize);
                        break;
                    case PACKET_TYPES::ONMOUSEPOSITIONCHANGED:
                        onMousePosition(socket, datastart, datasize);
                        break;
                    case PACKET_TYPES::ONMOUSESCROLL:
                        onMouseScroll(socket, datastart, datasize);
                        break;
                    case PACKET_TYPES::ONCLIPBOARDTEXTCHANGED:
                        onClipboardChanged(socket, datastart, datasize);
                        break;
                    default:
                        IServerDriver_->onMessage(socket, message);
                        break;
                    }
                }).listen(true, true);
            }
            ~ServerDriverImpl() {

            }
            void Send(const std::shared_ptr<WS_LITE::IWSocket>& socket, std::shared_ptr<unsigned char>& data, size_t len) {
                if (socket) {
                    auto msg = WS_LITE::WSMessage{ data.get(), len, WS_LITE::OpCode::BINARY, data };
                    socket->send(msg, false);
                }
                else {
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        for (auto& s : Clients) {
                            auto msg = WS_LITE::WSMessage{ data.get(), len, WS_LITE::OpCode::BINARY, data };
                            s->send(msg, false);
                        }
                    }
                }
            }

            void SendScreen(const std::shared_ptr<WS_LITE::IWSocket>& socket, const Screen_Capture::Image & img, const Screen_Capture::Monitor& monitor, PACKET_TYPES p) {

                if (Clients.empty() && !socket) return;
                Rect r(Point(img.Bounds.left, img.Bounds.top), Height(img), Width(img));

                auto set = Config_->SendGrayScaleImages ? TJSAMP_GRAY : TJSAMP_420;
                auto maxsize = tjBufSize(Screen_Capture::Width(img), Screen_Capture::Height(img), set) + sizeof(r) + sizeof(p) + sizeof(monitor.Id);

                auto jpegCompressor = tjInitCompress();
                auto  buffer = std::shared_ptr<unsigned char>(new unsigned char[maxsize], [](auto* p) { delete[] p; });

                auto dst = (unsigned char*)buffer.get();
                memcpy(dst, &p, sizeof(p));
                dst += sizeof(p);
                memcpy(dst, &monitor.Id, sizeof(monitor.Id));
                dst += sizeof(monitor.Id);
                memcpy(dst, &r, sizeof(r));
                dst += sizeof(r);

                auto srcbuffer = std::make_unique<unsigned char[]>(RowStride(img)*Height(img));
                Screen_Capture::Extract(img, srcbuffer.get(), RowStride(img)*Height(img));
                auto srcbuf = (unsigned char*)srcbuffer.get();


                auto colorencoding = TJPF_RGBX;
                auto outjpegsize = maxsize;

                if (tjCompress2(jpegCompressor, srcbuf, r.Width, 0, r.Height, colorencoding, &dst, &outjpegsize, set, Config_->ImageCompressionSetting, 2048 | TJFLAG_NOREALLOC) == -1) {
                    SL_RAT_LOG(Logging_Levels::ERROR_log_level, tjGetErrorStr());
                }
                //	std::cout << "Sending " << r << std::endl;
                auto finalsize = sizeof(p) + sizeof(r) + sizeof(monitor.Id) + outjpegsize;//adjust the correct size
                tjDestroy(jpegCompressor);
                Send(socket, buffer, finalsize);
            }
            void SendMouseImageChanged(const std::shared_ptr<WS_LITE::IWSocket>& socket, const Screen_Capture::Image & img) {

                if (Clients.empty() && !socket) return;
                Rect r(Point(0, 0), Height(img), Width(img));

                auto p = static_cast<unsigned int>(PACKET_TYPES::ONMOUSEIMAGECHANGED);
                auto finalsize = (Screen_Capture::RowStride(img) * Screen_Capture::Height(img)) + sizeof(p) + sizeof(r);
                auto  buffer = std::shared_ptr<unsigned char>(new unsigned char[finalsize], [](auto* p) { delete[] p; });

                auto dst = buffer.get();
                memcpy(dst, &p, sizeof(p));
                dst += sizeof(p);
                memcpy(dst, &r, sizeof(r));
                dst += sizeof(r);
                Screen_Capture::Extract(img, (unsigned char*)dst, Screen_Capture::RowStride(img) * Screen_Capture::Height(img));

                Send(socket, buffer, finalsize);

            }
            void SendMonitorsChanged(const std::shared_ptr<WS_LITE::IWSocket>& socket, const std::vector<Screen_Capture::Monitor>& monitors) {
              if (Clients.empty() && !socket) return;
                auto p = static_cast<unsigned int>(PACKET_TYPES::ONMONITORSCHANGED);
                const auto size = (monitors.size() * sizeof(Screen_Capture::Monitor)) + sizeof(p);

                auto  buffer = std::shared_ptr<unsigned char>(new unsigned char[size], [](auto* p) { delete[] p; });
                auto buf = buffer.get();
                memcpy(buf, &p, sizeof(p));
                buf += sizeof(p);
                for (auto& a : monitors) {
                    memcpy(buf, &a, sizeof(a));
                    buf += sizeof(Screen_Capture::Monitor);
                }
                Send(socket, buffer, size);

            }
            void SendMousePositionChanged(const std::shared_ptr<WS_LITE::IWSocket>& socket, const SL::Screen_Capture::Point& pos)
            {
               if (Clients.empty() && !socket) return;
                auto p = static_cast<unsigned int>(PACKET_TYPES::ONMOUSEPOSITIONCHANGED);
                const auto size = sizeof(pos) + sizeof(p);

                auto  buffer = std::shared_ptr<unsigned char>(new unsigned char[size], [](auto* p) { delete[] p; });
                memcpy(buffer.get(), &p, sizeof(p));
                memcpy(buffer.get() + sizeof(p), &pos, sizeof(pos));

                Send(socket, buffer, size);
            }

            void SendClipboardChanged(const std::shared_ptr<WS_LITE::IWSocket>& socket, const std::string& text) {
              if (Clients.empty() && !socket) return;
                auto p = static_cast<unsigned int>(PACKET_TYPES::ONCLIPBOARDTEXTCHANGED);
                auto size = text.size() + sizeof(p);

                auto  buffer = std::shared_ptr<unsigned char>(new unsigned char[size], [](auto* p) { delete[] p; });
                memcpy(buffer.get(), &p, sizeof(p));
                memcpy(buffer.get() + sizeof(p), text.data(), text.size());

                Send(socket, buffer, size);

            }
        };
        ServerDriver::ServerDriver(IServerDriver * r, std::shared_ptr<Server_Config> config) {
            ServerDriverImpl_ = std::make_unique<ServerDriverImpl>(r, config);

        }
        ServerDriver::~ServerDriver()
        {

        }
        void ServerDriver::SendMonitorsChanged(const std::shared_ptr<WS_LITE::IWSocket>& socket, const std::vector<Screen_Capture::Monitor>& monitors)
        {
            ServerDriverImpl_->SendMonitorsChanged(socket, monitors);
        }
        void ServerDriver::SendMonitorsChanged(const std::vector<Screen_Capture::Monitor>& monitors)
        {
            std::shared_ptr<WS_LITE::IWSocket> socket;
            ServerDriverImpl_->SendMonitorsChanged(socket, monitors);
        }
        void ServerDriver::SendFrameChanged(const std::shared_ptr<WS_LITE::IWSocket>& socket, const Screen_Capture::Image& image, const Screen_Capture::Monitor& monitor)
        {
            ServerDriverImpl_->SendScreen(socket, image, monitor, PACKET_TYPES::ONFRAMECHANGED);
        }
        void ServerDriver::SendFrameChanged(const Screen_Capture::Image& image, const Screen_Capture::Monitor& monitor)
        {
            std::shared_ptr<WS_LITE::IWSocket> socket;
            ServerDriverImpl_->SendScreen(socket, image, monitor, PACKET_TYPES::ONFRAMECHANGED);
        }
        void ServerDriver::SendNewFrame(const std::shared_ptr<WS_LITE::IWSocket>& socket, const Screen_Capture::Image& image, const Screen_Capture::Monitor& monitor)
        {
            ServerDriverImpl_->SendScreen(socket, image, monitor, PACKET_TYPES::ONNEWFRAME);
        }
        void ServerDriver::SendNewFrame(const Screen_Capture::Image& image, const Screen_Capture::Monitor& monitor)
        {
            std::shared_ptr<WS_LITE::IWSocket> socket;
            ServerDriverImpl_->SendScreen(socket, image, monitor, PACKET_TYPES::ONNEWFRAME);
        }
        void ServerDriver::SendMouseImageChanged(const std::shared_ptr<WS_LITE::IWSocket>& socket, const Screen_Capture::Image& image)
        {
            ServerDriverImpl_->SendMouseImageChanged(socket, image);
        }
        void ServerDriver::SendMouseImageChanged(const Screen_Capture::Image& image)
        {
            std::shared_ptr<WS_LITE::IWSocket> socket;
            ServerDriverImpl_->SendMouseImageChanged(socket, image);
        }
        void ServerDriver::SendMousePositionChanged(const std::shared_ptr<WS_LITE::IWSocket>& socket, const SL::Screen_Capture::Point & pos)
        {
            ServerDriverImpl_->SendMousePositionChanged(socket, pos);
        }
        void ServerDriver::SendMousePositionChanged(const SL::Screen_Capture::Point & pos)
        {
            std::shared_ptr<WS_LITE::IWSocket> socket;
            ServerDriverImpl_->SendMousePositionChanged(socket, pos);
        }
        void ServerDriver::SendClipboardChanged(const std::shared_ptr<WS_LITE::IWSocket>& socket, const std::string& text)
        {
            ServerDriverImpl_->SendClipboardChanged(socket, text);
        }
        void ServerDriver::SendClipboardChanged(const std::string& text)
        {
            std::shared_ptr<WS_LITE::IWSocket> socket;
            ServerDriverImpl_->SendClipboardChanged(socket, text);
        }
        size_t ServerDriver::getClientCount() const
        {
            return ServerDriverImpl_->Clients.size();
        }
    }
}
