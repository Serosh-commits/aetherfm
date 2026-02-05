#include "window.h"

enum {
    COLUMN_ICON,
    COLUMN_NAME,
    N_COLUMNS
};

typedef struct {
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *tree_view;
    GtkWidget *icon_view;
    GtkWidget *path_entry;
    GtkWidget *sidebar;
    GtkListStore *model;
    gchar *current_path;
    gboolean show_hidden;
    gboolean grid_mode;
} AetherWindow;

static void refresh_content(AetherWindow *app);

static void load_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *screen = gdk_display_get_default_screen(display);
    
    gtk_style_context_add_provider_for_screen(screen,
                                            GTK_STYLE_PROVIDER(provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    if (!gtk_css_provider_load_from_path(provider, "../src/style.css", NULL)) {
        gtk_css_provider_load_from_path(provider, "src/style.css", NULL);
    }
    
    g_object_unref(provider);
}

void aetherfm_app_startup(GApplication *app, gpointer user_data) {
    load_css();
}

static void show_error(AetherWindow *app, const gchar *message) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_CLOSE,
                                             "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void open_path(AetherWindow *app, const gchar *path) {
    GFile *file = g_file_new_for_path(path);
    
    if (!g_file_query_exists(file, NULL)) {
        show_error(app, "Path does not exist");
        g_object_unref(file);
        return;
    }

    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        g_free(app->current_path);
        app->current_path = g_strdup(path);
        gtk_entry_set_text(GTK_ENTRY(app->path_entry), app->current_path);
        refresh_content(app);
    } else {
        gtk_show_uri_on_window(GTK_WINDOW(app->window), "file://", GDK_CURRENT_TIME, NULL);
    }
    g_object_unref(file);
}

static void on_sidebar_open_location(GtkPlacesSidebar *sidebar, GFile *location, GtkPlacesOpenFlags flags, gpointer user_data) {
    AetherWindow *app = (AetherWindow *)user_data;
    gchar *path = g_file_get_path(location);
    if (path) {
        open_path(app, path);
        g_free(path);
    }
}

static void on_list_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    AetherWindow *app = (AetherWindow *)user_data;
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    gchar *name;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);
        gchar *full_path = g_build_filename(app->current_path, name, NULL);
        open_path(app, full_path);
        g_free(full_path);
        g_free(name);
    }
}

static void on_grid_item_activated(GtkIconView *icon_view, GtkTreePath *path, gpointer user_data) {
    AetherWindow *app = (AetherWindow *)user_data;
    GtkTreeModel *model = gtk_icon_view_get_model(icon_view);
    GtkTreeIter iter;
    gchar *name;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);
        gchar *full_path = g_build_filename(app->current_path, name, NULL);
        open_path(app, full_path);
        g_free(full_path);
        g_free(name);
    }
}

static void on_up_clicked(GtkButton *button, gpointer user_data) {
    AetherWindow *app = (AetherWindow *)user_data;
    gchar *parent = g_path_get_dirname(app->current_path);
    if (g_strcmp0(app->current_path, parent) != 0) {
        open_path(app, parent);
    }
    g_free(parent);
}

static void on_toggle_hidden(GtkToggleButton *button, gpointer user_data) {
    AetherWindow *app = (AetherWindow *)user_data;
    app->show_hidden = gtk_toggle_button_get_active(button);
    refresh_content(app);
}

static void on_toggle_view(GtkToggleButton *button, gpointer user_data) {
    AetherWindow *app = (AetherWindow *)user_data;
    app->grid_mode = gtk_toggle_button_get_active(button);
    gtk_stack_set_visible_child_name(GTK_STACK(app->stack), app->grid_mode ? "grid" : "list");
}

static void refresh_content(AetherWindow *app) {
    GDir *dir;
    const gchar *name;
    GError *error = NULL;

    gtk_list_store_clear(app->model);

    dir = g_dir_open(app->current_path, 0, &error);
    if (!dir) {
        if (error) {
            show_error(app, error->message);
            g_error_free(error);
        }
        return;
    }

    while ((name = g_dir_read_name(dir))) {
        if (!app->show_hidden && name[0] == '.') continue;

        GtkTreeIter iter;
        gchar *full_path = g_build_filename(app->current_path, name, NULL);
        gchar *content_type = g_content_type_guess(full_path, NULL, 0, NULL);
        GIcon *icon = g_content_type_get_icon(content_type);
        gchar *display_name = g_filename_to_utf8(name, -1, NULL, NULL, NULL);

        if (!display_name) {
            display_name = g_strescape(name, "");
        }

        gtk_list_store_append(app->model, &iter);
        gtk_list_store_set(app->model, &iter,
                           COLUMN_ICON, icon,
                           COLUMN_NAME, display_name,
                           -1);

        g_object_unref(icon);
        g_free(display_name);
        g_free(content_type);
        g_free(full_path);
    }
    g_dir_close(dir);
}

static void setup_sidebar(AetherWindow *app) {
    app->sidebar = gtk_places_sidebar_new();
    gtk_places_sidebar_set_open_flags(GTK_PLACES_SIDEBAR(app->sidebar), GTK_PLACES_OPEN_NORMAL);
    g_signal_connect(app->sidebar, "open-location", G_CALLBACK(on_sidebar_open_location), app);
}

static void setup_list_view(AetherWindow *app) {
    GtkCellRenderer *renderer_pixbuf;
    GtkCellRenderer *renderer_text;
    GtkTreeViewColumn *column;

    app->tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->model));
    
    column = gtk_tree_view_column_new();
    renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer_pixbuf, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer_pixbuf, "gicon", COLUMN_ICON);
    
    renderer_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer_text, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer_text, "text", COLUMN_NAME);
    
    gtk_tree_view_append_column(GTK_TREE_VIEW(app->tree_view), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(app->tree_view), FALSE);
    
    g_signal_connect(app->tree_view, "row-activated", G_CALLBACK(on_list_row_activated), app);
}

static void setup_grid_view(AetherWindow *app) {
    GtkCellRenderer *renderer_pixbuf;
    GtkCellRenderer *renderer_text;

    app->icon_view = gtk_icon_view_new_with_model(GTK_TREE_MODEL(app->model));
    gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(app->icon_view), GTK_SELECTION_SINGLE);
    gtk_icon_view_set_item_width(GTK_ICON_VIEW(app->icon_view), 80);
    gtk_icon_view_set_column_spacing(GTK_ICON_VIEW(app->icon_view), 20);
    gtk_icon_view_set_row_spacing(GTK_ICON_VIEW(app->icon_view), 20);
    gtk_icon_view_set_margin(GTK_ICON_VIEW(app->icon_view), 20);

    renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    g_object_set(renderer_pixbuf, "stock-size", GTK_ICON_SIZE_DIALOG, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(app->icon_view), renderer_pixbuf, FALSE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(app->icon_view), renderer_pixbuf, "gicon", COLUMN_ICON);

    renderer_text = gtk_cell_renderer_text_new();
    g_object_set(renderer_text, 
                 "alignment", PANGO_ALIGN_CENTER,
                 "wrap-mode", PANGO_WRAP_WORD_CHAR,
                 "wrap-width", 100,
                 NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(app->icon_view), renderer_text, TRUE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(app->icon_view), renderer_text, "text", COLUMN_NAME);

    g_signal_connect(app->icon_view, "item-activated", G_CALLBACK(on_grid_item_activated), app);
}

static GtkWidget* create_header_bar(AetherWindow *app) {
    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *up_btn, *hidden_btn, *view_btn;

    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), "AetherFM");

    up_btn = gtk_button_new_from_icon_name("go-up-symbolic", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(up_btn, "clicked", G_CALLBACK(on_up_clicked), app);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), up_btn);

    app->path_entry = gtk_entry_new();
    gtk_header_bar_set_custom_title(GTK_HEADER_BAR(header), app->path_entry);

    hidden_btn = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(hidden_btn), gtk_image_new_from_icon_name("view-hidden-symbolic", GTK_ICON_SIZE_BUTTON));
    g_signal_connect(hidden_btn, "toggled", G_CALLBACK(on_toggle_hidden), app);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), hidden_btn);

    view_btn = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(view_btn), gtk_image_new_from_icon_name("view-grid-symbolic", GTK_ICON_SIZE_BUTTON));
    g_signal_connect(view_btn, "toggled", G_CALLBACK(on_toggle_view), app);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), view_btn);

    return header;
}

void aetherfm_window_activate(GtkApplication *app_instance, gpointer user_data) {
    AetherWindow *app = g_new0(AetherWindow, 1);
    GtkWidget *paned, *scrolled_list, *scrolled_grid;

    app->current_path = g_get_current_dir();
    app->model = gtk_list_store_new(N_COLUMNS, G_TYPE_ICON, G_TYPE_STRING);
    app->window = gtk_application_window_new(app_instance);
    
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1000, 700);
    gtk_window_set_titlebar(GTK_WINDOW(app->window), create_header_bar(app));

    paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_container_add(GTK_CONTAINER(app->window), paned);

    setup_sidebar(app);
    gtk_paned_pack1(GTK_PANED(paned), app->sidebar, FALSE, FALSE);

    app->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    
    setup_list_view(app);
    scrolled_list = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_list), app->tree_view);
    gtk_stack_add_named(GTK_STACK(app->stack), scrolled_list, "list");

    setup_grid_view(app);
    scrolled_grid = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_grid), app->icon_view);
    gtk_stack_add_named(GTK_STACK(app->stack), scrolled_grid, "grid");

    gtk_paned_pack2(GTK_PANED(paned), app->stack, TRUE, FALSE);
    gtk_paned_set_position(GTK_PANED(paned), 250);

    open_path(app, app->current_path);
    gtk_widget_show_all(app->window);
}
