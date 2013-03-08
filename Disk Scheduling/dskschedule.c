#include <kernel.h>
#include <pa4.h>
int last = 0;
/*
 * Part B 1/1. dskschedule()
 * This function should be implemented to support each disk scheduling policy.
 * Parameters:
 *     ptr:	a descriptor of each device information
 *     option:	a disk scheduling policy to be used
 */
void dskschedule(disk_desc *ptr, int option) {
	if(option == DISK_SCHED_SSTF){
		request_desc_p target, final_target, final_prev, prev;
		int min = 0, blk_dif;
		target = ptr->request_head;
		min = ptr->head_sector - target->block_no;
		min = min>0?min:(0-min);
		final_target = target;
		final_prev = NULL;
		for(target = (ptr -> request_head)->next,prev = ptr->request_head ;target != NULL;prev = target,target = target -> next){
			blk_dif = ptr->head_sector - target->block_no;
			if(blk_dif < 0)
				blk_dif = 0 - blk_dif;
			if(min >= blk_dif){
				min = blk_dif;
				final_target = target;
				final_prev = prev;			
			}
		}
		
		/**set previous's next to final_target's next**/
		if(final_prev == NULL){
			if(ptr->request_head->next == NULL){
				last = 1;
				return;
			}
			else
				ptr->request_head = ptr->request_head->next;
		}
		else
			final_prev->next =  final_target->next;
		
		/**point the final target's next to nothing**/
		final_target->next = NULL;
		
		/**place the final target at the end of the queue**/
		for(target = ptr->request_head ; target->next != NULL; target = target->next);
		target->next = final_target;
	}
	
	else if(option == DISK_SCHED_CLOOK){
		request_desc_p target = NULL, final_target = NULL, final_prev = NULL,prev = NULL;
		int min = MAXINT, blk_dif, found = 0;
		
		/**if there is only one request in the queue**/
		
		if(ptr->request_head != NULL){
			if(ptr->request_head->next == NULL){
				last = 1;
				return;
		}
		}
		else
			return;
		for(target = ptr -> request_head,prev = NULL ;target != NULL;prev = target,target = target -> next){
			blk_dif = target->block_no - ptr->head_sector;
			if(blk_dif < 0)
				continue;
			if(min >= blk_dif){
				min = blk_dif;
				final_target = target;
				final_prev = prev;	
				found = 1;					
			}
		}
		min = MAXINT;
		if(found == 0){
			for(target = ptr -> request_head,prev = NULL ;target != NULL;prev = target,target = target -> next){
				if(min >= target->block_no)
				{
					min = target->block_no;
					final_target = target;
					final_prev = prev;
				}
			}
		}
		/**set previous's next to final_target's next**/
		if(final_prev == NULL){
			if(ptr->request_head != NULL && ptr->request_head->next == NULL)
				return;
			else
				ptr->request_head = ptr->request_head->next;
		}
		else
			final_prev->next =  final_target->next;
		/**point the final target's next to nothing**/
		final_target->next = NULL;
		/**place the final target at the end of the queue**/
		for(target = ptr->request_head ; target->next != NULL; target = target->next);
		target->next = final_target;
	}
}
