# Changelog

All notable changes to `Medicat_Installer.sh` (linux branch).

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
