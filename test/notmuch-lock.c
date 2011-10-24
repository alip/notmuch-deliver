/* vim: set cino=t0 fo=croql sw=4 ts=4 sts=0 noet cin fdm=syntax nolist : */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib.h>

#include <notmuch.h>


#define MIN_UWAIT 1000        // 1 millisecond
#define MAX_UWAIT 600000000   // 10 minutes



static gint sleep_option = -1;
static GOptionEntry entries[] = {
	{ "sleep", 's', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_INT, &sleep_option,
		"Sleep for N microseconds, while holding the lock to the database", "N" },
	{NULL, 0, 0, 0, NULL, NULL, NULL},
};



static gboolean
load_keyfile(const gchar *path, gchar **db_path, gchar ***tags)
{
	GKeyFile *fd;
	GError *error;

	fd = g_key_file_new();
	error = NULL;
	if (!g_key_file_load_from_file(fd, path, G_KEY_FILE_NONE,
				&error)) {
		g_printerr("Failed to parse `%s': %s", path,
				error->message);
		g_error_free(error);
		g_key_file_free(fd);
		return FALSE;
	}

	*db_path = g_key_file_get_string(fd, "database",
			"path", &error);
	if (*db_path == NULL) {
		g_critical("Failed to parse database.path from `%s': %s", path,
				error->message);
		g_error_free(error);
		g_key_file_free(fd);
		return FALSE;
	}

	*tags = g_key_file_get_string_list(fd, "new",
			"tags", NULL, NULL);

	g_key_file_free(fd);
	return TRUE;
}



static gchar *
get_db_path(void)
{
	gchar *conf_path, *db_path;

	// Get location of notmuch config
	if (g_getenv("NOTMUCH_CONFIG"))
		conf_path = g_strdup(g_getenv("NOTMUCH_CONFIG"));
	else if (g_getenv("HOME"))
		conf_path = g_build_filename(g_getenv("HOME"),
				".notmuch-config", NULL);
	else {
		g_critical("Neither NOTMUCH_CONFIG nor HOME set");
		return NULL;
	}

	// Get location of notmuch database
	gchar **conf_tags = NULL;
	g_print("Parsing configuration from `%s'\n", conf_path);
	if (!load_keyfile(conf_path, &db_path, &conf_tags)) {
		g_free(conf_path);
		return NULL;
	}

	g_free(conf_path);
	return db_path;
}


int
main(int argc, char *argv[])
{
	gchar *db_path;
	gint32 sleep_time = 0;
	notmuch_database_t *db;
	GError *error = NULL;
	GOptionContext *context;

	// parse command line options
	gboolean option_parse_success = FALSE;
	context = g_option_context_new( "COMMAND [ARGS...]");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context,
			"Utility to test behaviour of programs while the notmuch database is locked");
	option_parse_success = g_option_context_parse(context, &argc, &argv, &error);
	g_option_context_free(context);
	if (! option_parse_success) {
		g_printerr("option parsing failed: %s\n", error->message);
		return EXIT_FAILURE;
	}

	// Check new program args
	if ((argc - 1) <= 0) {
		g_printerr("no command supplied\n");
		return EXIT_FAILURE;
	}
	char **new_argv = argv + 1;
	if (g_strcmp0(new_argv[0], "--") == 0)
		new_argv++;

	// Get notmuch database path
	db_path = get_db_path();
	if (db_path == NULL)
		return EXIT_FAILURE;

	// Open notmuch database
	g_printerr("Opening notmuch database `%s'\n", db_path);
	db = notmuch_database_open(db_path, NOTMUCH_DATABASE_MODE_READ_WRITE);
	g_free(db_path);
	if (db == NULL)
		return EXIT_FAILURE;

	gboolean spawn_success = FALSE;
	GPid child_pid = 0;
	spawn_success = g_spawn_async(
			g_get_current_dir(),
			new_argv,
			NULL,
			G_SPAWN_SEARCH_PATH | G_SPAWN_CHILD_INHERITS_STDIN | G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&child_pid,
			NULL
			);
	if (!spawn_success) {
		g_printerr("faild to spawn child\n");
	}

	// Sleep for some time
	if (sleep_option >= 0) {
		sleep_time = sleep_option;
	} else {
		sleep_time = g_random_int_range(MIN_UWAIT, MAX_UWAIT);
	}
	g_printerr("Sleeping for %f secs\n", ((double)sleep_time)/(1000*1000));
	usleep(sleep_time);

	// Close database again
	notmuch_database_close(db);

	int child_status = 0;
	child_pid = waitpid(child_pid, &child_status, 0);

	return WEXITSTATUS(child_status);
}
