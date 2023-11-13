#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

#define CYCLES                                                                 \
	5  // Cantidad de ciclos completos para bajar de prioridad automaticamente.
#define WORST_PRIORITY 5  // peor prioridad
#define EXECUTIONS_TO_BOOST                                                    \
	5                // Cantidad de ejecuciones para que se realice un boost
#define BEST_PRIORITY 0  // mejor prioridad

// Estadísticas del scheduler
struct SchedStats {
	int n_boosts;  // Cantidad de veces que se reseteó la prioridad de los procesos.
	int n_sched_calls;  // Cantidad de veces total que se llamó al scheduler.
};

struct SchedStats sched_stats;

// ejecuciones totales
int executions = 0;

void sched_halt(void);


// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *idle;

	sched_stats.n_sched_calls++;  // Stats.

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// usar #ifdef para llamar a los diferentes schedulers

	/* comentario de discord ayuda:

	-Con la macro ENVX() pueden obtener el índice
	de un proceso a partir de su env_ind.

	- Pueden usar la operación módulo para obtener un
	índice que siempre se encuentre en el rango [0, NENV-1],
	así: i = n % NENV, donde n puede inicializarse como el índice
	del proceso actual más 1 si existe, o en 0 de lo contrario,
	e incrementarse más allá de los límites del arreglo sin ser un problema.
	*/

#define SHED_PR

#ifdef SHED_PR

	// verifico la cantidad de ejecuciones totales
	// si la cantidad de ejecuciones totales es mayor o igual a la necesaria para boostear,
	// boosteo a todos los env a la mejor prioridad para evitar starving.
	if (executions >= EXECUTIONS_TO_BOOST) {
		sched_stats.n_boosts++;  // Stats.
		int i = 0;
		while (i < NENV) {
			envs[i].priority = BEST_PRIORITY;
			i++;
		}
		// reseteo las ejecuciones totales
		executions = 0;
	} else {
		executions++;
	}

	int index;
	idle = NULL;

	// set initial index to iterate
	if (curenv) {
		index = ENVX(curenv->env_id) + 1;  // next to curenv
	} else {
		index = 0;  // beginning
	}

	int i = index;

	// para buscar el env con la mejor prioridad arranco seteando la peor.
	int chosen_priority = WORST_PRIORITY;

	// busco el próximo como en round robin pero además debe tener la mínima prioridad
	while (i < NENV + index) {
		if (envs[i % NENV].env_status == ENV_RUNNABLE) {
			if (envs[i % NENV].priority < chosen_priority) {
				idle = &envs[i % NENV];
				chosen_priority = idle->priority;
			}
		}
		i++;
	}

	// si el elegido ya se ejecuto una cantidad de veces (CYCLES)
	// se le baja la prioridad y se resetea el contador de ciclos para ese
	// env si ya está en la peor prioridad queda en esa
	if (idle) {
		idle->env_cycles++;
		if (idle->env_cycles >= CYCLES) {
			// ojo aca el < MAX_PRIORITY
			// quiero que si tienen la misma prioridad se ejecute el
			// primero, pero hay que tener cuidado en la ultima
			// prioridad entonces por ahi va <=
			if (idle->priority < WORST_PRIORITY) {
				idle->priority++;
				idle->n_priority_downgrades++;  // Stats.
			}
			idle->env_cycles = 0;
		}
	}

#endif

#ifdef SHED_RR

	int index;

	// set initial index to iterate
	if (curenv) {
		index = ENVX(curenv->env_id) + 1;  // next to curenv
	} else {
		index = 0;  // beginning
	}

	int i = index;

	// Search for next runnable env.
	while (i < NENV + index) {
		idle = &envs[i % NENV];
		if (idle->env_status == ENV_RUNNABLE) {
			break;
		}
		idle = NULL;
		i++;
	}

#endif

	// Runnable env found.
	if (idle) {
		env_run(idle);
	}

	// No runnable env found but curenv still running.
	if (!idle && curenv && curenv->env_status == ENV_RUNNING) {
		env_run(curenv);
	}

	// No envs to run. Halt.
	sched_halt();
}

void
print_sched_stats()
{
	cprintf("\n# Estadísticas del scheduler: \n");

	cprintf("\n## Estadísticas generales \n");

	cprintf("* Cantidad de veces que se llamó al sheduler: %d \n",
	        sched_stats.n_sched_calls);
	cprintf("* Cantidad de boosts a la prioridad: %d\n", sched_stats.n_boosts);

	cprintf("\n## Estadísticas por proceso \n");

	cprintf("Env Id \t | n runs \t | prior lost "
	        "\n");
	cprintf("———————————————————————————————————————————\n");

	for (int i = 0; i < NENV; i++) {
		// Si corrió, lo muestro, más la cantidad de runs.
		if (envs[i].env_runs != 0) {
			cprintf("%d \t | %d \t | %d \n",
			        envs[i].env_id,
			        envs[i].env_runs,
			        envs[i].n_priority_downgrades);
		};
	}
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		print_sched_stats();
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics
	// on performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
