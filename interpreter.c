#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "interpreter.h"
#include "common.h"
#include "at91sam.h"
#include "debug.h"
#include "nand.h"
#include "sdramc.h"
#include "pmc.h"

static lua_State *lua = NULL;

static at91_t *check_at91 (lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TUSERDATA);
    lua_getmetatable(L, index);
    if( ! lua_equal(L, lua_upvalueindex(1), -1) )
        luaL_typerror(L, index, "at91");  /* die */
    lua_pop(L, 1);
    return (at91_t *)*(void **)lua_touserdata(L, index);
}


#define lua_boxpointer(L,u) \
        (*(void **)(lua_newuserdata(L, sizeof(void *))) = (u))

#define lua_unboxpointer(L,i)   (*(void **)(lua_touserdata(L, i)))

static int at91_init(at91_t *at91)
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

static int l_at91open (lua_State *L)
{
    int vendor = luaL_checknumber(L, 1);  /* get argument */
    int product = luaL_checknumber(L, 2);  /* get argument */
    at91_t *at91;

    at91 = at91_open (vendor, product);
    if (at91) {
        at91_init (at91);
        /** FIXME: should be doing all the init calls here */
        *(void **)(lua_newuserdata(L, sizeof (void *))) = at91;
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_setmetatable(L, -2);
    } else
        lua_pushnil(L);
    return 1;  /* number of results */
}

static int l_at91close(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);

    at91_close (at91);
    return 0;
}

static int l_at91version(lua_State *L)
{
    char version[20];
    at91_t *at91 = check_at91(L, 1);

    if (at91_version (at91, version, sizeof (version)) < 0)
        return 0;

    lua_pushstring(L, version);
    return 1;
}

static int l_at91go(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);

    at91_go (at91, addr);
    return 0;
}

static int l_at91readb(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    int value = at91_read_byte (at91, addr);

    lua_pushnumber(L, value);
    return 1;
}

static int l_at91readw(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    int value = at91_read_half_word (at91, addr);

    lua_pushnumber(L, value);
    return 1;
}

static int l_at91readl(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    int value = at91_read_word (at91, addr);

    lua_pushnumber(L, value);
    return 1;
}

static int l_at91read_data(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    unsigned int length = luaL_checknumber(L, 3);
    char *data = malloc(length);

    at91_read_data (at91, addr, (unsigned char *)data, length);

    lua_pushlstring (L, data, length);
    free (data);

    return 1;
}

static int l_at91read_file(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    const char *filename = luaL_checkstring(L, 3);
    unsigned int length = luaL_checknumber(L, 4);

    at91_read_file (at91, addr, filename, length);

    return 0;
}

static int l_at91verify_file(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    const char *filename = luaL_checkstring(L, 3);

    int val = at91_verify_file (at91, addr, filename);

    printf ("verify: %d\n", val);

    lua_pushboolean(L, val);
    return 1;
}

static int l_at91writeb(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    unsigned int val = luaL_checknumber(L, 3);
    at91_write_byte (at91, addr, val);
    return 0;
}

static int l_at91writew(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    unsigned int val = luaL_checknumber(L, 3);
    at91_write_half_word (at91, addr, val);
    return 0;
}

static int l_at91writel(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    unsigned int val = luaL_checknumber(L, 3);
    at91_write_word (at91, addr, val);
    return 0;
}

static int l_at91write_data(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    size_t length;
    const char *data = luaL_checklstring(L, 3, &length);

    at91_write_data (at91, addr, (unsigned char *)data, length);

    return 0;
}

static int l_at91write_file(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    const char *file = luaL_checkstring(L, 3);

    at91_write_file (at91, addr, file);

    return 0;
}

static int l_at91dbg_init(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int baud = luaL_checknumber(L, 2);

    dbg_init (at91, BAUDRATE(MASTER_CLOCK, baud));

    return 0;
}

static int l_at91dbg_print(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    const char *string = luaL_checkstring(L, 2);

    dbg_print (at91, string);

    return 0;
}

static int l_at91nand_id(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    int id = nand_read_id (at91);

    lua_pushnumber(L, id);

    return 1;
}

static int l_at91nand_read_file(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    const char *file = luaL_checkstring(L, 3);
    unsigned int length = luaL_checknumber(L, 4);

    nand_read_file (at91, addr, file, length);

    return 0;
}

static int l_at91nand_read(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    unsigned int length = luaL_checknumber(L, 3);
    char *data = malloc (length);

    nand_read (at91, addr, data, length);
    lua_pushlstring (L, data, length);
    free (data);

    return 1;
}

static int l_at91nand_erase(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    unsigned int length = luaL_checknumber(L, 3);

    nand_erase (at91, addr, length);

    return 0;
}

static int l_at91nand_write(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    size_t length;
    const char *data = luaL_checklstring(L, 3, &length);

    nand_write (at91, addr, data, length);

    return 0;
}

static int l_at91nand_write_file(lua_State *L)
{
    at91_t *at91 = check_at91(L, 1);
    unsigned int addr = luaL_checknumber(L, 2);
    const char *file = luaL_checkstring(L, 3);

    nand_write_file (at91, addr, file);

    return 0;

}

static const luaL_reg meta_methods[] = {
    {"__gc", l_at91close },
    {0,0}
};

static const luaL_reg image_methods[] = {
    /* General functions */
    {"open", l_at91open},
    {"version", l_at91version},
    {"go", l_at91go},

    /* Reading memory */
    {"readb", l_at91readb},
    {"readw", l_at91readw},
    {"readl", l_at91readl},
    {"read_data", l_at91read_data},
    {"read_file", l_at91read_file},
    {"verify_file", l_at91verify_file},

    /* Writing memory */
    {"writeb", l_at91writeb},
    {"writew", l_at91writew},
    {"writel", l_at91writel},
    {"write_data", l_at91write_data},
    {"write_file", l_at91write_file},

    /* NAND */
    {"nand_id", l_at91nand_id},
    {"nand_read_file", l_at91nand_read_file},
    {"nand_read", l_at91nand_read},
    {"nand_erase", l_at91nand_erase},
    {"nand_write", l_at91nand_write},
    {"nand_write_file", l_at91nand_write_file},

    /* UART */
    {"dbg_init", l_at91dbg_init},
    {"dbg_print", l_at91dbg_print},

    {NULL, NULL}  /* sentinel */
};

#define newtable(L) (lua_newtable(L), lua_gettop(L))

int at91_lua_register (lua_State *L)
{
    int metatable, methods;

    lua_pushliteral(L, "at91");         /* name of Image table */
    methods   = newtable(L);           /* Image methods table */
    metatable = newtable(L);           /* Image metatable */
    lua_pushliteral(L, "__index");     /* add index event to metatable */
    lua_pushvalue(L, methods);
    lua_settable(L, metatable);        /* metatable.__index = methods */
    lua_pushliteral(L, "__metatable"); /* hide metatable */
    lua_pushvalue(L, methods);
    lua_settable(L, metatable);        /* metatable.__metatable = methods */
    luaL_openlib(L, 0, meta_methods,  0); /* fill metatable */
    luaL_openlib(L, 0, image_methods, 1); /* fill Image methods table */
    lua_settable(L, LUA_GLOBALSINDEX); /* add Image to globals */
    return 0;
}


int interpreter_init (void)
{
    lua = lua_open();
    if (!lua)
        return -1;

    luaL_openlibs(lua);
    at91_lua_register (lua);

    return 0;
}

int interpreter_process_line (const char *line)
{
    if (luaL_loadbuffer(lua, line, strlen(line), "line") ||
        lua_pcall(lua, 0, 0, 0)) {
        fprintf(stderr, "Failed to run '%s': %s", line, lua_tostring(lua, -1));
        lua_pop(lua, 1);  /* pop error message from the stack */
        return -1;
    }
    return 0;
}

int interpreter_process_file (const char *filename)
{
    if (luaL_dofile (lua, filename)) {
        fprintf (stderr, "Failed to run '%s': %s\n", 
                filename, lua_tostring(lua, -1));
        return -1;
    }
    return 0;
}

int interpreter_close (void)
{
    lua_close (lua);
    lua = NULL;
    return 0;
}


