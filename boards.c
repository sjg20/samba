#include "at91sam.h"
#include "common.h"
#include "pmc.h"
#include "sdramc.h"
#include "nand.h"
#include "boards.h"

int sn9260_init (at91_t *at91)
{
    /* Enable h/W reset */
    writel((0xA5000000 | AT91C_RSTC_URSTEN), AT91C_BASE_RSTC + RSTC_RMR);

    /** SETUP CLOCKS */
    /* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
    pmc_cfg_plla(at91, PLLA_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Switch MCK on PLLA output PCK = PLLA = 2 * MCK */
    pmc_cfg_mck(at91, MCKR_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Configure PLLB */
    pmc_cfg_pllb(at91, PLLB_SETTINGS, PLL_LOCK_TIMEOUT);

    /** SETUP SDRAM */

    /* Initialize the matrix */
    writel(readl(AT91C_BASE_CCFG + CCFG_EBICSA) | AT91C_EBI_CS1A_SDRAMC, AT91C_BASE_CCFG + CCFG_EBICSA);

    /* Configure SDRAM Controller */
    sdram_init (at91,
            AT91C_SDRAMC_NC_9  |
            AT91C_SDRAMC_NR_13 |
            AT91C_SDRAMC_CAS_2 |
            AT91C_SDRAMC_NB_4_BANKS |
            AT91C_SDRAMC_DBW_32_BITS |
            AT91C_SDRAMC_TWR_2 |
            AT91C_SDRAMC_TRC_7 |
            AT91C_SDRAMC_TRP_2 |
            AT91C_SDRAMC_TRCD_2 |
            AT91C_SDRAMC_TRAS_5 |
            AT91C_SDRAMC_TXSR_8,            /* Control Register */
            (MASTER_CLOCK * 7)/1000000,     /* Refresh Timer Register */
            AT91C_SDRAMC_MD_SDRAM);         /* SDRAM (no low power)   */

    nand_init (at91);

    return 0;
}
