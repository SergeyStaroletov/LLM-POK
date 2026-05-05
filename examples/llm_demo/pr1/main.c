/*
 *                               POK header
 *
 * The following file is a part of the POK project. Any modification should
 * be made according to the POK licence. You CANNOT use this file or a part
 * of a file for your own project.
 *
 * For more information on the POK licence, please see our LICENCE FILE
 *
 * Please follow the coding guidelines described in doc/CODING_GUIDELINES
 *
 *                                      Copyright (c) 2007-2025 POK team
 *                                      POK LLM Demo (c) 2026 Sergey Staroletov
 */

#include "activity.h"
#include <core/partition.h>
#include <core/semaphore.h>
#include <core/thread.h>
#include <libc/stdio.h>
#include <core/time.h>  

#include <types.h>

uint8_t sem_raw_empty;    // 1
uint8_t sem_raw_full;     // 0
uint8_t sem_est_empty;    // 1
uint8_t sem_est_full;     // 0

int main() {

  pok_ret_t ret;

  uint32_t tid;
  pok_thread_attr_t tattr;

  pok_sem_create(&sem_raw_empty, 1, 1, POK_QUEUEING_DISCIPLINE_DEFAULT);
  pok_sem_create(&sem_raw_full,  0, 1, POK_QUEUEING_DISCIPLINE_DEFAULT);
  pok_sem_create(&sem_est_empty, 1, 1, POK_QUEUEING_DISCIPLINE_DEFAULT);
  pok_sem_create(&sem_est_full,  0, 1, POK_QUEUEING_DISCIPLINE_DEFAULT);

  tattr.priority = 40;
  tattr.entry = llm_job;
  tattr.processor_affinity = 0;

  ret = pok_thread_create(&tid, &tattr);
  printf("[P1] pok_thread_create (llm_job) return=%d\n", ret);

  tattr.priority = 42;
  tattr.entry = sensor_job;
  tattr.processor_affinity = 0;

  ret = pok_thread_create(&tid, &tattr);
  printf("[P1] pok_thread_create (sensor_job) return=%d\n", ret);

  tattr.priority = 42;
  tattr.entry = estimator_job;
  tattr.processor_affinity = 0;

  ret = pok_thread_create(&tid, &tattr);
  printf("[P1] pok_thread_create (estimator_job) return=%d\n", ret);


  pok_partition_set_mode(POK_PARTITION_MODE_NORMAL);
  pok_thread_wait_infinite();

  return (0);
}
