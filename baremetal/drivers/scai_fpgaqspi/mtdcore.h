#ifdef __cplusplus
extern "C" {
#endif



struct mtd_oob_region;
struct mtd_info;
struct mtd_oob_region;
struct erase_info;
struct mtd_oob_ops;





int mtd_ooblayout_count_freebytes(struct mtd_info *mtd);
int mtd_ooblayout_free(struct mtd_info *mtd, int section,
		       struct mtd_oob_region *oobfree);
int mtd_ooblayout_set_databytes(struct mtd_info *mtd, const uint8_t *databuf,
				uint8_t *oobbuf, int start, int nbytes);
int mtd_ooblayout_get_databytes(struct mtd_info *mtd, uint8_t *databuf,
		const uint8_t *oobbuf, int start, int nbytes);

int mtd_block_isbad(struct mtd_info *mtd, off_t ofs);
int mtd_erase(struct mtd_info *mtd, struct erase_info *instr);
int mtd_read_oob(struct mtd_info *mtd, off_t from, struct mtd_oob_ops *ops);
int mtd_write_oob(struct mtd_info *mtd, off_t to,
				struct mtd_oob_ops *ops);



#ifdef __cplusplus
}
#endif
