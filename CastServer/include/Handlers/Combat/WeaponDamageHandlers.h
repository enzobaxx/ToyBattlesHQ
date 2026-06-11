#ifndef CAST_WEAPON_DAMAGE_HANDLERS_H
#define CAST_WEAPON_DAMAGE_HANDLERS_H

#include "Detail/IpcUtils.h"
#include "Handlers/Room/ArenaModeHandler.h"
#include "Managers/RoomsManager.h"
#include "Network/SessionsManager.h"
#include "Structures/Match/SuicideStruct.h"
#include "Structures/Player/PlayerPositionFromServer.h"
#include "Detail/Utilities.h"
#include <AntiCheat/AntiCheat.h>

namespace Cast
{
	namespace Handlers
	{
		inline bool handleAssassinMode(Cast::Classes::RoomsManager& roomsManager, std::shared_ptr<Cast::Classes::Room> room, const Main::Structures::UniqueId& attackerUid,
			const Main::Structures::UniqueId& targetUid,
			std::shared_ptr<Cast::Network::Session> session, std::shared_ptr<Cast::Network::Session> targetSession)
		{
			if (room->m_assassinBlueUid == targetUid || room->m_assassinRedUid == targetUid)
			{
				room->killTeam(room->m_assassinBlueUid == targetUid ? Common::Enums::TEAM_BLUE : Common::Enums::TEAM_RED);
				room->broadcastMessage("The assassin of team " +
					(room->m_assassinBlueUid == targetUid ? std::string("BLUE") : std::string("RED")) + " was killed by " + session->m_nickname + "!");
				return true;
			}
			else
			{
				if (attackerUid == room->m_assassinBlueUid || attackerUid == room->m_assassinRedUid)
				{
					targetSession->sendMessage("You were killed by the assassin! Wait until next round.");
					return true;
				}
				else
				{
					Common::Network::UnecryptedPacket response;
					response.setTcpHeader(session->getId());
					response.setOrder(276);
					Cast::Structures::PlayerRespawnPosition position;
					position.targetUniqueId = targetUid;
					if (targetSession->m_team == Common::Enums::TEAM_RED || targetSession->m_team == Common::Enums::TEAM_BLUE)
					{
						if (auto posOpt = roomsManager.getPositionFor(session->getId(), targetSession->m_team))
						{
							position = *posOpt;
						}
						else return true;
					}
					else return true;

					response.setData(reinterpret_cast<std::uint8_t*>(&position), sizeof(position));
					roomsManager.broadcastToMatch(session->getId(), response);
					return false;
				}
			}
		}

		// Returns false if the caller should return early (e.g. assassin position update was sent instead of a kill).
		inline bool handleTargetDeath(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
			std::shared_ptr<Cast::Network::Session> targetSession,
			Cast::Classes::RoomsManager& roomsManager, std::shared_ptr<Cast::Classes::Room>& room,
			const Main::Structures::UniqueId& attackerUid, const Main::Structures::UniqueId& targetUid,
			std::shared_ptr<Common::Network::Session> ipcSession)
		{
			if (room->m_isAssassinMode)
			{
				if (!handleAssassinMode(roomsManager, room, attackerUid, targetUid, session, targetSession)) return false;
				roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
			}
			else
			{
				if (!targetSession->isDead) roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				targetSession->isDead = true;
				if (room->isArenaMode())
				{
					handleArenaMode(roomsManager, room);
					return false;
				}
				if (Cast::Details::mustBroadcastDeath(roomsManager.getModeOf(session->getId())))
				{
					sendPlayerStateUpdate(targetSession->getAccountId(), true, ipcSession);
				}
			}
			return true;
		}

		inline void handleNormalWeaponDamage(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
			Cast::Classes::RoomsManager& roomsManager, Cast::Network::SessionsManager& sessionsManager, Ac::AntiCheatManager& acManager,
			std::shared_ptr<Common::Network::Session> ipcSession)
		{
			auto roomOpt = roomsManager.getRoom(session->getId());
			if (!roomOpt)
			{
				session->asyncWrite(const_cast<Common::Network::UnecryptedPacket&>(request));
				return;
			}
			auto& room = *roomOpt;

			const auto attackerUid = Cast::Details::parseData<Main::Structures::UniqueId>(request, 16);
			const auto targetUid = Cast::Details::parseData<Main::Structures::UniqueId>(request, 20);
			const std::uint16_t targetHp = Cast::Details::parseData<std::uint16_t>(request, 24);

			if (room->getMode() == Common::Enums::AiBattle || room->getMode() == Common::Enums::BossBattle
				|| request.getDataSize() == 12)
			{
				roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				return;
			}

			auto attackerSession = sessionsManager.getSession(attackerUid.session);
			if (attackerSession &&
				(attackerSession->m_team == Common::Enums::TEAM_OBSERVER || !attackerSession->m_isInMatch))
			{
				return;
			}

			if (auto targetSession = sessionsManager.getSession(targetUid.session))
			{
				if (targetHp)
				{
					if (!targetSession->isDead)
						roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
				else
				{
					if (!handleTargetDeath(request, session, targetSession, roomsManager, room, attackerUid, targetUid, ipcSession)) return;
				}
			}
		}

		inline void handleSpecialWeaponDamage(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
			Cast::Classes::RoomsManager& roomsManager,
			Cast::Network::SessionsManager& sessionsManager,
			Ac::AntiCheatManager& acManager,
			std::shared_ptr<Common::Network::Session> ipcSession)
		{
			auto roomOpt = roomsManager.getRoom(session->getId());
			if (!roomOpt)
			{
				session->asyncWrite(const_cast<Common::Network::UnecryptedPacket&>(request));
				return;
			}
			auto& room = *roomOpt;

			const std::uint16_t targetHp = Cast::Details::parseDataFromEnd<std::uint16_t>(request, 6);
			const auto targetUid = Cast::Details::parseDataFromEnd<Main::Structures::UniqueId>(request, 8);
			const auto attackerUid = Cast::Details::parseData<Main::Structures::UniqueId>(request, 16);

			if (room->getMode() == Common::Enums::AiBattle || room->getMode() == Common::Enums::BossBattle)
			{
				roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				return;
			}

			auto attackerSession = sessionsManager.getSession(attackerUid.session);
			if (attackerSession &&
				(attackerSession->m_team == Common::Enums::TEAM_OBSERVER || !attackerSession->m_isInMatch))
			{
				return;
			}

			if (auto targetSession = sessionsManager.getSession(targetUid.session))
			{
				if (targetHp)
				{
					if (!targetSession->isDead)
						roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
				}
				else
				{
					if (!handleTargetDeath(request, session, targetSession, roomsManager, room, attackerUid, targetUid, ipcSession)) return;
				}
			}
		}

		inline void handleExplosiveDamage(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Cast::Network::Session> session,
			Cast::Classes::RoomsManager& roomsManager,
			Cast::Network::SessionsManager& sessionsManager,
			std::shared_ptr<Common::Network::Session> ipcSession)
		{
			auto roomOpt = roomsManager.getRoom(session->getId());
			if (!roomOpt)
			{
				session->asyncWrite(const_cast<Common::Network::UnecryptedPacket&>(request));
				return;
			}
			auto& room = *roomOpt;

			if (request.getOption() == 0 || room->getMode() == Common::Enums::AiBattle || room->getMode() == Common::Enums::BossBattle)
			{
				roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
			}

			for (std::uint32_t i = 0; i < request.getOption(); ++i)
			{
				const auto targetUid = Cast::Details::parseData<Main::Structures::UniqueId>(request, 12 * (i + 1));
				const auto targetNewHp = Cast::Details::parseData<std::uint16_t>(request, 12 * (i + 1) + sizeof(Main::Structures::UniqueId));
				if (auto targetSession = sessionsManager.getSession(targetUid.session))
				{
					if (targetNewHp)
					{
						if (!targetSession->isDead)
							roomsManager.broadcastToMatch(session->getId(), const_cast<Common::Network::UnecryptedPacket&>(request));
					}
					else
					{
						if (!handleTargetDeath(request, session, targetSession, roomsManager, room, Main::Structures::UniqueId{ 0, 0, 1 }, targetUid, ipcSession)) return;
					}
				}
			}
		}
	}
}

#endif
