#ifndef __PLAT_MTU_H
#define __PLAT_MTU_H

void nmdk_timer_init(void __iomem *base);
void nmdk_clkevt_reset(void);
void nmdk_clksrc_reset(void);

struct clock_event_device *nmdk_clkevt_get(void);

#endif /* __PLAT_MTU_H */

