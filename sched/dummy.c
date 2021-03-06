/*
 * Dummy scheduling class, mapped to range of 5 levels of SCHED_NORMAL policy
 */

#include "sched.h"

/*
 * Timeslice and age threshold are represented in jiffies. Default timeslice
 * is 100ms. Both parameters can be tuned from /proc/sys/kernel.
 */

#define DUMMY_TIMESLICE		(100 * HZ / 1000)
#define DUMMY_AGE_THRESHOLD	(3 * DUMMY_TIMESLICE)

unsigned int sysctl_sched_dummy_timeslice = DUMMY_TIMESLICE;
static inline unsigned int get_timeslice()
{
	return sysctl_sched_dummy_timeslice;
}

unsigned int sysctl_sched_dummy_age_threshold = DUMMY_AGE_THRESHOLD;
static inline unsigned int get_age_threshold()
{
	return sysctl_sched_dummy_age_threshold;
}

/*
 * Init
 */

void init_dummy_rq(struct dummy_rq *dummy_rq, struct rq *rq)
{
	int i;
	for(i = 0; i < 5; i++) {
		INIT_LIST_HEAD(&dummy_rq->queues[i]);
	}
}

/*
 * Helper functions
 */

static inline struct task_struct *dummy_task_of(struct sched_dummy_entity *dummy_se)
{
	return container_of(dummy_se, struct task_struct, dummy_se);
}

static inline void _enqueue_task_dummy(struct rq *rq, struct task_struct *p)
{
	struct sched_dummy_entity *dummy_se = &p->dummy_se;
	int prio = p->prio - 131; // to get index between 0 and 4
	struct list_head *queue = &rq->dummy.queues[prio];
	list_add_tail(&dummy_se->run_list, queue);
}

static inline void _dequeue_task_dummy(struct task_struct *p)
{
	struct sched_dummy_entity *dummy_se = &p->dummy_se;
	list_del_init(&dummy_se->run_list);
}

/*
 * Scheduling class functions to implement
 */

static void enqueue_task_dummy(struct rq *rq, struct task_struct *p, int flags)
{
	_enqueue_task_dummy(rq, p);
	p->dummy_se.age_count = 0;
	//to check init quantum
	if(p->dummy_se.quantum >= get_timeslice()){
		p->dummy_se.quantum = 0;
	}
	inc_nr_running(rq);
	printk(KERN_CRIT "enqueue: %d\n",p->pid);
}

static void dequeue_task_dummy(struct rq *rq, struct task_struct *p, int flags)
{
	_dequeue_task_dummy(p);
	// Only those finishing executing have age_count max
	if(p->dummy_se.age_count == 0) {
		p->prio = p->static_prio;	
	}
	dec_nr_running(rq);
	printk(KERN_CRIT "dequeue: %d\n",p->pid);
}

static void yield_task_dummy(struct rq *rq)
{
	dequeue_task_dummy(rq, rq->curr, rq->curr->flags);
	enqueue_task_dummy(rq, rq->curr, rq->curr->flags);
	printk(KERN_CRIT "yield: %d\n",rq->curr->pid);
	// resched_task ? No
	// recover old priority ? 
}

static void check_preempt_curr_dummy(struct rq *rq, struct task_struct *p, int flags)
{
	if(p->prio < rq->curr->prio) {
		// dequeue_task_dummy(rq, rq->curr, flags);
		// enqueue_task_dummy(rq, rq->curr, flags);
		printk(KERN_CRIT "preempt: %d\n",p->pid);
		resched_task(rq->curr);
	}
}

static struct task_struct *pick_next_task_dummy(struct rq *rq)
{
	struct dummy_rq *dummy_rq = &rq->dummy;
	struct sched_dummy_entity *next;
	int i;
	for(i = 0; i < 5; i++) {
		if (!list_empty(&dummy_rq->queues[i])) {
			next = list_first_entry(&dummy_rq->queues[i], struct sched_dummy_entity, run_list);
			printk(KERN_CRIT "pick_next: %d\n",rq->curr->pid);
			return dummy_task_of(next);
		}
	}

	return NULL;
}

static void put_prev_task_dummy(struct rq *rq, struct task_struct *prev)
{
}

static void set_curr_task_dummy(struct rq *rq)
{
}

static void task_tick_dummy(struct rq *rq, struct task_struct *curr, int queued)
{
	curr->dummy_se.quantum++;
	if(curr->dummy_se.quantum >= get_timeslice()) {
		printk(KERN_CRIT "timesliced: %d\n",curr->pid);
		dequeue_task_dummy(rq, curr, curr->flags);
		enqueue_task_dummy(rq, curr, curr->flags);
		resched_task(curr);
	}

	struct dummy_rq *dummy_rq = &rq->dummy;
	struct sched_dummy_entity *entity;
	struct sched_dummy_entity *entity_temp;
	struct task_struct* task;

	// Don't loop over smaller or equal priorities than current task (aging is useless there)
	int start = rq->curr->prio - 131 + 1;
	int i;
	for(i = start; i < 5; i++) {
		// Iterate over elements of each queue
		list_for_each_entry_safe(entity, entity_temp, &dummy_rq->queues[i], run_list) {
			entity->age_count++;
			if(entity->age_count >= get_age_threshold()) {
				task = dummy_task_of(entity);
				task->prio--;
				printk(KERN_CRIT "aging: %d\n",task->pid);
				dequeue_task_dummy(rq, task, task->flags);
				enqueue_task_dummy(rq, task, task->flags);				
			}
		}
	}
}

static void switched_from_dummy(struct rq *rq, struct task_struct *p)
{
}

static void switched_to_dummy(struct rq *rq, struct task_struct *p)
{
}

static void prio_changed_dummy(struct rq *rq, struct task_struct *p, int oldprio)
{
}

static unsigned int get_rr_interval_dummy(struct rq *rq, struct task_struct *p)
{
	return get_timeslice();
}

/*
 * Scheduling class
 */

const struct sched_class dummy_sched_class = {
	.next			= &idle_sched_class,
	.enqueue_task		= enqueue_task_dummy,
	.dequeue_task		= dequeue_task_dummy,
	.yield_task		= yield_task_dummy,

	.check_preempt_curr 	= check_preempt_curr_dummy,

	.pick_next_task		= pick_next_task_dummy,
	.put_prev_task		= put_prev_task_dummy,

	.set_curr_task		= set_curr_task_dummy,
	.task_tick		= task_tick_dummy,

	.switched_from		= switched_from_dummy,
	.switched_to		= switched_to_dummy,
	.prio_changed		= prio_changed_dummy,

	.get_rr_interval	= get_rr_interval_dummy,
};

