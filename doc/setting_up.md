# 3.2 Setting up the emulator
Starting from version 2.0, setting up the emulator should be much easier than before.

A new `config.ini` file was now added inside the `Setup` folder (in the root directory). 

## config.ini Configuration Guide
This file contains all the configuration values used by the servers (Auth, Main, Cast) and supporting components (database, client, website). Here's how to fill it out correctly:
Note: the file is inside the `Setup` folder. It should already contain most of the things that will enable it to work for localhost (`127.0.0.1`).

### [AuthServer]
This section configures your authentication server, which handles player logins.

**LocalIp**: Your machine’s local IPv4 address (e.g., 192.168.x.x). To find it, use `ipconfig` on a terminal. (Note: if you have services like RadminVPN, you may have to use their IP). 

**Ip**: Your public IP or just 127.0.0.1 if everything runs locally.

**Port**: The port used by the Auth Server, default is 13000.

**GradedPort**: The port used by the Auth Server for moderator/administrator login, default is 13001. This is only required (and only used) when `EnhancedSecurity = true`.

**VpnIp**: VPN that the servers will listen to on port `GradedPort` for graded access. For more info, see the Enhanced Security section in this documentation.

**EnhancedSecurity**: Specifies whether graded access must require strict login. See the Enhanced Security section in this documentation.

### [MainServer_N]
Examples: MainServer_1, MainServer_2, ..., up to MainServer_9

You can run multiple Main Servers, each handling player data, inventory, shops, trades, rooms, and more.

**LocalIp**: Local IPv4 address of the machine it's running on. This is needed to differentiate multiple servers running in different machines.

**Ip**: Public IP or localhost (127.0.0.1)

**Port**: Port the server listens on (e.g., default 13005)

**IpcPort**: Port used for internal server communication

**IsPublic**: Set this to true if this server should be accessible to non-graded players from the channel list

### [CastServer_N]
Same as above, from CastServer_1 to CastServer_9
These handle all in-match/gameplay logic like damage, position updates, match rules, and more.

**LocalIp**: Local IP address

**Ip**: Public or localhost IP (127.0.0.1)

**Port**: Gameplay traffic port (e.g., default 13006)

**IpcPort**: Used for inter-server communication

**IPC_EnableDeadBroadcast**: This setting is particularly important in multi-region server deployments. For instance, if you run a NA VPS hosting a cast server, while your EU VPS hosts all main servers along with the EU cast server, you should set EnableDeadBroadcast to false on the NA server. Otherwise, every time a player dies, the NA cast server would need to communicate with the NA main server located on the EU VPS. This cross-region communication can introduce unnecessary delays and increase latency.


### [Database]
Connection settings for your MariaDB database.

**LocalIp**: Use your local Ipv4 as before.

**Ip**: Ip where the database is accessed, for example 127.0.0.1

**Port**: Port of the DB (commonly 3306, here it's 3305, and I will provide a `my.ini` file in the next chapter that uses `3305`)

**DatabaseName**: Name of the database (e.g., microvolts-db)

**Username**: Database user (e.g., root)

**PasswordEnvironmentName:** Name of the environment variable (not the actual password) that holds the DB password

### [Website]
Defines the API endpoint used for admin panel or external requests.

**Ip**: IP of the admin panel backend (often 127.0.0.1). Ideally you should also restrict the port access below via firewall rules.

**Port**: Port your panel or API runs on (e.g., 8080).

**JwtTokenEnvironmentName**: Name of the environment variable (not the token itself) holding the JWT secret the server uses to verify admin-panel requests (HS256).

**AllowedOrigins**: Comma-separated list of CORS origins allowed to call the API.

### [Client]
Used for version checking during client login.

**ClientVersion**: The version string expected by the servers (e.g., 0.0.3 for the ToyBattles Client).
#### Important: 
The servers check the ClientVersion. If you specify a client version (e.g. 1.1.1) but your client is actually using a different version than what you specified, you will **not be able to login** on the main servers with **non-graded accounts**. At ToyBattles we use this to force players to use the most recently updated client.

Graded accounts don't have this limitation, this allows graded accounts to enter the server even with different clients, for example for testing purposes.

Note that for ToyBattles Client (on the Release section on this Repository), the client version is 0.0.3. If you decide to use the original Microvolts Surge client (although discouraged) you will need to use version 1.1.1.


### [General]
This section is only required when `EnhancedSecurity = true`. When it is `false`, the whole section is ignored and can be left as-is.

**EmailSecret**: environment variable containing email encryption key (in hex form without 0x, 32 bytes) 

**2faSecret**: environment variable containing game 2FA encryption key (in hex form without 0x, 32 bytes) 

**SmtpServer**: SMTP server used to use the email alerting system, see Enhanced Security for more info.

**EmailSender**: The email used to send security emails.

**EmailUsername**: This can be the same as EmailSender or a specific username you may have.

**EmailToken**: The specific App password to use the above email for security comunication.

**SecurityNotificationReceiver**: Whoever will receive security related emails, ideally only administrators.

# ⚠️ Notes & Warnings
You can configure up to 9 Main Servers and 9 Cast Servers.

If anything is misconfigured (invalid IPs, missing ports, database issues, etc.), the server will print an error message on startup, so always check the console output when launching.

**Make sure that the ports you use for the emulator aren't used by other services running in your OS.**

I’ll also provide example setup.ini files for different setups in later chapters:

- Single local server (for testing everything on one machine)
- Single VPS server
- Multiple regional VPS servers (e.g., NA, EU, ASIA)


## Next
[3.2.1 Enhanced Security](https://github.com/DownWithTheFallen/ToyBattlesHQ/blob/toybattles_mvsurge/doc/enhanced_security.md)

