#include "defs.h"
#include "oled/Fonts/fonts.h"
#include "oled/GUI/GUI_Paint.h"
#include <stdint.h>

// thresholds
int CPU_THRESHOLD[] = { 50, 95 };
int MEM_THRESHOLD[] = { 50, 95 };

UBYTE * init_display()
{
    // setup the OLED display
    printf("Initializing display...\n");
    DEV_ModuleInit();
    OLED_1in5_rgb_Init();
    DEV_Delay_ms(500);
    OLED_1in5_rgb_Clear();
    DEV_Delay_ms(500);

    // create the background image buffer
    UBYTE *ImageBackground;
    UWORD Imagesize = (OLED_1in5_RGB_WIDTH*2) * OLED_1in5_RGB_HEIGHT;
    if((ImageBackground = (UBYTE *)malloc(Imagesize + 300)) == NULL) {
        printf("Failed to setup the background image...\n");
        return NULL;
    }

	Paint_NewImage(ImageBackground, OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, 0, BLACK);
	Paint_SetScale(65);
	Paint_SetRotate(270);

	// clear the display
	Paint_Clear(BLACK);
	OLED_1in5_rgb_Display(ImageBackground);

    // return the black image pointer
    // This memory will live throughout the program's lifetime
    // so we don't need to free it
    return ImageBackground;
}

void write_status_update(UBYTE *ImageBackground, struct DisplayData data)
{
    uint16_t val_color = WHITE;
    char buffer[60];
    char value[16];
    memset(buffer, 0, sizeof(buffer));
    memset(value, 0, sizeof(value));

    // Clear the display
    //Paint_Clear(BLACK);

    // Mesh service status
    if(strcmp(data.svc_state, "Ok") != 0)
    {
        val_color = RED;
    }
    else
    {
        val_color = GREEN;
    }
    write_line(ImageBackground, "Mesh: ", data.svc_state, val_color, 0);

    // Node count
    snprintf(value, sizeof(value), "%d", data.nodes);
    write_line(ImageBackground, "Nodes: ", value, WHITE, 1);

    // GPS sync
    snprintf(value, sizeof(value), "%s", data.gps_sync ? "yes" : "no");
    if(data.gps_sync == true)
    {
        val_color = GREEN;
    }
    else
    {
        val_color = RED;
    }
    write_line(ImageBackground, "GPS: ", value, val_color, 2);

    // CPU load
    snprintf(value, sizeof(value), "%d%%", data.cpu_load);
    if(data.cpu_load < CPU_THRESHOLD[0])
    {
        val_color = GREEN;
    }
    else if(data.cpu_load < CPU_THRESHOLD[1])
    {
        val_color = YELLOW;
    }
    else
    {
        val_color = RED;
    }
    write_line(ImageBackground, "CPU: ", value, val_color, 3);

    // Memory usage
    snprintf(value, sizeof(value), "%d%%", data.mem_usage);
    if(data.mem_usage < MEM_THRESHOLD[0])
    {
        val_color = GREEN;
    }
    else if(data.mem_usage < MEM_THRESHOLD[1])
    {
        val_color = YELLOW;
    }
    else
    {
        val_color = RED;
    }
    write_line(ImageBackground, "Mem: ", value, val_color, 4);
}

void write_line(UBYTE *ImageBackground, const char *key, const char *value, uint16_t val_color, int lineno)
{
    int y = 10 + lineno * Font16.Height;
    int x1 = 0;
    //int x2 = text_width(key, &Font16);
    // set our second column to start at character 8
    int x2 = 7 * Font16.Width;
    // draw key
    Paint_DrawString_EN(x1, y, key, &Font16, BLACK, WHITE);
    // clear old value
    Paint_DrawString_EN(x2, y, "    ", &Font16, BLACK, BLACK);
    // draw value
    Paint_DrawString_EN(x2, y, value, &Font16, BLACK, val_color);
    OLED_1in5_rgb_Display(ImageBackground);
}

void shutdown_display()
{
    printf("Cleaning up the OLED display...\n");
    // // let any pending updates finish
    // DEV_Delay_ms(1500);
    // OLED_1in5_rgb_Clear();
    // DEV_Delay_ms(500);
    OLED_Reset();

}

int text_width(const char *text, sFONT * Font)
{
    return strlen(text) * Font->Width;
}
