/*
 * Defines __TcsCoredumpRamStart and __TcsCoredumpRamEnd that point to beginning and end of the SRAM region.
 */

SECTIONS
{
  .tcs_coredump_symbols :
  {
    __TcsCoredumpRamStart = ORIGIN(SRAM);
    __TcsCoredumpRamEnd = ORIGIN(SRAM) + LENGTH(SRAM);
  }
}
