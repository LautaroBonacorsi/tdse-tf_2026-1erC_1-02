#include "main.h"
#include "task_telemetry_interface.h"

#define EMPTY			(255ul)
#define QUEUE_LENGTH	(16ul)

typedef struct
{
	uint32_t			head;
	uint32_t			tail;
	uint32_t			count;
	task_telemetry_ev_t	queue[QUEUE_LENGTH];
} event_task_telemetry_queue_t;

event_task_telemetry_queue_t event_task_telemetry_queue;

void init_event_task_telemetry(void)
{
	uint32_t i;

	event_task_telemetry_queue.head = 0;
	event_task_telemetry_queue.tail = 0;
	event_task_telemetry_queue.count = 0;

	for (i = 0; i < QUEUE_LENGTH; i++)
		event_task_telemetry_queue.queue[i] = (task_telemetry_ev_t)EMPTY;
}

void put_event_task_telemetry(task_telemetry_ev_t event)
{
    // Simple critical section to avoid interrupt/superloop race conditions
    __disable_irq();
	event_task_telemetry_queue.count++;
	event_task_telemetry_queue.queue[event_task_telemetry_queue.head++] = event;

	if (QUEUE_LENGTH == event_task_telemetry_queue.head)
		event_task_telemetry_queue.head = 0;
    __enable_irq();
}

task_telemetry_ev_t get_event_task_telemetry(void)
{
	task_telemetry_ev_t event;

    __disable_irq();
	event_task_telemetry_queue.count--;
	event = event_task_telemetry_queue.queue[event_task_telemetry_queue.tail];
	event_task_telemetry_queue.queue[event_task_telemetry_queue.tail++] = (task_telemetry_ev_t)EMPTY;

	if (QUEUE_LENGTH == event_task_telemetry_queue.tail)
		event_task_telemetry_queue.tail = 0;
    __enable_irq();

	return event;
}

bool any_event_task_telemetry(void)
{
  return (event_task_telemetry_queue.head != event_task_telemetry_queue.tail);
}
