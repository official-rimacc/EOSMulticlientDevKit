# Dependencies — Epic's EOS tools go here

This folder is where **you** drop the Epic-owned tools the kit works with. They are part of the
EOS SDK and Epic's terms don't allow this plugin to redistribute them, so the folder ships empty
(everything except this file is gitignored).

## Where to get them

Download the **EOS SDK** (plain C SDK is fine) from the
[Epic Developer Portal](https://dev.epicgames.com/portal) — *your product ▸ SDK ▸ Download* —
and extract it anywhere. Everything below comes from its `SDK/Tools/` folder.
Epic's walkthrough: [Set Up Your Account and Download the EOS SDK](https://dev.epicgames.com/docs/epic-online-services/eos-get-started/get-started-guide/set-up-account-and-download-eos-sdk).

## What to drop in, exactly

```
Dependencies/
  EOSBootstrapperTool.exe            from SDK/Tools — generates Play.exe for packaged builds
  EOSBootstrapper.exe                from SDK/Tools — the template Play.exe is stamped from
  EpicOnlineServicesInstaller.exe    from SDK/Tools — installs the EOS service on player machines
  DevAuthTool/
    EOS_DevAuthTool.exe              unzip SDK/Tools/EOS_DevAuthTool-win32-x64-<ver>.zip,
    (plus its DLLs, locales/, ...)   drop the whole extracted folder here as "DevAuthTool"
```

## What each one is for

- **[DevAuthTool](https://dev.epicgames.com/docs/epic-online-services/accounts-and-social/eos-epic-account-services/developer-authentication-tool)** —
  a small local login server holding your signed-in test accounts, so Quick Launch can start several
  clients with different EOS identities. Sign in one account per slot and name each login exactly
  `Player1`, `Player2`, `Player3`, `Player4` (matching the Setup tab). It has to be **running and
  signed in every time you test** — without it, launched clients can't log in.
- **EOSBootstrapperTool + EOSBootstrapper + EpicOnlineServicesInstaller** — used by the panel's
  **Package tab** after you package your game, so the EOS overlay works in the shipped build.
  See "Packaged builds" in the main README.

Only DevAuthTool is needed for everyday multi-client testing. The other three only matter when
you package.
