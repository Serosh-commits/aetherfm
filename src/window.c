#include "window.h"

enum {
    COLUMN_ICON,
    COLUMN_NAME,
    N_COLUMNS
};

typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkWidget *path_entry;
    GtkWidget *sidebar;
    gchar *current_path;
} AetherWindow;

static gchar *clipboard_path = NULL;

static void refresh_view(AetherWindow *app_window);

static void
on_copy_action(GtkMenuItem *item, gpointer user_data) {
    AetherWindow *app_window = (AetherWindow *)user_data;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;
    
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app_window->tree_view));
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);
        
        if (clipboard_path) {
            g_free(clipboard_path);
        }
        clipboard_path = g_build_filename(app_window->current_path, name, NULL);
        
        g_free(name);
    }
}

static void
on_paste_action(GtkMenuItem *item, gpointer user_data) {
    AetherWindow *app_window = (AetherWindow *)user_data;
    
    if (clipboard_path) {
        GFile *source = g_file_new_for_path(clipboard_path);
        gchar *basename = g_file_get_basename(source);
        gchar *dest_path = g_build_filename(app_window->current_path, basename, NULL);
        GFile *dest = g_file_new_for_path(dest_path);
        
        g_file_copy(source, dest, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL);
        
        g_object_unref(source);
        g_object_unref(dest);
        g_free(basename);
        g_free(dest_path);
        
        refresh_view(app_window);
    }
}

static void
on_delete_confirmed(GtkWidget *widget, gpointer user_data) {
    gchar *path = (gchar *)user_data;
    GFile *file = g_file_new_for_path(path);
    g_file_delete(file, NULL, NULL);
    g_object_unref(file);
    g_free(path);
    gtk_widget_destroy(gtk_widget_get_toplevel(widget));
}

static void
on_delete_action(GtkMenuItem *item, gpointer user_data) {
    AetherWindow *app_window = (AetherWindow *)user_data;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;
    gchar *full_path;
    GtkWidget *dialog;
    
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app_window->tree_view));
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);
        full_path = g_build_filename(app_window->current_path, name, NULL);
        
        dialog = gtk_message_dialog_new(GTK_WINDOW(app_window->window),
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_QUESTION,
                                        GTK_BUTTONS_YES_NO,
                                        "Are you sure you want to delete '%s'?", name);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
            GFile *file = g_file_new_for_path(full_path);
            g_file_delete(file, NULL, NULL);
            g_object_unref(file);
            refresh_view(app_window);
        }
        
        gtk_widget_destroy(dialog);
        g_free(full_path);
        g_free(name);
    }
}

static void
on_rename_action(GtkMenuItem *item, gpointer user_data) {
    AetherWindow *app_window = (AetherWindow *)user_data;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;
    gchar *full_path;
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *entry;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(app_window->tree_view));
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);
        full_path = g_build_filename(app_window->current_path, name, NULL);
        
        dialog = gtk_dialog_new_with_buttons("Rename",
                                             GTK_WINDOW(app_window->window),
                                             GTK_DIALOG_MODAL,
                                             "_Cancel", GTK_RESPONSE_CANCEL,
                                             "_Rename", GTK_RESPONSE_ACCEPT,
                                             NULL);
        
        content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), name);
        gtk_container_add(GTK_CONTAINER(content_area), entry);
        gtk_widget_show_all(dialog);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            const gchar *new_name = gtk_entry_get_text(GTK_ENTRY(entry));
            gchar *new_path = g_build_filename(app_window->current_path, new_name, NULL);
            GFile *file = g_file_new_for_path(full_path);
            GFile *new_file = g_file_new_for_path(new_path);
            
            g_file_move(file, new_file, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL);
            
            g_object_unref(file);
            g_object_unref(new_file);
            g_free(new_path);
            refresh_view(app_window);
        }
        
        gtk_widget_destroy(dialog);
        g_free(full_path);
        g_free(name);
    }
}

static void
on_create_folder_action(GtkMenuItem *item, gpointer user_data) {
    AetherWindow *app_window = (AetherWindow *)user_data;
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *entry;

    dialog = gtk_dialog_new_with_buttons("New Folder",
                                         GTK_WINDOW(app_window->window),
                                         GTK_DIALOG_MODAL,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Create", GTK_RESPONSE_ACCEPT,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "New Folder");
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry));
        gchar *path = g_build_filename(app_window->current_path, name, NULL);
        GFile *file = g_file_new_for_path(path);
        
        g_file_make_directory(file, NULL, NULL);
        
        g_object_unref(file);
        g_free(path);
        refresh_view(app_window);
    }

    gtk_widget_destroy(dialog);
}

static void
on_create_file_action(GtkMenuItem *item, gpointer user_data) {
    AetherWindow *app_window = (AetherWindow *)user_data;
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *entry;

    dialog = gtk_dialog_new_with_buttons("New File",
                                         GTK_WINDOW(app_window->window),
                                         GTK_DIALOG_MODAL,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Create", GTK_RESPONSE_ACCEPT,
                                         NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), "new_file.txt");
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const gchar *name = gtk_entry_get_text(GTK_ENTRY(entry));
        gchar *path = g_build_filename(app_window->current_path, name, NULL);
        GFile *file = g_file_new_for_path(path);
        
        g_file_create(file, G_FILE_CREATE_NONE, NULL, NULL);
        
        g_object_unref(file);
        g_free(path);
        refresh_view(app_window);
    }

    gtk_widget_destroy(dialog);
}

static gboolean
on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AetherWindow *app_window = (AetherWindow *)user_data;
    
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkWidget *menu;
        GtkWidget *item_new_folder;
        GtkWidget *item_new_file;
        GtkWidget *item_copy;
        GtkWidget *item_paste;
        GtkWidget *item_delete;
        GtkWidget *item_rename;
        GtkWidget *separator1;
        GtkWidget *separator2;
        
        menu = gtk_menu_new();

        item_new_folder = gtk_menu_item_new_with_label("New Folder");
        g_signal_connect(item_new_folder, "activate", G_CALLBACK(on_create_folder_action), app_window);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_new_folder);

        item_new_file = gtk_menu_item_new_with_label("New File");
        g_signal_connect(item_new_file, "activate", G_CALLBACK(on_create_file_action), app_window);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_new_file);

        separator1 = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator1);
        
        item_copy = gtk_menu_item_new_with_label("Copy");
        g_signal_connect(item_copy, "activate", G_CALLBACK(on_copy_action), app_window);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_copy);
        
        item_paste = gtk_menu_item_new_with_label("Paste");
        g_signal_connect(item_paste, "activate", G_CALLBACK(on_paste_action), app_window);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_paste);
        if (!clipboard_path) {
            gtk_widget_set_sensitive(item_paste, FALSE);
        }
        
        separator2 = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator2);
        
        item_rename = gtk_menu_item_new_with_label("Rename");
        g_signal_connect(item_rename, "activate", G_CALLBACK(on_rename_action), app_window);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_rename);
        
        item_delete = gtk_menu_item_new_with_label("Delete");
        g_signal_connect(item_delete, "activate", G_CALLBACK(on_delete_action), app_window);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item_delete);
        
        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
        
        return TRUE;
    }
    return FALSE;
}

static void
refresh_view(AetherWindow *app_window) {
    GtkListStore *store;
    GDir *dir;
    const gchar *entry_name;
    GError *error = NULL;
    GFile *location;

    store = gtk_list_store_new(N_COLUMNS, G_TYPE_ICON, G_TYPE_STRING);

    dir = g_dir_open(app_window->current_path, 0, &error);
    if (dir) {
        while ((entry_name = g_dir_read_name(dir))) {
            GtkTreeIter iter;
            gchar *full_path;
            gchar *content_type;
            GIcon *icon;
            
            if (g_str_has_prefix(entry_name, ".")) {
                continue;
            }

            full_path = g_build_filename(app_window->current_path, entry_name, NULL);
            content_type = g_content_type_guess(full_path, NULL, 0, NULL);
            icon = g_content_type_get_icon(content_type);

            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               COLUMN_ICON, icon,
                               COLUMN_NAME, entry_name,
                               -1);
            
            g_object_unref(icon);
            g_free(content_type);
            g_free(full_path);
        }
        g_dir_close(dir);
    }

    gtk_tree_view_set_model(GTK_TREE_VIEW(app_window->tree_view), GTK_TREE_MODEL(store));
    g_object_unref(store);
    
    gtk_entry_set_text(GTK_ENTRY(app_window->path_entry), app_window->current_path);

    location = g_file_new_for_path(app_window->current_path);
    gtk_places_sidebar_set_location(GTK_PLACES_SIDEBAR(app_window->sidebar), location);
    g_object_unref(location);
}

static void
on_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    AetherWindow *app_window = (AetherWindow *)user_data;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;
    gchar *new_path;

    model = gtk_tree_view_get_model(tree_view);
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, COLUMN_NAME, &name, -1);
        
        new_path = g_build_filename(app_window->current_path, name, NULL);
        if (g_file_test(new_path, G_FILE_TEST_IS_DIR)) {
            g_free(app_window->current_path);
            app_window->current_path = new_path;
            refresh_view(app_window);
        } else {
            GError *error = NULL;
            gtk_show_uri_on_window(GTK_WINDOW(app_window->window), "file://", GDK_CURRENT_TIME, &error);
            if (error) {
                gchar *uri = g_filename_to_uri(new_path, NULL, NULL);
                gtk_show_uri_on_window(GTK_WINDOW(app_window->window), uri, GDK_CURRENT_TIME, NULL);
                g_free(uri);
                g_error_free(error);
            }
            g_free(new_path);
        }
        
        g_free(name);
    }
}

static void
on_up_clicked(GtkButton *button, gpointer user_data) {
    AetherWindow *app_window = (AetherWindow *)user_data;
    gchar *parent;

    parent = g_path_get_dirname(app_window->current_path);
    if (g_strcmp0(app_window->current_path, parent) != 0) {
        g_free(app_window->current_path);
        app_window->current_path = parent;
        refresh_view(app_window);
    } else {
        g_free(parent);
    }
}

static void
on_sidebar_open_location(GtkPlacesSidebar *sidebar, GFile *location, GtkPlacesOpenFlags flags, gpointer user_data) {
    AetherWindow *app_window = (AetherWindow *)user_data;
    gchar *path;

    path = g_file_get_path(location);
    if (path) {
        g_free(app_window->current_path);
        app_window->current_path = path;
        refresh_view(app_window);
    }
}

static void
setup_tree_view_renderer(GtkWidget *tree_view) {
    GtkCellRenderer *renderer_pixbuf;
    GtkCellRenderer *renderer_text;
    GtkTreeViewColumn *column;

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, "Name");

    renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer_pixbuf, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer_pixbuf, "gicon", COLUMN_ICON);

    renderer_text = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer_text, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer_text, "text", COLUMN_NAME);

    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
}

void
aetherfm_window_activate(GtkApplication *app, gpointer user_data) {
    AetherWindow *app_window;
    GtkWidget *header;
    GtkWidget *box;
    GtkWidget *toolbar;
    GtkWidget *up_button;
    GtkWidget *paned;
    GtkWidget *scrolled_sidebar;
    GtkWidget *scrolled_view;

    app_window = g_new0(AetherWindow, 1);
    app_window->current_path = g_get_current_dir();

    app_window->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(app_window->window), "AetherFM");
    gtk_window_set_default_size(GTK_WINDOW(app_window->window), 900, 600);
    g_object_set_data_full(G_OBJECT(app_window->window), "aether-window", app_window, g_free);

    header = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), "AetherFM");
    gtk_window_set_titlebar(GTK_WINDOW(app_window->window), header);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app_window->window), box);

    toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar), 5);
    gtk_box_pack_start(GTK_BOX(box), toolbar, FALSE, FALSE, 0);

    up_button = gtk_button_new_from_icon_name("go-up-symbolic", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(up_button, "clicked", G_CALLBACK(on_up_clicked), app_window);
    gtk_box_pack_start(GTK_BOX(toolbar), up_button, FALSE, FALSE, 0);

    app_window->path_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(toolbar), app_window->path_entry, TRUE, TRUE, 0);

    paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(box), paned, TRUE, TRUE, 0);

    scrolled_sidebar = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_sidebar), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    app_window->sidebar = gtk_places_sidebar_new();
    gtk_places_sidebar_set_open_flags(GTK_PLACES_SIDEBAR(app_window->sidebar), GTK_PLACES_OPEN_NORMAL);
    g_signal_connect(app_window->sidebar, "open-location", G_CALLBACK(on_sidebar_open_location), app_window);
    gtk_container_add(GTK_CONTAINER(scrolled_sidebar), app_window->sidebar);
    
    gtk_paned_pack1(GTK_PANED(paned), scrolled_sidebar, FALSE, FALSE);

    scrolled_view = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_view), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    app_window->tree_view = gtk_tree_view_new();
    setup_tree_view_renderer(app_window->tree_view);
    g_signal_connect(app_window->tree_view, "row-activated", G_CALLBACK(on_row_activated), app_window);
    g_signal_connect(app_window->tree_view, "button-press-event", G_CALLBACK(on_button_press), app_window);
    gtk_container_add(GTK_CONTAINER(scrolled_view), app_window->tree_view);

    gtk_paned_pack2(GTK_PANED(paned), scrolled_view, TRUE, FALSE);
    gtk_paned_set_position(GTK_PANED(paned), 200);

    refresh_view(app_window);

    gtk_widget_show_all(app_window->window);
}
