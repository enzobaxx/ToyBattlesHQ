# Enhanced Security

## Graded Access

You can now enable advanced protection mechanisms for moderator and administrator accounts through the `config.ini` file by setting:
```cpp
EnhancedSecurity = true
```

The default value is `false`. When it is `false`, none of the mechanisms described below are active, and no VPN, email/SMTP, or 2FA configuration is required (see **When Enhanced Security Is Disabled** further below).


This flag activates **Graded Access**, a multi-layer security system designed specifically to protect privileged (mod/admin) accounts from unauthorized access, credential leaks, and brute-force attempts.

The goal of this system is simple: **even if one security layer fails, additional layers still prevent access.**

---

## What Changes When Enhanced Security Is Enabled?

When `EnhancedSecurity` is set to `true`, graded accounts are subject to strict access requirements.

### Access Requirements for Graded Accounts

A moderator or administrator account can only be accessed if **all** of the following conditions are met:

1. The correct **HWID (Hardware ID)** is provided.
2. The connection originates from the approved **VPN interface** (e.g., WireGuard).
3. The correct **username and password** are supplied.
4. A valid **2FA (Two-Factor Authentication)** code is entered.

If any of these conditions fail, access will be denied.

---

### In-Game Command Protection

Even after successful login, command execution for graded accounts still requires validation of the correct HWID.  

This ensures that:
- Stolen credentials alone are not sufficient.
- Session hijacking attempts are ineffective.
- Privileged actions remain bound to authorized hardware.

---

### Alerting & Intrusion Detection System

Enhanced Security introduces a built-in alerting system.

You must specify the recipient email address(es) for security alerts inside `config.ini`.

The alert system will trigger when:

- A graded account login is attempted without a valid HWID.
- A login attempt originates outside the authorized VPN interface.
- Incorrect credentials are repeatedly supplied.
- Suspicious or abnormal access patterns are detected.

---

### Automatic Account Lockout

To mitigate brute-force attacks:

- Graded accounts are automatically locked after **3 failed login attempts** (counted separately for wrong passwords and wrong 2FA codes).
- A security notification email is immediately sent to the configured recipients.
- Manual administrative action is required to unlock the account.

---

## When Enhanced Security Is Disabled (`EnhancedSecurity = false`)

This is the default mode. None of the Graded Access layers are used, and the server can run without any VPN, email/SMTP, or 2FA setup. Specifically:

- **Graded (mod/admin) accounts log in exactly like regular accounts** — only the correct username and password are required. There is no separate graded authentication port/server, no HWID check, no VPN requirement, and no 2FA.
- **`GradedPort` and `VpnIp` are not used** and are not required in `config.ini`.
- **No emails or security notifications are ever sent.** The entire `[General]` section (`EmailSecret`, `2faSecret`, `SmtpServer`, `EmailSender`, `EmailUsername`, `EmailToken`, `SecurityNotificationReceiver`) is not required and is ignored.
- **No 2FA anywhere.** `/changepw` and `/changeusername` work with just the password (no 2FA token, no confirmation email):
  - `/changepw <CurrentPassword> <NewPassword>`
  - `/changeusername <Password> <NewUsername>`
- **`/addplayer` and account recovery are disabled**, because they rely on email and a 2FA secret. Create accounts another way (for example, directly in the database).

To use any of the protections described above, set `EnhancedSecurity = true`.

---

# General Setup Guide for Enhanced Security

Below are the required configuration steps to properly enable Graded Access.

---

## 1. Enforcing HWID Validation

Each moderator or administrator must install a specific local client file: https://github.com/SoWeBegin/ToyBattlesHQ/tree/toybattles_mvsurge/GradedAccess/Release


### Installation Steps

- Download the provided file.
- Place it inside the game's **`Bin` directory**.
- Replace the original `steam_api.dll`.
- After everything else is set-up, the HWID that will be used to check correctness in the future will be registered in the database on the first valid login.
  
### Purpose

This modified file ensures that:
- The client securely transmits the user’s HWID to the game server.
- The server can validate the hardware identity before allowing privileged access.

This file **does not provide additional client-side functionality** — its only purpose is HWID transmission.

---

## 2. Restricting Access to a Dedicated Auth Port

Graded accounts must connect through a separate authentication port.

### Required Modifications

1. Modify `serverinfo.cdb` inside `cgd.dip`.
2. Change the authentication port for graded users  
   Example:
   - Default auth port: `13000`
   - Graded auth port: `13001`

3. Replace the public server IP with the **VPN server IP** (e.g., WireGuard IP).

---

### VPS Firewall Configuration

On the VPS hosting the server:

- Restrict port `13001` to accept connections **only from the VPN interface**.
- Ensure that the normal authentication port (`13000`) remains publicly accessible for ungraded (regular) users.

This ensures:
- Regular users are unaffected.
- Graded users must connect through the VPN.
- Public attackers cannot even reach the privileged authentication endpoint.

---

## 3. Mandatory VPN Connection (e.g., WireGuard)

All graded users must connect to the authentication server via VPN.

### Requirements

- A VPN service installed on your VPS (e.g., WireGuard).
- Each moderator/administrator must have a configured VPN client.

---

### Example Setup (WireGuard)

Server-side:
- Add each moderator's WireGuard public key to the WireGuard configuration.
- Ensure proper interface binding and routing rules are configured.

Client-side:
- Moderators configure their WireGuard client.
- The VPN must be active before attempting to log in with a graded account.

Without VPN connectivity, login attempts will fail.

---

# Two-Factor Authentication & Email Requirements

> This section applies only when `EnhancedSecurity = true`. When it is `false`, 2FA and email are not used at all (see **When Enhanced Security Is Disabled** above).

## Mandatory for Graded Accounts

Graded accounts now require:

- A verified email address
- A configured 2FA secret

Both values are:

- Encrypted using **AES-GCM**
- Stored securely
- Implemented as defined in `Common/Utils.h` (refer to source for exact format)

---

## Graded Access Login with 2FA
Username: you need to input `Username+2FaToken`, where 2FaToken is the 6 digit code from an authenticator application.
For example, if the mod's username is "mod" and their 6digit code is 123456, their username will need to be written as `mod+123456`.

---

## Ungraded Accounts

For regular users:

- Email and 2FA are only required when using:
  - `/changepw`
  - `/changeusername`

---

## Account Creation: `/addplayer`

> Available only when `EnhancedSecurity = true`. With Enhanced Security disabled, `/addplayer` is turned off (it depends on email and a 2FA secret).

A new administrative command is available: `/addplayer`

This command:

- Automatically generates a 2FA secret.
- Associates an email address.
- Sends login credentials to the specified email.

⚠ **Important:**  
This command is intended for emergency or manual administrative scenarios only.

For production environments, it is strongly recommended to:
- Implement a dedicated website for account registration.
- Provide proper password recovery workflows.
- Avoid manual account provisioning whenever possible.

---

[3.3 Setting up the database](https://github.com/SoWeBegin/MicrovoltsEmulator/blob/mv1.1_2.0/doc/database_setup.md)

