/*
 * Your code here for context_switch
 *
 * It should restore the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

.globl context_switch;
context_switch:

		// pseudocódigo
		// en el stack está primero el return adress y luego el puntero a trapframe
		mov 4(%esp), %esp // cargo en el stack pointer la dirección del trapframe

		popal // pop del último push que hice en trapentry.s (todos los reg de control)
		pop %es
		pop %ds

		// aca salteo trapno y err haciendo dos pops
		// también se podria saltear moviendo el stackpointer +8
		// mov 8%esp, %esp no anduvo -> no usarlo
		addl $8, %esp
		iret // arranca desde tf_eip
spin:
	jmp spin
