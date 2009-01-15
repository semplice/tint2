typedef struct Launcher
{
        long *icon_data;
        int icon_width;
        int icon_height;
        char *exec;
        char *icon;
        char *text;
        int text_width;
        int posx;
        int posy;
        int width;
        int height;
} Launcher;

typedef struct launcher_global
{
        Launcher *launcherlist;
        int launcher_count;
} launcher_global;

launcher_global launcher;

void launcher_add_exec (const char *exec);
void launcher_add_icon (const char *icon);
void launcher_add_text (const char *text);

