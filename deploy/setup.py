#!/usr/bin/env python3

import argparse
import ipaddress
import os
import shutil
import socket
import subprocess
import sys
import textwrap
import urllib.request
from pathlib import Path

REPO_URL = "https://github.com/DownWithTheFallen/ToyBattlesHQ.git"
REPO_BRANCH = "toybattles_mvsurge"
IMAGE_NAME = "toybattles-emu"
CONTAINER_NAME = "toybattles"
DB_VOLUME = "toybattles-db"

DEFAULTS = {
    "auth_port": 13000,
    "graded_port": 13001,
    "main_port": 13005,
    "main_ipc_port": 14005,
    "cast_port": 13006,
    "cast_ipc_port": 14006,
    "db_port": 3305,
    "website_port": 8080,
}

C = {
    "reset": "\033[0m", "bold": "\033[1m", "red": "\033[31m",
    "green": "\033[32m", "yellow": "\033[33m", "blue": "\033[34m", "cyan": "\033[36m",
}
if not sys.stdout.isatty():
    C = {k: "" for k in C}

_step_n = 0


def step(title):
    global _step_n
    _step_n += 1
    print(f"\n{C['bold']}{C['blue']}==> [{_step_n}] {title}{C['reset']}")


def info(msg):
    print(f"    {msg}")


def ok(msg):
    print(f"    {C['green']}✓ {msg}{C['reset']}")


def warn(msg):
    print(f"    {C['yellow']}! {msg}{C['reset']}")


def err(msg):
    print(f"    {C['red']}✗ {msg}{C['reset']}")


def ask(prompt, default=None):
    suffix = f" [{default}]" if default is not None else ""
    while True:
        ans = input(f"    {C['cyan']}? {prompt}{suffix}: {C['reset']}").strip()
        if ans:
            return ans
        if default is not None:
            return str(default)


def ask_yes_no(prompt, default=True):
    d = "Y/n" if default else "y/N"
    while True:
        ans = input(f"    {C['cyan']}? {prompt} [{d}]: {C['reset']}").strip().lower()
        if not ans:
            return default
        if ans in ("y", "yes"):
            return True
        if ans in ("n", "no"):
            return False


def ask_port(prompt, default):
    while True:
        ans = ask(prompt, default)
        try:
            p = int(ans)
            if 1 <= p <= 65535:
                return p
        except ValueError:
            pass
        err("Enter a valid port (1-65535).")


def ask_ip(prompt, default=None):
    while True:
        ans = ask(prompt, default)
        try:
            ipaddress.ip_address(ans)
            return ans
        except ValueError:
            err(f"'{ans}' is not a valid IP address. The servers bind to it, so it must be an IP, not a hostname.")


def run(cmd, *, check=True, capture=False, env=None, cwd=None, sudo=False, quiet=False):
    if sudo and os.geteuid() != 0:
        cmd = ["sudo"] + cmd
    if not quiet:
        info(f"{C['bold']}$ {' '.join(cmd)}{C['reset']}")
    while True:
        try:
            if capture:
                r = subprocess.run(cmd, check=check, text=True, capture_output=True, env=env, cwd=cwd)
                return r
            r = subprocess.run(cmd, check=check, env=env, cwd=cwd)
            return r
        except FileNotFoundError:
            err(f"Command not found: {cmd[0] if not sudo else cmd[1]}")
            choice = _recover()
        except subprocess.CalledProcessError as e:
            err(f"Command failed (exit {e.returncode}): {' '.join(cmd)}")
            if capture and e.stderr:
                print(e.stderr)
            choice = _recover()
        if choice == "retry":
            continue
        if choice == "skip":
            warn("Skipping this step (you may need to fix it manually).")
            return None
        err("Aborting at user request.")
        sys.exit(1)


def _recover():
    while True:
        ans = input(f"    {C['yellow']}Something went wrong. [r]etry / [s]kip / [a]bort: {C['reset']}").strip().lower()
        if ans in ("r", "retry", ""):
            return "retry"
        if ans in ("s", "skip"):
            return "skip"
        if ans in ("a", "abort"):
            return "abort"


def require_linux():
    if sys.platform != "linux":
        err("This installer only supports Linux.")
        sys.exit(1)


def detect_pkg_manager():
    for mgr in ("apt-get", "dnf", "yum", "pacman"):
        if shutil.which(mgr):
            return mgr
    return None


def detect_primary_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("1.1.1.1", 53))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except OSError:
        return None


def is_private_ip(ip):
    try:
        a = ipaddress.ip_address(ip)
        return a.is_private or a.is_loopback or a.is_link_local
    except ValueError:
        return True


def detect_public_ip():
    for url in ("https://api.ipify.org", "https://ifconfig.me/ip", "https://icanhazip.com"):
        try:
            with urllib.request.urlopen(url, timeout=5) as r:
                ip = r.read().decode().strip()
            ipaddress.ip_address(ip)
            return ip
        except Exception:
            continue
    return None


def port_in_use(port, host="0.0.0.0"):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.bind((host, port))
        return False
    except OSError:
        return True
    finally:
        s.close()


def check_port_conflicts(cfg):
    step("Checking for port conflicts (host networking shares the host's ports)")
    ports = {
        "AuthServer": cfg["auth_port"],
        "MainServer": cfg["main_port"],
        "MainServer IPC": cfg["main_ipc_port"],
        "CastServer": cfg["cast_port"],
        "CastServer IPC": cfg["cast_ipc_port"],
        "MariaDB": cfg["db_port"],
    }
    busy = {name: p for name, p in ports.items() if port_in_use(p)}
    if not busy:
        ok("All required ports are free.")
        return
    for name, p in busy.items():
        warn(f"Port {p} ({name}) is already in use on this host.")
    warn("With host networking this will collide (e.g. a MariaDB already running on this port).")
    if not ask_yes_no("Continue anyway?", default=False):
        err("Aborting so you can free those ports or re-run and choose different ones.")
        sys.exit(1)


def preflight():
    step("Pre-flight environment checks")
    if os.geteuid() == 0:
        ok("Running as root.")
    elif shutil.which("sudo"):
        ok("Not root, but sudo is available (you may be prompted for your password).")
    else:
        err("Not root and sudo is not installed. Re-run as root, or install sudo first.")
        sys.exit(1)
    mgr = detect_pkg_manager()
    if mgr:
        ok(f"Detected package manager: {mgr}")
    else:
        warn("No supported package manager (apt/dnf/yum/pacman). Host installs will be skipped; you must install Docker yourself.")
    try:
        free_gb = shutil.disk_usage("/").free / (1024 ** 3)
        if free_gb < 15:
            warn(f"Only {free_gb:.1f} GB free on / — the vcpkg build + image can need ~15-25 GB.")
        else:
            ok(f"Disk space on /: {free_gb:.1f} GB free.")
    except OSError:
        warn("Could not determine free disk space on /.")
    return mgr


def detect_ssh_port():
    parts = os.environ.get("SSH_CONNECTION", "").split()
    if len(parts) >= 4:
        try:
            return int(parts[3])
        except ValueError:
            pass
    return 22


def pkg_install(mgr, packages):
    if mgr == "apt-get":
        run(["apt-get", "update", "-y"], sudo=True)
        run(["apt-get", "install", "-y", *packages], sudo=True)
    elif mgr in ("dnf", "yum"):
        run([mgr, "install", "-y", *packages], sudo=True)
    elif mgr == "pacman":
        run(["pacman", "-Sy", "--noconfirm", *packages], sudo=True)


def ensure_docker(mgr):
    if shutil.which("docker"):
        ok("Docker already installed.")
    else:
        info("Installing Docker via the official convenience script...")
        run(["sh", "-c", "curl -fsSL https://get.docker.com | sh"], sudo=True)
    run(["systemctl", "enable", "--now", "docker"], sudo=True, check=False)
    r = run(["docker", "info"], capture=True, check=False, sudo=os.geteuid() != 0, quiet=True)
    if r is None or r.returncode != 0:
        warn("Docker daemon not reachable yet; it may need a moment or a re-login for group changes.")
    else:
        ok("Docker daemon is running.")


def gather_config():
    step("Gathering configuration")
    cfg = {}

    detected_local = detect_primary_ip()
    if detected_local:
        info(f"Auto-detected primary local IP: {C['bold']}{detected_local}{C['reset']}")
        warn("This must match the IP the servers detect internally (their primary outbound IP).")
        cfg["local_ip"] = ask_ip("Confirm or override LocalIp", detected_local)
    else:
        warn("Could not auto-detect the local IP.")
        cfg["local_ip"] = ask_ip("Enter LocalIp for the servers")

    guess_vps = bool(detected_local) and not is_private_ip(detected_local)
    if guess_vps:
        info("The detected IP looks public, so this is probably a VPS.")
    cfg["mode"] = "vps" if ask_yes_no(
        "Is this server running on a VPS (reachable from the internet)?", default=guess_vps) else "local"
    info(f"Deployment mode: {C['bold']}{cfg['mode']}{C['reset']}")

    if cfg["mode"] == "vps":
        info("Detecting public IP (querying an external service)...")
        detected_public = detect_public_ip()
        if detected_public:
            info(f"Auto-detected public IP: {C['bold']}{detected_public}{C['reset']}")
            default_public = detected_public
        elif detected_local and not is_private_ip(detected_local):
            default_public = detected_local
        else:
            warn("Could not auto-detect a public IP; please enter it manually.")
            default_public = None
        cfg["public_ip"] = ask_ip("Confirm or enter the public IP clients will connect to", default_public)
    else:
        cfg["public_ip"] = "127.0.0.1"
    info(f"Advertised/bind IP (Ip): {C['bold']}{cfg['public_ip']}{C['reset']}")

    info("Press Enter to accept default ports, or type a new value.")
    if ask_yes_no("Customize ports? (defaults are fine for most setups)", default=False):
        while True:
            cfg["auth_port"] = ask_port("AuthServer client port", DEFAULTS["auth_port"])
            cfg["main_port"] = ask_port("MainServer client port", DEFAULTS["main_port"])
            cfg["main_ipc_port"] = ask_port("MainServer IPC port", DEFAULTS["main_ipc_port"])
            cfg["cast_port"] = ask_port("CastServer client port", DEFAULTS["cast_port"])
            cfg["cast_ipc_port"] = ask_port("CastServer IPC port", DEFAULTS["cast_ipc_port"])
            cfg["db_port"] = ask_port("MariaDB port", DEFAULTS["db_port"])
            cfg["website_port"] = ask_port("Website/admin-panel API port", DEFAULTS["website_port"])
            chosen = [cfg["auth_port"], cfg["main_port"], cfg["main_ipc_port"],
                      cfg["cast_port"], cfg["cast_ipc_port"], cfg["db_port"], cfg["website_port"]]
            if len(set(chosen)) == len(chosen):
                break
            err("Those ports collide with each other; every port must be unique. Please re-enter.")
    else:
        for k, v in DEFAULTS.items():
            cfg[k] = v
    cfg["graded_port"] = DEFAULTS["graded_port"]

    cfg["expose_website"] = False
    if cfg["mode"] == "vps":
        cfg["expose_website"] = ask_yes_no("Expose the website/admin-panel API port to the internet?", default=False)

    db_pw = ask("Database root password (leave empty to auto-generate)", "")
    if not db_pw:
        db_pw = os.urandom(12).hex()
        info(f"Generated DB password: {C['bold']}{db_pw}{C['reset']}")
    cfg["db_password"] = db_pw
    cfg["jwt_secret"] = os.urandom(24).hex()

    cfg["enhanced_security"] = False
    return cfg


def locate_or_clone_repo(explicit_path):
    step("Locating the emulator source")
    if explicit_path:
        root = Path(explicit_path).resolve()
    else:
        here = Path(__file__).resolve().parent
        root = here.parent if (here.parent / "CMakeLists.txt").exists() else None
    if root and (root / "CMakeLists.txt").exists() and (root / "Setup").exists():
        ok(f"Using existing checkout at {root}")
        return root

    target = Path(ask("Directory to clone the repository into", str(Path.cwd() / "ToyBattlesHQ"))).resolve()
    if (target / "CMakeLists.txt").exists():
        ok(f"Repository already present at {target}")
        return target
    if not shutil.which("git"):
        err("git is required to clone the repository.")
        sys.exit(1)
    info(f"Cloning {REPO_URL} ({REPO_BRANCH}) into {target} ...")
    run(["git", "clone", "--branch", REPO_BRANCH, "--depth", "1", REPO_URL, str(target)])
    ok(f"Cloned into {target}")
    return target


def verify_cgd_data(root):
    eng = root / "ExternalLibraries" / "cgd_original" / "ENG"
    needed = [
        "iteminfo.cdb", "itemweaponsinfo.cdb", "setiteminfo.cdb", "upgradeinfo.cdb",
        "gachaponinfo.cdb", "gachaponpackageinfo.cdb", "rewardinfo.cdb", "gradeinfo.cdb",
        "itempackageinfo.cdb", "vendorinfo.cdb", "mapinfo.cdb", "collectioninfo.cdb",
        "effectinfo.cdb", "eventmissioninfo.cdb",
    ]
    if not eng.is_dir():
        err(f"Missing CDB data directory: {eng}")
        err("MainServer/CastServer cannot run without the unpacked cgd_original/ENG data.")
        return False
    missing = [f for f in needed if not (eng / f).exists()]
    if missing:
        warn(f"Some CDB files appear missing: {', '.join(missing)}")
        warn("MainServer may fail to start. Continuing anyway.")
        return True
    ok(f"Found all {len(needed)} CDB files in {eng}")
    return True


def verify_required_files(root):
    step("Verifying required source files")
    needed = {
        "microvolts-db.sql": root / "microvolts-db.sql",
        "RewardItemIDs.txt": root / "RewardItemIDs.txt",
        "CMakeLists.txt": root / "CMakeLists.txt",
        "vcpkg.json": root / "vcpkg.json",
        "deploy/Dockerfile": root / "deploy" / "Dockerfile",
    }
    missing = [name for name, p in needed.items() if not p.exists()]
    for name in missing:
        err(f"Missing required file: {name}")
    if not missing:
        ok("All required source files are present.")
    cgd_ok = verify_cgd_data(root)
    if missing and not ask_yes_no("Some required files are missing; continue anyway?", default=False):
        err("Aborting. Make sure you cloned the full repository.")
        sys.exit(1)
    if not cgd_ok and not ask_yes_no("CDB data is missing — MainServer and CastServer will fail to start. Continue anyway?", default=False):
        err("Aborting. The cgd_original/ENG data is required for the game servers.")
        sys.exit(1)


def render_config(cfg):
    g = "true" if cfg["enhanced_security"] else "false"
    return textwrap.dedent(f"""\
        [AuthServer]
        LocalIp = {cfg['local_ip']}
        Ip = {cfg['public_ip']}
        Port = {cfg['auth_port']}
        GradedPort = {cfg['graded_port']}
        VpnIp = 127.0.0.1
        EnhancedSecurity = {g}

        [MainServer_1]
        LocalIp = {cfg['local_ip']}
        Ip = {cfg['public_ip']}
        Port = {cfg['main_port']}
        IpcPort = {cfg['main_ipc_port']}
        IsPublic = true

        [CastServer_1]
        LocalIp = {cfg['local_ip']}
        Ip = {cfg['public_ip']}
        Port = {cfg['cast_port']}
        IpcPort = {cfg['cast_ipc_port']}
        IPC_EnableDeadBroadcast = true

        [Database]
        LocalIp = {cfg['local_ip']}
        Ip = 127.0.0.1
        Port = {cfg['db_port']}
        DatabaseName = microvolts-db
        Username = root
        PasswordEnvironmentName = MV_DB_PW

        [Website]
        Ip = {cfg['public_ip']}
        Port = {cfg['website_port']}
        JwtTokenEnvironmentName = MV_JWT
        AllowedOrigins = http://localhost

        [Client]
        ClientVersion = 0.0.3

        [General]
        EmailSecret = MVEMAIL_SECRET
        2faSecret = MV2FA_SECRET
        SmtpServer = smtp://smtp.example.com:587
        EmailSender = sender@example.com
        EmailUsername = sender@example.com
        EmailToken = MV_EMAIL_TOKEN
        SecurityNotificationReceiver = admin@example.com
        """)


def write_config(root, cfg):
    step("Writing Setup/config.ini")
    setup_dir = root / "Setup"
    setup_dir.mkdir(exist_ok=True)
    cfg_path = setup_dir / "config.ini"
    if cfg_path.exists():
        backup = setup_dir / "config.ini.bak"
        shutil.copy2(cfg_path, backup)
        info(f"Backed up existing config to {backup}")
    cfg_path.write_text(render_config(cfg))
    ok(f"Wrote {cfg_path}")
    return cfg_path


def write_env(root, cfg):
    env_path = root / "deploy" / ".env"
    env_path.write_text(
        f"MV_DB_PW={cfg['db_password']}\n"
        f"MV_JWT={cfg['jwt_secret']}\n"
        f"MV_DB_PORT={cfg['db_port']}\n"
    )
    os.chmod(env_path, 0o600)
    ok(f"Wrote secrets to {env_path} (chmod 600)")
    return env_path


_docker_prefix = None


def _resolve_docker_prefix():
    global _docker_prefix
    if _docker_prefix is not None:
        return _docker_prefix
    if os.geteuid() == 0:
        _docker_prefix = ["docker"]
        return _docker_prefix
    try:
        if subprocess.run(["docker", "info"], capture_output=True).returncode == 0:
            _docker_prefix = ["docker"]
            return _docker_prefix
    except FileNotFoundError:
        pass
    _docker_prefix = ["sudo", "docker"] if shutil.which("sudo") else ["docker"]
    return _docker_prefix


def docker_cli(args, **kw):
    return run(_resolve_docker_prefix() + args, **kw)


def build_image(root):
    step("Building the Docker image (this compiles the whole emulator; it can take a long time)")
    dockerfile = root / "deploy" / "Dockerfile"
    docker_cli(["build", "-f", str(dockerfile), "-t", IMAGE_NAME, str(root)],
               env={**os.environ, "DOCKER_BUILDKIT": "1"})
    r = docker_cli(["image", "inspect", IMAGE_NAME], capture=True, check=False, quiet=True)
    if r is None or r.returncode != 0:
        err(f"The image {IMAGE_NAME} was not produced (the build failed or was skipped). Cannot continue.")
        sys.exit(1)
    ok(f"Built image {IMAGE_NAME}")


def run_container(root, cfg, cfg_path, env_path):
    step("Starting the container")
    docker_cli(["rm", "-f", CONTAINER_NAME], check=False, quiet=True)
    docker_cli(["volume", "create", DB_VOLUME], quiet=True, check=False)
    args = [
        "run", "-d", "--name", CONTAINER_NAME,
        "--restart", "unless-stopped",
        "--network", "host",
        "--env-file", str(env_path),
        "-v", f"{cfg_path}:/app/emu/Setup/config.ini:ro",
        "-v", f"{DB_VOLUME}:/var/lib/mysql",
        IMAGE_NAME,
    ]
    docker_cli(args)
    ok(f"Container {CONTAINER_NAME} started (host networking).")


def configure_firewall(cfg):
    step("Configuring the host firewall")
    if cfg["mode"] != "vps":
        ok("Localhost mode: not opening any ports.")
        return
    if not shutil.which("ufw"):
        warn("ufw not found; skipping firewall rules. Open the game ports manually if needed.")
        return
    ssh_port = detect_ssh_port()
    ports = [cfg["auth_port"], cfg["main_port"], cfg["cast_port"]]
    if cfg["expose_website"]:
        ports.append(cfg["website_port"])
    info(f"Detected your SSH port as {C['bold']}{ssh_port}{C['reset']}.")
    info(f"Planned rules: allow SSH ({ssh_port}/tcp) and tcp {', '.join(str(p) for p in ports)}, then enable ufw.")
    info("Internal ports (IPC, MariaDB, graded) will stay closed.")
    warn("Enabling the firewall can drop active connections; verify the SSH port above is correct.")
    if not ask_yes_no("Apply these firewall rules and enable ufw now?", default=True):
        warn("Skipping firewall configuration at your request. Remember to open the ports yourself.")
        return
    run(["ufw", "allow", f"{ssh_port}/tcp"], sudo=True, check=False)
    run(["ufw", "allow", "OpenSSH"], sudo=True, check=False)
    for p in ports:
        run(["ufw", "allow", f"{p}/tcp"], sudo=True, check=False)
        ok(f"Opened port {p}/tcp")
    run(["sh", "-c", "yes | ufw enable"], sudo=True, check=False)
    ok("Firewall enabled.")


def install_enhanced_security_tools(mgr):
    step("Installing tools for (future) Enhanced Security")
    pkgs = ["wireguard", "wireguard-tools", "fail2ban"]
    if mgr in ("dnf", "yum"):
        pkgs = ["wireguard-tools", "fail2ban"]
    pkg_install(mgr, pkgs)
    run(["systemctl", "enable", "--now", "fail2ban"], sudo=True, check=False)
    ok("WireGuard tools + fail2ban installed (fail2ban enabled with default sshd jail).")
    info("WireGuard is installed but not configured; set up peers when enabling EnhancedSecurity.")


def health_check(cfg):
    step("Verifying the deployment")
    info("Recent container logs:")
    docker_cli(["logs", "--tail", "40", CONTAINER_NAME], check=False)
    r = docker_cli(["inspect", "-f", "{{.State.Running}}", CONTAINER_NAME], capture=True, check=False, quiet=True)
    running = bool(r) and r.stdout.strip() == "true"
    if running:
        ok(f"Container {CONTAINER_NAME} is running.")
    else:
        err(f"Container {CONTAINER_NAME} is not running. Inspect logs with: docker logs {CONTAINER_NAME}")


def final_summary(cfg, cfg_path):
    print(f"\n{C['bold']}{C['green']}Setup complete.{C['reset']}")
    print(textwrap.dedent(f"""
        {C['bold']}Summary{C['reset']}
          Mode             : {cfg['mode']}
          LocalIp          : {cfg['local_ip']}
          Public/Bind Ip   : {cfg['public_ip']}
          Auth / Main / Cast ports : {cfg['auth_port']} / {cfg['main_port']} / {cfg['cast_port']}
          MariaDB port     : {cfg['db_port']} (internal only)
          EnhancedSecurity : {cfg['enhanced_security']}
          Config file      : {cfg_path}

        {C['bold']}Useful commands{C['reset']}
          docker logs -f {CONTAINER_NAME}
          docker restart {CONTAINER_NAME}
          docker stop {CONTAINER_NAME}

        Default in-game test login: username 'test', password 'test'.
        """))


def confirm_plan(cfg, root, mgr, install_tools, skip_firewall):
    step("Review — nothing has been installed, built, or changed yet")
    if cfg["mode"] != "vps":
        fw = "none (localhost)"
    elif skip_firewall:
        fw = "skipped (--skip-firewall)"
    else:
        fw = f"open tcp {cfg['auth_port']}, {cfg['main_port']}, {cfg['cast_port']}"
        if cfg.get("expose_website"):
            fw += f", {cfg['website_port']}"
        fw += f" (+ SSH {detect_ssh_port()}), then enable ufw"
    tools = "Docker, WireGuard, fail2ban" if (install_tools and mgr) else "skipped"
    print(textwrap.dedent(f"""
        Mode                   : {cfg['mode']}
        LocalIp                : {cfg['local_ip']}
        Public/Bind Ip         : {cfg['public_ip']}
        Ports (auth/main/cast) : {cfg['auth_port']} / {cfg['main_port']} / {cfg['cast_port']}
        IPC (main/cast)        : {cfg['main_ipc_port']} / {cfg['cast_ipc_port']}
        MariaDB port           : {cfg['db_port']} (internal only)
        Website port           : {cfg['website_port']} ({'exposed' if cfg.get('expose_website') else 'internal'})
        EnhancedSecurity       : {cfg['enhanced_security']}
        Source repo            : {root}
        Host packages to add   : {tools}
        Firewall changes       : {fw}
        DB password            : {'(generated)' if cfg.get('db_password') else '(set)'}

        About to: install host packages, write Setup/config.ini (existing one is backed up),
        build the Docker image (compiles everything via vcpkg — can take a long time),
        start one container with MariaDB + all servers, and verify.
    """))
    if not ask_yes_no("Proceed?", default=True):
        err("Aborted by user. No system changes were made beyond locating/cloning the source.")
        sys.exit(0)


def main():
    parser = argparse.ArgumentParser(description="Automated Linux/Docker setup for the ToyBattles emulator.")
    parser.add_argument("--repo-path", help="Path to an existing checkout (skips cloning).")
    parser.add_argument("--skip-firewall", action="store_true")
    parser.add_argument("--skip-host-tools", action="store_true", help="Skip installing docker/wireguard/fail2ban on the host.")
    args = parser.parse_args()

    require_linux()
    print(f"{C['bold']}ToyBattles emulator — automated Linux setup{C['reset']}")
    mgr = preflight()

    cfg = gather_config()
    check_port_conflicts(cfg)
    root = locate_or_clone_repo(args.repo_path)
    verify_required_files(root)

    install_tools = not args.skip_host_tools
    confirm_plan(cfg, root, mgr, install_tools, args.skip_firewall)

    if install_tools and mgr:
        step("Installing host prerequisites")
        ensure_docker(mgr)
        install_enhanced_security_tools(mgr)
    else:
        warn("Skipping host tool installation.")

    cfg_path = write_config(root, cfg)
    env_path = write_env(root, cfg)
    build_image(root)
    run_container(root, cfg, cfg_path, env_path)
    if not args.skip_firewall:
        configure_firewall(cfg)
    health_check(cfg)
    final_summary(cfg, cfg_path)


if __name__ == "__main__":
    main()
