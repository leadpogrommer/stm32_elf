/* Warren W. Gay VE3WWG
 *
 * To use libopencm3 with FreeRTOS on Cortex-M3 platform, we must
 * define three interlude routines.
 */
#include "FreeRTOS.h"
#include "task.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <malloc.h>
#include <errno.h>

extern void vPortSVCHandler( void ) __attribute__ (( naked ));
extern void xPortPendSVHandler( void ) __attribute__ (( naked ));
extern void xPortSysTickHandler( void );

void sv_call_handler(void) {
    vPortSVCHandler();
}

void pend_sv_handler(void) {
    xPortPendSVHandler();
}

void sys_tick_handler(void) {
    xPortSysTickHandler();
}

/* end opncm3.c */
// implement sbrk
// https://github.com/libopencm3/libopencm3/wiki/Using-malloc-with-libopencm3
#define MAX_STACK_SIZE  512

void local_heap_setup(uint8_t **start, uint8_t **end);

#pragma weak local_heap_setup = __local_ram

/* these are defined by the linker script */
extern uint8_t _ebss, _stack;

static uint8_t *_cur_brk = NULL;
static uint8_t *_heap_end = NULL;

/*
 * If not overridden, this puts the heap into the left
 * over ram between the BSS section and the stack while
 * preserving MAX_STACK_SIZE bytes for the stack itself.
 */
static void
__local_ram(uint8_t **start, uint8_t **end)
{
    *start = &_ebss;
    *end = (uint8_t *)(&_stack - MAX_STACK_SIZE);
}


/* prototype to make gcc happy */
void *_sbrk_r(struct _reent *, ptrdiff_t );

void *_sbrk_r(struct _reent *reent, ptrdiff_t diff)
{
    uint8_t *_old_brk;

    if (_heap_end == NULL) {
        local_heap_setup(&_cur_brk, &_heap_end);
    }

    _old_brk = _cur_brk;
    if (_cur_brk + diff > _heap_end) {
        reent->_errno = ENOMEM;
        return (void *)-1;
    }
    _cur_brk += diff;
    return _old_brk;
}


// implement FreeRTOS memory api
// https://nadler.com/embedded/newlibAndFreeRTOS.html
void *pvPortMalloc( size_t xSize ) PRIVILEGED_FUNCTION {
    void *p = malloc(xSize);
    return p;
}
void vPortFree( void *pv ) PRIVILEGED_FUNCTION {
    free(pv);
};

size_t xPortGetFreeHeapSize( void ) PRIVILEGED_FUNCTION {
    struct mallinfo mi = mallinfo(); // available space now managed by newlib
    return mi.fordblks + (_heap_end - _cur_brk); // plus space not yet handed to newlib by sbrk
}

// GetMinimumEverFree is not available in newlib's malloc implementation.
// So, no implementation is provided: size_t xPortGetMinimumEverFreeHeapSize( void ) PRIVILEGED_FUNCTION;

//! No implementation needed, but stub provided in case application already calls vPortInitialiseBlocks
void vPortInitialiseBlocks( void ) PRIVILEGED_FUNCTION {};

// stack overflow hook
void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char * pcTaskName ){
    for(;;);
}