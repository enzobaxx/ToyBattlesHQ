# 3.4 Setting up the client
This is the final step so we're almost there.

We now need to make sure the client is configured to connect to your local server.

### 3.4.1 Download the client
You can technically use any client version newer than Surge, but using the ToyBattles client is highly recommended. Here's the download link: https://github.com/DownWithTheFallen/ToyBattlesHQ/releases/tag/TbClientRelease
(**Note**: That client has already localhost IPs).

Using the ToyBattles client **is highly recommended** as it will already contain some client modifications that allow specific features like the Gamble System, Custom error messages and correct map IDs.

**Note**: If you still decide to use the original Surge client, you will need to update the GameMaps enum since this branch is targeted towards a modified version of the client that uses a different order of the maps (the one given above).
Refer to the branch `mv.1_1` to get the correct GameMaps enum for the original Surge client.
Also, the original Surge client will not have the correct error messages in certain cases and some UI names will be wrong when used with this server emulator.

### Start the client
To launch the game, you have two options:
- Run the `Launcher.bat` script, or
- Open the client folder, navigate to the `Bin` folder and run MicroVolts.exe.
**Do not** run `Launcher.exe`, since that will try to connect to the ToyBattles public servers!

# Final Checklist: Starting Everything Up
By now, you should have completed the following steps:
- Compiled all the executables (Common, MainServer, AuthServer, and CastServer)
- Launched each server executable without seeing any red error messages
- Set up and started your MariaDB database
- Started your game client.
  
If everything’s in place and running correctly, you should be able to log in using the following test credentials:

Username: `test`

Password: `test`

After logging in, select the first channel - that's where your server is running if you used the default `config.ini` file provided on this repository.

## Next
[4.1 Config setup for localhost](https://github.com/DownWithTheFallen/MicrovoltsEmulator/blob/mv1.1_2.0/doc/example_localhost.md)
