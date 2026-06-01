# Automated Linux / Docker setup

`setup.py` provisions the whole ToyBattles emu on a Linux host: it installs the
host prerequisites, generates `Setup/config.ini`, builds a Docker image that contains
the three servers **and** MariaDB, initializes the database (with the default `test`
user), starts everything in one container, and finally (on a VPS only) it opens the required ports.

`EnhancedSecurity` is set to `false` by default, so no VPN / email / 2FA configuration
is needed (but this can be later changed manually).

## Usage

Run on the target Linux machine (needs `sudo` for package installs and firewall):

```bash
python3 deploy/setup.py
```

### Two ways to run it

- **You already cloned the repo** (you have this `deploy/` folder): run
  `python3 deploy/setup.py` from inside the checkout. It detects the repo and **skips
  cloning** (this is expected)
- **Fresh machine, nothing cloned yet**: download only `setup.py` and run it; it will
  clone the full repo for you (this is what brings the `Dockerfile` and the rest):

  ```bash
  curl -fsSL https://raw.githubusercontent.com/SoWeBegin/ToyBattlesHQ/toybattles_mvsurge/deploy/setup.py -o setup.py
  python3 setup.py
  ```

`setup.py` by itself cannot build anything — the `Dockerfile`, `Dockerfile.dockerignore`
and the `docker/` scripts live in this `deploy/` folder. The clone path therefore only
works once `deploy/` has been committed and pushed to the repository.

It will ask for: localhost vs VPS, `LocalIp` (auto-detected), public `Ip`, an optional
port override, and a DB password (auto-generated if left blank). Everything else is
automated. Just follow the instructions.

### What it checks and confirms

- **Pre initialization**: root/sudo availability, package manager, and free disk space.
- **VPS guess**: auto guessed from whether your IP is public, but you still have to confirm it.
- **IP detection**: local IP via the primary outbound route, public IP via an external
  lookup (eg. whois). Both are shown for confirmation and **required** to be
  entered if detection fails
- **Port conflicts**: every port is validated, checked for duplicates, and checked
  against ports already in use on the host 
- **Source files**: verifies the repo, `microvolts-db.sql`, `RewardItemIDs.txt`, and the
  `cgd_original/ENG` CDB data exist (asks before continuing if any is missing).
- **Build verification**: confirms the image was actually produced before starting it.
- **Firewall**: detects your real SSH port and confirms before enabling `ufw`, so you
  can't lock yourself out unintentionally

Flags:
- `--repo-path PATH`: use an existing checkout instead of cloning.
- `--skip-firewall`: don't touch `ufw`.
- `--skip-host-tools`: don't install docker/wireguard/fail2ban.

## What gets created

- `Setup/config.ini` => generated from your answers (existing one is backed up)
- `deploy/.env` => `MV_DB_PW`, `MV_JWT`, `MV_DB_PORT` (with permissions)
- Docker image `toybattles-emu`, container `toybattles` (host networking + auto-restart).
- Named volume `toybattles-db` for MariaDB

## How the container runs

A single container runs MariaDB + AuthServer + MainServer + CastServer under
`supervisord`, started in order (MariaDB => DB init => Auth => Cast => Main). The servers
run from `/app/emu/Output`.

```bash
docker logs -f toybattles
docker restart toybattles
docker stop toybattles
```

## Notes / caveats

- Linux only. Beware that this tool uses `--network host`
- `LocalIp` must equal the IP the servers detect internally (outbound IP), the
  installer auto detects it but you should double check and confirm always. 
- The image build compiles all vcpkg dependencies and can take time (~15 minutes, more or less)
- Enhanced Security tooling (WireGuard, fail2ban) is installed but not configured, set it
  up when you switch `EnhancedSecurity = true` (and follow the section regarding enhanced security)
