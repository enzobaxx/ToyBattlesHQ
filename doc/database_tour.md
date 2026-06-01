# 5.1 Tour of database tables

Version 2.0 of the emulator brings a major upgrade: the database backend has been moved **from SQLite to MariaDB.**

While SQLite was a simple and lightweight solution, it quickly became a bottleneck, especially when multiple servers needed to access the database at the same time.
It also lacked advanced features like proper user management, networked access, and better concurrency handling, which are essential as the project grows.

That said, the overall structure of the database is **still very similar to the previous version**, with a few new additions here and there.

Below is a quick overview of the most important database tables. If you're planning to contribute to the emulator, I strongly recommend **opening the database with a tool like HeidiSQL** to explore everything in detail.
This isn’t a full walkthrough: I’ll just highlight the key tables and point out some things I think are worth knowing.

### `Users` Table
This table contains all information regarding a specific player: accountID, usernames, nicknames, passwords, level, kills, experience, to name a few.
- For security reasons, the password is not in plain text. Instead, you must use **bcrypt** (12 rounds).
- The 2FA for graded accounts (grade > 2) (or even normal ones if you wish) must be setup in this table. The column name is `Secret` and it must be a base32 **encrypted** hash. The 2FA is time-based, any application like Google Authenticator will work. To see how encryption works, i.e. AES GCM, see the code in `Common/Utils.h`.
- This table also contains an encrypted email. Once again, check `Common/Utils.h` to see how it works and the correct format to use.
  
#### Examples
- To change the login and password for the "test" account, use `UPDATE Users SET Username="NEW_LOGIN", Password="NEW_BCRYPT12ROUNDS_PASSWORD" WHERE Username="test"`.
- You can convert your plain-text password to bcrypt (12 rounds) by using any online tool.
- To give yourself ingame currency: `UPDATE Users SET MicroPoints=50000, RockTotens=50000, Coupons=250 WHERE Username="test"`

#### Graded accounts require mandatory 2FA (when `EnhancedSecurity = true`). To enable it:
Log in the game via a graded account. Generate a new account through it with `/addplayer NewUsername NewNickname Email` (this command is only available when `EnhancedSecurity = true`). You will receive a random password and 2FA code (to setup 2FA). Then, you can simply update the new user's grade via the database.
   
**Note** The following is what each user grade is.
- Grade 1: Normal users - 2fa not mandatory. (lowest grade)
- Grade 2: Event Supporter 
- Grade 3: Moderator
- Grade 4: Game Master (GM)
- Grade 7: Developer

When `EnhancedSecurity = true`, all grades above 2 require mandatory 2FA (you won't be able to login otherwise, since the server checks this). When `EnhancedSecurity = false` (the default), 2FA is not used and graded accounts log in like normal accounts. See the Enhanced Security chapter for details.
To login with 2FA, you need this format instead of your usual username when logging in: `Username+6digitcode` (with the `+` in between).

Notes:
- Higher grade means higher power & more commands.
- GM and DEV have effectively the same commands. DEV grade is somewhat bugged, we suggest using the GM grade as the highest one instead.
- The `Nickname` can be at maximum 16 characters.

#### Clan Creation
All commands to create and handle clans are now available directly inside the game. Use the `/commands` command to see available commands. 
It is no longer necessary to modify the database directly for clan creation.

### `UserItems` Table
This table contains all information regarding user items. Each user is identified by an accountID, and each row represents an item that a player identified by their accountID has.
- `Stocks` represents how much the item can be used. For example: respawn items may be used 5 times, or 10 times depending on which itemID is used. This means that the Stock value will be 5 or 10 respectively.
- ItemDuration is a unix timestamp. `0` means that the item is unlimited.

You generally don't need to manually add user items manually on the database since there's in-game commands for that. Use the `/?` or `/commands` in the ingame chat to view the available commands for your current grade.

However, sometimes it may be useful to delete all existing items for an account. In that case, you can use: `DELETE FROM UserItems WHERE AccountID = (SELECT AccountID FROM Users WHERE Username="YOUR_ACCOUNT_USERNAME")`.

### `ItemLogs` Table
This table was added in version 2.0 to keep track of items that may have been lost by the players for whatever reasons (client crashing as an example).
Their purpose is to check whether the item was effectively in the inventory, and if it disappeared, why:
- Was the item deleted?
- Was the item traded?
Basically, the server logs any action related to any interaction with user items.

### `ChatLogs` Table
This table is yet another addition in version 2.0.
It contains all chat logs and, in case of whisper, the message will contain [Whisper To: ].
This was very useful during our public tests with ToyBattles for moderation purposes.

### `GiftBox` and `Mailbox` Tables
Both of these are needed for the gift and mail system in the game.
- for GiftBox, `sender` will automatically be `Team` in case the item is sent to a player through the `/sendgift` command.
- for Mailbox, `isNew` will be used to show a notification at login in case it is true.

### `PendingAccounts` Table
Finally, this table can be used to create a sign-up system with e-mail or Discord verification.
Once an user creates an account, you can insert their data in this table until they verify their identity (e.g. through an e-mail verification system).

After the user is confirmed, you can move their data from this table to the `Users` table.


### Final note regarding Monthly and Weekly rewards
The items that are sent through both Monthly and Weekly reward systems can be found in the root directory, inside the `.txt` file `RewardItemIDs.txt`.


## Next
[6.1 Website API overview and communication with MainServer](https://github.com/DownWithTheFallen/ToyBattlesHQ/blob/toybattles_mvsurge/doc/website_info.md)

