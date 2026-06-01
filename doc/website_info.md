# External Admin Panel setup & API

Since version 2.0 you are now able to set up your own external admin panel to monitor the game servers from it. This requires using the game server's API.

You will need to set up a JWT secret and the allowed CORS origins. Both are configured in the `[Website]` section of `config.ini` (no code editing or rebuild required):
- **JWT secret**: store it in the environment variable named by `JwtTokenEnvironmentName` (e.g. `MV_JWT`). The server uses this value (HS256) to verify incoming admin-panel tokens.
- **CORS origins**: list every allowed origin, comma-separated, in `AllowedOrigins`.

## API
Note: your JWT token must contain a claim named `role`, which is an integer that represents the grade of whoever is using the admin panel.
In particular:
- Grade 1 is normal user,
- Grade 2 is Event Supporter,
- Grade 3 is Moderator,
- Grade 4 is Game Master,
- Grade 7 is Developer (highest).


### Banning an user
```cpp
 /*
    Request: POST /ban   OR   /cheatban
    Header:
     - Authorization: Bearer <JWT_TOKEN>
    Body:
     - TargetNickname: (string) The nickname of the player to be banned, OR
     - TargetAccountId

    Response status:
     - 200 OK: success
     - 400 Bad Request: missing required fields or request body is not valid JSON object
     - 401 Unauthorized: invalid JWT token or user grade too low
     - 500: exception thrown in the server
    Response body: contains message (success or error)
*/
```

### Muting an user
```cpp
 /*
    Request: POST /mute
    Header:
     - Authorization: Bearer <JWT_TOKEN>
    Body:
     - TargetNickname: (string) The nickname of the player to be muted, OR
     - TargetAccountId

    Response status:
     - 200 OK: success
     - 400 Bad Request: missing required fields or request body is not valid JSON object
     - 401 Unauthorized: invalid JWT token or user grade too low
     - 500: exception thrown in the server
    Response body: contains message (success or error)
*/
```

### Sending announcements and tips
```cpp
 /*
   Request: POST /announce or POST /tip
   Header:  === IGNORE FOR NOW ===
   - Content-Type: application/json
   - Authorization: Bearer <JWT TOKEN>

   Body:
   - Text: (str) message to announce (max 512 chars)

   Response:
   - 200 OK: Response body will contain message "Announcement: '<text>'"
   - 400 bad req: missing "Text" field or invalid JSON format or text exceeds 512 chars
   - 401 unauthorized: invalid JWT token or too low grade
   - 500: server exception

   Response body: success or error message
 */
```

### Retrieving all online players
```cpp
  /*
       Request: GET /getOnlinePlayers
       Header:
       - Content-Type: application/json
       - Authorization: Bearer <JWT TOKEN>

       Body: not needed

       Response:
       - 200 OK: response body contains JSON array with online players + their session IDs and account IDs, example:
         {
             "online_players": [
                 {
                     "nickname": "player1",
                     "session_id": 1234,
                     "account_id": 5678
                 },
                 {
                     "nickname": "player2",
                     "session_id": 9101,
                     "account_id": 1121
                 }
             ]
         }

       - 400 Bad Request: bad JWT token or too low grade
       - 500: server exception
  */
```

### Disconnecting an user
```cpp
/*
    Request: POST /disconnect
    Header:
    - Content-Type: application/json
    - Authorization: Bearer <JWT TOKEN>

    Body:
    - TargetNickname: (str), OR
    - TargetAccountId: 

    Response:
    - 200 OK: whether player is disconnected or not
    - 400 Bad Request: missing required fields or (for account ID) invalid format
    - 500: server exception
*/
```

### Retrieve all rooms
```cpp
 /*
     Request: GET /getrooms
     Header:
     - Content-Type: application/json
     - Authorization: Bearer <JWT TOKEN>

     Body: -

     Response:
     - 200 OK: JSON array of all rooms + their info, example follows
       {
           "rooms": [
               {
                   "RoomNumber": 1,
                   "RoomTitle": "Room One",
                   "TotalPlayers": 5,
                   "TotalMaxPlayers": 10,
                   "Password": "password123"
               },
               {
                   "RoomNumber": 2,
                   "RoomTitle": "Room Two",
                   "TotalPlayers": 2,
                   "TotalMaxPlayers": 10,
                   "Password": "password456"
               }
           ]
       }
     - If no room available: the message is "No rooms available"
     - 400 Bad Request: bad JSON or bad grade
     - 500: server exception
 */
```

### Deleting a room
```cpp
 /*
     Request: POST /breakroom
     Header:
     - Content-Type: application/json
     - Authorization: Bearer <JWT TOKEN>

     Body:
     - RoomNumber: (STR) The room number to break (required)

     Response:
     - 200 OK: success message
     - 400 Bad Request: missing fields or bad format
     - 404 Not Found: room not found
     - 500 Internal Server Error: server exception
 */
```

### Kicking an user from a room
```cpp
/*
    Request: POST /kick
    Headers:
    - Content-Type: application/json
    - Authorization: Bearer <JWT TOKEN>

    Body:
    - TargetNickname: (STR) The nickname of the player to kick (required)

    Response:
    - 200 OK: Player '<TargetNickname>' was successfully kicked from the room.
    - 400 Bad Request:
        - Missing required field: TargetNickname
        - Failed to kick player (e.g., not in room or invalid state)
    - 404 Not Found:
        - Session for player not found
        - Room number not found
    - 500 Internal Server Error: Server error while processing the request
*/
```

### Changing a room's host
```cpp
/*
    Request: POST /changehost
    Headers:
    - Content-Type: application/json
    - Authorization: Bearer <JWT TOKEN>

    Body:
    - TargetNickname: (STR) The nickname of the player to promote to host (required)

    Response:
    - 200 OK: Player '<TargetNickname>' is now the host of the room.
    - 400 Bad Request:
        - Missing required field: TargetNickname
        - Failed to change host (e.g., player not in the room or already host)
    - 404 Not Found:
        - Session for player not found
        - Room number not found
    - 500 Internal Server Error: Server error while processing the request
*/
```

### Changing a room's title
```cpp
/*
    Request: POST /changeroomtitle
    Headers:
    - Content-Type: application/json
    - Authorization: Bearer <JWT TOKEN>

    Body:
    - RoomNumber: (STR) The room number whose title is to be changed (required, numeric string)
    - NewTitle: (STR) The new title for the room (required, max 29 characters)

    Response:
    - 200 OK: Room number '<RoomNumber>'s title was successfully changed
    - 400 Bad Request:
        - Missing required field: RoomNumber or NewTitle
        - Invalid room number format
        - Title too long (truncated)
        - Cannot change room title while the match is ongoing
    - 404 Not Found: Room number not found
    - 500 Internal Server Error: Server error while processing the request
*/
```

### Getting specific info about a room
```cpp
            /*
                Request: POST /getroominfo
                Header:
                - Content-Type: application/json
                - Authorization: Bearer <JWT TOKEN>

                Body:
                - RoomNumber: (STRING) self explainatory

                Response:
                - 200 OK: room details + player details
                - 400 Bad Request: Missing `RoomNumber` field or bad format (must be INTEGER)
                - 404 Not Found: room not found
                - 500 Internal Server Error: server errors

                Example Response (200 OK):
                {
                    "room": {
                        "Host": "PlayerOne",
                        "RoomNumber": 1234,
                        "RoomTitle": "Epic Battle Arena",
                        "TotalPlayers": 5,
                        "TotalMaxPlayers": 10,
                        "Password": "secret123",
                        "Players": [
                            {
                                "PlayerName": "PlayerOne",
                                "Team": "red",
                                "Ping": 32,
                                "SEID": 1001
                            },
                            {
                                "PlayerName": "PlayerTwo",
                                "Team": "blue",
                                "Ping": 45,
                                "SEID": 1002
                            },
                            {
                                "PlayerName": "PlayerThree",
                                "Team": "red",
                                "Ping": 50,
                                "SEID": 1003
                            }
                        ]
                    }
                }
            */
```

### Getting specific user info
```cpp
/*
    Request: POST /getplayerinfo
    Header:
    - Content-Type: application/json
    - Authorization: Bearer <JWT TOKEN>

    Body:
    - TargetNickname: (STRING), OR
    - TargetAccountId: (STRING) 

    Response:
    - 200 OK: detailed player info returned
    - 400 Bad Request: missing required fields, or if using "TargetAccountId" bad format
    - 404 Not Found: Player not found
    - 500 Internal Server Error: server exception

    Example Response (200 OK):
    {
        "player": {
            "PlayerName": "PlayerOne",
            "AccountID": 12345,
            "SessionID": 6789,
            "Grade": 3,
            "CurrentRoom": 101,
            "Level": 15,
            "MicroPoints": 1000,
            "RockTotens": 50,
            "MatchBanned": true,
            "BanReason": "obvious cheating",
            "BannedUntil": "permanent",
            "Muted": true,
            "MuteReason": "abusive behavior",
            "MutedBy": "Admin",
            "MuteExpiration": "2025-03-21T12:00:00Z",
            "RoomCreationDisabled": true,
            "RoomCreationDisabledUntil": "2025-04-01T00:00:00Z"
        }
    }
*/
```

### Sending rewards to a player
```cpp
    Request: POST /sendreward
   // Example:
   /*
      {
         "Nickname": "Player123",  ---STRING
         "ItemID": "10042",        ---STRING
         "Message": "Congratulations on your victory! Here's a special item for you."    --- STRING
      }

      RETURN CODES
      - Invalid JWT: https::UNAUTHORIZED
      - bad body or bad fields, bad itemID (not found) or message > 255 chars: http::BAD_REQUEST
      - player not found neither in DB nor online: BAD_REQUEST ==> you can continue search in other servers
      - success => https::SUCCESS ==> immediately stop sending gift to this user in other servers
   */
```

### Enabling / Disabling capsule events
```cpp
POST: /updatecapsuleevent
// Example:
  /*
      {
          "newCapsuleEventStartDate": 1651990000,
          "newCapsuleEventEndDate": 1652000000,
          "newMpPrice" : 500,
          "newRtPrice" : 800
      }
  */
```

### Enabling / Disabling EXP and MP bonuses
```cpp
POST: /updateexpmpevent
// Example:
 /*
     {
         "newExpMpBonusStartDate": 1651990000,
         "newExpMpBonusEndDate": 1652000000,
         "newExpBonusPercent": 20,
         "newMpBonusPercent": 15
     }
 */
```

### Enabling / Disabling Event Missions
```cpp
POST: /updateeventmission
// Example:
 /*
     {
         "newEventMissionStartDate": 1651990000,
         "newEventMissionEndDate": 1652000000,
     }
 */
```

### Enabling / Disabling the Trade System
```cpp
POST: /updatetradesystem
// Example:
 /*
     {
         "newTradeEventStartDate": 1651990000,
         "newTradeEventEndDate": 1652000000,
     }
 */
```



# Next
[7.1 The Updater: How the launcher retrieves updates](https://github.com/DownWithTheFallen/ToyBattlesHQ/blob/toybattles_mvsurge/doc/updater_overview.md)
