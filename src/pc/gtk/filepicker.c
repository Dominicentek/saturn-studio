#include <gtk/gtk.h>

#include "../platform.h"
#include "../cliopts.h"

// callback which sets the ROM path to the chosen file's path
static void finish_open_dialog_callback(GObject *source_object, GAsyncResult *res, GtkWidget *parent) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    GFile *file;
    GtkAlertDialog *alert_dialog;
    GError *err = NULL;

    file = gtk_file_dialog_open_finish(dialog, res, &err);
    if (file != NULL) {
        // the user clicked the "Open" button, so set that path.
        g_autofree gchar *filePath = g_file_get_path(file);
        strncpy(gCLIOpts.RomPath, filePath, SYS_MAX_PATH);
    } else {
        // if the the user clicked the "Cancel" button do nothing, but if a
        // different error happened, display that.
        if (err->code != GTK_DIALOG_ERROR_DISMISSED) {
            alert_dialog = gtk_alert_dialog_new("%s", err->message);
            gtk_alert_dialog_show(alert_dialog, GTK_WINDOW(dialog));
            g_object_unref(alert_dialog);
        }
        g_clear_error(&err);
    }

    // close the parent window so the Saturn GUI can take over. 
    // gtk_window_close() did not work here when gtk_window_present() is commented out!!!
    gtk_window_destroy(GTK_WINDOW(parent));
}

// callback which spawns the file picker
static void start_open_dialog_callback(GtkWidget *parentButton, GtkWidget *parent) {
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select ROM");
    gtk_file_dialog_open(dialog,
                         GTK_WINDOW(parent),
                         NULL,
                         (GAsyncReadyCallback)finish_open_dialog_callback,
                         parent);
    g_object_unref(dialog);
}

// callback which runs when a file is dropped on the window
static gboolean file_drop_callback(GtkDropTarget *target,
                                   const GValue  *value,
                                   double         x,
                                   double         y,
                                   gpointer       data) {
    GtkWindow *parent = data;
    GFile *file;

    if (G_VALUE_HOLDS(value, G_TYPE_FILE)) {
        file = g_value_get_object(value);
        if (file != NULL) {
            // the drag and dropped entity is a file, so pass the file path to Saturn
            g_autofree gchar *filePath = g_file_get_path(file);
            strncpy(gCLIOpts.RomPath, filePath, SYS_MAX_PATH);
        }
    }

    gtk_window_destroy(GTK_WINDOW(parent));
    return true;
}

// spawn a toplevel GTK window which seems to be required for
// g_application_run() to successfully block execution, then spawn the GTK
// file picker prefab. User interaction with the toplevel window is optional
// and it will be invisible and skipped if configGreeter is set to false.
static void launch_gtk_gui(GtkApplication *app) {
    GtkWidget *sFileDialogParentWindow = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(sFileDialogParentWindow), "Saturn Greeter");
    gtk_window_set_default_size(GTK_WINDOW(sFileDialogParentWindow), 960, 600);

    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(box), false);
    gtk_window_set_child(GTK_WINDOW(sFileDialogParentWindow), box);

    GtkDropTarget *dragAndDropTarget = gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);
    gtk_drop_target_set_gtypes (dragAndDropTarget, (GType [1]){G_TYPE_FILE}, 2);
    g_signal_connect(dragAndDropTarget, "drop", G_CALLBACK(file_drop_callback), sFileDialogParentWindow);
    gtk_widget_add_controller(GTK_WIDGET(sFileDialogParentWindow), GTK_EVENT_CONTROLLER(dragAndDropTarget));

    char greeterImagePath[SYS_MAX_PATH] = "";
    strncat(greeterImagePath, sys_user_path(), SYS_MAX_PATH - 1);
    strncat(greeterImagePath, "/res/saturn-readmeheader.png", SYS_MAX_PATH - 1);
    GtkWidget *image = gtk_picture_new_for_filename(greeterImagePath);

    GtkWidget *label = gtk_label_new("<span size=\"large\">Welcome to Saturn! Please drag and drop your US Super Mario 64 .z64 ROM file to start, or click \"Select ROM\" and choose it.</span>\n");
    gtk_label_set_use_markup(GTK_LABEL(label), true);

    GtkWidget *dialogOpenButton = gtk_button_new_with_label("<span size=\"xx-large\">Select ROM</span>");
    gtk_label_set_use_markup(GTK_LABEL(gtk_button_get_child(GTK_BUTTON(dialogOpenButton))), true);
    gtk_widget_set_vexpand(dialogOpenButton, true);
    gtk_widget_set_valign(dialogOpenButton, GTK_ALIGN_FILL);
    g_signal_connect(dialogOpenButton, "clicked", G_CALLBACK(start_open_dialog_callback), sFileDialogParentWindow);

    gtk_box_append(GTK_BOX(box), image);
    gtk_box_append(GTK_BOX(box), label);
    gtk_box_append(GTK_BOX(box), dialogOpenButton);

    if (gCLIOpts.SkipGreeter) {
        start_open_dialog_callback(NULL, sFileDialogParentWindow);
    } else {
        gtk_window_present(GTK_WINDOW(sFileDialogParentWindow)); // will make parent window visible
    }
}

// entrypoint to spawn the file picker GUI and block execution until return
int open_file_picker(void) {
    int status;

    GtkApplication *sFileDialogParentApp = gtk_application_new("com.Llennpie.Saturn", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(sFileDialogParentApp, "activate", G_CALLBACK(launch_gtk_gui), NULL);
    status = g_application_run(G_APPLICATION(sFileDialogParentApp), 0, NULL);
    g_object_unref(sFileDialogParentApp);

    // This point in execution will be reached after the parent window sFileDialogParentWindow
    // receives an internal GTK signal to close, in turn signalling the sFileDialogParentApp to finish and end
    return status;
}