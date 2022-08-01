#ifndef YMIRDB_H
#define YMIRDB_H

#define MAX_KEY (16)
#define MAX_LINE (1024)

#include <stddef.h>
#include <sys/types.h>

enum item_type {
    INTEGER = 0,
    ENTRY = 1
};

typedef struct element element;
typedef struct entry entry;
typedef struct snapshot snapshot;
typedef struct main_state main_state;

// Stores each value within an entry
struct element {
    enum item_type type;
    union {
        int value;
        struct entry *entry;
  };
};

// Stores the entry
struct entry {
    char key[MAX_KEY];
    bool is_simple;
    element *values;
    size_t length;
    entry *prev;
    entry *next;


    size_t forward_size;
    entry **forward;

    size_t backward_size;
    entry **backward;
};

// Stores the snapshot with the corresponding entries
struct snapshot {
    int id;
    entry *entries;
    int num_entries;
    snapshot *prev;
    snapshot *next;
};

// Stores the current state's entries and a linked list of snapshots
struct main_state {
    snapshot *snapshots;
    int num_snapshots;
    int next_snapshot_id;

    entry *current_entries;
    int num_current_entries;
};

// Help string
const char *HELP_STRING =
    "BYE   clear database and exit\n"
    "HELP  display this help message\n"
    "\n"
    "LIST KEYS       displays all keys in current state\n"
    "LIST ENTRIES    displays all entries in current state\n"
    "LIST SNAPSHOTS  displays all snapshots in the database\n"
    "\n"
    "GET <key>    displays entry values\n"
    "DEL <key>    deletes entry from current state\n"
    "PURGE <key>  deletes entry from current state and snapshots\n"
    "\n"
    "SET <key> <value ...>     sets entry values\n"
    "PUSH <key> <value ...>    pushes values to the front\n"
    "APPEND <key> <value ...>  appends values to the back\n"
    "\n"
    "PICK <key> <index>   displays value at index\n"
    "PLUCK <key> <index>  displays and removes value at index\n"
    "POP <key>            displays and removes the front value\n"
    "\n"
    "DROP <id>      deletes snapshot\n"
    "ROLLBACK <id>  restores to snapshot and deletes newer snapshots\n"
    "CHECKOUT <id>  replaces current state with a copy of snapshot\n"
    "SNAPSHOT       saves the current state as a snapshot\n"
    "\n"
    "MIN <key>  displays minimum value\n"
    "MAX <key>  displays maximum value\n"
    "SUM <key>  displays sum of values\n"
    "LEN <key>  displays number of values\n"
    "\n"
    "REV <key>   reverses order of values (simple entry only)\n"
    "UNIQ <key>  removes repeated adjacent values (simple entry only)\n"
    "SORT <key>  sorts values in ascending order (simple entry only)\n"
    "\n"
    "FORWARD <key> lists all the forward references of this key\n"
    "BACKWARD <key> lists all the backward references of this key\n"
    "TYPE <key> displays if the entry of this key is simple or general\n";

    // Function declarations
    void* my_calloc(size_t count, size_t size);
    void my_free(void *ptr);
    void remove_from_forward(entry *current_entry, entry *backward_entry);
    void remove_from_backward(entry *current_entry, entry *forward_entry);
    void free_entries(entry *head);
    int get_key(const char *line, char *key, const char *command);
    int get_id(const char *line, const char *command);
    bool get_item_type(char *current_value);
    entry* get_entry(const char *key, const main_state *current_state);
    int get_next_value(const char *line, char *value);
    int get_values(const char *line, int key_size, element *values, int num_values,
                    const char *command, main_state *current_state);
    int get_num_values(const char *line, int key_size, const char *command);
    bool check_key(const char *key, int size);
    void print_values(const entry *entry);
    int get_index(const char *line, int key_size, char *command);
    int cmp_values(const void *a, const void *b);
    void command_sort(const char *line, main_state *current_state);
    void command_rev(const char *line, main_state *current_state);
    void command_uniq(const char *line, main_state *current_state);
    void command_pick(const char *line, main_state *current_state);
    void entry_pluck(entry *current_entry, int index, main_state *current_state);
    void command_pluck(const char *line, main_state *current_state);
    void entry_pop(entry *current_entry, main_state *current_state);
    void command_pop(const char *line, main_state *current_state);
    snapshot* get_snapshot(int id, main_state *current_state);
    void command_drop(const char *line, main_state *current_state);
    element* get_value_general(element *values, int size, int num_key);
    void deepcopy_values(element *values, int num_values, element *dest,
                            bool is_simple);
    void deepcopy_entry(entry *src_head, entry *e);
    void calculate_references(entry *current_entry, entry *matching_entry);
    entry* deepcopy_get_entry(entry *entries, char *key);
    void deepcopy_references(entry *entries);
    void deepcopy_entries(entry *entries, entry *dest);
    void command_checkout(const char *line, main_state *current_state);
    int check_cache(entry *current_entry, entry **cache, int cache_size);
    void get_bwd_references(entry *current_entry, entry **cache, int *cache_size);
    void get_fwd_references(entry *current_entry, entry **cache, int *cache_size);
    void print_unique_references(entry **references, int size);
    int cmp_keys(const void *a, const void *b);
    void command_forward(const char *line, main_state *current_state);
    void command_backward(const char *line, main_state *current_state);
    void command_type(const char *line, main_state *current_state);
    void command_list_keys(main_state *current_state);
    void command_rollback(const char *line, main_state *current_state);
    void command_list_snapshots(main_state *current_state);
    void command_snapshot(main_state *current_state);
    void command_get(const char *line, main_state *current_state);
    int update_references(element *tmp_values, int num_values,
                            entry *current_entry);
    void command_append(const char *line, main_state *current_state);
    void command_push(const char *line, main_state *current_state);
    void update_bi_directional_references(entry *e);
    void entry_delete(entry *e, main_state *current_state);
    bool is_valid_del(entry *current_entry, main_state *current_state);
    void command_del(const char *line, main_state *current_state);
    entry* snapshot_get_entry(const char *key, snapshot *current_snapshot);
    void snapshot_entry_delete(entry *e, snapshot *current_snapshot);
    bool is_valid_purge(const char *key, main_state *current_state);
    void command_purge(const char *line, main_state *current_state);
    bool check_general_keys(element *new_values, int num_values,
                                int num_general_keys, entry *entry_ptr);
    void set_references(entry *entry_ptr, int num_general_keys,
                            element *new_values, int num_values);
    void entry_update(const char *line, main_state *current_state,
                        entry *entry_ptr);
    int get_max(entry **cache, int cache_size);
    int get_min(entry **cache, int cache_size);
    int get_len(entry **cache, int cache_size);
    int get_sum(entry **cache, int cache_size);
    int check_cache_entries(entry *current_entry, entry ***cache, int cache_size);
    entry** get_entries(entry *current_entry, entry **result, int *result_size_ptr,
                            entry ***cache, int *cache_size_ptr, int *lengths);
    void command_min(const char *line, main_state *current_state);
    void command_max(const char *line, main_state *current_state);
    void command_sum(const char *line, main_state *current_state);
    void command_len(const char *line, main_state *current_state);
    void command_set(const char *line, main_state *current_state);
    void command_bye();
    void command_help();
    int process_command(const char *line, main_state *current_state);
    void free_all(main_state *current_state);

#endif
