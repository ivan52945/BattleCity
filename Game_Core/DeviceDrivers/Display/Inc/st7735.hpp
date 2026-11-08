#pragma once

#include "main.h"
#include <cstdint>

/* The docking point: the driver plugs in as the typed display of the
 * Game Core - GameT<Tank<LcdT>> with LcdT = this exact instantiation
 * dials the members below directly, no interface idiom in between.
 *
 * The driver streams the panel directly from caller-owned sources:
 * no framebuffer, no scratch, no generated patterns at runtime. The
 * boot clear is a flash strip asset (CLEAR_IMG below) blitted in
 * per-instance row blocks; everything else on the panel is a caller
 * print() of some const array. */

enum st7735_sizes {
    st_128x160_def,
    st_128x160_drk,
    st_128x160_red,
    st_128x160_grn,
    st_128x128,
    st_80x160_red,
    st_80x160_drk,
};

typedef struct  {
    uint8_t opc;
    uint8_t arg_n;
    uint8_t arg[16];
} st7735_cmd_i_t;

#define SF_RST    0x01
#define SLP_OUT   0x11
#define NORM_ON   0x13
#define COL_MOD   0x3A
#define COL_565   0x05
#define MAD_CTL   0x36
#define INV__ON   0x21
#define CA_SET    0x2A
#define RA_SET    0x2B
#define RAM_WR    0x2C
#define DISP_ON   0x29
#define FRM_C1    0xB1
#define FRM_C2    0xB2
#define FRM_C3    0xB3
#define INV_C     0xB4
#define PW_CR1    0xC0
#define PW_CR2    0xC1
#define PW_CR3    0xC2
#define PW_CR4    0xC3
#define PW_CR5    0xC4
#define VM_CR     0xC5
#define GM_CRP    0xE0
#define GM_CRN    0xE1

#define INV_COL   INV__ON

/* Wiring of a concrete display module to the SPI bus - as the template
 * parameter list. There is no wiring struct anymore: every constant of
 * the wiring is a non-type template argument the compiler stamps into
 * the transaction bodies, and the driver object carries no wiring
 * fields at all - no state of any kind. A line the module does
 * not wire is passed as 0 and its branch folds away entirely.
 *
 *   HSPI        - the SPI HAL handle as a pointer NTP: the extern
 *                 handle of the platform (&hspi4 etc., declared by
 *                 spi.h) - its address is resolved by the linker, so
 *                 the constant stays a literal through all bodies.
 *   *_ADDR      - the GPIO ports as raw integer base addresses
 *                 (GPIOx_BASE from the device header). The Cube port
 *                 pointer (GPIOE) cannot act as a non-type template
 *                 argument: int->ptr->integer round-trips through a
 *                 reinterpret_cast, which is rejected in a constant
 *                 expression - hence the integer form; the driver
 *                 materializes the pointer itself in the port()
 *                 functions below.
 *   *_PIN       - GPIO_PIN_n macros of the lines. DC is always wired;
 *                 CS/RST passed as {0, 0} mean "the module does not
 *                 expose the line".
 *   CS_ADDR/#/SINGLE - CS wired, but the bus is the panel's own: the
 *                 line is picked once in init(), select()/unselect()
 *                 fold into empty bodies for the rest of the lifetime;
 *                 CS_ADDR 0 - no CS at all, same empty bodies.
 *   RST_ADDR/#  - 0 = no RST wired (tied to VCC): the hardware reset
 *                 pulse of init() folds away.
 *   SIZES       - one of st7735_sizes: the panel RAM offset table.
 *   Y_INV / X_INV / XY_CHG / COL_BGR - MADCTL bits MY/MX/MV/BGR - the
 *                 sides the drawing coordinates map to. XY_CHG also
 *                 folds the geometry below (w/h and the window offsets
 *                 swap), the init() command table becomes a fully
 *                 constexpr array, no runtime MADCTL assembly spins up.
 *   CLEAR_IMG  - pointer NTP to the boot-clear strip asset: the
 *                 flash-resident const array of the instantiation TU
 *                 (like &hspi4, the address links in; the pixels stay
 *                 outside the driver). The strip is geo.w x CLEAR_ROWS
 *                 entries of the panel's stream order - init() blits
 *                 it down the panel block by block.
 *   CLEAR_ROWS - rows of the strip blitted per print: the lever
 *                 between asset size in flash and the number of DMA
 *                 engagements of the boot clear. Must divide h().
 */
template <SPI_HandleTypeDef* HSPI,
          std::uintptr_t DC_ADDR, uint32_t DC_PIN,
          std::uintptr_t CS_ADDR, uint32_t CS_PIN, bool SINGLE,
          std::uintptr_t RST_ADDR, uint32_t RST_PIN,
          st7735_sizes SIZES,
          bool Y_INV, bool X_INV, bool XY_CHG, bool COL_BGR,
          const uint16_t* CLEAR_IMG, uint32_t CLEAR_ROWS>
class ST7735 final
{
public:
    struct geometry {
        uint32_t w;
        uint32_t h;

        /* Window offsets of the panel RAM: the tab variants of the
         * module shift the origin (see make_geo). */
        uint32_t x_s;
        uint32_t y_s;
    };

    /* The window offset table of the configured size. The XY swap
     * (MV): w/h and the offsets exchange in compile time - the init()
     * command table stays fully constexpr. */
    static constexpr geometry make_geo()
    {
        geometry g = {.w = 0, .h = 0, .x_s = 0, .y_s = 0};

        switch (SIZES)
        {
            case st_128x160_def:
            case st_128x160_drk:
                g.w = 128; g.h = 160; g.x_s = 0;  g.y_s = 0;
                break;
            case st_128x160_red:
            case st_128x160_grn:
                g.w = 128; g.h = 160; g.x_s = 2;  g.y_s = 1;
                break;
            case st_128x128:
                g.w = 128; g.h = 128; g.x_s = 2;  g.y_s = 1;
                break;
            case st_80x160_drk:
                g.w = 80;  g.h = 160; g.x_s = 26; g.y_s = 1;
                break;
            case st_80x160_red:
            default:
                g.w = 80;  g.h = 160; g.x_s = 2;  g.y_s = 1;
                break;
        }

        if (XY_CHG)
        {
            uint32_t tmp = g.w;
            g.w = g.h;
            g.h = tmp;

            tmp = g.x_s;
            g.x_s = g.y_s;
            g.y_s = tmp;
        }

        return g;
    }

    /* The panel geometry of this instantiation - a compile-time
     * constant. */
    static constexpr geometry geo = make_geo();

    /* The boot clear covers the panel with strips: the strip height
     * must tile the panel height exactly - the print path streams the
     * window contiguously, a clipped tail block has no honest way in. */
    static_assert(geo.h % CLEAR_ROWS == 0,
                  "CLEAR_ROWS must divide the panel height");

    /* The port materializers: the integer template constant enters the
     * bodies as an immediate (or a literal-pool word when the address
     * is out of reach of a movw); the int->ptr conversion happens here,
     * outside of the constant-expression rules of the NTP language. */
    static GPIO_TypeDef* dc_port()  { return (GPIO_TypeDef*)DC_ADDR; }
    static GPIO_TypeDef* cs_port()  { return (GPIO_TypeDef*)CS_ADDR; }
    static GPIO_TypeDef* rst_port() { return (GPIO_TypeDef*)RST_ADDR; }

    /* Lifecycle. */
    void init()
    {
        //  Hardware reset pulse when the module wires the RST pin: MX_GPIO_Init
        //  may leave RES low, the controller would otherwise never leave the
        //  reset until raised here. Pulse numbers as in ST7789 - the soft
        //  reset (0x01) in the table below stays as well.
        //  RST not wired (tied to VCC, RST_ADDR 0) - the pulse folds away.
        if (RST_ADDR)
        {
            HAL_GPIO_WritePin(rst_port(), RST_PIN, GPIO_PIN_SET);
            HAL_Delay(10);
            HAL_GPIO_WritePin(rst_port(), RST_PIN, GPIO_PIN_RESET);
            HAL_Delay(10);
            HAL_GPIO_WritePin(rst_port(), RST_PIN, GPIO_PIN_SET);
            HAL_Delay(120);
        }

        static constexpr uint8_t mad_ctl = (Y_INV   ? 1 << 7 : 0)   //  MY
                                         | (X_INV   ? 1 << 6 : 0)   //  MX
                                         | (XY_CHG  ? 1 << 5 : 0)   //  MV
                                         | (COL_BGR ? 1 << 3 : 0);  //  RGB(0) / BGR(1)

        //  The init sequence: the geometry comes from geo, the
        //  orientation from mad_ctl - the whole table is constexpr
        //  with the values folded, no runtime assembly of it remains.
        static constexpr st7735_cmd_i_t cmd[] = {
            { SF_RST , 255, {255}},
            { SLP_OUT, 255, {255} },
            { FRM_C1, 3, {0x01, 0x0C, 0x2D} },
            { FRM_C2, 3, {0x01, 0x0C, 0x2D} },
            { FRM_C3, 6, {0x01, 0x0C, 0x2D, 0x01, 0x0C, 0x2D} },
            { INV_C,  1, {0x07}  },
            { PW_CR1, 3, {0xA2, 0x02, 0x84} },
            { PW_CR2, 1, {0xC5} },
            { PW_CR3, 2, {0x0A, 0x00} },
            { PW_CR4, 2, {0x8A, 0x2A} },
            { PW_CR5, 2, {0x8A, 0xEE} },
            { VM_CR,  1, {0x0E} },
            { INV_COL, 0 },
            { MAD_CTL, 1, {mad_ctl} },
            { COL_MOD, 1, {COL_565} },
            { CA_SET,  4, {0, 0, 0, (uint8_t)(geo.w - 1)} },
            { RA_SET,  4, {0, 0, 0, (uint8_t)(geo.h - 1)} },
            { GM_CRP, 16, {
                0x02, 0x1c, 0x07, 0x12,
                0x37, 0x32, 0x29, 0x2d,
                0x29, 0x25, 0x2B, 0x39,
                0x00, 0x01, 0x03, 0x10,
                }
            },
            { GM_CRN, 16, {
                  0x03, 0x1d, 0x07, 0x06,
                  0x2E, 0x2C, 0x29, 0x2D,
                  0x2E, 0x2E, 0x37, 0x3F,
                  0x00, 0x00, 0x02, 0x10,
                }
            },
            { NORM_ON, 255, {10}},
            { DISP_ON, 255, {150}},
        };

        //  The device is picked at the first actual SPI use (the init
        //  table below): single-device bus picks directly into the GPIO
        //  and is never touched again; a shared bus goes through
        //  select() per usual. The cycle closes by unselect() at the end
        //  of this method (after the clear strips).
        if (CS_ADDR && SINGLE)
            HAL_GPIO_WritePin(cs_port(), CS_PIN, GPIO_PIN_RESET);
        else
            select();

        for(size_t i = 0; i < sizeof(cmd) / sizeof(cmd[0]); ++i)
        {
            ST7735_InitCMD(&cmd[i]);
        }

        //  Boot clear: the flash strip asset (geo.w x CLEAR_ROWS
        //  entries, stamped per instantiation) runs down the panel in
        //  row blocks - no scratch, no generated pattern, the fill
        //  source is plain const data DMA-ed straight out of flash.
        for (uint32_t y = 0; y < geo.h; y += CLEAR_ROWS)
            print(0, (uint16_t)y, (uint16_t)geo.w, CLEAR_ROWS, CLEAR_IMG);

        //  The init() drawing cycle closes here: a shared bus releases
        //  CS (the Game then picks the device per its drawing cycles);
        //  single_device bus - unselect is a no-op, the line stays
        //  asserted for the lifetime of the driver.
        unselect();
    }

    /* Static blit: no per-object state anywhere on the print path
     * (the transaction chain is wired by the template arguments), so
     * Entity dials the type directly and the per-slot display pointer
     * of the core is gone. This is the whole drawing surface of the
     * driver: set the window, stream the caller's pixels - flash
     * assets, LVGL-style RAM buffers, anything - as long as the source
     * is contiguous over w x h. noinline keeps each core call site one
     * 'bl' - the inlined SetWindow+DMA body (~300 B) would replicate
     * in every print/clear site otherwise. */
    __attribute__((noinline)) static void print(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t img[])
    {
        ST7735_SetWindow(x, y, x + w - 1, y + h - 1);

        ST7735_BigDAT(reinterpret_cast<const uint8_t *>(&img[0]), w * h * 2);
    }

    /* User-led CS control: the driver never touches the line inside
     * its transactions - the GameT drawing cycles call select()/
     * unselect() around every frame, and only on a shared bus
     * (the device is not the only one) the pin is actually driven.
     * single_device bus: picked once in init(), both methods are pure
     * no-ops afterwards. No cs_* wiring - no-ops as well. init() runs
     * its own cycle. No CS and/or single-device instantiations fold
     * into empty bodies here. */
    void select() const
    {
        if (CS_ADDR && !SINGLE)
            HAL_GPIO_WritePin(cs_port(), CS_PIN, GPIO_PIN_RESET);
    }

    void unselect() const
    {
        if (CS_ADDR && !SINGLE)
            HAL_GPIO_WritePin(cs_port(), CS_PIN, GPIO_PIN_SET);
    }

    /* Geometry is a compile-time constant of the instantiation
     * (constexpr static - the Game folds the field edges and the
     * spawn ladder from these at compile time). */
    static constexpr uint16_t w()  { return (uint16_t)geo.w; }
    static constexpr uint16_t h() { return (uint16_t)geo.h; }

private:
    static void ST7735_TrnsmtCMD(uint8_t cmd)
    {
        HAL_GPIO_WritePin(dc_port(), DC_PIN, GPIO_PIN_RESET);
        HAL_SPI_Transmit(HSPI, &cmd, 1, 15);
        while(HSPI->State != HAL_SPI_STATE_READY) { }
        HAL_GPIO_WritePin(dc_port(), DC_PIN, GPIO_PIN_SET);
    }

    static void ST7735_TrnsmtDAT(const uint8_t data[], uint8_t size)
    {
        HAL_SPI_Transmit(HSPI, &data[0], size, 15);
        while(HSPI->State != HAL_SPI_STATE_READY) { }
    }

    static void ST7735_SmallCMD(uint8_t cmd, const uint8_t arg[], uint8_t arg_n)
    {
        ST7735_TrnsmtCMD(cmd);
        ST7735_TrnsmtDAT(&arg[0], arg_n);
    }

    static void ST7735_BigDAT(const uint8_t data[], uint32_t size)
    {
        while (size > 0)
        {
            uint32_t chunk = size > 0xFFFF ? 0xFFFF : size;

            HAL_SPI_Transmit_DMA(HSPI, &data[0], chunk);
            while(HSPI->State != HAL_SPI_STATE_READY) { }

            data += chunk;
            size -= chunk;
        }
    }

    static void ST7735_InitCMD(const st7735_cmd_i_t* cmd)
    {
        ST7735_TrnsmtCMD(cmd->opc);

        if (cmd->arg_n != 0xFF)
            ST7735_TrnsmtDAT(&cmd->arg[0], cmd->arg_n);
        else
            HAL_Delay(cmd->arg[0]);
    }

    //  Low-byte addresses only: ST7735 RAM fits 256 rows/cols, so the
    //  high halves of the window coordinates stay zero.
    static void ST7735_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
    {
        static uint8_t data[4] = { 0 };

        data[1] = x0 + geo.x_s;
        data[3] = x1 + geo.x_s;

        ST7735_SmallCMD(CA_SET, &data[0], 4);

        data[1] = y0 + geo.y_s;
        data[3] = y1 + geo.y_s;

        ST7735_SmallCMD(RA_SET, &data[0], 4);

        ST7735_TrnsmtCMD(RAM_WR);
    }
};
