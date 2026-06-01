# 4.3 Config setup for multiple servers across different VPS's

## Initial Assumptions
Assume that:
- I have 2 VPS.
   - The first VPS public address is 199.222.32.3
   - The first VPS local IP is 123.456.78.9 (you can find this by opening a command prompt and using `ipconfig`)

   - The second VPS public address is 182.243.34.5
   - The second  VPS local IP is 987.543.21.1 (you can find this by opening a command prompt and using `ipconfig`)

- My client version is 0.0.3 (i.e. I did not modify it through the client tools as explained in the next chapters)
- The database is placed inside the first VPS, and is named microvolts-db 
- My database username is root
- My database password is stored in the environment variable named "MV_DB_PW" (inside the first VPS)
- My MariaDB runs on port 3305 (inside the first VPS)
- I do not have a website running (i.e. no admin panel)
- Both VPS allow incoming connections to all the ports below through firewall settings
- Both VPS allow incoming and outgoing connections from/to each other.
Then, here's a simple setup for a VPS:

## Initial setup
The first VPS must contain:
- The auth server executable.
- The main server for the first VPS executable.
- The cast server for the first VPS executable.
- The main server for the second VPS executable.

The second VPS must contain:
- Only the cast server.

To be sure of the setup in all VPS, just copy paste the emulator in all of them. But make sure that:
- Once "central" VPS runs the Auth server and all main servers (even those for other regions).
- Each VPS runs its own cast server.
- Basically: the central VPS runs the auth server, all main servers and its own cast server. All other VPS's run only their respective cast servers.

## Setup (config.ini) for the first VPS
```cpp
[AuthServer]
LocalIp = 123.456.78.9
Ip = 199.222.32.3
Port = 13000
GradedPort = 13001 # Block this port on firewall and only allow Wireguard peers to use it!
VpnIp = 127.0.0.1  # Use e.g. Wireguard IP (server side) - server will listen to this IP + GradedPort for graded access
EnhancedSecurity = false

[MainServer_1] # This is for the first server (the first VPS)
LocalIp = 123.456.78.9
Ip = 199.222.32.3
Port = 13005
IpcPort = 14005
IsPublic = true

[CastServer_1] # This is for the first server (the first VPS)
LocalIp = 123.456.78.9
Ip = 199.222.32.3
Port = 13006
IpcPort = 14006
IPC_EnableDeadBroadcast = true

[MainServer_2]                # This is for the second server (the second VPS)
LocalIp = 127.0.0.1           # Localhost here on purpose!
Ip = 199.222.32.3             # First VPS ip here on purpose -- All main servers will run in one central VPS!
Port = 13027
IpcPort = 14027
IsPublic = true

[CastServer_2]                # This is for the second server (the second VPS)
LocalIp = 127.0.0.1           # Localhost here on purpose!
Ip = 182.243.34.5             # Second VPS ip here -- The cast server goes in its respective VPS!
Port = 13028
IpcPort = 14028
IPC_EnableDeadBroadcast = false

[Database]
LocalIp = 123.456.78.9        # First VPS local IP
Ip = 199.222.32.3             # First VPS public IP
Port = 3305
DatabaseName = microvolts-db
Username = root
PasswordEnvironmentName = MV_DB_PW

[Website]
Ip = 127.0.0.1               # We don't have a website / admin panel
Port = 8080
JwtTokenEnvironmentName = MV_JWT
AllowedOrigins = www.allowedOriginTest.com

[Client]
ClientVersion = 0.0.3 # For ToyBattles Client in this repository; for original MV Surge client use 1.1.1

[General]
EmailSecret = MVEMAIL_SECRET # environment variable containing email encryption key (in hex form without 0x, 32 bytes) 
2faSecret = MV2FA_SECRET     # environment variable containing game 2FA encryption key (in hex form without 0x, 32 bytes) 
SmtpServer = smtp://smtp.gmail.com:587  # example if you use gmail
EmailSender = sendertest@gmail.com
EmailUsername = emailusername # or use EmailSender depending on the service you're using
EmailToken = MV_EMAIL_TOKEN   # environment variable containing email token to send emails using EmailSender above
SecurityNotificationReceiver = receiver1@gmail.com,  receiver2@gmail.com  # who will receive security notifications
```

## Setup (config.ini) for the seoond VPS
```cpp
[AuthServer]
LocalIp = 123.456.78.9      # First VPS local ip on purpose
Ip = 199.222.32.3           # First VPS public ip on purpose
Port = 13000
GradedPort = 13001 # Block this port on firewall and only allow Wireguard peers to use it!
VpnIp = 127.0.0.1  # Use e.g. Wireguard IP (server side) - server will listen to this IP + GradedPort for graded access
EnhancedSecurity = false

# Same as for the previous setup here.
[MainServer_1]              # This is for the first server (the first VPS)
LocalIp = 123.456.78.9
Ip = 199.222.32.3
Port = 13005
IpcPort = 14005
IsPublic = true

# Same as for the previous setup here.
[CastServer_1] # This is for the first server (the first VPS)
LocalIp = 123.456.78.9
Ip = 199.222.32.3
Port = 13006
IpcPort = 14006
IPC_EnableDeadBroadcast = true

[MainServer_2]                # This is for the second server (the second VPS)
LocalIp = 127.0.0.1           # Localhost here on purpose!
Ip = 199.222.32.3             # First VPS here, because that's where all main servers are
Port = 13027
IpcPort = 14027
IsPublic = true

[CastServer_2]                # This is for the second server (the second VPS)
LocalIp = 987.543.21.1        # Second VPS local IP
Ip = 182.243.34.5             # Second VPS ip here, because the cast server goes in its respective VPS
Port = 13028
IpcPort = 14028
IPC_EnableDeadBroadcast = false

[Database]
LocalIp = 123.456.78.9        # First VPS local IP
Ip = 199.222.32.3             # First VPS public IP
Port = 3305
DatabaseName = microvolts-db
Username = root
PasswordEnvironmentName = MV_DB_PW

[Website]
Ip = 127.0.0.1               # We don't have a website / admin panel
Port = 8080
JwtTokenEnvironmentName = MV_JWT
AllowedOrigins = www.allowedOriginTest.com

[Client]
ClientVersion = 0.0.3 # For ToyBattles Client in this repository; for original MV Surge client use 1.1.1

[General]
EmailSecret = MVEMAIL_SECRET # environment variable containing email encryption key (in hex form without 0x, 32 bytes) 
2faSecret = MV2FA_SECRET     # environment variable containing game 2FA encryption key (in hex form without 0x, 32 bytes) 
SmtpServer = smtp://smtp.gmail.com:587  # example if you use gmail
EmailSender = sendertest@gmail.com
EmailUsername = emailusername # or use EmailSender depending on the service you're using
EmailToken = MV_EMAIL_TOKEN   # environment variable containing email token to send emails using EmailSender above
SecurityNotificationReceiver = receiver1@gmail.com,  receiver2@gmail.com  # who will receive security notifications



```


## Next
[5.1 Tour of database tables](https://github.com/DownWithTheFallen/ToyBattlesHQ/blob/toybattles_mvsurge/doc/database_tour.md)


