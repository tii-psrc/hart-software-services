# SCAI Notes: Building and Flashing HSS for NAVC/DPU

This is the SpacecraftAI (SCAI) clone of Microchip's Hart Software Services (HSS), the first-stage bootloader for PolarFire SoC. The main branch is `psrc2025`. For the overall boot architecture and board bring-up sequence see the top-level [scai_obc_bootloader](https://github.com/tii-psrc/scai_obc_bootloader) repository.

SCAI-specific changes on top of upstream HSS include:

* Board ports under `boards/`: `scai-navc250`, `scai-navc460`, `scai-dpu250`, `scai-dpu460` (one per computer and FPGA part), each with its MSS configuration
* Boot delay increased to 5 seconds so the Tiny CLI can be entered on the console
* Image upload/flashing support in the Tiny CLI (`ymodem` menu: ymodem and JTAG receive, compressed image support)
* Compressed-image tooling in `tools/compression`

## Building HSS for a SCAI Board

The supported `BOARD` values are `scai-navc250`, `scai-navc460`, `scai-dpu250`, `scai-dpu460`. Using `scai-dpu460` as the example:

1. Apply the board's default configuration:

   ```bash
   cd <hss-source>
   cp boards/scai-dpu460/def_config .config
   ```

2. Choose the side this build is for — **required**, the build fails without it
   (see [below](#set-the-boot-device-side-config_service_boot_device_name)):

   ```bash
   sed -i 's/^CONFIG_SERVICE_BOOT_DEVICE_NAME=.*/CONFIG_SERVICE_BOOT_DEVICE_NAME="nom"/' .config
   ```

3. (Optional) Adjust other build options:

   ```bash
   make BOARD=scai-dpu460 menuconfig
   ```

4. Build:

   ```bash
   make BOARD=scai-dpu460 -j 1
   ```

The eNVM images are produced in `build/`, notably `hss-envm-wrapper.elf` and `build/bootmode1/hss-envm-wrapper-bm1-p0.hex`.

### Set the boot device side (`CONFIG_SERVICE_BOOT_DEVICE_NAME`)

There is no runtime detection of which side a board is on: HSS cannot tell nominal from
redundant, so the side is fixed at build time by this one string, and nominal and
redundant need two separate builds.

**Every board's `def_config` leaves this empty on purpose.** A plain copy-and-build
therefore fails at compile time, in `init/hss_boot_init.c`, rather than quietly producing
an HSS that reports no side:

```
error: static assertion failed: "CONFIG_SERVICE_BOOT_DEVICE_NAME must be set to
"nom" or "red" - the board def_config leaves it empty on purpose, see README-SCAI.md"
```

Set it to `"nom"` or `"red"` before building, either with `sed` as in step 2 above, or
under `make BOARD=<board> menuconfig` → **Boot Service** → *Boot Device Name* (the symbol
depends on `SERVICE_BOOT && SERVICE_FPGA_QSPI`; both are enabled in all four SCAI board
configs, so it is always visible there).

Note the build guard can only check the length — `"nom"` and `"red"` cannot be told apart
from any other three-character value at compile time. It catches an unset or malformed
value, not a build for the wrong side.

Only `"nom"` and `"red"` are accepted downstream. The value is compiled into
`boot_info.__boot_device`, handed to U-Boot, and ends up on the kernel command line as
`boot_device=`, where the Linux boot-count service reads it.

Anything else — including the Kconfig default of `""` if the symbol is ever lost — is
refused rather than guessed at, deliberately, since guessing would attribute a boot to
the wrong side. The side is reported as `255` in boot telemetry, U-Boot prints
`ERROR: boot_device not supplied by HSS` and passes `boot_device=unknown`, and
`scai-boot-count` then exits `FAILED` and leaves the persistent counters untouched. If
you see any of those, the HSS build is the thing to fix.

Nothing downstream can catch a build made for the wrong side: a `"nom"` build on a
redundant board boots normally, but its boot telemetry and its persistent boot counters
are both attributed to the nominal side. Keep the side in the image filename and check it
before flashing.


## Flashing HSS to eNVM

HSS lives in the 128 KB eNVM of the SoC and is flashed via FlashPro. It must match the FPGA design on the board (same MSS configuration, and the design must provide the expected fabric peripherals/'SergioAPI'). There are two ways to install it:

* **Option 1 — `make program`:** with the board connected over FlashPro and powered, and a compatible FPGA design already flashed:

  ```bash
  make BOARD=scai-dpu460 program
  ```

  NB: this does not work on the first batch of DPUs — they require setting I/O states in Libero before flashing due to power issues; use option 2 instead.

* **Option 2 — via Libero:** open the FPGA design in Libero (`Project > Open > *.prjx`), add `build/bootmode1/hss-envm-wrapper-bm1-p0.hex` to the design's eNVM client, and program the whole design. FPGA designs are stored [here](https://tiiuae.sharepoint.com/:f:/r/sites/psrc/psrcteam/40-SPACE_EXPLORATION/01-Spacecraft%20AI/30_Software_and_FPGA_Artifacts?d=w5cc89f80bfda44bdab1dafaa29a59474&csf=1&web=1&e=HdxNJ8).

## MSS Configuration Files

Each board directory carries the master copy of its MSS configuration, e.g.:

```
boards/scai-dpu460/mss_config/
├── SCAI_DPU460.cfg
├── SCAI_DPU460.cxz
├── SCAI_DPU460_Report.html
└── SCAI_DPU460_mss_cfg.xml
```

(similarly `SCAI_NAVC250`, `SCAI_NAVC460`, `SCAI_DPU250` in their board directories).

To modify the hardware configuration, open the `.cfg` file in the Microchip MSS Configuration tool, change the values, save, and click **Generate** — the other files in the directory are regenerated automatically. The same configuration must be imported into the FPGA design; HSS and the FPGA design must never diverge in MSS configuration.

## Payload Generator

The payload generator packages bootloader 2 (U-Boot) into the `payload.bin` that HSS loads from the boot storage.

Build it:

```bash
sudo apt-get install libyaml-dev libelf-dev libssl-dev
cd tools/hss-payload-generator
make clean
make
```

Use it (see `tools/hss-payload-generator/README.md` for details of the YAML config):

```bash
cp u-boot-<version>.bin ./u-boot.bin
./hss-payload-generator -v -c ./uboot-linux.yaml ./payload.bin
```

Note: in the normal SCAI flow the payload is built and packaged into the NAND image by the [Yocto build](https://github.com/tii-psrc/scai-build-workspace); the manual steps above are for development/debugging.

## Compression Tool

`tools/compression` holds the miniz-based tool used to compress full NAND images before uploading them to the board over slow links (HSS decompresses on the fly when flashing):

```bash
cd tools/compression
./hss-deflate.py --verbose <image>.nand.mtdimg <image>.nand.mtdimg.miniz
```

Optionally validate a compressed file:

```bash
cd miniz
gcc -g -o validate validate.c miniz.c
./validate <file>.miniz
```

How to use the compressed image to flash a board (ymodem and JTAG procedures) is described in the [deployment guide](https://github.com/tii-psrc/scai-build-workspace/blob/main/README-deploy-scai-boards.md).
