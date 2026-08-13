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

2. (Optional) Adjust build options:

   ```bash
   make BOARD=scai-dpu460 menuconfig
   ```

3. Build:

   ```bash
   make BOARD=scai-dpu460 -j 1
   ```

The eNVM images are produced in `build/`, notably `hss-envm-wrapper.elf` and `build/bootmode1/hss-envm-wrapper-bm1-p0.hex`.


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
