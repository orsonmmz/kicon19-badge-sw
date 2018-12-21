#ifndef BOARD_H
#define BOARD_H

#define BOARD_FREQ_MAINCK_XTAL      (12000000U)
#define BOARD_FREQ_MAINCK_BYPASS    (12000000U)
#define BOARD_OSC_STARTUP_US        (15625UL)

/* SLCK crystal is not mounted, use the default values to mute warnings */
#define BOARD_FREQ_SLCK_XTAL        (32768U)
#define BOARD_FREQ_SLCK_BYPASS      (32768U)

#endif /* BOARD_H */
