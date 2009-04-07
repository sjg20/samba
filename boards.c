#include "at91sam.h"
#include "common.h"
#include "pmc.h"
#include "sdramc.h"
#include "nand.h"
#include "boards.h"


#define PLL_LOCK_TIMEOUT        1000000

unsigned int master_clock = 0;

/********** 9260 ****************/
#define SN9260_MASTER_CLOCK (180000000/2)
/* PLLA = 18.432MHz * (126 + 1) / 13 = 180.07 MHz */
#define SN9260_PLLA_SETTINGS ((1 << 29) | (126 << 16) | (0 << 14) | (0x3F << 8) | (13))
/* Set PLLB to 18.432 MHz * (119 + 1) / ( 23 * 2 ^ 1 ) = 48.08 MHz */
#define SN9260_PLLB_SETTINGS ((1 << 28) | (119 << 16) | (1 << 14) | (0x3f << 8) | 23)
/* Switch MCK on PLLA output PCK = PLLA = 2 * MCK */
#define SN9260_MCKR_SETTINGS (AT91C_PMC_CSS_PLLA_CLK | AT91C_PMC_PRES_CLK | AT91C_PMC_MDIV_2)


int sn9260_init (at91_t *at91)
{
    master_clock = SN9260_MASTER_CLOCK;

    /* Enable h/W reset */
    writel((0xA5000000 | AT91C_RSTC_URSTEN), AT91C_BASE_RSTC + RSTC_RMR);

    /** SETUP CLOCKS */
    /* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
    pmc_cfg_plla(at91, SN9260_PLLA_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Switch MCK on PLLA output PCK = PLLA = 2 * MCK */
    pmc_cfg_mck(at91, SN9260_MCKR_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Configure PLLB */
    pmc_cfg_pllb(at91, SN9260_PLLB_SETTINGS, PLL_LOCK_TIMEOUT);

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
            (SN9260_MASTER_CLOCK * 7)/1000000,     /* Refresh Timer Register */
            AT91C_SDRAMC_MD_SDRAM);         /* SDRAM (no low power)   */

    nand_init (at91);

    return 0;
}

/********* 9g20 ****************/
#define SN9G20_MASTER_CLOCK  (793576000/6)
/* PLLA = 18.432MHz * (42 + 1) / 1 = 793 MHz */
#define SN9G20_PLLA_SETTINGS   ((1 << 29) | (42 << 16) | (1 << 14)| (0x3F << 8) | (1))
/* Set PLLB to 18.432 MHz * (25 + 1) / ( 5 * 2 ^ 1 ) = 47.92 MHz */
#define SN9G20_PLLB_SETTINGS   ((1 << 28) | (25 << 16) | (1 << 14) | (0x3F << 8) | (5))
/* Switch MCK on PLLA output PCK = PLLA/2 = 3 * MCK */
#define         AT91C_PMC_MDIV_6 (0x3 <<  8) // (PMC) The processor clock is six times faster than the master clock
#define         AT91C_PMC_PDIV_2 (0x1 <<  12) // (PMC) PCLK = PLL/2
#define SN9G20_MCKR_SETTINGS   (AT91C_PMC_PDIV_2|AT91C_PMC_MDIV_6 | AT91C_PMC_PRES_CLK | AT91C_PMC_CSS_PLLA_CLK)


int sn9g20_init (at91_t *at91)
{
    master_clock = SN9G20_MASTER_CLOCK;

    /* Enable h/W reset */
    writel((0xA5000000 | AT91C_RSTC_URSTEN), AT91C_BASE_RSTC + RSTC_RMR);

    /** SETUP CLOCKS */
    /* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
    pmc_cfg_plla(at91, SN9G20_PLLA_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Switch MCK on PLLA output PCK = PLLA = 2 * MCK */
    pmc_cfg_mck(at91, SN9G20_MCKR_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Configure PLLB */
    pmc_cfg_pllb(at91, SN9G20_PLLB_SETTINGS, PLL_LOCK_TIMEOUT);

    /** SETUP SDRAM */

    /* Initialize the matrix */
    writel(readl(AT91C_BASE_CCFG + CCFG_EBICSA) | AT91C_EBI_CS1A_SDRAMC, AT91C_BASE_CCFG + CCFG_EBICSA);

    /* Configure SDRAM Controller */
    sdram_init (at91,
            AT91C_SDRAMC_NC_9  |
            AT91C_SDRAMC_NR_13 |
            AT91C_SDRAMC_CAS_3 |
            AT91C_SDRAMC_NB_4_BANKS |
            AT91C_SDRAMC_DBW_32_BITS |
            AT91C_SDRAMC_TWR_2 |
            AT91C_SDRAMC_TRC_7 |
            AT91C_SDRAMC_TRP_2 |
            AT91C_SDRAMC_TRCD_2 |
            AT91C_SDRAMC_TRAS_5 |
            AT91C_SDRAMC_TXSR_8,            /* Control Register */
            (SN9G20_MASTER_CLOCK * 7)/1000000,     /* Refresh Timer Register */
            AT91C_SDRAMC_MD_SDRAM);         /* SDRAM (no low power)   */

    nand_init (at91);

    return 0;
}

#define BIGEYE_MASTER_CLOCK        (180000000/2)
#define BIGEYE_PLLA_SETTINGS   ((1 << 29) | (17 << 16) | (0 << 14) | (0x3F << 8) | (5))
#define BIGEYE_PLLB_SETTINGS   ((1 << 28) | (72 << 16) | (1 << 14) | (0x3f << 8) | 38)
#define BIGEYE_MCKR_SETTINGS           (AT91C_PMC_PRES_CLK | AT91C_PMC_MDIV_2)
#define BIGEYE_MCKR_CSS_SETTINGS       (AT91C_PMC_CSS_PLLA_CLK | BIGEYE_MCKR_SETTINGS)

int bigeye_init (at91_t *at91)
{
    master_clock = BIGEYE_MASTER_CLOCK;

    /* Enable h/W reset */
    writel((0xA5000000 | AT91C_RSTC_URSTEN), AT91C_BASE_RSTC + RSTC_RMR);

    /** SETUP CLOCKS */
    /* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
    pmc_cfg_plla(at91, BIGEYE_PLLA_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Switch MCK on PLLA output PCK = PLLA = 2 * MCK */
    pmc_cfg_mck(at91, BIGEYE_MCKR_SETTINGS, PLL_LOCK_TIMEOUT);
    pmc_cfg_mck(at91, BIGEYE_MCKR_CSS_SETTINGS, PLL_LOCK_TIMEOUT);

    /* Configure PLLB */
    pmc_cfg_pllb(at91, BIGEYE_PLLB_SETTINGS, PLL_LOCK_TIMEOUT);

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
            (BIGEYE_MASTER_CLOCK * 7)/1000000,     /* Refresh Timer Register */
            AT91C_SDRAMC_MD_SDRAM);         /* SDRAM (no low power)   */

    nand_init (at91);

    return 0;
}

