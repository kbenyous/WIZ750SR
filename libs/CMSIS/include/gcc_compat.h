/**
 * @file    gcc_compat.h
 * @brief   Compatibility macros for Keil -> GCC migration (WIZ750SR)
 *
 * Include this header at the top of files that use Keil-specific keywords
 * like __weak, __packed, etc. when building with GCC.
 */

#ifndef __GCC_COMPAT_H
#define __GCC_COMPAT_H

#if defined(__GNUC__) && !defined(__CC_ARM)

  /* Keil __weak keyword -> GCC attribute */
  #ifndef __weak
    #define __weak   __attribute__((weak))
  #endif

  /* Keil __packed keyword -> GCC attribute */
  #ifndef __packed
    #define __packed __attribute__((packed))
  #endif

  /* __IO is normally defined in core_cm0.h (CMSIS), but just in case */
  #ifndef __IO
    #define __IO     volatile
  #endif

#endif /* __GNUC__ && !__CC_ARM */

#endif /* __GCC_COMPAT_H */
