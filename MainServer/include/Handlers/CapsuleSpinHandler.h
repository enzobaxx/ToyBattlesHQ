#ifndef CAPSULE_SPIN_HANDLER_H
#define CAPSULE_SPIN_HANDLER_H

#include "../Network/MainSession.h"
#include "../../include/Structures/AccountInfo/MainAccountInfo.h"
#include "Network/Packet.h"
#include "../Structures/Capsule/CapsuleSpin.h"
#include <random>
#include "../Detail/CdbUtils.h"

namespace Main
{
	namespace Handlers
	{
		inline std::optional<std::pair<std::uint32_t, std::uint32_t>> itemSelectionAlgorithm(std::uint32_t gi_infoid, std::uint32_t gi_price, std::uint32_t gi_type,
			bool isLuckySpin)
		{
			static const std::array<double, 3> averageSpinCostByCurrency = Main::CdbUtils::getAverageSpinCostByCurrency();

			if (isLuckySpin)
			{
				static std::mt19937 rng(std::random_device{}()); 
				std::uniform_int_distribution<int> dist(0, 1);
				const std::uint32_t selected = (dist(rng) == 0) ? 5336503 : 5336504; // silver / gold lucky box
				return std::pair{ selected, 0 };
			}

			double averageSpinCost = 0.0;
			if (gi_type < averageSpinCostByCurrency.size())
			{
				averageSpinCost = averageSpinCostByCurrency[gi_type];
			}
			else
			{
				return std::nullopt;
			}

			static thread_local std::random_device rd;
			static thread_local std::mt19937 gen(rd());

			if (auto items = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbCapsulePackageInfo>::getInstance().getEntriesFor(gi_infoid))
			{
				std::vector<double> probabilities;
				probabilities.reserve(items->size());
				std::vector<std::pair<std::uint32_t, std::uint32_t>> itemIds;
				itemIds.reserve(items->size());

				for (const auto& item : *items)
				{
					double probability = static_cast<double>(item.gi_prob) / 200'000 * 100.0;

					if (item.gi_type == 1) 
					{
						if (gi_price > averageSpinCost)
							probability *= (1.0 + Common::Constants::capsulePriceFactor * 2.0);
						else if (gi_price < averageSpinCost)
							probability *= (1.0 - Common::Constants::capsulePriceFactor * 1.5);

						probability += 0.5; 
					}

					probabilities.push_back(probability);
					itemIds.emplace_back(item.gi_itemid, item.gi_type);
				}

				std::discrete_distribution<int> itemDistribution(probabilities.begin(), probabilities.end());
				auto wonItemPair = itemIds[itemDistribution(gen)];
				return wonItemPair;
			}
			return std::nullopt;
		}


		inline void removeCurrencyByCapsuleType(
			const std::shared_ptr<Main::Network::Session>& session,
			const Main::Structures::AccountInfo& accountInfo,
			Main::Enums::CapsuleCurrencyType capsuleCurrencyType,
			std::uint32_t toRemove,
			const Main::Structures::CapsuleListDatabase& capsuleSaleEvent)
		{
			auto safeSubtract = [](std::uint32_t balance, std::uint32_t remove) {
				return (remove >= balance) ? 0u : balance - remove;
				};

			const std::uint64_t now = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

			const bool inSalePeriod = (now >= capsuleSaleEvent.saleEventStartDate && now <= capsuleSaleEvent.saleEventEndDate);

			switch (capsuleCurrencyType)
			{
			case Main::Enums::CapsuleCurrencyType::CAPSULE_COINS:
				session->setAccountCoins(safeSubtract(accountInfo.coins, toRemove));
				break;

			case Main::Enums::CapsuleCurrencyType::CAPSULE_ROCKTOTENS:
				if (inSalePeriod)
					toRemove = capsuleSaleEvent.newRtPrice;
				session->setAccountRockTotens(safeSubtract(accountInfo.rockTotens, toRemove));
				break;

			case Main::Enums::CapsuleCurrencyType::CAPSULE_MICROPOINTS:
			default:
				if (inSalePeriod)
					toRemove = capsuleSaleEvent.newMpPrice;
				session->setAccountMicroPoints(safeSubtract(accountInfo.microPoints, toRemove));
				break;
			}
		}


		inline void handleCapsuleSpin(const Common::Network::Packet& request, std::shared_ptr<Main::Network::Session> session,
			Main::Network::SessionsManager& sessionsManager,
			const Main::ClientData::CapsuleSpin& capsuleSpinData,
			const Main::Structures::CapsuleListDatabase& capsuleSaleEvent)
		{
			START_BENCHMARK

			Common::Network::Packet response;
			response.setTcpHeader(request.getSession(), Common::Enums::USER_LARGE_ENCRYPTION);
			response.setCommand(request.getOrder(), Main::Enums::CapsuleSpinMission::CAPSULE_NORMAL_SPIN, Main::Enums::CapsuleSpinExtra::CAPSULE_SPIN_SUCCESS, 1);

			if (auto capsuleInfo = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbCapsuleInfo>::getInstance().getEntry(capsuleSpinData.capsuleId))
			{
				const auto& accountInfo = session->getAccountInfo();
				const auto totalSpins = request.getOption();

				const std::uint64_t now = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
				const bool inSalePeriod = (now >= capsuleSaleEvent.saleEventStartDate && now <= capsuleSaleEvent.saleEventEndDate);
				std::uint32_t pricePerSpin = capsuleInfo->gi_price;

				if (inSalePeriod)
				{
					switch (capsuleInfo->gi_type)
					{
					case Main::Enums::CapsuleCurrencyType::CAPSULE_ROCKTOTENS:
						pricePerSpin = capsuleSaleEvent.newRtPrice;
						break;
					case Main::Enums::CapsuleCurrencyType::CAPSULE_MICROPOINTS:
						pricePerSpin = capsuleSaleEvent.newMpPrice;
						break;
					default:
						break;
					}
				}

				const std::uint32_t totalPrice = pricePerSpin * totalSpins;
				bool notEnoughCurrency = false;

				switch (capsuleInfo->gi_type)
				{
				case Main::Enums::CapsuleCurrencyType::CAPSULE_COINS:
					notEnoughCurrency = accountInfo.coins < totalPrice;
					break;
				case Main::Enums::CapsuleCurrencyType::CAPSULE_ROCKTOTENS:
					notEnoughCurrency = accountInfo.rockTotens < totalPrice;
					break;
				case Main::Enums::CapsuleCurrencyType::CAPSULE_MICROPOINTS:
					notEnoughCurrency = accountInfo.microPoints < totalPrice;
					break;
				default:
					break;
				}

				if (notEnoughCurrency)
				{
					response.setExtra(Main::Enums::CapsuleSpinExtra::CAPSULE_SPIN_NOT_ENOUGH_CURRENCY);
					response.setOption(capsuleInfo->gi_type);
					session->asyncWrite(response);
					return;
				}
				else if (!session->getPlayer().getInventory().hasEnoughInventorySpace(totalSpins))
				{
					response.setExtra(Main::Enums::CapsuleSpinExtra::CAPSULE_SPIN_INVENTORY_FULL);
					session->asyncWrite(response);
					return;
				}

				std::size_t i = 0;
				if (request.getOption() > 20) return; 

				constexpr std::size_t maxRetries = 10;
				std::size_t retryCount = 0;
				Main::Enums::CapsuleSpinMission lastMission = Main::Enums::CapsuleSpinMission::CAPSULE_NORMAL_SPIN;

				while (i < request.getOption() && retryCount < maxRetries)
				{
					if (session->getPlayer().getLuckyPoints() >= Common::Constants::maxLuckySpin)
					{
						session->setLuckyPoints(0);
						response.setMission(Main::Enums::CapsuleSpinMission::CAPSULE_LUCKY_SPIN);
					}
					else
					{
						response.setMission(Main::Enums::CapsuleSpinMission::CAPSULE_NORMAL_SPIN);
					}

					if (lastMission == Main::Enums::CapsuleSpinMission::CAPSULE_LUCKY_SPIN &&
						response.getMission() == Main::Enums::CapsuleSpinMission::CAPSULE_LUCKY_SPIN)
					{
						response.setMission(Main::Enums::CapsuleSpinMission::CAPSULE_NORMAL_SPIN);
					}
					lastMission = static_cast<Main::Enums::CapsuleSpinMission>(response.getMission());

					const auto wonItemIdAndType = itemSelectionAlgorithm(capsuleInfo->gi_infoid, capsuleInfo->gi_price,
						capsuleInfo->gi_type >= 3 ? 1 : capsuleInfo->gi_type, response.getMission() == Main::Enums::CapsuleSpinMission::CAPSULE_LUCKY_SPIN);
					if (!wonItemIdAndType || !Main::CdbUtils::itemExists(wonItemIdAndType->first))
					{
						response.setExtra(Main::Enums::CapsuleSpinExtra::CAPSULE_SPIN_FAIL);
						response.setMission(1);
						session->asyncWrite(response);
						return;
					}
					if (wonItemIdAndType->second == 1)
					{ // rare item
						if (auto optName = CdbUtils::getItemName(wonItemIdAndType->first); optName)
						{
							std::string_view itemNameView{ *optName };
							itemNameView = itemNameView.substr(0, itemNameView.find('\0'));
							sessionsManager.broadcastMessageToLobby("[" + std::string{ session->getAccountInfo().nickname } + "] won a [" + std::string{ itemNameView }
								+ "] item from the capsule machine."
							);
						}
					}

					Main::Structures::ItemSerialInfo serialInfo;
					serialInfo.itemNumber = session->getPlayer().getInventory().getLatestItemNumber() + 1;
					Main::Structures::CapsuleSpin capsuleSpin{ wonItemIdAndType->first, serialInfo };
					session->getPlayer().getInventory().setLatestItemNumber(capsuleSpin.itemSerialInfo.itemNumber);
					Main::Structures::Item capsuleItem{ capsuleSpin };
					if (session->addItem(capsuleItem))
					{
						session->logItemInfo(capsuleItem.serialInfo.itemNumber, capsuleItem.itemId.itemId, capsuleItem.expirationDate,
							"Item won through the capsule machine");
					}
					else
					{
						session->sendMessage("Error while getting the item from the capsule machine - please report this issue!");
						++retryCount;
						continue;
					}
				
					response.setData(reinterpret_cast<std::uint8_t*>(&capsuleSpin), sizeof(Main::Structures::CapsuleSpin));
					session->asyncWrite(response);
					if (response.getMission() != Main::Enums::CapsuleSpinMission::CAPSULE_LUCKY_SPIN)
					{ // lucky spin is free 
						removeCurrencyByCapsuleType(session, accountInfo, static_cast<Main::Enums::CapsuleCurrencyType>(capsuleInfo->gi_type), capsuleInfo->gi_price, capsuleSaleEvent);
						session->addLuckyPoints(capsuleInfo->gi_luckypoint);
						++i;
					}
					if (request.getOption() == 1 && response.getMission() == Main::Enums::CAPSULE_LUCKY_SPIN)
					{ // single lucky spin
						break;
					}
				}
			}
			else
			{
				response.setExtra(Main::Enums::CapsuleSpinExtra::CAPSULE_SPIN_FAIL);
				response.setMission(1);
				session->asyncWrite(response);
			}

			END_BENCHMARK(handleCapsuleSpin, session)
		}
	}
}

#endif