#include <kernel.h>
#include <pa4.h>
#include <proc.h>

extern int dskread(struct devsw *pdev, char *buffer, int block_no, int count);
extern int dskwrite(struct devsw *pdev, char *buffer, int block_no, int count);

extern int memncpy(void *dest, void *src, int num);
/*
 * Part A 3/4. buf_fetch()
 * buf_fetch() does:
 *     if there is a buffer that already store the block data of block_no,
 *         then return the buffer.
 *     otherwise
 *         read the block of block_no, store it on a buffer, and return the buffer.
 * 
 * parameters:
 *     pdev:		device descriptor
 *     block_no: 	a block number to read
 *     policy: 		buffer replacement policy
 */
dsk_buffer_p buf_fetch(struct devsw *pdev, int block_no, int policy) {
	if(pdev == &devtab[DISK0] && disk1_request == -1) 
		return SYSERR;
	if(pdev == &devtab[DISK1] && disk2_request == -1) 
		return SYSERR;
		
	
	disk_desc *ptr;
	ptr = (disk_desc *)pdev -> dvioblk;
	STATWORD ps;
	dsk_buffer *dsk_buf,*dsk_buf_prefetch,*dsk_buf_ret;
	dsk_buf = buf_head;
	int found= 0,i,victim_block_no;
	int num_blocks = 0;
	if(block_no >= ptr->logical_blocks)
		return SYSERR;
	if(block_no + PA4_PREFETCH_SIZE + 1 > ptr->logical_blocks)
		num_blocks = ptr->logical_blocks - block_no;
	else
		num_blocks = PA4_PREFETCH_SIZE+1;
	char* buf = (char*)getmem(128*num_blocks);
	
	if(buf_head == NULL){
		char* buf1 = (char*) getmem(128);
		return dskread(pdev, buf1, block_no,1);
	}
	
	
	while(dsk_buf != NULL){
		if(dsk_buf->pdev == pdev && dsk_buf->block_no == block_no){
			found = 1;
			break;
		}
		dsk_buf = dsk_buf->next;
	}
	if(found == 1){
		wait(dsk_buf->buf_sem);
		
		if(dsk_buf !=  buf_head && PA4_BUFFER_REPLACEMENT == POLICY_LRU){
			disable(ps);
			reorder(dsk_buf);
			restore(ps);
		}
		return dsk_buf;
	}
	else{
		dsk_buf = buf_head;
		while(dsk_buf != NULL){
			for(i = block_no; i <= block_no+num_blocks-1; i++){
				if(dsk_buf->pdev == pdev && dsk_buf->block_no == i){
					/***flush the block***/
					wait(dsk_buf->buf_sem);
					
					if(dsk_buf->isDirty == 1){
						dskwrite(dsk_buf->pdev, dsk_buf->data,dsk_buf->block_no, 1);
						dsk_buf->isDirty = 0;
					}
					signal(dsk_buf->buf_sem);
					break;
				}
			}
			dsk_buf = dsk_buf->next;
		}
		
		if(dskread(pdev, buf, block_no,num_blocks) == SYSERR){
				return SYSERR;
			}
		found = 0;
		int j;
		for(i = block_no,j=0; i <= block_no+num_blocks-1; i++,j++){
			dsk_buf = buf_head;
			while(dsk_buf != NULL){
				if(dsk_buf->pdev == pdev && dsk_buf->block_no == i){
					found = 1;
					break;
				}
				dsk_buf = dsk_buf->next;
			}
			if(found != 1){
				disable(ps);
				dsk_buf = buf_victim();
				wait(dsk_buf->buf_sem);
				victim_block_no = dsk_buf->block_no;
				dsk_buf->block_no = i;
				if(dsk_buf != buf_head){
					reorder(dsk_buf);
				}
				if(dsk_buf->isDirty == 1){
					restore(ps);
					dskwrite(dsk_buf->pdev, dsk_buf->data,victim_block_no, 1);
					disable(ps);
				}
				
				/**check first**/
				memncpy(dsk_buf->data, buf+(j*dsk_buf->size), dsk_buf->size);
				dsk_buf->isDirty = 0;
				dsk_buf->pdev = pdev;
				restore(ps);
				
				if(!(dsk_buf->block_no == block_no && dsk_buf->pdev == pdev)){
					signal(dsk_buf->buf_sem);
				}
			}
			if(i == block_no) {
				dsk_buf_ret = dsk_buf;
			}
		}
		return dsk_buf_ret;
	}
	freemem(buf,128*num_blocks);
}
