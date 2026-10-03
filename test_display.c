#include "unity.h"
#include "defs.h"
#include "oled/Fonts/fonts.h"
#include "oled/GUI/GUI_Paint.h"
#include <stdlib.h>
#include <string.h>

sFONT Font16 = { NULL, 11, 16 };   /* Width, Height (matches Waveshare Font16) */

typedef struct {
    int x, y;
    char text[32];
    uint16_t bg, fg;
} draw_call_t;

#define MAX_CALLS 64
static draw_call_t calls[MAX_CALLS];
static int draw_count;
static int display_count;
static int new_image_count;
static int reset_count;

void run_display_test();

// It's probably better to use CMock here, but
// I didn't want to pull in a dependency on ruby
// to make this test run.

// Function overrides
UBYTE DEV_ModuleInit(void) { return 0; }
void DEV_Delay_ms(UDOUBLE ms) { (void)ms; }
void OLED_1in5_rgb_Init(void) {}
void OLED_1in5_rgb_Clear(void) {}
void OLED_Reset(void) { reset_count++; }
void OLED_1in5_rgb_Display(UBYTE *img) { (void)img; display_count++; }

void Paint_NewImage(UBYTE *image, UWORD w, UWORD h, UWORD rot, UWORD color)
{
    (void)image; (void)w; (void)h; (void)rot; (void)color;
    new_image_count++;
}
void Paint_SetScale(UBYTE scale) { (void)scale; }
void Paint_SetRotate(UWORD rotate) { (void)rotate; }
void Paint_Clear(UWORD color) { (void)color; }

void Paint_DrawString_EN(UWORD x, UWORD y, const char *str, sFONT *font,
                         UWORD bg, UWORD fg)
{
    (void)font;
    if (draw_count < MAX_CALLS) {
        draw_call_t *c = &calls[draw_count];
        c->x = x;
        c->y = y;
        snprintf(c->text, sizeof(c->text), "%s", str);
        c->bg = bg;
        c->fg = fg;
    }
    draw_count++;
}

// Helpers
#define VALUE_CALL(line) (calls[(line) * 3 + 2])
enum { LINE_MESH, LINE_NODES, LINE_GPS, LINE_CPU, LINE_MEM };

static uint16_t cpu_color(int load)
{
    UBYTE dummy;
    struct DisplayData d = { .svc_state = "Ok", .cpu_load = load };
    setUp();
    write_status_update(&dummy, d);
    return VALUE_CALL(LINE_CPU).fg;
}

static uint16_t mem_color(int usage)
{
    UBYTE dummy;
    struct DisplayData d = { .svc_state = "Ok", .mem_usage = usage };
    setUp();
    write_status_update(&dummy, d);
    return VALUE_CALL(LINE_MEM).fg;
}

// Lifecycle
void setUp(void)
{
    memset(calls, 0, sizeof(calls));
    draw_count = display_count = new_image_count = reset_count = 0;
}

void tearDown(void)
{

}

// Tests
void test_text_width_is_length_times_font_width(void)
{
    TEST_ASSERT_EQUAL_INT(0, text_width("", &Font16));
    TEST_ASSERT_EQUAL_INT(4 * Font16.Width, text_width("test", &Font16));
}

void test_write_line_draws_key_clear_then_value(void)
{
    UBYTE dummy;
    write_line(&dummy, "CPU: ", "42%", GREEN, 3);

    TEST_ASSERT_EQUAL_INT(3, draw_count);
    TEST_ASSERT_EQUAL_STRING("CPU: ", calls[0].text);
    TEST_ASSERT_EQUAL_STRING("    ", calls[1].text);   /* clears old value */
    TEST_ASSERT_EQUAL_UINT16(BLACK, calls[1].fg);
    TEST_ASSERT_EQUAL_STRING("42%", calls[2].text);
    TEST_ASSERT_EQUAL_UINT16(GREEN, calls[2].fg);
    TEST_ASSERT_EQUAL_INT(1, display_count);
}

void test_write_line_positions(void)
{
    UBYTE dummy;
    write_line(&dummy, "K: ", "V", WHITE, 2);

    TEST_ASSERT_EQUAL_INT(0, calls[0].x);
    TEST_ASSERT_EQUAL_INT(10 + 2 * Font16.Height, calls[0].y);
    TEST_ASSERT_EQUAL_INT(7 * Font16.Width, calls[2].x);   /* second column */
    TEST_ASSERT_EQUAL_INT(calls[0].y, calls[2].y);
}

void test_status_update_writes_five_lines(void)
{
    UBYTE dummy;
    struct DisplayData d = { .svc_state = "Ok", .nodes = 7, .gps_sync = true,
                             .cpu_load = 10, .mem_usage = 20 };
    write_status_update(&dummy, d);

    TEST_ASSERT_EQUAL_INT(15, draw_count);
    TEST_ASSERT_EQUAL_INT(5, display_count);
    TEST_ASSERT_EQUAL_STRING("Ok", VALUE_CALL(LINE_MESH).text);
    TEST_ASSERT_EQUAL_STRING("7", VALUE_CALL(LINE_NODES).text);
    TEST_ASSERT_EQUAL_STRING("yes", VALUE_CALL(LINE_GPS).text);
    TEST_ASSERT_EQUAL_STRING("10%", VALUE_CALL(LINE_CPU).text);
    TEST_ASSERT_EQUAL_STRING("20%", VALUE_CALL(LINE_MEM).text);
}

void test_mesh_state_colors(void)
{
    UBYTE dummy;
    struct DisplayData ok  = { .svc_state = "Ok" };
    struct DisplayData bad = { .svc_state = "Down" };

    write_status_update(&dummy, ok);
    TEST_ASSERT_EQUAL_UINT16(GREEN, VALUE_CALL(LINE_MESH).fg);

    setUp();
    write_status_update(&dummy, bad);
    TEST_ASSERT_EQUAL_UINT16(RED, VALUE_CALL(LINE_MESH).fg);
}

void test_gps_colors_and_text(void)
{
    UBYTE dummy;
    struct DisplayData yes = { .svc_state = "Ok", .gps_sync = true };
    struct DisplayData no  = { .svc_state = "Ok", .gps_sync = false };

    write_status_update(&dummy, yes);
    TEST_ASSERT_EQUAL_STRING("yes", VALUE_CALL(LINE_GPS).text);
    TEST_ASSERT_EQUAL_UINT16(GREEN, VALUE_CALL(LINE_GPS).fg);

    setUp();
    write_status_update(&dummy, no);
    TEST_ASSERT_EQUAL_STRING("no", VALUE_CALL(LINE_GPS).text);
    TEST_ASSERT_EQUAL_UINT16(RED, VALUE_CALL(LINE_GPS).fg);
}

void test_cpu_thresholds(void)
{
    TEST_ASSERT_EQUAL_UINT16(GREEN,  cpu_color(0));
    TEST_ASSERT_EQUAL_UINT16(GREEN,  cpu_color(CPU_THRESHOLD[0] - 1));
    TEST_ASSERT_EQUAL_UINT16(YELLOW, cpu_color(CPU_THRESHOLD[0]));
    TEST_ASSERT_EQUAL_UINT16(YELLOW, cpu_color(CPU_THRESHOLD[1] - 1));
    TEST_ASSERT_EQUAL_UINT16(RED,    cpu_color(CPU_THRESHOLD[1]));
    TEST_ASSERT_EQUAL_UINT16(RED,    cpu_color(100));
}

void test_mem_thresholds(void)
{
    TEST_ASSERT_EQUAL_UINT16(GREEN,  mem_color(0));
    TEST_ASSERT_EQUAL_UINT16(GREEN,  mem_color(MEM_THRESHOLD[0] - 1));
    TEST_ASSERT_EQUAL_UINT16(YELLOW, mem_color(MEM_THRESHOLD[0]));
    TEST_ASSERT_EQUAL_UINT16(YELLOW, mem_color(MEM_THRESHOLD[1] - 1));
    TEST_ASSERT_EQUAL_UINT16(RED,    mem_color(MEM_THRESHOLD[1]));
    TEST_ASSERT_EQUAL_UINT16(RED,    mem_color(100));
}

void test_init_display_returns_buffer_and_clears_screen(void)
{
    UBYTE *img = init_display();

    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL_INT(1, new_image_count);
    TEST_ASSERT_EQUAL_INT(1, display_count);
    free(img);
}

void test_shutdown_display_resets_oled(void)
{
    shutdown_display();
    TEST_ASSERT_EQUAL_INT(1, reset_count);
}

void run_display_test()
{
    RUN_TEST(test_text_width_is_length_times_font_width);
    RUN_TEST(test_write_line_draws_key_clear_then_value);
    RUN_TEST(test_write_line_positions);
    RUN_TEST(test_status_update_writes_five_lines);
    RUN_TEST(test_mesh_state_colors);
    RUN_TEST(test_gps_colors_and_text);
    RUN_TEST(test_cpu_thresholds);
    RUN_TEST(test_mem_thresholds);
    RUN_TEST(test_init_display_returns_buffer_and_clears_screen);
    RUN_TEST(test_shutdown_display_resets_oled);
}
