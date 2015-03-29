/****************************************************************************
 *  alloc.c
 *
 *  This is an optimized memory allocation scheme.  Since in this 
 *  application, CANbus needs to be treated as the highest priority ISR,
 *  we pre-allocate memory when the driver is loaded, then pull messages
 *  out of the pool as needed.  This is not the most efficient or fair 
 *  use of resources, but again this is the highest priority ISR and 
 *  anything we can do to minimize latency is desirable.
 *
 ***************************************************************************/
#include "can_private.h"

/*
 *  From LDD3 - max guaranteed kmalloc size is 128kB
 */
#define CHUNK_SIZE  (128 * 1024)

/**
 *  How many canbus messages can we fit inside here?
 */
#define NUM_MSGS_IN_CHUNK   (CHUNK_SIZE / sizeof(struct kcanbus_message))

/*
 *  Dynamic array of pointers to arrays.
 */
static struct kcanbus_message **chunk_array = NULL;
static int num_chunks_allocated;

/*
 *  Our kcanbus message pool itself.  The list head for the 
 *  free linked data structures.
 */
static struct list_head msg_pool;

/*
 *  We use our own self-contained lock here.
 */
static spinlock_t msg_pool_lock;

/*
 *  If we run out of messages, don't stream the error.
 *  But also don't spend a lot of time trying not to either.
 */
static int in_nomem_condition;


/*
 *  Our kmalloc() function.  Returns a pointer or NULL.
 */
struct kcanbus_message * alloc_kcanbus_message(void)
{
	unsigned long flags;
    struct kcanbus_message *msg;
    struct list_head *entry;

	spin_lock_irqsave(&msg_pool_lock, flags);

    if (list_empty(&msg_pool)){
        /*
         *  Limit the streaming error messages.
         */
        if (!in_nomem_condition){
            in_nomem_condition = 1;
            printk(KERN_ERR "alloc_kcanbus_message Failed!\n");
        }
        msg = NULL;
    }
    else{
        entry = msg_pool.next;
        list_del_init(entry);
        msg = list_entry(entry, struct kcanbus_message, entry);

        if (msg->signature != KCANBUS_SIGNATURE){
            printk(KERN_ERR "Message Signature check Failed! %s %d\n", __FILE__, __LINE__);
            msg = NULL;
            goto EXIT;
        }

        memset(&msg->user_message, 0, sizeof(CANBUS_MESSAGE));
    }

EXIT:    
    spin_unlock_irqrestore(&msg_pool_lock, flags);

    return msg;
}


/*
 *  Our kfree() function.  We verify that it's our memory via
 *  signature or we complain and leave, without touching it.
 */
void free_kcanbus_message(struct kcanbus_message *msg)
{
	unsigned long flags;
    struct list_head *entry;

    /*
     *  kfree() accepts NULLs, but we don't expect to.
     */
    if (msg == NULL){
        printk(KERN_ERR "free_kcanbus_message got a NULL\n");
        return;
    }

    if (msg->signature != KCANBUS_SIGNATURE){
        printk(KERN_ERR "Message Signature check Failed! %s %d\n", 
                __FILE__, __LINE__);
        return;
    }

    entry = &msg->entry;

	spin_lock_irqsave(&msg_pool_lock, flags);

    INIT_LIST_HEAD(entry);
    list_add(entry, &msg_pool);
    in_nomem_condition = 0;

    spin_unlock_irqrestore(&msg_pool_lock, flags);
}


/*
 *  Init the kcanbus message pool.
 */
int init_kcanbus_message_pool(int max_msg_count)
{
    int malloc_size;
    int i, j;
    struct kcanbus_message *msg;

    INIT_LIST_HEAD(&msg_pool);
    spin_lock_init(&msg_pool_lock);

    malloc_size = max_msg_count * sizeof(struct kcanbus_message);
    num_chunks_allocated = malloc_size / CHUNK_SIZE;
    if (malloc_size % CHUNK_SIZE){
        num_chunks_allocated++;
    }

    chunk_array = kmalloc(num_chunks_allocated * sizeof(void *), GFP_KERNEL);

    for (i = 0; i<num_chunks_allocated; i++){

        chunk_array[i] = kmalloc(CHUNK_SIZE, GFP_KERNEL);
        memset(chunk_array[i], 0, CHUNK_SIZE);
        
        for (j=0; j<NUM_MSGS_IN_CHUNK; j++){
            msg = &chunk_array[i][j];
            INIT_LIST_HEAD(&msg->entry);
            msg->signature = KCANBUS_SIGNATURE;
            list_add(&msg->entry, &msg_pool);
        }
    }

    in_nomem_condition = 0;

    return 0;
}


/*
 *  Cleanup
 */
void destroy_kcanbus_message_pool(void)
{
    int i;

    for (i = 0; i<num_chunks_allocated; i++){
        if (chunk_array[i])
            kfree(chunk_array[i]);
    }

    if (chunk_array)
        kfree(chunk_array);
}



