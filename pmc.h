#ifndef PMC_H
#define PMC_H

int pmc_cfg_plla(at91_t *at91, unsigned int pmc_pllar, unsigned int timeout);
int pmc_cfg_pllb(at91_t *at91, unsigned int pmc_pllbr, unsigned int timeout);
int pmc_cfg_mck(at91_t *at91, unsigned int pmc_mckr, unsigned int timeout);
int pmc_cfg_pck(at91_t *at91, unsigned char x, unsigned int clk_sel, unsigned int prescaler);

#endif
