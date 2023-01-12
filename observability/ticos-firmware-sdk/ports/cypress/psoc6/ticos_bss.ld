/*
  Custom section to be included in the linker command file to support recovery of all FreeRTOS tasks when
  a coredump is collected.
*/

SECTIONS
{
    .tcs_bss (NOLOAD) :
    {
        __ticos_capture_bss_start = .;
         *tasks.o(.bss COMMON .bss*)
         *timers*.o(.bss COMMON .bss*)
        __ticos_capture_bss_end = .;
    }
}
INSERT BEFORE .bss;
