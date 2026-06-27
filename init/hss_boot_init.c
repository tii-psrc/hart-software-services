/*******************************************************************************
 * Copyright 2019-2025 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 * MPFS HSS Embedded Software
 *
 */

/**
 * \file HSS Boot Initalization
 * \brief Boot Initialization
 */

#include "config.h"
#include "hss_types.h"

#include "hss_init.h"
#include "hss_boot_service.h"
#include "hss_boot_init.h"
#include "hss_sys_setup.h"
#include "hss_progress.h"
#include "hss_trigger.h"
#include "u54_state.h"

#if IS_ENABLED(CONFIG_SERVICE_SPI)
#  include <mss_sys_services.h>
#  define SPI_FLASH_BOOT_ENABLED (CONFIG_SERVICE_BOOT_SPI_FLASH_OFFSET != 0xFFFFFFFF)
#else
#  define SPI_FLASH_BOOT_ENABLED 0
#endif

#if IS_ENABLED(CONFIG_SERVICE_OPENSBI)
#  include "opensbi_service.h"
#endif

#if IS_ENABLED(CONFIG_SERVICE_YMODEM)
#  include "ymodem.h"
#  include "ddr_service.h"
#  include "uart_helper.h"
#endif

#if IS_ENABLED(CONFIG_SERVICE_QSPI)
#  include "qspi_service.h"
#endif

#if IS_ENABLED(CONFIG_SERVICE_FPGA_QSPI)
#  include "fpga_qspi_service.h"
#endif

#if IS_ENABLED(CONFIG_SERVICE_MMC)
#  include "mmc_service.h"
#  include "gpt.h"
#endif

#if (SPI_FLASH_BOOT_ENABLED)
#  include "mss_sys_services.h"
#endif

#include "hss_state_machine.h"
#include "hss_debug.h"
#include "hss_perfctr.h"

#include <string.h>
#include <assert.h>

#if IS_ENABLED(CONFIG_COMPRESSION)
#  include "hss_decompress.h"
#endif

#if IS_ENABLED(CONFIG_CRYPTO_SIGNING)
#  include "hss_boot_secure.h"
#endif

#include "hss_boot_pmp.h"
#include "hss_atomic.h"

#include "sbi_bitops.h"

//
// local module functions

#if IS_ENABLED(CONFIG_SERVICE_BOOT)
typedef bool (*HSS_BootImageCopyFnPtr_t)(void *pDest, size_t srcOffset, size_t byteCount);
static bool copyBootImageToDDR_(struct HSS_BootImage *pBootImage, char *pDest,
    size_t srcOffset, HSS_BootImageCopyFnPtr_t pCopyFunction);

static void printBootImageDetails_(struct HSS_BootImage const * const pBootImage);
static bool tryBootFunction_(struct HSS_Storage *pStorage, HSS_GetBootImageFnPtr_t getBootImageFunction);
#endif

static bool getBootImageFromQSPI_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage);
static bool getBootImageFromFPGAQSPI_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage);
static bool getBootImageFromMMC_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage);
static bool getBootImageFromSpiFlash_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage);
static bool getBootImageFromPayload_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage);
static bool getBootImageFromYModemPayload_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage);


#if IS_ENABLED(CONFIG_SERVICE_FPGA_QSPI) || IS_ENABLED(CONFIG_SERVICE_QSPI)
unsigned long __active_slot = 0;
#endif

#if IS_ENABLED(CONFIG_SERVICE_FPGA_QSPI)
static struct HSS_Storage fpgaqspiStorage_ = {
    .name = "FPGA_QSPI",
    .getBootImage = getBootImageFromFPGAQSPI_,
    .init = HSS_FPGA_QSPIInit,
    .readBlock = NULL,
    .writeBlock = NULL,
    .getInfo = HSS_FPGA_GetInfo,
    .flushWriteBuffer = NULL
};
#endif
#if IS_ENABLED(CONFIG_SERVICE_QSPI)
static struct HSS_Storage qspiStorage_ = {
    .name = "QSPI",
    .getBootImage = getBootImageFromQSPI_,
    .init = HSS_CachedQSPIInit,
    .readBlock = HSS_CachedQSPI_ReadBlock,
    .writeBlock = HSS_CachedQSPI_WriteBlock,
    .getInfo = HSS_CachedQSPI_GetInfo,
    .flushWriteBuffer = HSS_CachedQSPI_FlushWriteBuffer
};
#endif
#if IS_ENABLED(CONFIG_SERVICE_MMC)
static struct HSS_Storage mmcStorage_ = {
    .name = "MMC",
    .getBootImage = getBootImageFromMMC_,
    .init = HSS_MMCInit,
    .readBlock = HSS_MMC_ReadBlock,
    .writeBlock = HSS_MMC_WriteBlockSDMA,
    .getInfo = HSS_MMC_GetInfo,
    .flushWriteBuffer = NULL
};
#endif
#if IS_ENABLED(CONFIG_SERVICE_SPI)
static struct HSS_Storage spiStorage_ = {
    .name = "SPI",
    .getBootImage = getBootImageFromSpiFlash_,
    .init = NULL,
    .readBlock = NULL,
    .writeBlock = NULL,
    .getInfo = NULL,
    .flushWriteBuffer = NULL
};
#endif
#if IS_ENABLED(CONFIG_SERVICE_BOOT_USE_PAYLOAD)
static struct HSS_Storage payloadStorage_ = {
    .name = "Payload",
    .getBootImage = getBootImageFromPayload_,
    .init = NULL,
    .readBlock = NULL,
    .writeBlock = NULL,
    .getInfo = NULL,
    .flushWriteBuffer = NULL
};
#endif
#if IS_ENABLED(CONFIG_SERVICE_YMODEM)
static struct HSS_Storage ymodemPayload_ = {
    .name = "YMODEM_PAYLOAD",
    .getBootImage = getBootImageFromYModemPayload_,
    .init = NULL,
    .readBlock = NULL,
    .writeBlock = NULL,
    .getInfo = NULL,
    .flushWriteBuffer = NULL
};
#endif

static struct HSS_Storage *pStorages[] =
{
#if IS_ENABLED(CONFIG_SERVICE_FPGA_QSPI)
	&fpgaqspiStorage_,
#endif
#if IS_ENABLED(CONFIG_SERVICE_QSPI)
	&qspiStorage_,
#endif
#if IS_ENABLED(CONFIG_SERVICE_SPI)
	&spiStorage_,
#endif
#if IS_ENABLED(CONFIG_SERVICE_MMC)
	&mmcStorage_,
#endif
#if IS_ENABLED(CONFIG_SERVICE_BOOT_USE_PAYLOAD)
	&payloadStorage_,
#endif
#if IS_ENABLED(CONFIG_SERVICE_YMODEM)
	&ymodemPayload_,
#endif
};

static struct HSS_Storage *pDefaultStorage = NULL;

#if IS_ENABLED(CONFIG_SERVICE_MMC) || IS_ENABLED(CONFIG_SERVICE_QSPI) || IS_ENABLED(CONFIG_SERVICE_FPGA_QSPI) || (IS_ENABLED(CONFIG_SERVICE_SPI) && (SPI_FLASH_BOOT_ENABLED)) || IS_ENABLED(CONFIG_SERVICE_YMODEM)
struct HSS_BootImage bootImage __attribute__((aligned(8)));
#elif IS_ENABLED(CONFIG_SERVICE_BOOT_USE_PAYLOAD)
//
#else
#    error Unable to determine boot mechanism
#endif

struct HSS_Storage *HSS_BootGetActiveStorage(void);
struct HSS_Storage *HSS_BootGetActiveStorage(void)
{
    struct HSS_Storage *pResult = pDefaultStorage;

    if (!pResult) {
        pResult = pStorages[0];
    }

    return pResult;
}

void HSS_BootListStorageProviders(void)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(pStorages); i++) {
        mHSS_DEBUG_PRINTF_EX(" - %s\n", pStorages[i]->name);
    }
}

void HSS_BootHarts(void)
{
#if IS_ENABLED(CONFIG_SERVICE_BOOT)
        union HSSHartBitmask restartHartBitmask = { .uint = 0u };

        for (int i = HSS_HART_U54_1; i < HSS_HART_NUM_PEERS; i++) {
            //mHSS_DEBUG_PRINTF(LOG_ERROR, "%s(): checking u54_%d\n", __func__, i);
            if (HSS_U54_GetState_Ex(i) == HSS_State_Idle) {
                //mHSS_DEBUG_PRINTF(LOG_ERROR, "%s(): => rebooting u54_%d\n", __func__, i);
                restartHartBitmask.uint |= BIT(i);
            }
        }

        if (restartHartBitmask.uint) {
            HSS_Boot_RestartCores_Using_Bitmask(restartHartBitmask);
        }
#endif
}

bool HSS_BootInit(void)
{
    bool result = true;
#if IS_ENABLED(CONFIG_SERVICE_BOOT)

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Initializing Boot Image ...\n");

    int perf_ctr_index = PERF_CTR_UNINITIALIZED;
    HSS_PerfCtr_Allocate(&perf_ctr_index, "Boot Image Init");

    if (pDefaultStorage) {
        if (pDefaultStorage->init) { result = pDefaultStorage->init(); }
        if (result) {
            result = tryBootFunction_(pDefaultStorage, pDefaultStorage->getBootImage);
        }
    } else {
        for (int i = 0; i < ARRAY_SIZE(pStorages); i++) {
            mHSS_DEBUG_PRINTF(LOG_NORMAL, "Trying to get boot image via %s ...\n", pStorages[i]->name);

            if (pStorages[i]->init) {
                result = pStorages[i]->init();
            } else {
                result = true;
            }

            if (result) {
                result = tryBootFunction_(pStorages[i], pStorages[i]->getBootImage);
                if (result) { break; }
            }
        }
    }

    HSS_PerfCtr_Lap(perf_ctr_index);
#endif

    return result;
}

#if IS_ENABLED(CONFIG_SERVICE_BOOT)
bool tryBootFunction_(struct HSS_Storage *pStorage, HSS_GetBootImageFnPtr_t const bootImageFunction)
{
    bool result = false;
    (void)pStorage;

    struct HSS_BootImage *pBootImage = NULL;
    bool decompressedFlag = false;

    (void)decompressedFlag;


    result = bootImageFunction(pStorage, &pBootImage);
    //
    // check if this image is compressed...
    // if so, decompress it to DDR
    //
    // for now, compression only works with a source already in DDR
#  if IS_ENABLED(CONFIG_COMPRESSION)
    if (result && pBootImage && (pBootImage->magic == mHSS_COMPRESSED_MAGIC)) {
        decompressedFlag = true;

        mHSS_DEBUG_PRINTF(LOG_NORMAL, "Preparing to decompress to DDR ...\n");
        void* const pInput = (void*)pBootImage;
        void * const pOutputInDDR = (void *)(CONFIG_SERVICE_BOOT_DDR_TARGET_ADDR);

        int outputSize = HSS_Decompress(pInput, pOutputInDDR);
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "decompressed %d bytes ...\n", outputSize);

        if (outputSize) {
            pBootImage = (struct HSS_BootImage *)pOutputInDDR;
        } else {
            pBootImage = NULL;
        }
    } else if (!result) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to get boot image, cannot decompress\n");
        result = false;
    } else if (!pBootImage) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Boot Image NULL, ignoring\n");
        result = false;
    }
#  endif

    if (result) {
        HSS_Register_Boot_Image(pBootImage);
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s: Boot Image registered ...\n", pStorage->name);

#if IS_ENABLED(CONFIG_SERVICE_FPGA_QSPI) || IS_ENABLED(CONFIG_SERVICE_QSPI)
        if (!strncmp(pStorage->name, "FPGA_QSPI", 9)) {
          __active_slot = (unsigned long)'b';
        } else if (!strncmp(pStorage->name, "QSPI", 4)) {
          __active_slot = (unsigned long)'a';
        } else {
          __active_slot = 0;
        }
#endif

    } else {
        HSS_Register_Boot_Image(NULL);
    }

    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////

static void printBootImageDetails_(struct HSS_BootImage const * const pBootImage)
{
#  ifdef BOOT_DEBUG
    mHSS_DEBUG_PRINTF(LOG_NORMAL, " - set name is >>%s<<\n", pBootImage->set_name);
    mHSS_DEBUG_PRINTF(LOG_NORMAL, " - magic is    %08x\n", pBootImage->magic);
    mHSS_DEBUG_PRINTF(LOG_NORMAL, " - length is   %08x\n", pBootImage->bootImageLength);
#  endif
}
#endif

#if IS_ENABLED(CONFIG_SERVICE_BOOT) && (IS_ENABLED(CONFIG_SERVICE_YMODEM) || IS_ENABLED(CONFIG_SERVICE_FPGA_QSPI))
static void __print_boot_img_info(int hartid, struct HSS_BootImage const * const pBootImage)
{
  int i;
  char buf[1024];

	memset(buf, 0, sizeof(buf));

  format_log(hartid, buf, "HSS Boot Image Header\r\n");
  format_log(hartid, buf, "  - magic              : 0x%08X \r\n",
			pBootImage->magic);
  format_log(hartid, buf, "  - version            : 0x%08X \r\n",
			pBootImage->version);
  format_log(hartid, buf, "  - headerLength       : 0x%08X(%d) \r\n",
			pBootImage->headerLength, pBootImage->headerLength);
  format_log(hartid, buf, "  - headerCrc          : 0x%08X \r\n",
			pBootImage->headerCrc);
  format_log(hartid, buf, "  - chunkTableOffset   : 0x%08X \r\n",
			pBootImage->chunkTableOffset);
  format_log(hartid, buf, "  - ziChunkTableOffset : 0x%08X \r\n",
			pBootImage->ziChunkTableOffset);
  format_log(hartid, buf, "  - set name           : %s     \r\n",
			pBootImage->set_name);
  format_log(hartid, buf, "  - bootImageLength    : 0x%08X(%d) \r\n",
			pBootImage->bootImageLength, pBootImage->bootImageLength);

  for (i=0; i<MAX_NUM_HARTS-1; i++) {
    format_log(hartid, buf, "    - U54 hart[%d].entryPoint    : 0x%08X \r\n",
				i+1, pBootImage->hart[i].entryPoint);
    format_log(hartid, buf, "    - U54 hart[%d].privMode      : 0x%02X \r\n",
				i+1, (int)pBootImage->hart[i].privMode);
    format_log(hartid, buf, "    - U54 hart[%d].flags         : 0x%02X \r\n",
				i+1, (int)pBootImage->hart[i].flags);
    format_log(hartid, buf, "    - U54 hart[%d].numChunks     : 0x%08X \r\n",
				i+1, pBootImage->hart[i].numChunks);
    format_log(hartid, buf, "    - U54 hart[%d].firstChunk    : 0x%08X \r\n",
				i+1, pBootImage->hart[i].firstChunk);
    format_log(hartid, buf, "    - U54 hart[%d].lastChunk     : 0x%08X \r\n",
				i+1, pBootImage->hart[i].lastChunk);
    format_log(hartid, buf, "    - U54 hart[%d].name          : %s     \r\n",
				i+1, pBootImage->hart[i].name);
  }

  format_log(hartid, buf, "  - signature.digest   : ");
  for (i=0; i<sizeof(pBootImage->signature.digest); i++) {
    if (i % 0x10 == 0)
      format_log(hartid, buf, "\r\n     [0x%08X] ", i);
    format_log(hartid, buf, " 0x%02X",
				(int)pBootImage->signature.digest[i]);
  }
  format_log(hartid, buf, "\r\n");

  format_log(hartid, buf, "  - signature.ecdsaSig   : ");
  for (i=0; i<sizeof(pBootImage->signature.ecdsaSig); i++) {
    if (i % 0x10 == 0)
      format_log(hartid, buf, "\r\n     [0x%08X] ", i);
    format_log(hartid, buf, " 0x%02X",
				(int)pBootImage->signature.ecdsaSig[i]);
  }
  format_log(hartid, buf, "\r\n");
}
#endif

#if IS_ENABLED(CONFIG_SERVICE_BOOT)
static bool copyBootImageToDDR_(struct HSS_BootImage *pBootImage, char *pDest,
    size_t srcOffset, HSS_BootImageCopyFnPtr_t pCopyFunction)
{
    bool result = true;

    printBootImageDetails_(pBootImage);

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Copying %lu bytes to 0x%lx\n",
        pBootImage->bootImageLength, pDest);
    result = pCopyFunction(pDest, srcOffset, pBootImage->bootImageLength);

    return result;
}
#endif

static bool getBootImageFromMMC_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage)
{
    bool result = false;

#if IS_ENABLED(CONFIG_SERVICE_BOOT) && IS_ENABLED(CONFIG_SERVICE_MMC)
    assert(ppBootImage);

    // if we are using MMC, then we need to do an initial copy of the
    // boot header into our structure, for subsequent use
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Preparing to copy from MMC to DDR ...\n");

    size_t srcLBAOffset = 0u;
    assert(pStorage);

    uint32_t blockSize, eraseSize, blockCount;
    pStorage->getInfo(&blockSize, &eraseSize, &blockCount);

# if (IS_ENABLED(CONFIG_SERVICE_BOOT_MMC_USE_GPT))
    {
        HSS_GPT_t gpt;

        gpt.lbaSize = blockSize;
        GPT_Init(&gpt, pStorage);
        result = GPT_ReadHeader(&gpt);

        if (result) {
            size_t srcIndex = 0u;

            if (GPT_GetBootPartitionIndex(&gpt, &srcIndex)) {
                mHSS_DEBUG_PRINTF(LOG_WARN, "Using manually set partition index\n");
            } else {
                result = GPT_FindBootSectorIndex(&gpt, &srcIndex, NULL);

                if (!result) {
                    mHSS_DEBUG_PRINTF(LOG_ERROR, "GPT_FindBootSectorIndex() failed\n");
                } else {
                    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Boot Partition found at index %lu\n",
                        srcIndex);
                }
            }

            if (result) {
                result = GPT_PartitionIdToLBAOffset(&gpt, srcIndex, &srcLBAOffset);
            }
        }
    }

    if (!result) {
        mHSS_DEBUG_PRINTF(LOG_WARN, "GPT_PartitionIdToLBAOffset() failed - using offset %lu\n", srcLBAOffset);
    } else {
        //mHSS_DEBUG_PRINTF(LOG_WARN, "GPT_PartitionIdToLBAOffset() returned %lu\n", srcLBAOffset);
    }
#endif

    //
    // Even if we have GPT enabled and it fails to find a GPT parttion, we'll still
    // try to boot
    {
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "Attempting to read image header (%d bytes) ...\n",
            sizeof(struct HSS_BootImage));
        result = HSS_MMC_ReadBlock(&bootImage, srcLBAOffset * blockSize,
            sizeof(struct HSS_BootImage));

        if (!result) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "HSS_MMC_ReadBlock() failed\n");
        } else {
            result = HSS_Boot_VerifyMagic(&bootImage);

            if (!result) {
                mHSS_DEBUG_PRINTF(LOG_ERROR, "HSS_Boot_VerifyMagic() failed\n");
            } else {
                int perf_ctr_index = PERF_CTR_UNINITIALIZED;
                HSS_PerfCtr_Allocate(&perf_ctr_index, "Boot Image MMC Copy");

                result = copyBootImageToDDR_(&bootImage,
                    (char *)(CONFIG_SERVICE_BOOT_DDR_TARGET_ADDR), srcLBAOffset * blockSize,
                    HSS_MMC_ReadBlock);
                *ppBootImage = (struct HSS_BootImage *)(CONFIG_SERVICE_BOOT_DDR_TARGET_ADDR);

                HSS_PerfCtr_Lap(perf_ctr_index);

                if (!result) {
                     mHSS_DEBUG_PRINTF(LOG_ERROR, "copyBootImageToDDR_() failed\n");
                }
            }
        }
    }
#endif

    return result;
}

void HSS_BootSelectSDCARD(void)
{
#if IS_ENABLED(CONFIG_SERVICE_MMC)
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Selecting SDCARD as boot source ...\n");
    pDefaultStorage = &mmcStorage_;
    HSS_MMC_SelectSDCARD();
#  if IS_ENABLED(CONFIG_SERVICE_BOOT)
    HSS_Register_Boot_Image(NULL);
#  endif
#else
    (void)getBootImageFromMMC_;
#endif
}

void HSS_BootSelectMMC(void)
{
#if IS_ENABLED(CONFIG_SERVICE_MMC)
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Selecting SDCARD/MMC (fallback) as boot source ...\n");
    pDefaultStorage = &mmcStorage_;
    HSS_MMC_SelectMMC();
#  if IS_ENABLED(CONFIG_SERVICE_BOOT)
    HSS_Register_Boot_Image(NULL);
#  endif
#else
    (void)getBootImageFromMMC_;
#endif
}

void HSS_BootSelectEMMC(void)
{
#if IS_ENABLED(CONFIG_SERVICE_MMC)
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Selecting EMMC as boot source ...\n");
    pDefaultStorage = &mmcStorage_;
    HSS_MMC_SelectEMMC();
#  if IS_ENABLED(CONFIG_SERVICE_BOOT)
    HSS_Register_Boot_Image(NULL);
#  endif
#else
    (void)getBootImageFromMMC_;
#endif
}

static bool getBootImageFromQSPI_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage)
{
    bool result = false;

#if IS_ENABLED(CONFIG_SERVICE_BOOT) && IS_ENABLED(CONFIG_SERVICE_QSPI)
    assert(ppBootImage);

    // need to do an initial copy of the boot header into our structure, for subsequent use
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Preparing to copy from QSPI to DDR ...\n");

    size_t srcLBAOffset = 0u;
    assert(pStorage);

    uint32_t blockSize, eraseSize, blockCount;
    pStorage->getInfo(&blockSize, &eraseSize, &blockCount);

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Attempting to read image header (%d bytes) ...\n",
        sizeof(struct HSS_BootImage));
    result = HSS_QSPI_ReadBlock(&bootImage, srcLBAOffset * blockSize,
        sizeof(struct HSS_BootImage));
    if (!result) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "HSS_QSPI_ReadBlock() failed\n");
    } else {
        result = HSS_Boot_VerifyMagic(&bootImage);

        if (!result) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "HSS_Boot_VerifyMagic() failed\n");
        } else {
            int perf_ctr_index = PERF_CTR_UNINITIALIZED;
            HSS_PerfCtr_Allocate(&perf_ctr_index, "Boot Image QSPI Copy");

            result = copyBootImageToDDR_(&bootImage,
                (char *)(CONFIG_SERVICE_BOOT_DDR_TARGET_ADDR), srcLBAOffset * blockSize,
                HSS_QSPI_ReadBlock);
            *ppBootImage = (struct HSS_BootImage *)(CONFIG_SERVICE_BOOT_DDR_TARGET_ADDR);

            HSS_PerfCtr_Lap(perf_ctr_index);

            if (!result) {
                 mHSS_DEBUG_PRINTF(LOG_ERROR, "copyBootImageToDDR_() failed\n");
            }
        }
    }
#endif

    return result;
}

void HSS_BootSelectQSPI(void)
{
#if IS_ENABLED(CONFIG_SERVICE_QSPI)
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Selecting QSPI as boot source ...\n");
    pDefaultStorage = &qspiStorage_;
#  if IS_ENABLED(CONFIG_SERVICE_BOOT)
    HSS_Register_Boot_Image(NULL);
#  endif
#else
    (void)getBootImageFromQSPI_;
#endif
}

static bool getBootImageFromFPGAQSPI_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage)
{
  bool result = false;

#if IS_ENABLED(CONFIG_SERVICE_BOOT) && IS_ENABLED(CONFIG_SERVICE_FPGA_QSPI)
  assert(ppBootImage);
  assert(pStorage);

  uint32_t off = 0;
  uint64_t len = sizeof(struct HSS_BootImage);
  uintptr_t dest = CONFIG_SERVICE_BOOT_DDR_TARGET_ADDR;

  mHSS_DEBUG_PRINTF(LOG_NORMAL, "Preparing to copy from FPGA QSPI to DDR ...\n");
  mHSS_DEBUG_PRINTF(LOG_NORMAL, "Attempting to read image header (%d bytes) ...\n",
      sizeof(struct HSS_BootImage));

  off = 0;
  len = sizeof(struct HSS_BootImage);
  result = HSS_FPGA_QSPIRead((uintptr_t)&bootImage, off, len);
  if (!result) {
    mHSS_DEBUG_PRINTF(LOG_ERROR, "HSS_FPGA_QSPIRead() failed\n");
  } else {
    //memcpy((void *)&bootImage, (void *)dest, sizeof(struct HSS_BootImage));
    result = HSS_Boot_VerifyMagic(&bootImage);
    if (!result) {
      mHSS_DEBUG_PRINTF(LOG_ERROR, "HSS_Boot_VerifyMagic() failed\n");
    } else {
      __print_boot_img_info(HSS_HART_E51, &bootImage);
      __print_boot_img_info(HSS_HART_U54_1, &bootImage);

      int perf_ctr_index = PERF_CTR_UNINITIALIZED;
      HSS_PerfCtr_Allocate(&perf_ctr_index, "Boot Image QSPI Copy");

      off = 0;
      len = bootImage.bootImageLength;
      result = HSS_FPGA_QSPIRead(dest, off, len);
      if (!result) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "copyBootImageToDDR_() failed\n");
				return result;
      }

      *ppBootImage = (struct HSS_BootImage *)dest;

      HSS_PerfCtr_Lap(perf_ctr_index);

    }
  }

#endif

  return result;
}

void HSS_BootSelectFPGAQSPI(void)
{
#if IS_ENABLED(CONFIG_SERVICE_FPGA_QSPI)
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Selecting FPGA QSPI as boot source ...\n");
    pDefaultStorage = &fpgaqspiStorage_;
#  if IS_ENABLED(CONFIG_SERVICE_BOOT)
    HSS_Register_Boot_Image(NULL);
#  endif
#else
    (void)getBootImageFromFPGAQSPI_;
#endif
}

static bool getBootImageFromPayload_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage)
{
    bool result = false;
    (void)pStorage;

#if IS_ENABLED(CONFIG_SERVICE_BOOT) && IS_ENABLED(CONFIG_SERVICE_BOOT_USE_PAYLOAD)
    assert(ppBootImage);

#if IS_ENABLED(CONFIG_SERVICE_BOOT_USE_PAYLOAD_IN_FABRIC)
    *ppBootImage = (struct HSS_BootImage *)(CONFIG_SERVICE_BOOT_USE_PAYLOAD_IN_FABRIC_ADDRESS);
#else
    extern struct HSS_BootImage _payload_start;
    *ppBootImage = (struct HSS_BootImage *)&_payload_start;
#endif

    result = HSS_Boot_VerifyMagic(*ppBootImage);
    printBootImageDetails_(*ppBootImage);
#endif

    return result;
}

void HSS_BootSelectPayload(void)
{
#if IS_ENABLED(CONFIG_SERVICE_BOOT_USE_PAYLOAD)
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Selecting Payload as boot source ...\n");
    pDefaultStorage = &payloadStorage_;
#  if IS_ENABLED(CONFIG_SERVICE_BOOT)
    HSS_Register_Boot_Image(NULL);
#  endif
#else
    (void)getBootImageFromPayload_;
#endif
}

#if IS_ENABLED(CONFIG_SERVICE_BOOT) && IS_ENABLED(CONFIG_SERVICE_SPI)
static bool spiFlashReadBlock_(void *dst, size_t offs, size_t count) {
   int retval = MSS_SYS_spi_copy((uintptr_t)dst, offs, count, /* options */ 3, /* mb_offset */ 0);

   if (retval) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to read 0x%lx bytes from SPI flash @0x%lx (error code %d)!\n", count, offs, retval);
   }

   return (retval == 0);
}
#endif

static bool getBootImageFromSpiFlash_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage) {
    bool result = false;
    (void)pStorage;

#if IS_ENABLED(CONFIG_SERVICE_BOOT) && IS_ENABLED(CONFIG_SERVICE_SPI)

    assert(ppBootImage);

    size_t srcOffset = CONFIG_SERVICE_BOOT_SPI_FLASH_OFFSET;

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Preparing to copy from SPI Flash +0x%lx to DDR ...\n", srcOffset);
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Attempting to read image header (%d bytes) ...\n",
        sizeof(struct HSS_BootImage));

    MSS_SYS_select_service_mode(MSS_SYS_SERVICE_POLLING_MODE, NULL);

    result = spiFlashReadBlock_(&bootImage, srcOffset, sizeof(struct HSS_BootImage));
    if (!result) {
        return false;
    }

    result = HSS_Boot_VerifyMagic(&bootImage);
    if (!result) {
        return false;
    }

    result = copyBootImageToDDR_(&bootImage, (char *)(CONFIG_SERVICE_BOOT_DDR_TARGET_ADDR),
        srcOffset, spiFlashReadBlock_);
    *ppBootImage = (struct HSS_BootImage *)(CONFIG_SERVICE_BOOT_DDR_TARGET_ADDR);
#endif

    return result;
}

void HSS_BootSelectSPI(void)
{
#if IS_ENABLED(CONFIG_SERVICE_SPI)
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Selecting SPI Flash as boot source ...\n");
    pDefaultStorage = &spiStorage_;
    HSS_Register_Boot_Image(NULL);
#else
    (void)getBootImageFromSpiFlash_;
#endif
}

static bool getBootImageFromYModemPayload_(struct HSS_Storage *pStorage, struct HSS_BootImage **ppBootImage)
{
  bool result = false;

#if IS_ENABLED(CONFIG_SERVICE_BOOT) && IS_ENABLED(CONFIG_SERVICE_YMODEM)
  uint32_t receivedCount = 0u;
  uint8_t *pDest = (uint8_t *)(CONFIG_SERVICE_BOOT_DDR_TARGET_ADDR);
  uint32_t g_rx_size = HSS_DDRHi_GetSize();

  assert(ppBootImage);

  uart_putstring(HSS_HART_E51, (char *)"Attempting to receive payload.bin via YMODEM ...\r\n");
  uart_putstring(HSS_HART_U54_1, (char *)"Attempting to receive payload.bin via YMODEM ...\r\n");
  receivedCount = ymodem_receive(pDest, g_rx_size);
  flush_ymodem_footer();
  if (receivedCount == 0) {
    uart_putstring(HSS_HART_E51, (char *)"YMODEM failed to receive file...\r\n");
    uart_putstring(HSS_HART_U54_1, (char *)"YMODEM failed to receive file...\r\n");

    return result;
  }

  uart_putstring(HSS_HART_E51, (char *)"Done......\r\n");
  uart_putstring(HSS_HART_U54_1, (char *)"Done......\r\n");

  memcpy((void *)&bootImage, (void *)pDest, sizeof(struct HSS_BootImage));

  result = HSS_Boot_VerifyMagic(&bootImage);
  if (!result) {
    uart_putstring(HSS_HART_E51, (char *)"HSS_Boot_VerifyMagic() failed...\n");
    uart_putstring(HSS_HART_U54_1, (char *)"HSS_Boot_VerifyMagic() failed...\n");
  } else {
    __print_boot_img_info(HSS_HART_E51, &bootImage);
    __print_boot_img_info(HSS_HART_U54_1, &bootImage);

    *ppBootImage = (struct HSS_BootImage *)pDest;
  }
#endif

  return result;
}

void HSS_BootSelectYModemPayload(void)
{
#if IS_ENABLED(CONFIG_SERVICE_YMODEM)
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Selecting YMODEM PAYLOAD as boot source ...\n");
    pDefaultStorage = &ymodemPayload_;
    HSS_Register_Boot_Image(NULL);
#else
    (void)getBootImageFromYModemPayload_;
#endif
}

bool HSS_Storage_Init(void);
bool HSS_Storage_ReadBlock(void *pDest, size_t srcOffset, size_t byteCount);
bool HSS_Storage_WriteBlock(size_t dstOffset, void *pSrc, size_t byteCount);
void HSS_Storage_GetInfo(uint32_t *pBlockSize, uint32_t *pEraseSize, uint32_t *pBlockCount);
void HSS_Storage_FlushWriteBuffer(void);

bool HSS_Storage_Init(void)
{
    bool result = true;

    struct HSS_Storage *pStorage = pDefaultStorage ? pDefaultStorage : pStorages[0];
    assert(pStorage);

    if (pStorage->init) {
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "initialize %s\n", pStorage->name);
        result = pStorage->init();
    }

    return result;
}

bool HSS_Storage_ReadBlock(void *pDest, size_t srcOffset, size_t byteCount)
{
    bool result = true;

    struct HSS_Storage *pStorage = pDefaultStorage ? pDefaultStorage : pStorages[0];
    assert(pStorage);

    if (pStorage->readBlock) {
        result = pStorage->readBlock(pDest, srcOffset, byteCount);
    }
    return result;
}

bool HSS_Storage_WriteBlock(size_t dstOffset, void *pSrc, size_t byteCount)
{
    struct HSS_Storage *pStorage = pDefaultStorage ? pDefaultStorage : pStorages[0];
    assert(pStorage);

    return pStorage->writeBlock(dstOffset, pSrc, byteCount);
}

void HSS_Storage_GetInfo(uint32_t *pBlockSize, uint32_t *pEraseSize, uint32_t *pBlockCount)
{
    struct HSS_Storage *pStorage = pDefaultStorage ? pDefaultStorage : pStorages[0];
    assert(pStorage);

    if (pStorage->getInfo) {
        pStorage->getInfo(pBlockSize, pEraseSize, pBlockCount);
    }
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s - %u byte pages, %u byte blocks, %u pages\n", pStorage->name, *pBlockSize, *pEraseSize, *pBlockCount);
}

void HSS_Storage_FlushWriteBuffer(void)
{
    struct HSS_Storage *pStorage = pDefaultStorage ? pDefaultStorage : pStorages[0];
    assert(pStorage);

    if (pStorage->flushWriteBuffer) {
        pStorage->flushWriteBuffer();
    }
}
