## How to apply SCAI_NAVC250 configuration file to HSS bootloader

- - - -

1. Set configuration for definition

        $ cd $(HSS source)
        $ cp boards/scai-navc250/def_config .config # When navc250 firmware should be compiled

2. Choose build options if some features are needed to be changed

        $ make BOARD=scai-navc250 menuconfig

3. Build

        $ make BOARD=scai-navc250 -j 1

- - - -

## MSS Configuration Files
To modify hardware configuration, open SCAI_NAVC250.cfg file with MSS configuration tool.
And, save changed values, and click generate button. 
As a result, new below files are updated automatically.

<pre>
scai-navc250/mss_config/
├── SCAI_NAVC250.cfg
├── SCAI_NAVC250.cxz
├── SCAI_NAVC250_Report.html
└── SCAI_NAVC250_mss_cfg.xml
</pre>

- - - -

## How to apply SCAI_NAVC460 configuration file to HSS bootloader

- - - -

1. Set configuration for definition

        $ cd $(HSS source)
        $ cp boards/scai-navc460/def_config .config # When navc460 firmware should be compiled

2. Choose build options if some features are needed to be changed

        $ make BOARD=scai-navc460 menuconfig

3. Build

        $ make BOARD=scai-navc460 -j 1

- - - -

## MSS Configuration Files
To modify hardware configuration, open SCAI_NAVC460.cfg file with MSS configuration tool.
And, save changed values, and click generate button. 
As a result, new below files are updated automatically.

<pre>
scai-navc460/mss_config/
├── SCAI_NAVC460.cfg
├── SCAI_NAVC460.cxz
├── SCAI_NAVC460_Report.html
└── SCAI_NAVC460_mss_cfg.xml
</pre>

- - - -

## How to apply SCAI_DPU250 configuration file to HSS bootloader

- - - -

1. Set configuration for definition

        $ cd $(HSS source)
        $ cp boards/scai-dpu250/def_config .config # When dpu250 firmware should be compiled

2. Choose build options if some features are needed to be changed

        $ make BOARD=scai-dpu250 menuconfig

3. Build

        $ make BOARD=scai-dpu250 -j 1

- - - -

## MSS Configuration Files
To modify hardware configuration, open SCAI_DPU250.cfg file with MSS configuration tool.
And, save changed values, and click generate button. 
As a result, new below files are updated automatically.

<pre>
scai-dpu250/mss_config/
├── SCAI_DPU250.cfg
├── SCAI_DPU250.cxz
├── SCAI_DPU250_Report.html
└── SCAI_DPU250_mss_cfg.xml
</pre>

- - - -

## How to apply SCAI_DPU460 configuration file to HSS bootloader
- - - -

1. Set configuration for definition

        $ cd $(HSS source)
        $ cp boards/scai-dpu460/def_config .config # When dpu460 firmware should be compiled

2. Choose build options if some features are needed to be changed

        $ make BOARD=scai-dpu460 menuconfig

3. Build

        $ make BOARD=scai-dpu460 -j 1

- - - -

## MSS Configuration Files
To modify hardware configuration, open SCAI_DPU460.cfg file with MSS configuration tool.
And, save changed values, and click generate button. 
As a result, new below files are updated automatically.

<pre>
scai-dpu460/mss_config/
├── SCAI_DPU460.cfg
├── SCAI_DPU460.cxz
├── SCAI_DPU460_Report.html
└── SCAI_DPU460_mss_cfg.xml
</pre>

- - - -
