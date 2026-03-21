#ifdef __cplusplus
extern "C" {
#endif















































#include "nand.h"

struct nand_device;
enum nand_bbt_block_status;

int nanddev_bbt_init(struct nand_device *nand);
int nanddev_bbt_update(struct nand_device *nand);
int nanddev_bbt_get_block_status(const struct nand_device *nand, 
		unsigned int entry);
int nanddev_bbt_set_block_status(struct nand_device *nand,
		unsigned int entry, enum nand_bbt_block_status status);

#ifdef __cplusplus
}
#endif


