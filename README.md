# MediCat USB — Linux installer

Bash installer for MediCat USB on Linux: **`Medicat_Installer.sh`**.

Website: [medicatusb.com](https://medicatusb.com/) · Discord: [url.medicatusb.com/discord](https://url.medicatusb.com/discord)

## Quick start

```bash
curl -fsSL https://raw.githubusercontent.com/mon5termatt/medicat_installer/linux/Medicat_Installer.sh | bash
```

Or clone this branch and run locally:

```bash
git clone -b linux https://github.com/mon5termatt/medicat_installer.git
cd medicat_installer
chmod +x Medicat_Installer.sh
./Medicat_Installer.sh
```

## Compatibility

Ubuntu · Arch · Debian · CentOS · FreeBSD · Fedora · Void · NixOS (and similar)

## Requirements

* Terminal access
* Basic Linux knowledge (disks, mounts, package managers)
* Network access to install packages and download Ventoy / MediCat

The script installs or uses packages such as `wget`, `p7zip`/`7z`, `ntfs-3g` / `ntfsprogs` (`mkntfs`), `dosfstools`, `exfatprogs`, `parted`, and `aria2` when needed.

## What it does

1. Resolves and installs dependencies for your distribution
2. Downloads and extracts Ventoy (Linux) when required
3. Optionally downloads `MediCat.USB.v21.12.7z` via BitTorrent
4. Verifies the archive SHA-256 hash
5. Installs Ventoy on the chosen disk and extracts MediCat onto the USB volume

## Main file

### `Medicat_Installer.sh`

Command-line installer for Linux.

*Originally coded by: [@SkeletonMan03](https://github.com/SkeletonMan03)*

## Contributing

Improvements to the Linux installer are welcome:

* Join the Discord: https://url.medicatusb.com/discord
* Fork this project and open a pull request against the **`linux`** branch

## Credits

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

* Along with everyone helping in the Discord server!

## Star History

<a href="https://star-history.com/#mon5termatt/medicat_installer&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=mon5termatt/medicat_installer&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=mon5termatt/medicat_installer&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=mon5termatt/medicat_installer&type=Date" />
 </picture>
</a>
