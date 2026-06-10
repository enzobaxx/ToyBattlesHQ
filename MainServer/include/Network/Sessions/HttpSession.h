#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include <asio.hpp>
#include "Network/MainSessionManager.h"
#include "boost/beast.hpp"
#include "boost/json.hpp"
#include "boost/beast/ssl/ssl_stream.hpp"
#include <format>
#include "jwt-cpp/jwt.h"
#include <cstring>
#include <string>
#include <optional>

namespace Main
{
    namespace Network
    {
        namespace http = boost::beast::http;

        // Note: This serves HTTP only, if you need HTTPS you can use NGINX
        class HttpSession : public std::enable_shared_from_this<HttpSession>
        {
            using tcp = boost::asio::ip::tcp;

        public:
            explicit HttpSession(tcp::socket&& socket, Main::Network::SessionsManager& sm, Main::Classes::RoomsManager& rm,
                Main::Persistence::MainScheduler& ms)
                : m_socket(std::move(socket))
                , m_sessionsManager{ sm }
                , m_roomsManager{ rm }
                , m_scheduler{ ms }
            {
            }

            void start() { readRequest(); }

        private:
            boost::beast::tcp_stream m_socket;
            boost::beast::flat_buffer m_buffer;
            http::request<http::string_body> m_request;
            Main::Network::SessionsManager& m_sessionsManager;
            Main::Classes::RoomsManager& m_roomsManager;
            Main::Persistence::MainScheduler& m_scheduler;

            bool verifyTokenAndGrade(const std::string& token, const std::string& secret, std::uint32_t requiredGrade);
            std::optional<std::string> extractToken(const http::request<http::string_body>& request);
            bool validateRequest(const http::request<http::string_body>& m_request, std::string& responseBody, http::status& statusCode,
                boost::json::object& obj, std::uint32_t grade);
            void readRequest();
            void handleRequest();
            void sendResponse(const std::string& body, http::status statusCode);
            bool isOriginOk(const std::string& origin);
            void handleBanCommand(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleMute(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleAnnounceCommand(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request, bool isTip);
            void getOnlinePlayers(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleDisconnect(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleGetRooms(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleBreakroom(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleKick(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleHostChange(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleRoomTitleChange(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleGetRoomInfo(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleGetPlayerInfo(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleReward(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleCapsuleEvent(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleTradeEvent(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleEventMission(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);
            void handleExpMpBonus(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request);

            template<typename F>
            void handleGenericEventInfoUpdate(std::string& responseBody, http::status& statusCode, const http::request<http::string_body>& m_request,
                const std::string& startFieldName, const std::string& endFieldName, F updater)
            {
                try
                {
                    auto tokenOpt = extractToken(m_request);
                    if (!tokenOpt)
                    {
                        responseBody = "Invalid request (invalid JWT token provided)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    const std::string secret = "secret_temp";
                    if (!verifyTokenAndGrade(*tokenOpt, secret, 4))
                    {
                        responseBody = "Invalid request (invalid JWT token provided or too low grade)";
                        statusCode = http::status::unauthorized;
                        return;
                    }

                    auto body = boost::json::parse(m_request.body());
                    if (!body.is_object())
                    {
                        responseBody = "Invalid request (request body is not a JSON object)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    const auto& obj = body.as_object();

                    if (!obj.contains(startFieldName) || !obj.contains(endFieldName))
                    {
                        responseBody = "Invalid request (missing required fields)";
                        statusCode = http::status::bad_request;
                        return;
                    }

                    Main::Structures::EventMissionInfo info;
                    info.startDate = obj.at(startFieldName).as_uint64();
                    info.endDate = obj.at(endFieldName).as_uint64();

                    if (updater(info))
                    {
                        responseBody = "Event info updated successfully";
                        statusCode = http::status::ok;
                    }
                    else
                    {
                        responseBody = "Failed to update event info";
                        statusCode = http::status::bad_request;
                    }
                }
                catch (const std::exception& e)
                {
                    Utils::Logger::log("Error: " + std::string{ e.what() }, Utils::LogType::Error, "HttpSession::handleGenericEventInfoUpdate");
                    responseBody = "Server error while processing the request";
                    statusCode = http::status::internal_server_error;
                }
            }
        };
    }
}

#endif
