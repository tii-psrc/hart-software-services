#ifdef __cplusplus
extern "C" {
#endif































struct nand_device;
struct nand_pos;
struct erase_info;
struct mtd_info;
struct nand_ops;

bool nanddev_isbad(struct nand_device *nand, const struct nand_pos *pos);
int nanddev_markbad(struct nand_device *nand, const struct nand_pos *pos);
bool nanddev_isreserved(struct nand_device *nand, const struct nand_pos *pos);

int nanddev_mtd_erase(struct mtd_info *mtd, struct erase_info *einfo);

int nanddev_init(struct nand_device *nand, const struct nand_ops *ops);


#ifdef __cplusplus
}
#endif
