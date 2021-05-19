#include <glib.h>
#include <gio/gio.h>

int main (int argc, char *argv[]) {

    // initialize glib
    g_type_init();

    GError *error = NULL;

    // create a new connection
    GSocketConnection *connection = NULL;
    GSocketClient *client = g_socket_client_new();

    // connect to the host
    connection = g_socket_client_connect_to_host (client, (gchar*)argv[1], atoi(argv[2]), NULL, &error);

    // check for errors
    if (error != NULL) {
        g_error(error->message);
    }
    else {
        g_print("Connection successful\n");
    }

    // use the connection
    const char *test_data = "message from client to server";
    GInputStream *in_stream = g_io_stream_get_input_stream(G_IO_STREAM(connection));
    GOutputStream *out_stream = g_io_stream_get_output_stream(G_IO_STREAM(connection));
    g_output_stream_write(out_stream, test_data, strlen(test_data), NULL, &error);
    // check for errors
    if (error != NULL) {
      g_error (error->message);
    }
    return 0;
}

