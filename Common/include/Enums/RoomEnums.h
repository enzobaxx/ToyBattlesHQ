#ifndef ENUMS_COMMON_ROOM_H
#define ENUMS_COMMON_ROOM_H

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Common
{
	namespace Enums
	{
		enum GameModes : std::uint32_t
		{
			TeamDeathMatch = 0,
			FreeForAll = 1,
			ItemMatch = 2,
			CaptureTheBattery = 3,
			CloseCombat = 4,
			Elimination = 5,
			SuperItemMatch = 6,
			ZombieMode = 7,
			ArmsRace = 8,
			Scrimmage = 9,
			BombBattle = 10,
			SniperMode = 11,
			SquareMode = 12,
			BossBattle = 13,
			AiBattle = 14,
            MODES_MAX
		};

        inline std::string getGameModeString(std::uint32_t mode)
        {
            static const std::unordered_map<std::uint32_t, std::string> modeNames = {
                {0, "TeamDeathMatch"}, {1, "FreeForAll"}, {2, "ItemMatch"}, {3, "CaptureTheBattery"},
                {4, "CloseCombat"}, {5, "Elimination"}, {6, "SuperItemMatch"}, {7, "ZombieMode"},
                {8, "ArmsRace"}, {9, "Scrimmage"}, {10, "BombBattle"}, {11, "SniperMode"},
                {12, "SquareMode"}, {13, "BossBattle"}, {14, "AiBattle"}
            };

            auto it = modeNames.find(mode);
            return (it != modeNames.end()) ? it->second : "Unknown";
        }


        enum ClanGameModes : std::uint32_t
        {
            Clan_CaptureTheBattery = 16,
            Clan_Elimination = 17,
            Clan_TeamDeathMatch = 18,
            Clan_BombBattle = 19,
            CLANMODES_MAX
        };

		enum RoomChangeHostExtra
		{
			CHANGE_HOST_SUCCESS = 1,
			CHANGE_HOST_FAIL = 2, // Unknown error. Please restart the client
			CHANGE_HOST_DOESNT_EXIST = 0xD,
			CHANGE_HOST_NO_PERMISSION = 0x10,
		};

		enum WeaponRestriction : std::uint32_t
		{
			MeleeOnly = 0,
			RifleOnly = 1,
			ShotgunOnly = 2,
			SniperOnly = 3,
			GatlingOnly = 4,
			BazookaOnly = 5,
			GrenadeOnly = 6,
			All = 7,
			WeaponSelect = 9,
		};

        inline std::string getWeaponRestrictionString(std::uint32_t restriction)
        {
            static const std::unordered_map<std::uint32_t, std::string> restrictionNames = {
                {0, "MeleeOnly"}, {1, "RifleOnly"}, {2, "ShotgunOnly"}, {3, "SniperOnly"},
                {4, "GatlingOnly"}, {5, "BazookaOnly"}, {6, "GrenadeOnly"}, {7, "All"},
                {9, "WeaponSelect"}
            };

            auto it = restrictionNames.find(restriction);
            return (it != restrictionNames.end()) ? it->second : "Unknown";
        }

        enum GameMaps : std::uint32_t
        {
            Random = 0,
            Academy = 1,
            AcademyInvasion = 2,
            AcademyTrainingGround = 3,
            BattleMine = 4,
            Beach = 5,
            Bitmap = 6,
            Bitmap2 = 7,
            BitmapPlant = 8,
            Cargo = 9,
            Castle = 10,
            ChampionshipCastle = 11,
            Chess = 12,
            Craftwork = 13,
            Football = 14,
            ForgottenJunkYard = 15,
            ForgottenJunkYardLarge = 16,
            HobbyShop = 17,
            HouseTop = 18,
            InvasionEasy = 19,
            InvasionHard = 20,
            JunkYard = 21,
            MagicPaperLand = 22,
            MagicPaperLandLarge = 23,
            ModelShip = 24,
            Neighboorhood = 25,
            PVCFactory = 26,
            PVCFactoryNight = 27,
            RockBand = 28,
            RockBandS = 29,
            RockBandW = 30,
            RumpusRoom = 31,
            RumpusRoom2 = 32,
            RumpusWars = 33,
            SquareModeMap = 34,
            SquareModeMap2 = 35,
            SummerChess = 36,
            TempleRuins = 37,
            TheAftermath = 38,
            TheStudio = 39,
            TownSquare = 40,
            ToyFleet = 41,
            ToyGarden = 42,
            ToyGarden2 = 43,
            TrackersFactory = 44,
            Tutorial = 45,
            WildWest = 46,
            RumpusRoomREV = 47,
            OldCargo = 49,
            Plaza = 50,
            Warzone = 55,
            CityHall = 57,
            MAPS_MAX
        };

        inline std::string getMapString(std::uint32_t map)
        {
            static const std::unordered_map<std::uint32_t, std::string> mapNames = {
                {0, "Random"}, {1, "Academy"}, {2, "AcademyInvasion"}, {3, "AcademyTrainingGround"},
                {4, "BattleMine"}, {5, "Beach"}, {6, "Bitmap"}, {7, "Bitmap2"}, {8, "BitmapPlant"},
                {9, "Cargo"}, {10, "Castle"}, {11, "ChampionshipCastle"}, {12, "Chess"},
                {13, "Craftwork"}, {14, "Football"}, {15, "ForgottenJunkYard"}, {16, "ForgottenJunkYardLarge"},
                {17, "HobbyShop"}, {18, "HouseTop"}, {19, "InvasionEasy"}, {20, "InvasionHard"},
                {21, "JunkYard"}, {22, "MagicPaperLand"}, {23, "MagicPaperLandLarge"}, {24, "ModelShip"},
                {25, "Neighboorhood"}, {26, "PVCFactory"}, {27, "PVCFactoryNight"}, {28, "RockBand"},
                {29, "RockBandS"}, {30, "RockBandW"}, {31, "RumpusRoom"}, {32, "RumpusRoom2"},
                {33, "RumpusWars"}, {34, "SquareModeMap"}, {35, "SquareModeMap2"}, {36, "SummerChess"},
                {37, "TempleRuins"}, {38, "TheAftermath"}, {39, "TheStudio"}, {40, "TownSquare"},
                {41, "ToyFleet"}, {42, "ToyGarden"}, {43, "ToyGarden2"}, {44, "TrackersFactory"},
                {45, "Tutorial"}, {46, "WildWest"}, {47, "RumpusRoomREV"}, {49, "OldCargo"},
                {50, "Plaza"}
            };

            auto it = mapNames.find(map);
            return (it != mapNames.end()) ? it->second : "Unknown";
        }
	}
}

#endif
