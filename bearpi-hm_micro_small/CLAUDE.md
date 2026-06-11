# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OpenHarmony 3.0 embedded OS for the **BearPi-HM Micro** development board (STM32MP157, ARM Cortex-A7). Runs the **LiteOS-A** real-time microkernel. Written primarily in C/C++, with some JavaScript (app framework) and Python (build tooling).

## Build Commands

The build system uses **GN + Ninja**, orchestrated by shell scripts and the `hb` (HarmonyOS Build) tool.

```bash
# Full build (recommended)
hb set                          # select product: choose "." then "bearpi_hm_micro"
hb build -t notest --tee -f     # build firmware

# Alternative direct build
./build/build_scripts/build.sh --product-name bearpi_hm_micro

# Build with ccache
hb build -t notest --tee -f --ccache

# Build specific target
hb build -t notest --tee -f --build-target <target_name>
```

**Output** lands in `out/bearpi_hm_micro/bearpi_hm_micro/`:
- `OHOS_Image.stm32` — system image
- `rootfs_vfat.img` — root filesystem
- `userfs_vfat.img` — user filesystem

**Build prerequisites** (Ubuntu 18.04+): `build-essential`, `gcc-multilib`, `g++-multilib`, `lib32ncurses5-dev`, `lib32z-dev`, `ccache`, `libgl1-mesa-dev`, `libxml2-utils`, `xsltproc`, `flex`, `bison`, `gperf`, `curl`, `zip`, `unzip`, `m4`. Pre-built toolchains (clang, GN, Ninja, Python 3.8.5) are downloaded via `build/prebuilts_download.sh`.

## Testing

```bash
# XTS test suite
cd test/xts/acts
./build.sh
./runtest.sh

# Kernel tests
kernel/liteos_a/testsuites/
```

Google Test is available at `third_party/googletest`. The test execution framework is at `test/xdevice/`.

## Architecture

### Layer Model (bottom-up)

1. **Kernel** (`kernel/liteos_a/`) — LiteOS-A microkernel: process scheduling, memory management, IPC, VFS, FAT/JFFS2/NFS filesystems, lwip networking, POSIX compatibility, HDF driver framework.

2. **Drivers** (`drivers/`) — OpenHarmony Driver Foundation (HDF). Peripheral HALs for display, input, WLAN, audio, camera, USB, sensors under `drivers/peripheral/`. Board-specific hardware adapter at `device/st/hardware`.

3. **System Services** — split across two trees:
   - `foundation/` — core frameworks: **ACE** (UI rendering), **graphic** (surface compositing, windowing), **aafwk** (ability framework), **appexecfwk** (app execution), **communication** (IPC lite), **distributedschedule** (samgr/safwk/dmsfwk lite)
   - `base/` — platform services: **startup** (init, appspawn, bootstrap), **security** (permissions, hichainsdk), **powermgr**, **sensors**, **notification**, **telephony**, **global** (i18n, resource mgr)

4. **Applications** (`applications/BearPi/BearPi-HM_Micro/samples/`) — C++ apps packaged as HAP (HarmonyOS Ability Package): launcher, screensaver, setting, communication (hostapd/wpa_supplicant).

5. **Third-party** (`third_party/`) — cJSON, jerryscript (JS engine), mbedtls, lwip, freetype, libpng, libjpeg, openssl, zlib, FatFs, googletest, and more.

### Build System Internals

- **GN templates**: `ohos_executable`, `ohos_shared_library`, `ohos_static_library`, `ohos_source_set`, `ohos_prebuilt_etc`, `hap_pack` — defined in `build/core/`.
- **Product config**: `vendor/bearpi/bearpi_hm_micro/config.json` — declares all subsystems and their components.
- **Board config**: `device/st/bearpi_hm_micro/liteos_a/config.gni` — CPU (cortex-a7), toolchain (clang), float ABI (softfp), NEON flags, eMMC storage.
- **Subsystem mapping**: `build/subsystem_config.json` — maps subsystem names to source paths.

### Key Config Files

| File | Purpose |
|---|---|
| `ohos_config.json` | Active product/board/kernel selection |
| `vendor/bearpi/bearpi_hm_micro/config.json` | Product subsystem/component manifest |
| `device/st/bearpi_hm_micro/liteos_a/config.gni` | Board-level GN build flags |
| `build/subsystem_config.json` | Subsystem name → source path mapping |

## Firmware Flashing

Flash output images via **STM32CubeProgrammer** to the STM32MP157 board. Monitor via serial console.
