# Changelog

All notable changes to `Medicat_Installer.sh` (linux branch).

## 0024

- Install `ntfsprogs` for `mkntfs` on Arch and Fedora. `ntfs-3g` no longer ships the NTFS userspace tools after the 2026 package split, so the script no longer treats a missing `mkntfs` as a missing `ntfs-3g`. Falls back to `ntfsprogs` if `mkntfs` is still absent after the first install.
- Drop the files.dog TLS bypass. The mirror now serves a valid Cloudflare / Google Trust Services certificate, so aria2c/wget verify SSL normally.

## 0023

- Skip package-index refresh (`apt update` / `pacman -Syy` / etc.) when every required command is already installed (#139).

## 0022

- Fix NTFS mount for distros without in-kernel ntfs3: try ntfs3, ntfs, ntfs-3g, then auto; verify `mountpoint` before extract so a failed mount cannot dump MediCat onto the local disk (#81).

## 0021

- Accept Google Drive / Mega multi-volume archives (`MediCat.USB.v21.12.zip.001`-.006): verify each part MD5, then extract with `7z` from `.001` (addresses #87).

## 0020

- Fix Ventoy download/rename when GitHub tag parsing failed (`mv ventoy-` / missing `Ventoy2Disk.sh`) by stripping the tag to digits+dots and validating the extracted folder (addresses #99 / #142-style `venver` bugs).
- Make wget `--show-progress` optional so older wget builds still download Ventoy.
- Reject truncated MediCat archives before hashing; clearer SHA256 mismatch hints for wrong format / incomplete downloads.
- Detect Google Drive `.zip.001-.006` in the working directory and point users at the solid `.7z`.
- Fix `Medicat7zFull` path quoting for the nested `MediCat USB v21.12/` layout.

## 0019

- Warn when free space is low: working directory (~1 GiB), MediCat download (~22 GiB), and USB extract (~26 GiB). Shows `df` and asks before continuing; download/extract gates use a stronger confirm.
- Note that 32GB sticks work but are just barely enough (prefer 64GB+).

## 0018

- Non-USB wipe gate requires typing exact `CONFIRM` (not Y/N) before the normal install prompt.

## 0017

- Warn and require an extra confirmation when the selected disk is not USB (`TRAN!=usb`), to reduce accidental wipes of internal HDD/SSD/NVMe.
- Include `TRAN` in the drive list so USB devices are easier to spot.

## 0016

- All interactive prompts (keypress wait, Yes/No, device name) always read from `/dev/tty` so they never skip under `curl | bash`, pipes, or empty stdin.
- USB device entry is re-prompted until a real block device is chosen; empty `/dev/` no longer reaches the wipe confirmation.
- Correct first-partition path for `nvme*` (`…p1`) vs `sd*` (`…1`).

## 0015

- Fix colour init: stop shadowing `/usr/bin/clear` with the sgr0 variable (could break the welcome banner); safer `colEcho` via `printf`.
- Fix Debian/Ubuntu package install: use `exfatprogs` (`exfat-utils` is gone on Bookworm+), which previously aborted the whole apt transaction so aria2/7z never installed.
- Print script version at start.

## 0014

- Install dependencies one package at a time so one missing/obsolete package cannot cancel the rest of the apt/dnf transaction.
- Pass `-y` / `--noconfirm` / `--no-interactive` on package installs (apt, yum, dnf, pkg, apk, xbps; pacman already used `--noconfirm`).
- Abort if required commands are still missing after install; re-check afterward.
- Direct download falls back to `wget` when `aria2c` is missing.
- Cap aria2c concurrency at `-x16 -s16` (aria2 max is 16; was `-x32 -s32`).

## 0013

- Allow running as root/sudo after an explicit warning confirmation (useful for root-only test VMs).
- Privileged commands use `$sudo` so they work when already root (empty sudo) or as a normal user.

## 0012

- Offer download method choice: direct HTTP (aria2c multi-connection), BitTorrent, or local path when the MediCat archive is missing.
- Primary direct mirror: `files.medicatusb.com` (`aria2c -x16 -s16 -k1M -c`).
- Fallback mirror: `files.dog` (known broken SSL; aria2c without cert check).

## 0011

- Fix YesNo infinite loop: empty/EOF input no longer spins forever on "Invalid input". Read failures exit cleanly; non-TTY stdin falls back to `/dev/tty` (`curl | bash`, pipelines).
- YesNo returns exit status (`0` = yes, `1` = no) instead of echoing `true`/`false`; call sites no longer use command substitution `$(YesNo ...)`.
- Trim CR/whitespace; accept `Y`/`Yes` and `N`/`No` (case-insensitive prefix).
