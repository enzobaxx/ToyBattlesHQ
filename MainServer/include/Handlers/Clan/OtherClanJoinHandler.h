

#ifndef OTHER_CLAN_JOIN_HANDLER_H
#define OTHER_CLAN_JOIN_HANDLER_H

#include "Network/Packet.h"
#include "../../Classes/PartiesManager.h"
#include "../Room/ClanRoomCreation.h"
#include "../../Network/MainSession.h"
#include "../../Classes/RoomsManager.h"
#include "../../Structures/Room/RoomJoinLatestInfo.h"
#include <Enums/PlayerEnums.h>
#include "../Room/RoomJoinHandler.h"
#include <cstring> 

namespace Main
{
	namespace Handlers
	{
		enum OtherClanJoinExtra
		{
			ERROR1 = 1, // you may not battle with this clan
			TARGET_CLAN_IN_BATTLE = 5, // This clan is currently in a battle
			TARGET_CLAN_NOT_FOUND = 6, // The clan that created this match doesn't exist
			ERROR4 = 35 // you may not battle with this clan
		};


		inline void handleClanRoomJoin(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session, Main::Classes::RoomsManager& roomsManager,
			std::uint32_t team)
		{
			const Main::ClientData::RoomInfo requestStructure = Details::parseData<Main::ClientData::RoomInfo>(request); 
			const bool hasInputtedPassword = request.getDataSize() == 20;

			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);
			response.setOrder(request.getOrder());

			const auto& accountInfo = session->getAccountInfo();

			if (Main::Classes::Room* room = roomsManager.getRoomByNumber(requestStructure.roomNumber + 1))
			{
				Main::Structures::RoomJoin roomInfo = room->getRoomJoinInfo();
				roomInfo.isClanMatch = true;
				const auto& roomSettings = room->getRoomSettings();

				if (hasInputtedPassword && roomSettings.hasPassword && room->getPassword() != std::string{ requestStructure.password })
				{
					response.setExtra(RoomJoinExtra::JOIN_INVALID_PASSWORD);
					session->asyncWrite(response);
					return;
				}

				// Extra + Option = uint16_t = Room Number
				response.setCommand(RoomJoinOrder::RoomInfo, room->getRoomInfo().hasPassword, room->getRoomInfo().roomNumber, 0);
				response.setData(reinterpret_cast<std::uint8_t*>(&roomInfo), sizeof(roomInfo));
				session->asyncWrite(response);

				// Player infos
				auto allPlayers = room->getAllPlayers();
				response.setCommand(RoomJoinOrder::RoomPlayersInfos, room->getRoomInfo().hasPassword, 37, allPlayers.size());
				response.setData(reinterpret_cast<std::uint8_t*>(allPlayers.data()), sizeof(Main::Structures::RoomPlayerInfo) * allPlayers.size());
				session->asyncWrite(response);

				// Player items
				response.setOrder(RoomJoinOrder::RoomPlayersItems);
				auto allPlayersItems = room->getPlayersItems();
				std::size_t totalSize = Common::Constants::headerSize + sizeof(Main::Structures::RoomPlayerItems) * allPlayersItems.size();
				if (totalSize < Common::Constants::maxPacketBytes)
				{
					response.setOption(allPlayersItems.size());
					response.setExtra(37);
					response.setData(reinterpret_cast<std::uint8_t*>(allPlayersItems.data()), sizeof(Main::Structures::RoomPlayerItems) * allPlayersItems.size());
					session->asyncWrite(response);
				}
				else
				{
					std::size_t currentItemIndex = 0;
					std::uint16_t packetExtra = 0;
					const std::size_t maxPayloadSize = Common::Constants::maxPacketBytes - Common::Constants::headerSize;
					const std::size_t itemsToSend = maxPayloadSize / sizeof(Main::Structures::RoomPlayerItems);

					while (currentItemIndex < allPlayersItems.size())
					{
						std::vector<Main::Structures::RoomPlayerItems> packetItems(
							allPlayersItems.begin() + currentItemIndex,
							allPlayersItems.begin() + std::min(currentItemIndex + itemsToSend, allPlayersItems.size())
						);
						packetExtra = currentItemIndex == 0 ? 37 : 0;
						response.setExtra(packetExtra);
						response.setOption(packetItems.size());
						response.setData(reinterpret_cast<std::uint8_t*>(packetItems.data()), packetItems.size() * sizeof(Main::Structures::RoomPlayerItems));
						session->asyncWrite(response);
						currentItemIndex += itemsToSend;
					}
				}

				// Player clan infos; option => again num of players with clans 
				auto allPlayersClans = room->getPlayersClans();
				response.setCommand(RoomJoinOrder::RoomPlayersClans, 0, 37, allPlayersClans.size());
				response.setData(reinterpret_cast<std::uint8_t*>(allPlayersClans.data()), sizeof(Main::Structures::PlayerClan) * allPlayersClans.size());
				session->asyncWrite(response);

				// Success in joining the room
				response.setCommand(request.getOrder(), 0, RoomJoinExtra::JOIN_SUCCESS, 0);
				response.setData(nullptr, 0);
				// extra 0, mission 0 ==> observer mode
				// extra 0, mission 1 ==> seemingly invisible mode?!
				session->asyncWrite(response);

				// Check if this is needed for clan-room join
				response.setCommand(RoomJoinOrder::RoomLatestInfo, 0, 0, roomSettings.mode);
				response.setData(nullptr, 0);
				if (roomSettings.mode == Common::Enums::TeamDeathMatch || roomSettings.mode == Common::Enums::ItemMatch
					|| roomSettings.mode == Common::Enums::CloseCombat || roomSettings.mode == Common::Enums::SuperItemMatch
					|| roomSettings.mode == Common::Enums::SniperMode || roomSettings.mode == Common::Enums::AiBattle
					|| roomSettings.mode == Common::Enums::Clan_CaptureTheBattery || roomSettings.mode == Common::Enums::CaptureTheBattery
					|| roomSettings.mode == Common::Enums::Clan_TeamDeathMatch || roomSettings.mode == Common::Enums::Clan_Elimination
					|| roomSettings.mode == Common::Enums::Elimination || roomSettings.mode == Common::Enums::ZombieMode)
				{
					Main::Structures::ModeInfoTDM info;
					info.state = room->hasMatchStarted() ? 3 : 0;
					info.timelimited = roomSettings.time;
					info.weaponlimited = roomSettings.weaponRestriction;
					info.winrule = room->getSpecificSetting();
					info.kitdrop = roomSettings.isItemOn;
					response.setData(reinterpret_cast<std::uint8_t*>(&info), sizeof(info));
					session->asyncWrite(response);
				}
				else if (roomSettings.mode == Common::Enums::BombBattle || roomSettings.mode == Common::Enums::Clan_BombBattle
					|| roomSettings.mode == Common::Enums::BossBattle || roomSettings.mode == Common::Enums::AiBattle)
				{ // Temporary fix, can't seem to find these structs
					response.setOrder(125);
					response.setMission(0);
					response.setExtra(0);
					response.setOption(roomSettings.mode);
					auto settings = room->getRoomSettingsUpdate();
					response.setData(reinterpret_cast<std::uint8_t*>(&settings), sizeof(settings));
					session->asyncWrite(response);
				}

				// Send the info of the player that just joined to the whole room
				Main::Structures::RoomLatestEnteredPlayerInfo latestEnteredPlayerInfo;
				latestEnteredPlayerInfo.character = accountInfo.latestSelectedCharacter;
				latestEnteredPlayerInfo.level = accountInfo.playerLevel;
				latestEnteredPlayerInfo.ping = session->getPlayer().getPing();
				latestEnteredPlayerInfo.uniqueId = accountInfo.uniqueId;
				std::memcpy(latestEnteredPlayerInfo.playerName, accountInfo.nickname, Common::Constants::maxNicknameSize);
				auto separatedItems = session->getPlayer().getEquippedItemsSeparated(); // first=items, second=weapons
				latestEnteredPlayerInfo.equippedItems = separatedItems.first;
				latestEnteredPlayerInfo.equippedWeapons = separatedItems.second;
				latestEnteredPlayerInfo.team = team;
				response.setCommand(RoomJoinOrder::RoomLatestEnteredPlayerInfo, 0, 0, 0);
				response.setData(reinterpret_cast<std::uint8_t*>(&latestEnteredPlayerInfo), sizeof(latestEnteredPlayerInfo));
				room->broadcastToRoom(response);

				// Latest Entered player clan info
				// Send the clan info of the player that just joined to the whole room
				Main::Structures::PlayerClan playerClan;
				if (accountInfo.clanId > 8)
				{
					playerClan.clanLogoBackId = accountInfo.clanLogoBackId;
					playerClan.clanLogoFrontId = accountInfo.clanLogoFrontId;
					std::memcpy(playerClan.clanName, accountInfo.clanName, 16);
					playerClan.unknown2 = accountInfo.clanId;
					playerClan.playerRoomIdx = allPlayers.size();

					response.setCommand(RoomJoinOrder::RoomPlayersClans, 0, 37, 1);
					response.setData(reinterpret_cast<std::uint8_t*>(&playerClan), sizeof(Main::Structures::PlayerClan));
					room->broadcastToRoomExceptSelf(response, accountInfo.uniqueId);
				}

				room->addPlayer(session, latestEnteredPlayerInfo.team);
			}
			else
			{
				response.setExtra(RoomJoinExtra::JOIN_ROOM_IS_DELETED);
				session->asyncWrite(response);
			}
		}

		// Reviewed 22.04.2026
		inline void handleOtherClanJoin(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Classes::PartiesManager& partiesManager, Main::Classes::RoomsManager& roomsManager)
		{
			Common::Network::Packet response = request;
			response.setTcpHeader(request.getSession(), Common::Enums::NO_ENCRYPTION);

			const Main::ClientData::ClanRoomInfo requestStructure = Details::parseData<Main::ClientData::ClanRoomInfo>(request); 
			const auto& ainfo = session->getAccountInfo();
			const std::uint16_t selfPartyRoomNumber = session->getPlayer().getPartyRoomNumber();

			if (auto targetPartyRoom = partiesManager.getExactRoomFor(requestStructure.clanId, requestStructure.roomNumber);
				auto selfPartyRoom = partiesManager.getExactRoomFor(ainfo.clanId, selfPartyRoomNumber))
			{
				if (targetPartyRoom->getClanId() == session->getAccountInfo().clanId)
				{
					response.setExtra(OtherClanJoinExtra::ERROR1);
					session->asyncWrite(response);
				}
				else if (!targetPartyRoom->isRegistered() || targetPartyRoom->getClanMatchRoomNumber() >= Common::Constants::clanRoomNumberStart)
				{
					response.setExtra(OtherClanJoinExtra::TARGET_CLAN_IN_BATTLE);
					session->asyncWrite(response);
				}
				else if (targetPartyRoom->getPlayersSize() > selfPartyRoom->getPlayersSize())
				{
					response.setExtra(OtherClanJoinExtra::ERROR4);
					session->asyncWrite(response);
				}
				else if (auto otherTargetLeader = targetPartyRoom->getLeaderSession())
				{ 
					// the target leader will host the new room where clan vs clan happens
					Main::Structures::CompleteRoomInfo completeRoomInfo{ targetPartyRoom->getRoomSettings(), "" /* title, unused */ };
					Common::Network::Packet roomCreationRequest;
					roomCreationRequest.setTcpHeader(otherTargetLeader->getId(), Common::Enums::NO_ENCRYPTION);
					roomCreationRequest.setCommand(138, 0, targetPartyRoom->getSpecificSetting(), 0);
					roomCreationRequest.setData(reinterpret_cast<std::uint8_t*>(&completeRoomInfo), sizeof(completeRoomInfo));
					if (!Main::Handlers::handleRoomCreationClan(roomCreationRequest, otherTargetLeader, roomsManager))
					{
						session->sendMessage("Server error: Failed to create clan room for the target clan. Please report this issue if it persists");
						response.setExtra(OtherClanJoinExtra::TARGET_CLAN_NOT_FOUND);
						session->asyncWrite(response);
						return;
					}

					// Let all the other players join the room with the JoinRoom handler, where roomNumber = otherTargetLeader.getPlayer().getRoomNumber()
					const std::uint16_t roomNumber = otherTargetLeader->getPlayer().getRoomNumber();
					Main::ClientData::RoomInfo joinInfo{ roomNumber - 1, 2 };
					response.setCommand(140, 0, 0, 0);
					response.setData(reinterpret_cast<std::uint8_t*>(&joinInfo), sizeof(joinInfo));

					if (Main::Classes::Room* room = roomsManager.getRoomByNumber(roomNumber))
					{
						std::string targetClanName = "";
						std::string selfClanName = "";

						for (auto targetPlayer : targetPartyRoom->getPlayerSessions())
						{
							if (targetClanName.empty()) targetClanName = targetPlayer->getAccountInfo().clanName;
							if (targetPlayer->getId() == otherTargetLeader->getId()) continue; // the target leader is already in the room
							Main::Handlers::handleClanRoomJoin(response, targetPlayer, roomsManager, Common::Enums::TEAM_BLUE);
						}
						targetPartyRoom->setTeam(Common::Enums::TEAM_BLUE);

						for (auto player : selfPartyRoom->getPlayerSessions())
						{
							if (selfClanName.empty()) selfClanName = player->getAccountInfo().clanName;
							Main::Handlers::handleClanRoomJoin(response, player, roomsManager, Common::Enums::TEAM_RED);
						}
						selfPartyRoom->setTeam(Common::Enums::TEAM_RED);

						auto result = partiesManager.addClanMatch(roomNumber, selfPartyRoom, targetPartyRoom);
						if (!result)
						{
							room->broadcastMessage("ERROR: " + result.error() + " - Continuing this clan match may result in crashes! Report this issue!");
						}
						room->setRoomTitle(std::string{ "CW: " + targetClanName + " VS " + selfClanName });
					}
					else
					{
						session->sendMessage("Server error: the room that was just created was not found. Please report this issue.");
						response.setExtra(OtherClanJoinExtra::TARGET_CLAN_NOT_FOUND);
						session->asyncWrite(response);
					}
				}
				else
				{
					session->sendMessage("Server error: target leader session was not found. Please report this issue if it persists");
					response.setExtra(OtherClanJoinExtra::TARGET_CLAN_NOT_FOUND);
					session->asyncWrite(response);
				}
			}
		}
	}
}

#endif