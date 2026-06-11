#ifndef MAIN_ENUMERATIVES_H
#define MAIN_ENUMERATIVES_H

#include <cstdint>

namespace Main
{
	namespace Enums
	{
		enum class RoomType
		{
			Room,
			Clan
		};

		enum RoomSimpleSetting
		{
			SETTING_ITEM,
			SETTING_OPEN,
			SETTING_OBSERVER,
			SETTING_TEAMBALANCE,
			SETTING_MAP,
			SETTING_PLAYERS_PER_TEAM,
			SETTING_TIME,
			SETTING_SPECIFIC
		};

		enum MatchEnd
		{
			MATCH_WON,
			MATCH_LOST,
			MATCH_DRAW,
			MATCH_DO_NOTHING // e.g. FFA, ZM, boss battle, arms race, square mode
		};

		enum SellItemExtra
		{
			SELL_SUCCESS = 1,
			SELL_ERROR = 21,
		};

		enum MailboxExtra
		{
			MAILBOX_SENT = 0,
			RECEIVER_NOT_FOUND = 4,
			RECEIVER_NO_SPACE_LEFT = 8,
			SENDER_NO_SPACE_LEFT = 17,
			MAILBOX_RECEIVER_BLOCKED_SENDER = 42,
			MAILBOX_RECEIVED = 46,
			MAILBOX_HAS_BEEN_READ = 53,
			MAILBOX_DB_ERROR
		};

		enum MailboxMission
		{
			MISSION_MAILBOX_RECEIVED = 0,
			MISSION_MAILBOX_SENT = 1,
		};

		enum InitialPacketOption
		{
			CHANNEL1 = 1,
			CHANNEL2,
			CHANNEL3,
			CHANNEL4,
			CHANNEL5,
			CHANNEL6
		};

		enum PlayerGrade
		{
			GRADE_NORMAL = 1,
			GRADE_ES = 2,
			GRADE_MOD = 3,
			GRADE_TESTER = 4,
			GRADE_GM = 7,
		};

		enum class FriendLogType
		{
			LOGOUT = 0,
			LOGIN = 46,
		};

		enum class AuthorizationExtra
		{
			SUCCESS = 1,
			WRONG_CLIENT_VER_OR_SERVER_FULL_OR_OFFLINE = 6, 
			ID_AUTHORIZE_FULL = 7, // not sure why but the client ignores this as if we used SUCCESS
			INCORRECT_CLIENT_VERSION = 16,  // not sure why but the client ignores this as if we used SUCCESS
			AUTHORIZATION_FAILED = 35,  // "Your account authorization has failed (...) => back to login
			AUTHORIZATION_OFFLINE_SERVER = 47,  // not sure why but the client ignores this as if we used SUCCESS
		};

		enum ChatGrade
		{
			CHAT_NORMAL = 0,
			CHAT_MOD,
			CHAT_GM,
			CHAT_TESTER,
		};

		enum ChatExtra
		{
			NORMAL = 0,
			HELP = 1,
			WHISPER = 2,
			COMMAND = 3,
			TEAM = 5,
			CLAN = 7,
			INFO = 10,
			GM = 11,
			TIP = 12,
		};

		enum WhisperExtra
		{
			WHISPER_SENT = 1,
			WHISPER_SENDER_BLOCKED_RECEIVER = 2, // Whisper failed => Change to "Cannot whisper to a blocked player.
			RECEIVER_OFFLINE = 13,
			WHISPER_SELF = 15,
			WHISPER_RECEIVER_BLOCKED_SENDER = 35, // "Recipient has refused your whisper" => Change to "The player you're trying to whisper to has blocked you."
		};

		enum GiftSystemExtra
		{
			GIFT_RECEIVED_NOTICE = 46,
			RECEIVER_NOT_ENOUGH_SPACE = 8,
			GIFT_SENT_OK = 37,
			SENDER_NOT_ENOUGH_CASH = 14,
			BLOCKED_ERROR = 42,
			GIFT_RECEIVER_OFFLINE = 4
		};

		enum ItemUpgradeExtra
		{
			UPGRADE_SUCCESS = 1,
			UPGRADE_FAIL = 6, // also 2
			ITEM_MUST_BE_REPAIRED_FIRST = 8,
			NOT_ENOUGH_MP_FOR_UPGRADE = 14,
			ENERGY_ADD = 32,
		};

		enum ID_QUIT
		{
			ID_QUIT_DEFAULT_ACCOUNT_BLOCKED = 0,// ID_QUIT_LOCK
			ID_QUIT_DATAERROR = 4,
			ID_QUIT_BUSY = 5,
			ID_QUIT_UNKNOWN_ERROR = 27,         // ID_QUIT_CLOSE
			ID_QUIT_REMOVED_BYMOD = 35,         // ID_QUIT_DENY
			ID_QUIT_MULTIPLE_USERS = 42,        // ID_QUIT_BLOCK
			ID_QUIT_MAINTENANCE = 47            // ID_QUIT_OFFLINE
		};

		enum WeaponUpgradeType
		{
			NOT_UPGRADE = 0,
			TYPE_1,
			TYPE_2,
			TYPE_3,
			UPGRADE_TYPE_MAX = 4
		};

		enum ItemCurrencyType
		{
			ITEM_MP = 0,
			ITEM_RT,
			ITEM_COUPON,
			ITEM_COIN,
			TOTAL_CURRENCIES,
		};

		enum CashItem 
		{
			MP_100 = 4400001,
			MP_500 = 4400005,
			MP_1000 = 4400010,
			MP_2000 = 4400020,
			MP_3000 = 4400030,
			MP_3500 = 4400035,
			MP_4000 = 4400040,
			MP_5000 = 4400050,
			MP_6000 = 4400060,
			MP_7000 = 4400070,
			MP_8000 = 4400080,
			MP_9000 = 4400090,
			MP_10000 = 4400100,
			MP_20000 = 4400200,
			MP_30000 = 4400300,
			MP_50000 = 4400500,
			MP_100000 = 4401000,
			MP_150000 = 4401500,
			MP_500000 = 4405000,
			MP_1000000 = 4410000,

			RT_20000_1 = 9400100,
			RT_20000_2 = 9400300,

			COIN_1 = 4308001,
			COIN_2 = 4308002,
			COIN_3 = 4308003,
			COIN_4 = 4308004,
			COIN_5 = 4308005,
			COIN_6 = 4308006,
			COIN_7 = 4308007,
			COIN_8 = 4308008,
			COIN_9 = 4308009,
			COIN_10 = 4308010,
			COIN_20 = 4308011,
			COIN_30 = 4308012,
			COIN_40 = 4308013,
			COIN_50 = 4308014,
			COIN_60 = 4308015,
			COIN_70 = 4308016,
			COIN_80 = 4308017,
			COIN_90 = 4308018,
			COIN_100 = 4308019,

			COINB_1 = 4308101,
			COINB_2 = 4308108,
			COINB_3 = 4308109,
			COINB_4 = 4308110,
			COINB_5 = 4308102,
			COINB_6 = 4308111,
			COINB_7 = 4308112,
			COINB_8 = 4308113,
			COINB_9 = 4308114,
			COINB_10 = 4308103,
			COINB_15 = 4308104,
			COINB_20 = 4308105,
			COINB_25 = 4308106,
			COINB_30 = 4308107,
			COINB_40 = 4308115,
			COINB_50 = 4308116,
			COINB_100 = 4308117,

			COUPON_1 = 4305019,
			COUPON_1_AGAIN = 4305025,
			COUPON_2 = 4305027,
			COUPON_3 = 4305028,
			COUPON_4 = 4305029,
			COUPON_5 = 4305020,
			COUPON_6 = 4305030,
			COUPON_7 = 4305031,
			COUPON_8 = 4305032,
			COUPON_9 = 4305033,
			COUPON_10 = 4305021,
			COUPON_15 = 4305022,
			COUPON_20 = 4305023,
			COUPON_25 = 4305024,
			COUPON_30 = 4305026,
			COUPON_40 = 4305034,
			COUPON_50 = 4305035,
			COUPON_100 = 4305036,
		};

		enum BoxIds
		{
			BRILLIANT_HAMMER = 4530000,
			BRILLIANT_SWORD = 4530001,
			BRILLIANT_PENCIL = 4530002,
			BRILLIANT_SNIPER = 4530003,
			BRILLIANT_BAZOOKA = 4530004,
			BRILLIANT_GRENADE = 4530005,
			BRILLIANT_KUKRI = 4530007,
			BRILLIANT_MENTHA = 4530008,
			BRILLIANT_CORAL = 4530009,
			BRILLIANT_BLASTER = 4530010,
		};

		enum ItemIds
		{
			RECORD_RESET = 4302000,
			KILLDEATH_RESET = 4303000,
			INVENTORY_EXPANSION_10 = 4305000,
			INVENTORY_EXPANSION_20 = 4305001,
			INVENTORY_EXPANSION_40 = 4305002,
			INVENTORY_EXPANSION_80 = 4305003,
			BATTERY_500_RT = 4305005,
			BATTERY_500_MP = 4305009,
			BATTERY_1000_RT = 4305006,
			BATTERY_1000_MP = 4305010,
			BATTERY_1000 = 4305201,
			BATTERY_EXPANSION = 4305007,
		};

		enum IngameItemIds
		{
			INSTANTRESPAWN_1 = 6304000,
			INSTANTRESPAWN_2 = 4300300,
			INSTANTRESPAWN_3 = 4300301,
			INSTANTRESPAWN_4 = 4300302,
			INSTANTRESPAWN_5 = 4300303,
			INSTANTRESPAWN_6 = 4300304,
			INSTANTRESPAWN_7 = 4300305,
			INSTANTRESPAWN_8 = 4304000,
			NOPENALTY_1 = 6300002,
			NOPENALTY_2 = 4300002,
			NOPENALTY_3 = 4300000,
			NOPENALTY_4 = 6300000,
			NOPENALTY_5 = 4300001,
			NOPENALTY_6 = 4301000,
			NOPENALTY_7 = 4301001,
		};

		enum ItemExpirationType
		{
			UNLIMITED = 0,
			UNUSED,
			BOMB,
			EXPIRED // >= 3
		};

		enum ItemFrom
		{
			SHOP = 0,
			GIFT,
			UNKNOWN,
		};

		// Sent to the source who sent the friend request
		enum AddFriendServerExtra
		{
			REQUEST_SENT = 0,
			REQUEST_ACCEPTED = 1, // Friend is registered as your friend ??!!
			TARGET_NOT_FOUND = 6, // Cannot find player
			TARGET_OR_SENDER_FRIEND_LIST_FULL = 7, // ok
			SEND_REQUEST_TO_TARGET = 28, // ok
			REQUEST_RECEIVED_WITH_OPTION = 37, // ????????
			RECEIVER_BLOCKED_SENDER = 42,
			DB_ERROR,
		};

		enum AddFriendServerMission
		{
			SENDER_FRIENDLIST_FULL,
			RECEIVER_FRIENDLIST_FULL
		};

		enum ClientFriendExtra
		{
			FRIEND_REQUEST_SENT = 28,
			INCOMING_FRIEND_REQUEST_ACCEPTED = 30
		};

		enum CapsuleCurrencyType : std::uint32_t
		{
			CAPSULE_COINS = 0,
			CAPSULE_ROCKTOTENS = 1,
			CAPSULE_MICROPOINTS = 2
		};

		enum CapsuleSpinExtra : std::uint8_t
		{
			CAPSULE_SPIN_SUCCESS = 1,
			CAPSULE_SPIN_FAIL = 4,
			CAPSULE_SPIN_INVENTORY_FULL = 7,
			CAPSULE_SPIN_NOT_ENOUGH_CURRENCY = 14,
		};

		enum CapsuleSpinMission : std::uint8_t
		{
			CAPSULE_LUCKY_SPIN = 0,
			CAPSULE_NORMAL_SPIN = 1
		};

		enum PartyLeaveExtra
		{
			PARTY_LEAVE_SUCCESS = 1,
			PARTY_LEAVE_OFFLINE = 47,
			PARTY_LEAVE_OUT_OF_CONDITION = 21,
			PARTY_LEAVE_DATA_ERROR = 4,
			PARTY_LEAVE_CLOSE = 27,
			PARTY_LEAVE_KICKED = 42,
			PARTY_LEAVE_GENERAL_ERROR = 43,
		};

		enum TradeSystemExtra
		{
			TRADE_SUCCESS = 1,
			TRADE_CANCELLED = 1,
			TRADE_CONFIRMED_NOTIFY_OTHER_PLAYER = 1,
			TARGET_NOT_ENOUGH_INVENTORY_SPACE = 7,
			TRADE_NOT_ENOUGH_MONEY = 14,
			TRADE_DISABLED_FOR_MAINTENANCE = 15, // currently unused by the server
			TRADE_COOLDOWN = 21, // currently unused by the server; "you can trade again in X hours"] where option is X
			TRADE_DECLINED = 31,
			ITEMS_CANNOT_BE_CHANGED_AFTER_LOCK = 44, // currently unused by the server (the client prevents changing items after lock already)
			BOTH_PLAYERS_MUST_CONFIRM_TRADE_BEFORE_FINALIZATION = 45,
			CANNOT_TRADE_NOW_OR_PLAYER_OFFLINE = 47,
			PLAYERS_NOT_FRIENDS = 73,
			LEVEL_TOO_LOW = 74,
			MAX_NUM_OF_ITEMS_AT_ONCE_REACHED = 75,
		};

		enum ClanEnlist
		{
			CLAN_ENLIST_SUCCESS,
			CLAN_FULL,
			CLAN_NOT_FOUND,
			USER_NOT_FOUND,
			USER_ALREADY_IN_CLAN,
			CLAN_ENLIST_DB_ERROR
		};

		enum class GetPendingRequestsResult
		{
			SUCCESS,
			NOT_IN_CLAN,          
			NOT_LEADER,            
			NO_PENDING_REQUESTS,   
			DB_ERROR             
		};

		enum class AcceptClanRequestResult
		{
			SUCCESS,
			NOT_IN_CLAN,           
			NOT_LEADER,           
			TARGET_NOT_FOUND,      
			TARGET_NOT_IN_REQUESTS, 
			TARGET_ALREADY_IN_CLAN,
			CLAN_FULL,             
			DB_ERROR               
		};

		enum class DenyClanRequestResult
		{
			SUCCESS,
			NOT_IN_CLAN,           
			NOT_LEADER,          
			TARGET_NOT_FOUND,     
			TARGET_NOT_IN_REQUESTS,
			DB_ERROR            
		};

		enum class KickClanMemberResult
		{
			SUCCESS,
			NOT_IN_CLAN,           
			NOT_LEADER,            
			TARGET_NOT_FOUND,     
			TARGET_NOT_IN_CLAN,   
			CANNOT_KICK_SELF,      
			DB_ERROR             
		};

		enum class LeaveClanResult
		{
			SUCCESS,
			NOT_IN_CLAN,
			IS_LEADER,
			DB_ERROR
		};

		enum class DisbandClanResult
		{
			SUCCESS,
			NOT_IN_CLAN,          
			NOT_LEADER,            
			DB_ERROR            
		};

		enum class TransferOwnershipResult
		{
			SUCCESS,
			NOT_IN_CLAN,           
			NOT_LEADER,           
			TARGET_NOT_FOUND,      
			TARGET_NOT_IN_CLAN,   
			TARGET_IS_SELF,      
			DB_ERROR          
		};

		enum class UpdateClanIconResult
		{
			SUCCESS,
			NOT_IN_CLAN,          
			NOT_LEADER,            
			INVALID_BACK_ICON,    
			INVALID_FRONT_ICON,    
			DB_ERROR            
		};
	}
}
#endif