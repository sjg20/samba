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
#define SN9260_PLLA_SETTINGS ((1 << 29) | (126 << 16) | (2 << 13) | (0x3F << 8) | (13))
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

	/* Configure the EBI Slave Slot Cycle to 64 */
	writel( (readl((AT91C_BASE_MATRIX + MATRIX_SCFG3)) & ~0xFF) | 0x40, (AT91C_BASE_MATRIX + MATRIX_SCFG3));

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


/* Write DDRC register */
#define write_ddramc(a,o,v) writel(v, (a) + (o))
/* Read DDRC registers */
#define read_ddramc(a,o) readl((a) + (o))

typedef struct SDdramConfig
{
	unsigned int ddramc_mdr;
	unsigned int ddramc_cr;
	unsigned int ddramc_rtr;
	unsigned int ddramc_t0pr;
	unsigned int ddramc_t1pr;
	unsigned int ddramc_t2pr;
} SDdramConfig, *PSDdramConfig;

#ifdef AT91SAM9G45_INC_H

//*----------------------------------------------------------------------------
//* \fn    sdram_init
//* \brief Initialize the SDDRC Controller
//*----------------------------------------------------------------------------
static int ddram_init(at91_t *at91,unsigned int ddram_controller_address, unsigned int ddram_address, struct SDdramConfig *ddram_config)
{
	unsigned int cr = 0;

	// Step 1: Program the memory device type
	// Configure the DDR controller
	write_ddramc(ddram_controller_address, HDDRSDRC2_MDR, ddram_config->ddramc_mdr);

	// Program the DDR Controller
	write_ddramc(ddram_controller_address, HDDRSDRC2_CR, ddram_config->ddramc_cr);

	// assume timings for 7.5ns min clock period
	write_ddramc(ddram_controller_address, HDDRSDRC2_T0PR, ddram_config->ddramc_t0pr);

	// pSDDRC->HDDRSDRC2_T1PR
	write_ddramc(ddram_controller_address, HDDRSDRC2_T1PR, ddram_config->ddramc_t1pr);

	// pSDDRC->HDDRSDRC2_T2PR
	write_ddramc(ddram_controller_address, HDDRSDRC2_T2PR, ddram_config->ddramc_t2pr);

	// Initialization Step 3: NOP command -> allow to enable clk
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_NOP_CMD);
        writel(0, ddram_address);

	// Initialization Step 3 (must wait 200 us) (6 core cycles per iteration, core is at 396MHz: min 13200 loops)
        usleep(300);

	// Step 4:  An NOP command is issued to the DDR2-SDRAM
	// NOP command -> allow to enable cke
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_NOP_CMD);
        writel(0, ddram_address);

	// wait 400 ns min
        usleep(2);

	// Initialization Step 5: Set All Bank Precharge
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_PRCGALL_CMD);
        writel(0, ddram_address);

	// wait 400 ns min
        usleep(2);

       // Initialization Step 6: Set EMR operation (EMRS2)
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_EXT_LMR_CMD);
        writel(0, ddram_address + 0x4000000);

	// wait 2 cycles min
        usleep(2);

	// Initialization Step 7: Set EMR operation (EMRS3)
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_EXT_LMR_CMD);
        writel(0, ddram_address + 0x6000000);

	// wait 2 cycles min
        usleep(2);

	// Initialization Step 8: Set EMR operation (EMRS1)
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_EXT_LMR_CMD);
        writel(0, ddram_address + 0x2000000);

	// wait 200 cycles min
        usleep(2);

	// Initialization Step 9: enable DLL reset
	cr = read_ddramc(ddram_controller_address, HDDRSDRC2_CR);
	write_ddramc(ddram_controller_address, HDDRSDRC2_CR, cr | AT91C_DDRC2_DLL_RESET_ENABLED);

	// Initialization Step 10: reset DLL
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_EXT_LMR_CMD);
        writel(0, ddram_address);

	// wait 2 cycles min
        usleep(2);

	// Initialization Step 11: Set All Bank Precharge
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_PRCGALL_CMD);
        writel(0, ddram_address);

	// wait 400 ns min
        usleep(2);

	// Initialization Step 12: Two auto-refresh (CBR) cycles are provided. Program the auto refresh command (CBR) into the Mode Register.
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_RFSH_CMD);
        writel(0, ddram_address);

	// wait 10 cycles min
        usleep(2);

	// Set 2nd CBR
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_RFSH_CMD);
        writel(0, ddram_address);

	// wait 10 cycles min
        usleep(2);

	// Initialization Step 13: Program DLL field into the Configuration Register to low(Disable DLL reset).
	cr = read_ddramc(ddram_controller_address, HDDRSDRC2_CR);
	write_ddramc(ddram_controller_address, HDDRSDRC2_CR, cr & (~AT91C_DDRC2_DLL_RESET_ENABLED));

	// Initialization Step 14: A Mode Register set (MRS) cycle is issued to program the parameters of the DDR2-SDRAM devices.
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_LMR_CMD);
        writel(0, ddram_address);

	// Step 15: Program OCD field into the Configuration Register to high (OCD calibration default).
	cr = read_ddramc(ddram_controller_address, HDDRSDRC2_CR);
	write_ddramc(ddram_controller_address, HDDRSDRC2_CR, cr | AT91C_DDRC2_OCD_DEFAULT);

	// Step 16: An Extended Mode Register set (EMRS1) cycle is issued to OCD default value.
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_EXT_LMR_CMD);
        writel(0, ddram_address + 0x2000000);

	// wait 2 cycles min
        usleep(2);

	// Step 17: Program OCD field into the Configuration Register to low (OCD calibration mode exit).
	cr = read_ddramc(ddram_controller_address, HDDRSDRC2_CR);
	write_ddramc(ddram_controller_address, HDDRSDRC2_CR, cr & (~AT91C_DDRC2_OCD_EXIT));

	// Step 18: An Extended Mode Register set (EMRS1) cycle is issued to enable OCD exit.
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_EXT_LMR_CMD);
        writel(0, ddram_address + 0x6000000);

	// wait 2 cycles min
        usleep(2);

	// Step 19,20: A mode Normal command is provided. Program the Normal mode into Mode Register.
	write_ddramc(ddram_controller_address, HDDRSDRC2_MR, AT91C_DDRC2_MODE_NORMAL_CMD);
        writel(0, ddram_address);

	// Step 21: Write the refresh rate into the count field in the Refresh Timer register. The DDR2-SDRAM device requires a
	// refresh every 15.625 ŠIs or 7.81 ŠÌs. With a 100MHz frequency, the refresh timer count register must to be set with
	// (15.625 /100 MHz) = 1562 i.e. 0x061A or (7.81 /100MHz) = 781 i.e. 0x030d.

	// Set Refresh timer
	write_ddramc(ddram_controller_address, HDDRSDRC2_RTR, ddram_config->ddramc_rtr);

	// OK now we are ready to work on the DDRSDR

	// wait for end of calibration
        usleep(2);

	return 0;
}

static  SDdramConfig ddram_config = {
    .ddramc_mdr  = (AT91C_DDRC2_DBW_16_BITS | AT91C_DDRC2_MD_DDR2_SDRAM),

    .ddramc_cr   = (AT91C_DDRC2_NC_DDR10_SDR9  |            // 10 column bits (1K)
            AT91C_DDRC2_NR_14          |            // 14 row bits    (8K)
            AT91C_DDRC2_CAS_3          |            // CAS Latency 3
            AT91C_DDRC2_DLL_RESET_DISABLED),        // DLL not reset

    .ddramc_rtr  =  0x24B,

    .ddramc_t0pr = (AT91C_DDRC2_TRAS_6  |           //  6 * 7.5 = 45   ns
            AT91C_DDRC2_TRCD_2  |           //  2 * 7.5 = 22.5 ns
            AT91C_DDRC2_TWR_2   |           //  2 * 7.5 = 15   ns
            AT91C_DDRC2_TRC_8  |            //  8 * 7.5 = 75   ns
            AT91C_DDRC2_TRP_2   |           //  2 * 7.5 = 22.5 ns
            AT91C_DDRC2_TRRD_1  |           //  1 * 7.5 = 7.5   ns
            AT91C_DDRC2_TWTR_1  |           //  1 clock cycle
            AT91C_DDRC2_TMRD_2),            //  2 clock cycles

    .ddramc_t1pr = (AT91C_DDRC2_TXP_2  |            //  2 * 7.5 = 15 ns
            200 << 16           |           // 200 clock cycles, TXSRD: Exit self refresh delay to Read command
            16 << 8             |           // 16 * 7.5 = 120 ns TXSNR: Exit self refresh delay to non read command
            AT91C_DDRC2_TRFC_14 << 0),      // 14 * 7.5 = 142 ns (must be 140 ns for 1Gb DDR)

    .ddramc_t2pr = (AT91C_DDRC2_TRTP_1   |          //  1 * 7.5 = 7.5 ns
            AT91C_DDRC2_TRPA_0   |          //  0 * 7.5 = 0 ns
            AT91C_DDRC2_TXARDS_7 |          //  7 clock cycles
            AT91C_DDRC2_TXARD_2),           //  2 clock cycles
};

/** Tiny gurnard settings */
#define BOARD_CKGR_PLLA         (AT91C_CKGR_SRCA | AT91C_CKGR_OUTA_0)
#define BOARD_PLLACOUNT         (0x3F << 8)
#define BOARD_MULA              (AT91C_CKGR_MULA & (199 << 16))
#define BOARD_DIVA              (AT91C_CKGR_DIVA & 3)
#define BOARD_PRESCALER         (0x00001300)

#define PLLA_SETTINGS           ( BOARD_CKGR_PLLA \
        | BOARD_PLLACOUNT \
        | BOARD_MULA \
        | BOARD_DIVA)

#define MCKR_SETTINGS           (AT91C_PMC_CSS_MAIN_CLK | BOARD_PRESCALER)
#define MCKR_CSS_SETTINGS       (AT91C_PMC_CSS_PLLA_CLK | BOARD_PRESCALER)

static int do_at91sam9g45_init(at91_t *at91, uint32_t plla_settings,
        uint32_t mckr_settings, uint32_t mckr_css_settings,
        SDdramConfig *ddram_config)
{
    /* Disable watchdog */
    if (writel(AT91C_WDTC_WDDIS, AT91C_BASE_WDTC + WDTC_WDMR) < 0)
        return -1;

    /* Enable User Reset*/
    if (writel(AT91C_RSTC_URSTEN | (0xA5<<24), AT91C_BASE_RSTC + RSTC_RMR) < 0)
        return -1;

    /* At this stage the main oscillator is supposed to be enabled
     * PCK = MCK = MOSC */
    if (writel(0x00, AT91C_BASE_PMC + PMC_PLLICPR) < 0)
        return -1;

    /* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
    if (pmc_cfg_plla(at91, plla_settings, PLL_LOCK_TIMEOUT) < 0)
        return -1;
    /* PCK = PLLA/2 = 3 * MCK */
    if (pmc_cfg_mck(at91, mckr_settings, PLL_LOCK_TIMEOUT) < 0)
        return -1;

    /* Switch MCK on PLLA output */
    if (pmc_cfg_mck(at91, mckr_css_settings, PLL_LOCK_TIMEOUT) < 0)
        return -1;

    // ENABLE DDR2 clock
    writel(AT91C_PMC_DDR, AT91C_BASE_PMC + PMC_SCER);

    if (ddram_init(at91, AT91C_BASE_DDR2C, AT91C_DDR2, ddram_config) < 0)
        return -1;
//     		writel(readl(AT91C_BASE_CCFG + CCFG_EBICSA) | AT91C_EBI_CS1A_SDRAMC,
// 	               AT91C_BASE_CCFG + CCFG_EBICSA);

//     if (ddram_init(at91, AT91C_BASE_DDR2CP1, AT91C_EBI_CS1, ddram_config) < 0)
//         return -1;
	/* Now set up the EBI into 32bit nCS controlled Byte select mode */
// 	writel(readl(AT91C_BASE_CCFG + CCFG_EBICSA) & (~AT91C_EBI_CS1A_SDRAMC),
// 	               AT91C_BASE_CCFG + CCFG_EBICSA);

    return 0;
}

int gurnard_init(at91_t *at91)
{
	return do_at91sam9g45_init(at91, PLLA_SETTINGS, MCKR_SETTINGS,
                MCKR_CSS_SETTINGS, &ddram_config);
}

#endif
