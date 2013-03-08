#include <kernel.h>
#include <pa4.h>
#include <proc.h>

dsk_buffer_p buf_victim(void){
	dsk_buffer *dsk_buf;
	dsk_buf = buf_head;
	if(buf_count < PA4_BUFFER_SIZE){
		while(dsk_buf->block_no != -1 && dsk_buf->isDirty != -1){
			dsk_buf = dsk_buf->next;
		}
		buf_count++;
		return dsk_buf;
	}
	else if(buf_count == PA4_BUFFER_SIZE){
		while(dsk_buf->next != NULL)
			dsk_buf = dsk_buf->next;
		return dsk_buf;
	}
}

void reorder(dsk_buffer_p dsk_buf_new){
	dsk_buffer *dsk_buf,*prev;
	dsk_buf = buf_head;
	prev = buf_head;
	while(dsk_buf != NULL){
		if(dsk_buf == dsk_buf_new){
			prev->next = dsk_buf->next;
			dsk_buf->next = buf_head;
			buf_head = dsk_buf;
			return;
		}
		prev = dsk_buf;
		dsk_buf = dsk_buf->next;
	}
}
/*
 * Part A 4/4. buf_put()
 * buf_put() processes each write request.
 * If policy is POLICY_WRITE_THROUGH,
 *     then find a buffer that already stores the block_no block, stores new data to it
 *     and finally store the data into disk.
 * Otherwise (POLICY_DELAYED_WRITE),
 *     then you have to handle requests based on the policy.
 */
int buf_put(struct devsw *pdev, int block_no, char *buffer, int policy) {
	STATWORD ps;
	int victim_block_no;
	if(pdev == &devtab[DISK0] && disk1_request == -1) 
		return SYSERR;
	if(pdev == &devtab[DISK1] && disk2_request == -1) 
		return SYSERR;
	
	
	if(policy == POLICY_WRITE_THROUGH){
		dsk_buffer *dsk_buf,*prev;
		dsk_buf = buf_head;
		prev = buf_head;
		while(dsk_buf != NULL){
			if(dsk_buf->block_no == block_no && (struct devsw*)dsk_buf->pdev == pdev){ 
			disable(ps);
			wait(dsk_buf->buf_sem);
				memncpy(dsk_buf->data, buffer, dsk_buf -> size);
				dsk_buf->isDirty = 0;
				if(PA4_BUFFER_REPLACEMENT == POLICY_LRU && dsk_buf != buf_head){ 
					reorder(dsk_buf);
				}
				restore(ps);
				signal(dsk_buf->buf_sem);
			}
			prev = dsk_buf;
			dsk_buf = dsk_buf->next;
		}
		return dskwrite(pdev, buffer,block_no, 1);
	}
	/**POLICY DELAYED WRITE**/
	
	else if(policy == POLICY_DELAYED_WRITE){
		dsk_buffer *dsk_buf,*prev;
		dsk_buf = buf_head;
		int blockPresent = 0, code;
		prev = buf_head;
		while(dsk_buf != NULL){
			/**Block is present**/
			if(dsk_buf->block_no == block_no && (struct devsw *) dsk_buf->pdev == pdev){
				disable(ps);
				wait(dsk_buf->buf_sem);
				if(PA4_BUFFER_REPLACEMENT == POLICY_LRU && dsk_buf != buf_head){ 
					/**modify buffer only if policy is LRU and if current block is not the head**/
					reorder(dsk_buf);
					
				}
				restore(ps);
				blockPresent = 1;
				memncpy(dsk_buf->data, buffer, dsk_buf -> size);
				dsk_buf->isDirty = 1;
				signal(dsk_buf->buf_sem);
				break;
			}
			prev = dsk_buf;
			dsk_buf = dsk_buf->next;
		}
		if(blockPresent == 0){
			disable(ps);
			dsk_buf = buf_victim();
			wait(dsk_buf->buf_sem);
			victim_block_no  = dsk_buf->block_no;
			dsk_buf->block_no = block_no;
			if(dsk_buf != buf_head){
				reorder(dsk_buf);
			}
			if(dsk_buf->isDirty == 1) {
				restore(ps);
				dskwrite(dsk_buf->pdev, dsk_buf->data,victim_block_no, 1);  /* write the victim to the disk if its dirty*/
				disable(ps);
			}
			restore(ps);
			memncpy(dsk_buf->data, buffer, dsk_buf -> size);
			
			dsk_buf->pdev = pdev;
			dsk_buf->isDirty = 1;
			signal(dsk_buf->buf_sem);
		}
		return 0;
	}
}



