#ifndef SESSION_HEADER
#define SESSION_HEADER

#include <asio.hpp>
#include <array>
#include <vector>
#include <unordered_map>
#include <functional>
#include <utility>
#include <type_traits>
#include <source_location>

#include "../Cryptography/Crypt.h"
#include "../Enums/MiscellaneousEnums.h"

#include "Packet.h"
#include "SessionIdManager.h"
#include <queue>
#include "../Enums/GameEnums.h"

#ifdef ENABLE_BENCHMARKING
#include <chrono>
#include <boost/system/detail/throws.hpp>
#define START_BENCHMARK \
        auto start = std::chrono::high_resolution_clock::now();

#define END_BENCHMARK(functionName, session) \
        auto end = std::chrono::high_resolution_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count(); \
        session->sendMessage("[" #functionName "] took microseconds: " + std::to_string(duration));
#else
#define START_BENCHMARK
#define END_BENCHMARK(functionName, session)
#endif

namespace Common
{
	namespace Network
	{
		using tcp = asio::ip::tcp;

		class Session : public std::enable_shared_from_this<Session>
		{
		protected:
			tcp::socket m_socket;
			std::array<std::uint8_t, 1024> m_buffer{};
			std::vector<std::uint8_t> m_reader{};
			Common::Cryptography::Crypt m_crypt{};
			Common::Cryptography::Crypt m_defaultCrypt{};
			std::function<void(std::size_t)> m_onCloseSocketCallback{};
			std::size_t m_id = 0;
			std::uint32_t m_aid = 0;
			std::string m_ip;
			std::string m_gradedIp{ "" };
			std::uint_least16_t m_port;
			std::deque<std::vector<std::uint8_t>> m_writeQueue{};
			bool m_writing{ false };

		public:
			std::string m_hwid = "";
			bool m_checkValidSession{true};
			bool m_isValidSession{};
			bool m_isFirstRead{ true };
			static inline std::unordered_set<std::string> s_loggedIps;
			static inline std::deque<std::string> ipQueue;

			template<PacketType T>
			using PacketTypeTrait = std::conditional_t<T == PacketType::ENCRYPTED, Common::Network::Packet, Common::Network::UnecryptedPacket>;

			template<PacketType packetType, class T>
			inline static std::unordered_map<std::uint16_t, std::function<void(const PacketTypeTrait<packetType>&, std::shared_ptr<T>)>> callbacks;

			inline static std::size_t id_counter = 1;
			inline static SessionIdManager sessionIdManager{ 500 };

		public:
			Session() = default;

			explicit Session(tcp::socket&& socket, std::function<void(std::size_t)> fnct)
				: m_socket{ std::move(socket) }
				, m_onCloseSocketCallback{ fnct }
			{
				m_reader.reserve(Common::Constants::maxPacketBytes);
				m_socket.set_option(asio::ip::tcp::no_delay(true));

				try
				{
					m_ip = m_socket.remote_endpoint().address().to_string();
					m_port = m_socket.local_endpoint().port();
				}
				catch (const std::exception& e)
				{
					m_ip = "Unknown";
					m_port = 0;
				}

				auto newID = sessionIdManager.getNewSessionID();
				if (newID.has_value())
				{
					m_id = newID.value();
				}
				else
				{
					closeSocket();
				}
			}


			virtual ~Session()
			{
				closeSocket();
			}

			void setSessionId(std::size_t id)
			{
				if (m_id != id)
				{
					sessionIdManager.releaseSessionID(m_id);
					m_id = id;
				}
			}

			void setAccountId(std::uint32_t accountId)
			{
				m_aid = accountId;
			}

			std::uint32_t getAccountId() const noexcept { return m_aid; }

			void sendMessage(const std::string& message, std::uint32_t extra = 1);
			bool asyncWrite(const Common::Network::Packet& message);
			bool asyncWrite(const Common::Network::UnecryptedPacket& message);
			void asyncRead();
			void onRead(asio::error_code error, std::size_t bytes_transferred);
			void closeSocket();
			virtual void onPacket(std::vector<std::uint8_t>& data);
			std::uint8_t* getBufferData();
			std::size_t getBufferSize() const;
			Common::Cryptography::Crypt getUserCrypt() const;
			Common::Cryptography::Crypt getDefaultCrypt() const;
			std::size_t getId() const;
			bool is_open() const {
				return m_socket.is_open();
			}
			void sendConnectionACK(Common::Enums::ServerType serverType);
			void flushWriteQueue();

			template<PacketType packetType, typename Packet>
			bool asyncWriteImpl(const Packet& message)
			requires (packetType == PacketType::ENCRYPTED || packetType == PacketType::UNECRYPTED)
			{
				if (!m_socket.is_open())
				{
					return false;
				}

				std::vector<std::uint8_t> packetData;

				if constexpr (packetType == PacketType::ENCRYPTED)
				{
					if (!message.isValidMain())
					{
						return false;
					}
					packetData = message.generateOutgoingPacket(m_crypt.UserKey, m_crypt.isUsed);
				}
				else if constexpr (packetType == PacketType::UNECRYPTED)
				{
					if (!message.isValidCast())
					{
						return false;
					}
					packetData = message.generateOutgoingPacket();
				}

				m_writeQueue.push_back(std::move(packetData));
				if (!m_writing)
				{
					flushWriteQueue();
				}

				return true;
			}

			template<PacketType packetType = PacketType::ENCRYPTED, typename T>
			static inline void addCallback(std::uint16_t idx, std::function<void(const PacketTypeTrait<packetType>&, std::shared_ptr<T>)> fnct)
			{
				callbacks<packetType, T>[idx] = std::move(fnct);
			}

			const std::string& getIp() const noexcept { return m_ip; }
			void setGradedIp(const std::string& ip) { m_gradedIp = ip; }
			void setIpFromGraded() { m_ip = m_gradedIp; }
			const std::uint_least16_t getPort() const noexcept { return m_port; }
		};

	}
}

#endif
