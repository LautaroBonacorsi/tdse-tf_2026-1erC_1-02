/*
 * task_actuator_interface.c
 */

#include "main.h"
#include "task_actuator_attribute.h"
#include "task_actuator_interface.h"

#define EMPTY			(255ul)
#define QUEUE_LENGTH	(16ul)

typedef struct
{
	uint32_t			head;
	uint32_t			tail;
	uint32_t			count;
	task_actuator_ev_t	queue[QUEUE_LENGTH];
} event_task_actuator_queue_t;

static event_task_actuator_queue_t event_task_actuator_queue;

void init_event_task_actuator(void)
{
	uint32_t i;

	event_task_actuator_queue.head = 0;
	event_task_actuator_queue.tail = 0;
	event_task_actuator_queue.count = 0;

	for (i = 0; i < QUEUE_LENGTH; i++)
		event_task_actuator_queue.queue[i] = EMPTY;
}

void put_event_task_actuator(task_actuator_ev_t event)
{
	event_task_actuator_queue.count++;
	event_task_actuator_queue.queue[event_task_actuator_queue.head++] = event;

	if (QUEUE_LENGTH == event_task_actuator_queue.head)
		event_task_actuator_queue.head = 0;
}

task_actuator_ev_t get_event_task_actuator(void)
{
	task_actuator_ev_t event;

	event_task_actuator_queue.count--;
	event = event_task_actuator_queue.queue[event_task_actuator_queue.tail];
	event_task_actuator_queue.queue[event_task_actuator_queue.tail++] = EMPTY;

	if (QUEUE_LENGTH == event_task_actuator_queue.tail)
		event_task_actuator_queue.tail = 0;

	return event;
}

bool any_event_task_actuator(void)
{
  return (event_task_actuator_queue.head != event_task_actuator_queue.tail);
}
