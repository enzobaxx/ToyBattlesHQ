# 4.1 Config.ini setup for localhost

Assume that:
- My local IP is 123.456.78.9
- My client version is 0.0.3 (i.e. I did not modify it through the client tools as explained in the next chapters)
- My database is named microvolts-db
- My database username is `root`
- My database password is stored in the environment variable named "MV_DB_PW"
- My MariaDB runs on port 3305
- I do not have a website running  (i.e. no admin panel)

Then, here's a simple setup for a completely local server:

```cpp
[AuthServer]
LocalIp = 123.456.78.9
Ip = 127.0.0.1
Port = 13000
GradedPort = 13001 
VpnIp = 127.0.0.1  
EnhancedSecurity = false

[MainServer_1]
LocalIp = 123.456.78.9
Ip = 127.0.0.1
Port = 13005
IpcPort = 14005
IsPublic = true

[CastServer_1]
LocalIp = 123.456.78.9
Ip = 127.0.0.1
Port = 13006
IpcPort = 14006
IPC_EnableDeadBroadcast = true

[Database]
LocalIp = 123.456.78.9
Ip = 127.0.0.1
Port = 3305
DatabaseName = microvolts-db
Username = root
PasswordEnvironmentName = MV_DB_PW

[Website]
Ip = 127.0.0.1 # or any website IP you may have
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
[4.2 Config setup for VPS](https://github.com/DownWithTheFallen/MicrovoltsEmulator/blob/mv1.1_2.0/doc/example_vps.md)
