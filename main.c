#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXLINE 1024

char roku_address[64];

struct GRID_ITEM {
    int x_pos;
    int y_pos;
    int width;
    int height;
    char *tooltip;
    char *command;
    char *icon_location;
};

int button_count = 12;
struct GRID_ITEM grid_items[] = { 
    0, 0, 1, 1, "Power", "Power", "/usr/share/icons/Adwaita/48x48/actions/system-shutdown-symbolic.symbolic.png", 
    0, 2, 1, 1, "Back", "Back", "/usr/share/icons/Adwaita/48x48/actions/edit-undo-symbolic.symbolic.png",
    2, 2, 1, 1, "Options", "Info", "/usr/share/icons/Adwaita/48x48/emblems/emblem-system-symbolic.symbolic.png",
    2, 0, 1, 1, "Home", "Home", "/usr/share/icons/Adwaita/48x48/actions/go-home-symbolic.symbolic.png", 
    1, 0, 1, 1, "Up", "Up", "/usr/share/icons/Adwaita/48x48/actions/go-up-symbolic.symbolic.png", 
    1, 2, 1, 1, "Down", "Down", "/usr/share/icons/Adwaita/48x48/actions/go-down-symbolic.symbolic.png",
    0, 1, 1, 1, "Left", "Left", "/usr/share/icons/Adwaita/48x48/actions/go-previous-symbolic.symbolic.png",
    2, 1, 1, 1, "Right", "Right", "/usr/share/icons/Adwaita/48x48/actions/go-next-symbolic.symbolic.png",
    1, 1, 1, 1, "Select", "Select", "/usr/share/icons/Adwaita/48x48/actions/object-select-symbolic.symbolic.png",
    0, 3, 1, 1, "Volume Down", "VolumeDown", "/usr/share/icons/Adwaita/48x48/status/audio-volume-low-symbolic.symbolic.png",
    1, 3, 1, 1, "Volume Up", "VolumeUp", "/usr/share/icons/Adwaita/48x48/status/audio-volume-high-symbolic.symbolic.png",
    2, 3, 1, 1, "Volume Mute", "VolumeMute", "/usr/share/icons/Adwaita/48x48/status/audio-volume-muted-symbolic.symbolic.png"
};

int get_roku_address()
{
    int sockfd;
    int n, len, ret;
    char buffer[MAXLINE];
    char *location;
    char *message = "M-SEARCH * HTTP/1.1\r\n"
                    "Host: 239.255.255.250:1900\r\n"
                    "Man: \"ssdp:discover\"\r\n"
                    "ST: roku:ecp\r\n"
                    "MX: 3\r\n"
                    "\r\n";
    struct sockaddr_in servaddr;

    printf("finding address...\n");

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("no response\n");
        return 0;
    }


    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1900);
    servaddr.sin_addr.s_addr = inet_addr("239.255.255.250");

    ret = sendto(sockfd, (const char *)message, strlen(message),
                 MSG_CONFIRM, (const struct sockaddr *)&servaddr,
                 sizeof(servaddr));

    n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                 MSG_WAITALL, (struct sockaddr *)&servaddr,
                 &len);

    buffer[n] = '\0';

    location = strstr(buffer, "LOCATION");
    if (location == NULL)
    {
        printf("no response\n");
        return 0;
    }

    sscanf(location, "LOCATION: %s", &roku_address);
    printf("%s\n", roku_address);

    close(sockfd);

    return 1;
}

static void roku_command(GtkWidget *widget, gpointer data)
{
    gchar buf[128];
    gchar *cmdfmt = "curl -d '' %s/keypress/%s";

    if (roku_address == NULL)
        return;

    sprintf(buf, cmdfmt, roku_address, (gchar *)data);

    int status = system(buf);
    if (status)
    {
        g_print("Command failed. Result: %d\n", status);
    }
}

static void activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *button;
    GtkWidget *image;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Roku TV");  
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);  
    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    for (int i = 0; i < button_count; i++)
    {
        button = gtk_button_new();
        image = gtk_image_new_from_file(grid_items[i].icon_location);
        gtk_button_set_image(GTK_BUTTON(button), image);
        gtk_widget_set_tooltip_text(button, grid_items[i].tooltip);
        g_signal_connect(button, "clicked", G_CALLBACK(roku_command), grid_items[i].command);
        gtk_grid_attach(GTK_GRID(grid), button, grid_items[i].x_pos, grid_items[i].y_pos, grid_items[i].width, grid_items[i].height);
    }
    
    gtk_widget_show_all(window);
}

int main(int argc, char **argv)
{
    GtkApplication *app;
    int status;

    if (get_roku_address() == 0)
    {
        printf("Roku TV not found\n");


        // try old address

        
        exit(0);
    }

    app = gtk_application_new("com.dclabs.roku-gtk", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
