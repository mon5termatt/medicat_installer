# Medicat Installer
![Logo](icon.png)

> Old readmes (batch era): [![French](https://img.shields.io/badge/French-blue)](https://github.com/mon5termatt/medicat_installer/blob/legacy/README.FR.md) [![Spanish](https://img.shields.io/badge/Spanish-blue)](https://github.com/mon5termatt/medicat_installer/blob/legacy/README.ES.md) [![Turkish](https://img.shields.io/badge/Turkish-blue)](https://github.com/mon5termatt/medicat_installer/blob/legacy/README.TR.md)

# [Visit the Medicat website](https://medicatusb.com/)

The Windows installer is now a native C++ app (`MedicatInstaller.exe`). Same job as before: Ventoy, optional format, extract MediCat, verify files. Linux still has the shell script on the `linux` branch.

### We appreciate some code improvements to the installer!
If you want to help improve Medicat installer, you can:
* Join the Discord: (https://url.medicatusb.com/discord),

OR:

* Fork this project, and create a pull request with modified files. PRs welcome on **`main`**.

# Compatibility
* Windows 10/11 (Insider builds might break the installer)
* Linux via `Medicat_Installer.sh` (Ubuntu / Arch / Debian / Fedora / Void / friends)

#### Requirements for Windows
* Windows 10/11 (1703+ is fine)
* Administrator (UAC)
* Half a brain
* A USB (or VHD) with about **30 GiB+** free, ideally 64GB+
* `MediCat.USB.v21.12.7z` beside the exe, or grab it from the built-in mirrors / Drive parts

#### Requirements for Linux
* Terminal
* Like 75% of a brain
* General Linux knowledge
* Script lives on [`linux`](https://github.com/mon5termatt/medicat_installer/tree/linux) and is attached to each release

# Grab a build

[GitHub Releases](https://github.com/mon5termatt/medicat_installer/releases)

| File | Platform |
|------|----------|
| `MedicatInstaller.exe` | Windows x64 |
| `MedicatInstaller-x86.exe` | Windows 32-bit |
| `Medicat_Installer.sh` | Linux |

# What it does (short version)

* Installs or updates **Ventoy**
* Optional **NTFS** format when you need a clean stick
* Extracts **MediCat** with progress
* **MD5 verify** + selective re-extract if something failed
* GUI (dark theme) and a proper **CLI** (`/help`, `/install`, `/verify`, ...)

More detail: [`FEATURES.md`](FEATURES.md) · [`CLI.md`](CLI.md) · [`UPDATER.md`](UPDATER.md)

# Quick start

1. Download the exe from Releases.
2. Drop `MediCat.USB.v21.12.7z` next to it (or use the in-app download).
3. Run as Administrator.
4. Pick your USB. Install. Drink water.

```bat
MedicatInstaller.exe /help
MedicatInstaller.exe /install /drive:E /yes
MedicatInstaller.exe /verify /drive:E /yes
```

Logs land beside the exe as `medicat_installer.log`. If something blows up and you upload logs, the dialog gives you a **Diag code** for Discord.

# Branches (for nerds)

| Branch | What |
|--------|------|
| `main` | Active C++ installer (you are here) |
| `linux` | Shell installer |
| `legacy` / `legacy-archive` | Old batch scripts, frozen |
| `archive/main-cpp-rewrite` | Backup of the short rewrite-era main |

Batch history is also on tag `3520`.

# Build from source

Visual Studio 2022+, CMake, Python 3, plus `bin/7z/.../7za.exe` and `MedicatFiles.md5`.

```bat
rebuild.bat
```

Outputs land in `build/Release/`. Ask in Discord if you get stuck.

# Credits

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tbody>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/SkeletonMan03"><img src="https://avatars.githubusercontent.com/u/96273359?v=4?s=100" width="100px;" alt="Lord SkeletonMan"/><br /><sub><b>Lord SkeletonMan</b></sub></a><br /><a href="#code-SkeletonMan03" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://edm115.dev"><img src="https://avatars.githubusercontent.com/u/82015596?v=4?s=100" width="100px;" alt="EDM115"/><br /><sub><b>EDM115</b></sub></a><br /><a href="#code-EDM115" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Ludo-code"><img src="https://avatars.githubusercontent.com/u/56892223?v=4?s=100" width="100px;" alt="Ludovic"/><br /><sub><b>Ludovic</b></sub></a><br /><a href="#code-Ludo-code" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Manganar"><img src="https://avatars.githubusercontent.com/u/22703860?v=4?s=100" width="100px;" alt="David Thomson"/><br /><sub><b>David Thomson</b></sub></a><br /><a href="#code-Manganar" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://dablog.pages.dev"><img src="https://avatars.githubusercontent.com/u/42101257?v=4?s=100" width="100px;" alt="Ronald Cantillo"/><br /><sub><b>Ronald Cantillo</b></sub></a><br /><a href="#code-Rooyca" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Samega7Cattac"><img src="https://avatars.githubusercontent.com/u/25128554?v=4?s=100" width="100px;" alt="Samega7Cattac"/><br /><sub><b>Samega7Cattac</b></sub></a><br /><a href="#code-Samega7Cattac" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Sipper1236"><img src="https://avatars.githubusercontent.com/u/82241081?v=4?s=100" width="100px;" alt="Sipping "/><br /><sub><b>Sipping </b></sub></a><br /><a href="#code-Sipper1236" title="Code">💻</a></td>
    </tr>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/SuperRedPanda1"><img src="https://avatars.githubusercontent.com/u/120546867?v=4?s=100" width="100px;" alt="SuperRedPanda1"/><br /><sub><b>SuperRedPanda1</b></sub></a><br /><a href="#code-SuperRedPanda1" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Teknoist"><img src="https://avatars.githubusercontent.com/u/37031361?v=4?s=100" width="100px;" alt="Mahmut Sözen"/><br /><sub><b>Mahmut Sözen</b></sub></a><br /><a href="#code-Teknoist" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Wyzzro"><img src="https://avatars.githubusercontent.com/u/57268445?v=4?s=100" width="100px;" alt="Le Touzic Ethan"/><br /><sub><b>Le Touzic Ethan</b></sub></a><br /><a href="#code-Wyzzro" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://benhampson.co.uk"><img src="https://avatars.githubusercontent.com/u/77866043?v=4?s=100" width="100px;" alt="Ben Hampson"/><br /><sub><b>Ben Hampson</b></sub></a><br /><a href="#code-ben-hampson" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://fedoraproject.org/wiki/User:Eclipseo"><img src="https://avatars.githubusercontent.com/u/30413512?v=4?s=100" width="100px;" alt="Robert-André Mauchin"/><br /><sub><b>Robert-André Mauchin</b></sub></a><br /><a href="#code-eclipseo" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/id3v1669"><img src="https://avatars.githubusercontent.com/u/57532211?v=4?s=100" width="100px;" alt="id3v1669"/><br /><sub><b>id3v1669</b></sub></a><br /><a href="#code-id3v1669" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://link.itrio.pet"><img src="https://avatars.githubusercontent.com/u/15737258?v=4?s=100" width="100px;" alt="Itrio"/><br /><sub><b>Itrio</b></sub></a><br /><a href="#code-itsitrio" title="Code">💻</a></td>
    </tr>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/keelnar"><img src="https://avatars.githubusercontent.com/u/198622?v=4?s=100" width="100px;" alt="Neelnavo Kar"/><br /><sub><b>Neelnavo Kar</b></sub></a><br /><a href="#code-keelnar" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/randompersononinternet69"><img src="https://avatars.githubusercontent.com/u/107446530?v=4?s=100" width="100px;" alt="a random person on the internet"/><br /><sub><b>a random person on the internet</b></sub></a><br /><a href="#code-randompersononinternet69" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/shenqingyi9"><img src="https://avatars.githubusercontent.com/u/37582641?v=4?s=100" width="100px;" alt="La vaguelette"/><br /><sub><b>La vaguelette</b></sub></a><br /><a href="#code-shenqingyi9" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/tolgabalper"><img src="https://avatars.githubusercontent.com/u/60055681?v=4?s=100" width="100px;" alt="Tolga Boran Alper"/><br /><sub><b>Tolga Boran Alper</b></sub></a><br /><a href="#code-tolgabalper" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/FabienRCT"><img src="https://avatars.githubusercontent.com/u/56532663?v=4?s=100" width="100px;" alt="FabienRCT"/><br /><sub><b>FabienRCT</b></sub></a><br /><a href="#code-FabienRCT" title="Code">💻</a></td>
    </tr>
  </tbody>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

* Along with all the others helping in the Discord server!

  ## Star History

<a href="https://star-history.com/#mon5termatt/medicat_installer&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=mon5termatt/medicat_installer&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=mon5termatt/medicat_installer&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=mon5termatt/medicat_installer&type=Date" />
 </picture>
</a>
