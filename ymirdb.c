/**
 * COMP2017 - Assignment 2
 * Guillaume Troadec
 * gtro3802
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

#include "ymirdb.h"

// Wrapper function for calloc
void* my_calloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    return ptr;
}

// Wrapper function for free
void my_free(void *ptr) {
    free(ptr);
}

// Removes the current entry from the backward entry's forward pointers
void remove_from_forward(entry *current_entry, entry *backward_entry) {
    // Check for invalid parameters
    if (NULL == backward_entry) {
        return;
    }

    // Initialise array one smaller than current size
    entry **new_forward = (entry **) my_calloc(backward_entry->forward_size - 1,
                                                sizeof(entry *));

    // Ensure only one entry is removed, allowing for duplicate general entry values
    bool found = false;

    // Copy over previous values except for the entry to be removed
    int j = 0;
    for (int i = 0; i < backward_entry->forward_size; i++) {
        entry *e = backward_entry->forward[i];
        if ((NULL != e) && (found || (0 != strcmp(current_entry->key, e->key)))) {
            new_forward[j] = e;
            j++;
        } else {
            found = true;
        }
    }

    // Free previous pointer, update members
    my_free(backward_entry->forward);
    backward_entry->forward = new_forward;
    backward_entry->forward_size--;

    // Free allocated memory if the new reference size is 0
    if (0 == backward_entry->forward_size) {
        my_free(backward_entry->forward);
        backward_entry->forward = NULL;
    }
}

// Removes the current entry from the forward entry's backward pointers
void remove_from_backward(entry *current_entry, entry *forward_entry) {
    // Check for invalid parameters
    if (NULL == forward_entry) {
        return;
    }

    // Initialise array one smaller than current size
    entry **new_backward = (entry **) my_calloc(forward_entry->backward_size - 1,
                                                    sizeof(entry *));

    // Ensure only one entry is removed, allowing for duplicate general entry values
    bool found = false;

    // Copy over previous values except for the entry to be removed
    int j = 0;
    for (int i = 0; i < forward_entry->backward_size; i++) {
        entry *e = forward_entry->backward[i];
        if ((NULL != e) && (found || (0 != strcmp(current_entry->key, e->key)))) {
            new_backward[j] = e;
            j++;
        } else {
            found = true;
        }
    }

    // Free previous pointer, update members
    my_free(forward_entry->backward);
    forward_entry->backward = new_backward;
    forward_entry->backward_size--;

    // Free allocated memory if the new reference size is 0
    if (0 == forward_entry->backward_size) {
        my_free(forward_entry->backward);
        forward_entry->backward = NULL;
    }
}

// Frees all associated memory with a linked list of entries
void free_entries(entry *head) {
    // Travese the linked list of entries
    while(NULL != head) {
        // Store entry pointer in a temporary variable
        entry *tmp = head;

        head = head->next;

        // Free memory associated with the entry's values
        if (NULL != tmp->values) {
            my_free(tmp->values);
        }

        // Free the memory associated with the backward references
        if (NULL != tmp->forward) {
            for (int i = 0; i < tmp->forward_size; i++) {
                remove_from_backward(tmp, tmp->forward[i]);
            }
            my_free(tmp->forward);
        }

        // Free the memory associated with the forward references
        if (NULL != tmp->backward) {
            for (int i = 0; i < tmp->backward_size; i++) {
                remove_from_forward(tmp, tmp->backward[i]);
            }
            my_free(tmp->backward);
        }

        // Free the entry pointer
        if (NULL != tmp) {
            my_free(tmp);
        }
    }
}

// Gets the key from the input line and returns the key size
int get_key(const char *line, char *key, const char *command) {
    // Store the size of the key
    int size = 0;

    // Offset from the space between command and key
    int key_location = strlen(command) + 1;

    int i = key_location;

    // Iterate through the line
    while (('\0' != line[i]) && ('\n' != line[i]) && (line[i] != ' ')) {
        // Return -1 if the size exceeds maximum key size
        if (size >= (MAX_KEY - 1)) {
            return -1;
        }

        // Update the key
        key[size] = line[i];

        // Update increments
        size++;
        i++;
    }
    return size;
}

// Gets the id given a command and input line
int get_id(const char *line, const char *command) {
    // Offset from the space between command and id
    int id_location = strlen(command) + 1;

    // Check that the id given is numeric
    for (int i = id_location; i < strlen(line)-1; i++) {
        if (0 == isdigit(line[i])) {
            return -1;
        }
    }
    return strtol(line + id_location, NULL, 10);
}

// Determine whether a value is ENTRY or INTEGER
bool get_item_type(char *current_value) {
    bool is_entry_type = false;

    if (0 != isalpha(current_value[0])) {
        is_entry_type = true;
    }

    return is_entry_type;
}

// Searches for a entry in the current state that has the matching key
entry* get_entry(const char *key, const main_state *current_state) {
    bool found = false;
    entry *head = current_state->current_entries;

    // Iterate through the linked list, comparing keys
    while(NULL != head) {
        if (0 == strcmp(key, head->key)) {
            found = true;
            break;
        }
        head = head->next;
    }

    // Return the entry pointer if found, else return NULL
    if (!found || NULL == head) {
        return NULL;
    } else {
        return head;
    }
}

// Gets the next value stored in the input line
int get_next_value(const char *line, char *value) {
    // Store the size of the value
    int size = 0;

    // Iterate through the line
    int i = 0;
    while (('\0' != line[i]) && ('\n' != line[i]) && (line[i] != ' ')) {
        // Return -1 if the size exceeds maximum key size
        if (size >= (MAX_KEY - 1)) {
            return -1;
        }

        // Update the value
        value[size] = line[i];

        // Update increments
        size++;
        i++;
    }
    return size;
}

// Iterates through the input line, appends values into an array of elements
// Returns the size of array of elements
int get_values(const char *line, int key_size, element *values, int num_values,
                const char *command, main_state *current_state) {
    // Get the key from the input line that corresponds to the values
    char key[MAX_KEY] = "";
    get_key(line, key, command);

    // Get the number of ENTRY values given
    int num_general_keys = 0;

    // Get the index where the values begin
    int values_location = strlen(command) + key_size + 2;
    int i = values_location;

    char current_value[MAX_KEY] = "";

    // Iterate through the values
    int num = 0;
    while(num < num_values) {
        // Get the length of the value to properly increment pointers
        int value_length = get_next_value(line + i, current_value);

        if (value_length == -1) {
            return -1;
        }

        // Get whether the value is ENTRY or INTEGER
        bool is_entry_type = get_item_type(current_value);

        element *element_ptr = (element *) my_calloc(1, sizeof(element));

        element_ptr->type = is_entry_type;

        // Set element value
        if (is_entry_type) {
            // Set element pointer to respective address
            entry *entry_ptr = get_entry(current_value, current_state);

            // Invalid command
            if (NULL == entry_ptr) {
                if (0 == strcmp(current_value, key)) {
                    // Prevent self references
                    printf("not permitted\n\n");
                } else {
                    // Key does not exist in current state
                    printf("no such key\n\n");
                }
                my_free(element_ptr);
                return -1;
            } else {
                // Set the ENTRY element to point to the corresponding entry
                element_ptr->entry = entry_ptr;
                num_general_keys++;
            }

        } else {
            // Set element value to int
            element_ptr->value = atoi(current_value);
        }

        // Add the element to the array of values
        values[num] = *element_ptr;

        // Free and zero associated memory
        my_free(element_ptr);
        memset(current_value, 0, MAX_KEY);

        i += (value_length + 1);
        num++;
    }

    return num_general_keys;
}

// Returns the number of values following a command + key combination
int get_num_values(const char *line, int key_size, const char *command) {
    // Store the number of values
    int num_values = 0;

    // Get the location of the values, offsetted by 2 spaces (command-key, key-value)
    int values_location = strlen(command) + key_size + 2;

    int i = values_location;

    // Count the number of spaces and hence determine the number of values
    while('\0' != line[i]) {
        if ('\n' == line[i]) {
            num_values++;
            break;
        } else if (' ' == line[i]) {
            num_values++;
        }
        i++;
    }
    return num_values;
}

// Returns true if the input key is valid
bool check_key(const char *key, int size) {
    if (size <= 0) {
        // If size is invalid
        printf("not permitted\n\n");
        return false;
    } else if (0 == isalpha(key[0])) {
        // Check that the first character is alphabetical
        printf("not permitted\n\n");
        return false;
    }

    // Check that all characters are alphanumerical
    for (int i = 0; i < size; i++) {
        if (0 == isalnum(key[i])) {
            printf("not permitted\n\n");
            return false;
        }
    }
    return true;
}

// Prints the values of an entry
void print_values(const entry *entry) {
    printf("[");

    // Iterate through the array of values
    for (int i = 0; i < entry->length; i++) {
        // Get each element, print its contents
        element *e = &entry->values[i];
        if ((NULL != e) && (INTEGER == e->type)) {
            if (i != entry->length-1) {
                printf("%d ", e->value);
            } else {
                printf("%d", e->value);
            }
        } else if ((NULL != e) && (ENTRY == e->type)){
            if (i != entry->length-1) {
                printf("%s ", e->entry->key);
            } else {
                printf("%s", e->entry->key);
            }
        }
    }
    printf("]\n");
    return;
}

// Get the associated index given a PICK, PLUCK command
int get_index(const char *line, int key_size, char *command) {
    // Offset by the two spaces between the command-key, key-inedx
    int index_location = strlen(command) + key_size + 2;

    // Check that the index value is valid
    for (int i = index_location; i < strlen(line) - 1; i++) {
        // Check that only the start can have a negative sign
        if (i == index_location && line[i] == '-') {
            continue;
        }
        // Return if the index is not numeric
        if (0 == isdigit(line[i])) {
            return -1;
        }
    }
    return strtol(line + index_location, NULL, 10);
}

// Comparator function that compares two INTEGER elements
int cmp_values(const void *a, const void *b) {
    element *element_a = (element *) a;
    element *element_b = (element *) b;
    return element_a->value - element_b->value;
}

// Sort the values of a simple entry in ascending order
void command_sort(const char *line, main_state *current_state) {
    // Get the key
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "SORT");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the entry that corresponds to the key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    } else if (!(current_entry->is_simple)) {
        printf("not permitted\n\n");
        return;
    }

    // Sort the entry's values in ascending order
    qsort(current_entry->values, current_entry->length, sizeof(element),
            cmp_values);

    printf("ok\n\n");
}

// Reverse the values of a simple entry
void command_rev(const char *line, main_state *current_state) {
    // Get the key
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "REV");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the entry that corresponds to the key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    } else if (!(current_entry->is_simple)) {
        printf("not permitted\n\n");
        return;
    }

    // Allocate new memory
    element *new_values = (element *) my_calloc(current_entry->length,
                                                    sizeof(element));

    // Iterate through the values in reverse order
    // Add them to the array of new values
    int j = 0;
    for (int i = current_entry->length - 1; i >= 0; i--) {
        new_values[j] = current_entry->values[i];
        j++;
    }

    // Free existing memory and update new values
    my_free(current_entry->values);
    current_entry->values = new_values;
    printf("ok\n\n");
}

// Remove duplicate adjacent values in a simple entry
void command_uniq(const char *line, main_state *current_state) {
    // Get the key
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "UNIQ");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the entry that corresponds to the key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    } else if (!(current_entry->is_simple)) {
        printf("not permitted\n\n");
        return;
    }

    // Store the new number of values post-uniq
    int new_num_values = 0;

    // Allocate new memory
    element *new_values = (element *) my_calloc(current_entry->length,
                                                    sizeof(element));

    // Iterate through the array of values
    for (int i = 0; i < current_entry->length; i++) {
        // Boolean shortcircuit if we are at the start of the array
        // Do not add the value to the new array if it is the same as the previous value
        if ((0 == i) || ((current_entry->values)[i].value !=
                            (current_entry->values)[i-1].value)) {
            new_values[new_num_values] = current_entry->values[i];
            new_num_values++;
        }
    }

    // Free previous pointers and update entry members
    my_free(current_entry->values);
    current_entry->values = new_values;
    current_entry->length = new_num_values;

    printf("ok\n\n");
}

// Gets the value at the given index (N.B 1-based indexing is used)
void command_pick(const char *line, main_state *current_state) {
    // Get the key
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "PICK");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the entry that corresponds to the key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    }

    // Get the index that is associated with the PICK command
    int index = get_index(line, key_size, "PICK");

    if (index < 1 || index > current_entry->length) {
        printf("index out of range\n\n");
    } else {
        // Print the value
        element * elem = &((current_entry->values)[index-1]);
        if (INTEGER == elem->type) {
            printf("%d\n\n", elem->value);
        } else {
            printf("%s\n\n", elem->entry->key);
        }
    }
}

// Removes the value at the given index (N.B 1-based indexing is used)
void entry_pluck(entry *current_entry, int index, main_state *current_state) {
    // Initiaise a new array that will store the new values
    element *new_values = (element *) my_calloc((current_entry->length - 1),
                                                    sizeof(element));

    bool is_simple = true;
    entry *entry_ptr = NULL;

    // Iterate through the array of values
    int j = 0;
    for (int i = 0; i < current_entry->length; i++) {
        // Copy the values at aren't at the plucked index
        element *elem = &current_entry->values[i];
        if (i != index - 1) {
            // If it contains an ENTRY value, it is a general entry
            if (ENTRY == elem->type) {
                is_simple = false;
            }

            // Add the value to the array of new values
            new_values[j] = *elem;
            j++;
        } else if ((i == index - 1) && (ENTRY == elem->type)) {
            // Save the pointer to the plucked value for freeing later
            entry_ptr = elem->entry;
        }
    }

    // Update bi-directional references
    if (NULL != entry_ptr) {
        // Remove the current entry from the forward entry's backward pointer
        remove_from_backward(current_entry, entry_ptr);

        // Remove the plucked entry from the current entry's forward pointer
        remove_from_forward(entry_ptr, current_entry);

    }

    // Free associated memory and update entry members
    my_free(current_entry->values);
    current_entry->is_simple = is_simple;
    current_entry->values = new_values;
    current_entry->length--;
}

// Parse the pluck command, pluck the index if the entry exists and the index is in range
void command_pluck(const char *line, main_state *current_state) {
    // Get the key
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "PLUCK");

    if (!check_key(key, key_size)) {
        return;
    }

    entry *current_entry = get_entry(key, current_state);

    // Get the entry that corresponds to the key
    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    }

    // Get the index value associated with the PLUCK command
    int index = get_index(line, key_size, "PLUCK");

    if (index < 1 || index > current_entry->length) {
        printf("index out of range\n\n");
    } else {
        // Print the value and pluck it
        element *elem = &((current_entry->values)[index-1]);
        if (INTEGER == elem->type) {
            printf("%d\n\n", elem->value);
        } else {
            printf("%s\n\n", elem->entry->key);
        }
        entry_pluck(current_entry, index, current_state);
    }
}

// Pops the first element from the current entry
void entry_pop(entry *current_entry, main_state *current_state) {
    // Get the first value within the array of values
    element *element_ptr = &(current_entry->values[0]);

    // Print out the element value/key
    if (INTEGER == element_ptr->type) {
        printf("%d\n\n", element_ptr->value);
    } else {
        printf("%s\n\n", (element_ptr->entry)->key);

        // Remove the current entry from the popped entry's backward pointer
        remove_from_backward(current_entry, element_ptr->entry);

        // Remove the popped entry from the current entry's forward pointer
        remove_from_forward(element_ptr->entry, current_entry);
    }

    element *new_values = (element *) my_calloc((current_entry->length - 1),
                                                    sizeof(element));

    bool is_simple = true;
    for (int i = 0; i < current_entry->length - 1; i++) {
        // Copy over old values
        element *e = &current_entry->values[i + 1];

        if (ENTRY == e->type) {
            is_simple = false;
        }

        new_values[i] = *e;
    }

    // Free associated memory and update entry members
    my_free(current_entry->values);
    current_entry->is_simple = is_simple;
    current_entry->values = new_values;
    current_entry->length--;
}

// Parse the pop command, pop the index if the entry exists and the index is in range
void command_pop(const char *line, main_state *current_state){
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "POP");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the entry that corresponds to the key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    } else if (current_entry->length <= 0) {
        printf("nil\n\n");
    } else {
        entry_pop(current_entry, current_state);
    }
}

// Return a pointer to the snapshot with the corresponding id
snapshot* get_snapshot(int id, main_state *current_state) {
    snapshot *head = current_state->snapshots;
    bool found = false;

    // Iterate through the linked list of snapshots
    // Return the snapshot pointer if it has the corresponding id
    while(NULL != head) {
        if (id == head->id) {
            found = true;
            break;
        }
        head = head->next;
    }

    if (found) {
        return head;
    } else {
        return NULL;
    }
}

// Deletes the snapshot corresponding to the id
void command_drop(const char *line, main_state *current_state) {
    // Get the drop id from the input line
    int drop_id = get_id(line, "DROP");

    // Check that the id is valid
    if (drop_id <= 0) {
        printf("no such snapshot\n\n");
        return;
    }

    // Check that the snapshot exists
    snapshot *current_snapshot = get_snapshot(drop_id, current_state);
    if (NULL == current_snapshot) {
        printf("no such snapshot\n\n");
        return;
    }

    // Update the linked list
    if ((NULL == current_snapshot->prev) && (NULL == current_snapshot->next)) {
        // Head and only entry
        current_state->snapshots = NULL;
    } else if ((NULL == current_snapshot->prev) &&
                    (NULL != current_snapshot->next)) {
        // Head and there are other entries
        snapshot *new_head = current_snapshot->next;
        new_head->prev = NULL;
        current_state->snapshots = new_head;
    } else if ((NULL != current_snapshot->prev) &&
                    (NULL == current_snapshot->next)) {
        // Tail and there are other entries
        (current_snapshot->prev)->next = NULL;
    } else if ((NULL != current_snapshot->prev) &&
                    (NULL != current_snapshot->next)) {
        // In the middle of other entries
        // Update pointers such that they skip over the deleted snapshot
        (current_snapshot->next)->prev = current_snapshot->prev;
        (current_snapshot->prev)->next = current_snapshot->next;
    } else {
        printf("Error in state\n");
        return;
    }

    // Free memory and update associated members
    current_snapshot->prev = NULL;
    current_snapshot->next = NULL;

    free_entries(current_snapshot->entries);
    my_free(current_snapshot);
    current_state->num_snapshots--;

    printf("ok\n\n");
}

// Iterate through the array of elements, find the i'th ENTRY value
element* get_value_general(element *values, int size, int num_key) {
    element *e = NULL;
    bool found = false;
    int current_key = 0;

    // Iterate through the array of elements
    for(int i = 0; i < size; i++) {
        e = &values[i];
        if ((NULL != e) && (ENTRY == e->type)) {
            if (current_key == num_key) {
                found = true;
                break;
            } else {
                current_key++;
            }
        }
    }

    // Return a pointer to the element if found
    if (found) {
        return e;
    } else {
        return NULL;
    }
}

// Deepcopy the array of elements from source to destination
void deepcopy_values(element *values, int num_values, element *dest,
                        bool is_simple) {
    if (is_simple) {
        // Copy each of the elements by value
        for (int i = 0; i < num_values; i++) {
            element *element_ptr = (element *) my_calloc(1, sizeof(element));
            element_ptr->type = INTEGER;
            element_ptr->value = (values[i]).value;
            dest[i] = *element_ptr;
            my_free(element_ptr);
        }
    } else {
        // Iterate through the array of elements
        for (int i = 0; i < num_values; i++) {
            element *element_ptr = (element *) my_calloc(1, sizeof(element));
            element_ptr->type = (values[i]).type;

            if (INTEGER == (values[i]).type) {
                // Copy the element's value
                element_ptr->value = (values[i]).value;
            } else {
                // Create a placeholder entry with a copied key
                // The references will be calculated later
                entry *tmp_entry_ptr = (entry *) my_calloc(1, sizeof(entry));
                strcpy(tmp_entry_ptr->key, (values[i]).entry->key);
                tmp_entry_ptr->is_simple = false;
                element_ptr->entry = tmp_entry_ptr;
            }

            // Update the destination array
            dest[i] = *element_ptr;
            my_free(element_ptr);
        }
    }
}

// Deepcopy struct members from source into entry e
void deepcopy_entry(entry *src_head, entry *e) {
    // Copy the key
    strcpy(e->key, src_head->key);

    // Copy the values and associated members
    e->values = (element *) my_calloc(src_head->length, sizeof(element));
    deepcopy_values(src_head->values, src_head->length, e->values, src_head->is_simple);
    e->length = src_head->length;
    e->is_simple = src_head->is_simple;

    // Reset reference values (to be re-calculated later)
    e->forward_size = 0;
    e->forward = NULL;

    e->backward_size = 0;
    e->backward = NULL;
}

// Establishes a bi-directional link between the current and matching entry
void calculate_references(entry *current_entry, entry *matching_entry) {
    // Set current_entry forward pointers
    entry **tmp_forward = NULL;

    if (NULL == current_entry->forward) {
        // Initialise memory and store the forward pointer
        tmp_forward = (entry **) my_calloc(1, sizeof(entry *));
        tmp_forward[0] = matching_entry;
        current_entry->forward_size = 1;
    } else {
        // Else re-alloc and append the forward pointer
        int new_size = (current_entry->forward_size + 1) * sizeof(entry *);
        tmp_forward = (entry **) realloc(current_entry->forward, new_size);
        tmp_forward[current_entry->forward_size] = matching_entry;
        current_entry->forward_size++;
    }

    current_entry->forward = tmp_forward;

    // Set matching entry backward pointers
    entry **tmp_backward = NULL;

    if (NULL == matching_entry->backward) {
        // Initialise memory and store the backward pointer
        tmp_backward = (entry **) my_calloc(1, sizeof(entry *));
        tmp_backward[0] = current_entry;
        matching_entry->backward_size = 1;
    } else {
        // Else re-alloc and append the backward pointer
        int new_size = (matching_entry->backward_size + 1) * sizeof(entry *);
        tmp_backward = (entry **) realloc(matching_entry->backward, new_size);
        tmp_backward[matching_entry->backward_size] = current_entry;
        matching_entry->backward_size++;
    }

    matching_entry->backward = tmp_backward;
}

// Get the corresponding entry given a key
entry* deepcopy_get_entry(entry *entries, char *key) {
    bool found = false;
    entry *head = entries;

    // Search the linked list
    while (NULL != head) {
        if (0 == strcmp(head->key, key)) {
            found = true;
            break;
        }

        head = head->prev;
    }

    if (found) {
        return head;
    } else {
        return NULL;
    }
}

// Deepcopies general keys by re-calculating bi-directional references between entries
void deepcopy_references(entry *entries) {
    // Traverse the linked list of entries
    entry *head = entries;
    while (NULL != head) {
        if (!head->is_simple) {
            // Iterate through the array of elements
            for (int i = 0; i < head->length; i++) {
                // Get the current element
                element *elem = &head->values[i];
                if ((NULL != elem) && (ENTRY == elem->type)) {
                    // Find the corresponding entry to the ENTRY element
                    entry *matching_entry= deepcopy_get_entry(entries,
                                                                elem->entry->key);

                    // Free the element and replace it with the proper entry
                    my_free(elem->entry);
                    elem->entry = matching_entry;

                    // Establish bi-directional linkages
                    calculate_references(head, matching_entry);
                }
            }
        }
        head = head->prev;
    }
}

// Deep copies the entries from source to destination
void deepcopy_entries(entry *entries, entry *dest) {
    entry *src_head = entries;

    if (NULL == src_head) {
        return;
    }

    // Iterate through the linked list of entries
    bool is_head = true;
    while(NULL != src_head) {

        if (is_head) {
            // Deepcopy the head
            deepcopy_entry(src_head, dest);
            is_head = false;
        } else {
            // Deepcopy the other entries
            entry *e = (entry *) my_calloc(1, sizeof(entry));
            deepcopy_entry(src_head, e);

            // Update linked list pointers
            dest->next = e;
            e->prev = dest;
            dest = dest->next;
        }
        src_head = src_head->next;
    }

    // Establish bi-directional references between the general entries
    if (NULL != dest) {
        deepcopy_references(dest);
    }
}

// Replaces the current state with a deepcopy of the corresponding snapshot
void command_checkout(const char *line, main_state *current_state) {
    int checkout_id = get_id(line, "CHECKOUT");

    // Invalid checkout id
    if (checkout_id <= 0) {
        printf("no such snapshot\n\n");
        return;
    }

    // Find the snapshot corresponding to the id
    snapshot *current_snapshot = get_snapshot(checkout_id, current_state);

    if (NULL != current_snapshot) {
        // Reset current state entries
        if (NULL != current_state->current_entries) {
            free_entries(current_state->current_entries);
            current_state->current_entries = NULL;
        }

        // Allocate new memory for the new entries
        current_state->current_entries = (entry *) my_calloc(1, sizeof(entry));

        // Deepcopy the snapshot entries into the current state
        if (0 != current_snapshot->num_entries) {
            deepcopy_entries(current_snapshot->entries,
                                current_state->current_entries);
        }

        current_state->num_current_entries = current_snapshot->num_entries;

        // Free the pointer if there were no entries allocated
        if (0 == current_state->num_current_entries) {
            my_free(current_state->current_entries);
            current_state->current_entries = NULL;
        }

        printf("ok\n\n");
    } else {
        printf("no such snapshot\n\n");
    }
}

// Check that an entry exists within a given cache
int check_cache(entry *current_entry, entry **cache, int cache_size) {
    bool found = false;
    int i = 0;
    for (; i < cache_size; i++) {
        if (0 == strcmp(current_entry->key, cache[i]->key)) {
            found = true;
            break;
        }
    }

    if (found) {
        return i;
    } else {
        return -1;
    }
}

// Store the entries corresponding to the backwards references of a given entry into a cache
void get_bwd_references(entry *current_entry, entry **cache, int *cache_size) {
    // Base Case
    if (NULL == current_entry->backward) {
        return;
    }

    // Recursive Case
    for (int i = 0; i < current_entry->backward_size; i++) {
        entry *e = current_entry->backward[i];

        // Store the current entry's backward references into the cache
        // Assuming it isn't already in the cache. Recurse on the backward references
        if (-1 == check_cache(e, cache, *cache_size)) {
            cache[*cache_size] = e;
            (*cache_size)++;
            get_bwd_references(e, cache, cache_size);
        }
    }
}

// Store the entries corresponding to the forward references of a given entry into a cache
void get_fwd_references(entry *current_entry, entry **cache, int *cache_size) {
    // Base Case
    if (current_entry->is_simple) {
        return;
    }

    // Recursive Case
    for (int i = 0; i < current_entry->forward_size; i++) {
        entry *e = current_entry->forward[i];

        // Store the current entry's forward references into the cache
        // If it isn't already in the cache, recurse on the forward references
        if (-1 == check_cache(e, cache, *cache_size)) {
            cache[*cache_size] = e;
            (*cache_size)++;
            get_fwd_references(e, cache, cache_size);
        }
    }
}

// Prints all the keys contained within an array of entry pointers
void print_unique_references(entry **references, int size) {
    for(int i = 0; i < size-1; i++) {
        printf("%s, ", references[i]->key);
    }
    printf("%s\n\n", references[size-1]->key);
}

// Comparator function that compares the keys between two entries
int cmp_keys(const void *a, const void *b) {
    entry *entry_a = *(entry **) a;
    entry *entry_b = *(entry **) b;
    return strcmp(entry_a->key, entry_b->key);
}

// Prints all the unique forward references of the current entry,
// Sort in lexographical order
void command_forward(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "FORWARD");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    } else if (0 == current_entry->forward_size) {
       printf("nil\n\n");
       return;
    }

    // Allocate a cache to store the references
    entry **cache = (entry **) my_calloc(current_state->num_current_entries,
                                            sizeof(entry *));

    int *cache_size_ptr;
    int cache_size = 0;
    cache_size_ptr = &cache_size;

    // Get the forward references and add them to the cache
    get_fwd_references(current_entry, cache, cache_size_ptr);

    // Sort the references lexographically
    qsort(cache, *cache_size_ptr, sizeof(entry *), cmp_keys);

    // Print the references
    print_unique_references(cache, *cache_size_ptr);

    my_free(cache);
}

// Prints all the unique backward references of the current entry
// Sort in lexographical order
void command_backward(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "BACKWARD");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    } else if (0 == current_entry->backward_size) {
        printf("nil\n\n");
        return;
    }

    // Allocate a cache to store the references
    entry **cache = (entry **) my_calloc(current_state->num_current_entries,
                        sizeof(entry *));

    int *cache_size_ptr;
    int cache_size = 0;
    cache_size_ptr = &cache_size;

    // Get the backward references and add them to the cache
    get_bwd_references(current_entry, cache, cache_size_ptr);

    // Sort the references lexographically
    qsort(cache, *cache_size_ptr, sizeof(entry *), cmp_keys);

    // Print the references
    print_unique_references(cache, *cache_size_ptr);

    my_free(cache);
}

// Print whether an entry is general or simple
void command_type(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";

    int key_size = get_key(line, key, "TYPE");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    // Print the entry type
    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    } else {
        if (current_entry->is_simple) {
            printf("simple\n\n");
        } else {
            printf("general\n\n");
        }
    }
}

// List all the keys in the current state
void command_list_keys(main_state *current_state) {
    // Get the number of keys in the current state
    int num_keys = current_state->num_current_entries;

    if (0 == num_keys) {
        printf("no keys\n\n");
    } else {
        // If there are entries, iterate through the linked list of entries
        // Print they keys
        entry *head = current_state->current_entries;
        while(NULL != head) {
            printf("%s\n", head->key);
            head = head->next;
        }
        printf("\n");
    }
}

// Deletes all existing snapshots with id greater than the rollback id
// Load the corresponding snapshot into the current state
void command_rollback(const char *line, main_state *current_state) {
    // Get the rollback id
    int rollback_id = get_id(line, "ROLLBACK");

    // Check that the number is valid
    if (rollback_id <= 0) {
        printf("no such snapshot\n\n");
        return;
    }

    // Check that the corresponding snapshot exists
    if (NULL == get_snapshot(rollback_id, current_state)) {
        printf("no such snapshot\n\n");
        return;
    }

    snapshot *head = current_state->snapshots;

    // Iterate through the linked list of snapshots
    // Remove them until you find the snapshot with the corresponding id
    bool found = false;
    while(NULL != head) {
        if (rollback_id == head->id) {
            found = true;
            break;
        } else {
            snapshot *tmp = head;
            head = head->next;

            // Remove from the linked list and free associated memory
            tmp->prev = NULL;
            tmp->next = NULL;
            free_entries(tmp->entries);
            my_free(tmp);
            current_state->num_snapshots--;
        }
    }

    if (found) {
        // Update prev pointer (as the current snapshot is now the head)
        head->prev = NULL;

        // Update snapshots head
        current_state->snapshots = head;

        // Load the snapshot into current state
        char line[MAX_LINE] = "CHECKOUT ";
        char id[MAX_KEY];
        sprintf(id, "%d", rollback_id);

        strcat(line, id);
        command_checkout(line, current_state);
    } else {
        printf("no such snapshot\n\n");
    }
}

// Lists all the entries that exist in the current state
void command_list_entries(main_state *current_state) {
    // Get the number of entries
    int num_entries = current_state->num_current_entries;
    if (0 == num_entries) {
        printf("no entries\n\n");
    } else {
        // Iterate through the linked list of entries and print the key, value pairs
        entry *head = current_state->current_entries;
        while(NULL != head) {
            printf("%s ", head->key);
            print_values(head);

            head = head->next;
        }
        printf("\n");
    }
}

// Lists all the currently active snapshots
void command_list_snapshots(main_state *current_state) {
    // Get the number of snapshots
    int num_snapshots = current_state->num_snapshots;
    if (0 == num_snapshots) {
        printf("no snapshots\n\n");
    } else {
        // Iterate through the linked list of snapshots and print the id's
        snapshot *head = current_state->snapshots;
        while(NULL != head) {
            printf("%d\n", head->id);
            head = head->next;
        }
        printf("\n");
    }
}

// Deepcopy the current state into a snapshot
void command_snapshot(main_state *current_state) {
    // Create new snapshot and initialise struct members
    snapshot *new_snapshot = (snapshot *) my_calloc(1, sizeof(snapshot));

    new_snapshot->id = current_state->next_snapshot_id;
    new_snapshot->entries = (entry *) my_calloc(1, sizeof(entry));

    // Deepcopy the entries from the current state to the snapshot
    if (0 != current_state->num_current_entries) {
        deepcopy_entries(current_state->current_entries, new_snapshot->entries);
    }
    new_snapshot->num_entries = current_state->num_current_entries;

    // Add the new snapshot to the linked list of snapshots
    if (0 == current_state->num_snapshots) {
        // Make the snapshot the head
        new_snapshot->prev = NULL;
        new_snapshot->next = NULL;

        current_state->snapshots = new_snapshot;
    } else {
        // Add the snapshot as the new head
        snapshot *head = current_state->snapshots;
        head->prev = new_snapshot;
        new_snapshot->next = head;
        current_state->snapshots = new_snapshot;
    }

    printf("saved as snapshot %d\n\n", current_state->next_snapshot_id);

    // Update the main state
    current_state->num_snapshots++;
    current_state->next_snapshot_id++;
}

// Get the values corresponding to the input key
void command_get(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "GET");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
    } else {
        print_values(current_entry);
        printf("\n");
    }
}

// Updates the references following a push/append of a ENTRY element(s)
int update_references(element *tmp_values, int num_values, entry *current_entry) {

    // Iterate through the list of values
    for (int i = 0; i < num_values; i++) {
        // Get the next ENTRY element
        element *general_value = get_value_general(tmp_values, num_values, i);

        if (NULL == general_value) {
            return -1;
        }
        // Establish bi-directional linkages
        calculate_references(current_entry, general_value->entry);
    }

    return 0;
}

// Append a list of values to the end of a given entry's values
void command_append(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "APPEND");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    }

    // Get the number of values to be appended
    int num_append_values = get_num_values(line, key_size, "APPEND");

    // Initialise a temporary array to store the new values
    element *tmp_values = (element *) my_calloc(num_append_values,
                                                    sizeof(element));

    // Get the number of ENTRY elements that are being added
    int num_general_keys = get_values(line, key_size, tmp_values,
                                        num_append_values, "APPEND",
                                        current_state);

    if (-1 == num_general_keys) {
        my_free(tmp_values);
        return;
    }

    // If the pushed values are ENTRY, but the current entry is simple,
    // then change it to general
    if ((num_general_keys > 0) && (true == current_entry->is_simple)) {
        current_entry->is_simple = false;
    }

    // Re-allocate the memory, implicitly copying the old values to the start
    int new_length = (current_entry->length + num_append_values);
    element *all_values = (element *) realloc(current_entry->values,
                                                new_length * sizeof(element));

    // Copy the new values to the end
    for (int i = current_entry->length; i < new_length; i++) {
        all_values[i] = tmp_values[i - current_entry->length];
    }

    // Update forward and backward pointers
    update_references(tmp_values, num_append_values, current_entry);

    // Free the temporary array of values
    my_free(tmp_values);

    // Update entry struct members
    current_entry->values = all_values;
    current_entry->length += num_append_values;

    printf("ok\n\n");
}

// Push a list of values to the start of a given entry's values
void command_push(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "PUSH");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    }

    // Get the number of values to be pushed
    int num_push_values = get_num_values(line, key_size, "PUSH");

    // Initialise a temporary array to store the new values
    element *tmp_values = (element *) my_calloc(num_push_values, sizeof(element));

    // Get the number of ENTRY elements that are being added
    int num_general_keys = get_values(line, key_size, tmp_values, num_push_values,
                                        "PUSH", current_state);

    if (-1 == num_general_keys) {
        my_free(tmp_values);
        return;
    }
    // If the pushed values are ENTRY, but the current entry is simple,
    // then change it to general
    if ((num_general_keys > 0) && (true == current_entry->is_simple)) {
        current_entry->is_simple = false;
    }

    // Allocate new memory size to store enough for the new values and old values
    int new_length = current_entry->length + num_push_values;
    element *all_values = (element *) my_calloc(new_length, sizeof(element));

    // Push the new values to the start
    int j = 0;
    for (int i = num_push_values - 1; i >= 0; i--) {
        all_values[j] = tmp_values[i];
        j++;
    }

    // Copy the old values
    for (int i = num_push_values; i < new_length; i++) {
        all_values[i] = (current_entry->values)[i - num_push_values];
    }

    // Update forward and backward pointers
    update_references(tmp_values, num_push_values, current_entry);

    // Free the old pointer
    my_free(current_entry->values);
    my_free(tmp_values); // is this bad to do?

    // Update entry struct members
    current_entry->values = all_values;
    current_entry->length += num_push_values;

    printf("ok\n\n");
}

void update_bi_directional_references(entry *e) {
    if (!e->is_simple) {
        // Remove forward pointers and free the memory
        if (NULL != e->forward) {
            for (int i = 0; i < e->forward_size; i++) {
                remove_from_backward(e, e->forward[i]);
            }

            my_free(e->forward);
        }
    }

    if (NULL != e->backward) {
        // Remove the entry from other entry's backward pointers, free the memory
        for (int i = 0; i < e->backward_size; i++) {
            remove_from_forward(e, e->backward[i]);
        }

        my_free(e->backward);
    }
}

// Deletes the entry from the current state
void entry_delete(entry *e, main_state *current_state) {

    if ((NULL == e->prev) && (NULL == e->next)) {
        // Head and only entry
        current_state->current_entries = NULL; // is this okay?
    } else if ((NULL == e->prev) && (NULL != e->next)) {
        // Head and there are other entries
        entry *new_head = e->next;
        new_head->prev = NULL;
        current_state->current_entries = new_head;
    } else if ((NULL != e->prev) && (NULL == e->next)) {
        // Tail and there are other entries
        (e->prev)->next = NULL;
    } else if ((NULL != e->prev) && (NULL != e->next)) {
        // In the middle of other entries
        // Update pointers such that they skip over the deleted entry
        (e->next)->prev = e->prev;
        (e->prev)->next = e->next;
    } else {
        return;
    }

    // Update bi-directional links
    update_bi_directional_references(e);

    // Reset values
    memset(e->key, 0, MAX_KEY);
    e->length = 0;

    // Free memory
    my_free(e->values);
    my_free(e);

    // Update number of current elements
    current_state->num_current_entries--;
}

// Returns true if the to be deleted entry has no backwards references
bool is_valid_del(entry *current_entry, main_state *current_state) {
    bool is_null = (NULL != current_entry);
    bool has_no_backwards_ptrs = (0 != current_entry->backward_size);

    if (is_null && has_no_backwards_ptrs) {
        return false;
    } else {
        return true;
    }
}

// Deletes the entry from the current state
void command_del(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "DEL");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry * current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
    } else if (!is_valid_del(current_entry, current_state)) {
        printf("not permitted\n\n");
    } else {
        entry_delete(current_entry, current_state);
        printf("ok\n\n");
    }
}

// Gets the entry with the corresponding key from the current snapshot
entry* snapshot_get_entry(const char *key, snapshot *current_snapshot) {
    bool found = false;
    entry *head = current_snapshot->entries;

    // Search for the entry within the snapshot's list of entries
    while(NULL != head) {
        if (0 == strcmp(key, head->key)) {
            found = true;
            break;
        }
        head = head->next;
    }

    if (!found || NULL == head) {
        return NULL;
    } else {
        return head;
    }
}

// Deletes the entry in the current snapsnot
void snapshot_entry_delete(entry *e, snapshot *current_snapshot) {
    if ((NULL == e->prev) && (NULL == e->next)) {
        // Head and only entry
        current_snapshot->entries = NULL;
    } else if ((NULL == e->prev) && (NULL != e->next)) {
        // Head and there are other entries
        entry *new_head = e->next;
        new_head->prev = NULL;
        current_snapshot->entries = new_head;
    } else if ((NULL != e->prev) && (NULL == e->next)) {
        // Tail and there are other entries
        (e->prev)->next = NULL;
    } else if ((NULL != e->prev) && (NULL != e->next)) {
        // In the middle of other entries
        // Update pointers such that they skip over the deleted entry
        (e->next)->prev = e->prev;
        (e->prev)->next = e->next;
    } else {
        printf("Error in state\n");
        return;
    }

    // Update bi-directional links
    update_bi_directional_references(e);

    // Reset values
    memset(e->key, 0, MAX_KEY);
    e->length = 0;
    e->next = NULL;
    e->prev = NULL;

    // Free memory
    my_free(e->values);
    my_free(e);

    // Update number of current elements in current snapshot
    current_snapshot->num_entries--;
}

// Checks that if the entry exists in the current state or any snapshot
// then it does not have any back references
bool is_valid_purge(const char *key, main_state *current_state) {
    // Check the current state
    entry *current_entry = get_entry(key, current_state);

    // Check no backward references
    if ((NULL != current_entry) && (0 != current_entry->backward_size)) {
        return false;
    }

    // Check the snapshots
    snapshot *head = current_state->snapshots;
    while (NULL != head) {
        entry *e = snapshot_get_entry(key, head);
        // Check no backward references
        if ((NULL != e) && (0 != e->backward_size)) {
            return false;
        }
        head = head->next;
    }

    return true;
}

// Deletes entry (if it exists) from all snapshots and the current state
void command_purge(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "PURGE");

    if (!check_key(key, key_size)) {
        return;
    } else if (!is_valid_purge(key, current_state)) {
        printf("not permitted\n\n");
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    // Remove the entry from the current state
    if (NULL != current_entry) {
        entry_delete(current_entry, current_state);
    }

    // Iterate through the linked list of snapshots
    snapshot *head = current_state->snapshots;
    while (NULL != head) {
        entry *e = snapshot_get_entry(key, head);
        if (NULL != e) {
            // Remove the entry from the current snapshot
            snapshot_entry_delete(e, head);
        }
        head = head->next;
    }

    printf("ok\n\n");
}

// Returns false if the user tries to add a key that does not exist or create a self-reference
bool check_general_keys(element *new_values, int num_values,
                            int num_general_keys, entry *entry_ptr) {
    // Iterate through the array of elements
    for (int i = 0; i < num_general_keys; i++) {
        // Get the next value that points to an entry
        element *general_value = get_value_general(new_values, num_values, i);
        if (NULL == general_value) {
            printf("no such key\n\n");
            return false;
        } else if (0 == strcmp(entry_ptr->key, general_value->entry->key)) {
            printf("not permitted\n\n");
            return false;
        }
    }
    return true;
}

// Sets the forward and backward references of an entry
void set_references(entry *entry_ptr, int num_general_keys,
                        element *new_values, int num_values) {

    // Iterate through the array of elements and get each ENTRY element
    for (int i = 0; i < num_general_keys; i++) {
        element *general_value = get_value_general(new_values, num_values, i);

        // Establish bi-directional linkages
        calculate_references(entry_ptr, general_value->entry);
    }
}

// Updates the existing entry with a new set of values
void entry_update(const char *line, main_state *current_state, entry *entry_ptr) {
    // Get the number of values
    int num_values = get_num_values(line, strlen(entry_ptr->key), "SET");

    // Initialise an array to store the new values
    element *new_values = (element *) my_calloc(num_values, sizeof(element));

    // Get the number of ENTRY elements
    int num_general_keys = get_values(line, strlen(entry_ptr->key), new_values,
                                        num_values, "SET", current_state);

    // Check that no self-reference or reference to non-existing key was made
    bool is_valid = check_general_keys(new_values, num_values,
                                        num_general_keys, entry_ptr);

    if ((-1 == num_general_keys) || !is_valid) {
        my_free(new_values);
        return;
    }

    // Free previous value pointer
    my_free(entry_ptr->values);

    // Remove the bi-directional relationship
    if (NULL != entry_ptr->forward) {
        // Remove current entry from other entry's backwards
        for (int i = 0; i < entry_ptr->forward_size; i++) {
            remove_from_backward(entry_ptr, entry_ptr->forward[i]);
        }

        // Free forward pointer
        my_free(entry_ptr->forward);

        entry_ptr->forward_size = 0;
        entry_ptr->forward = NULL;
    }

    entry_ptr->values = NULL;

    // Set the length of the entry
    entry_ptr->length = num_values;

    if (-1 == num_general_keys) {
        return;
    } else if (0 == num_general_keys) {
        entry_ptr->is_simple = true;
    } else if (num_general_keys > 0) {
        entry_ptr->is_simple = false;
        set_references(entry_ptr, num_general_keys, new_values, num_values);
    }

    entry_ptr->values = new_values;

    printf("ok\n\n");
}

// Iterate through the cache, get all the values and store the maximum
int get_max(entry **cache, int cache_size) {
    // Iterate through all the entries
    int current_max = INT_MIN;
    for (int i = 0; i < cache_size; i++) {
        entry *e = cache[i];
        // Iterate through all the values of that entry
        for (int j = 0; j < e->length; j++) {
            // Get the current elements value
            // Check if it is greater than the current max
            element v = e->values[j];
            if ((INTEGER == v.type) && (current_max < v.value)) {
                current_max = v.value;
            }
        }
    }
    return current_max;
}

// Iterate through the cache, get all the values and store the minimum
int get_min(entry **cache, int cache_size) {
    // Iterate through all the entries
    int current_min = INT_MAX;
    for (int i = 0; i < cache_size; i++) {
        entry *e = cache[i];
        // Iterate through all the values of that entry
        for (int j = 0; j < e->length; j++) {
            // Get the current elements value
            // Check if it is smaller than the current min
            element v = e->values[j];
            if ((INTEGER == v.type) && (current_min > v.value)) {
                current_min = v.value;
            }
        }
    }
    return current_min;
}

// Iterate through the cache, get the total number of INTEGER values
int get_len(entry **cache, int cache_size) {
    // Iterate through all the entries
    int len = 0;
    for (int i = 0; i < cache_size; i++) {
        entry *e = cache[i];
        // Increment the length each time an INTEGER element is encountered
        for (int j = 0; j < e->length; j++) {
            if (INTEGER == e->values[j].type) {
                len++;
            }
        }
    }
    return len;
}

// Iterate through the cache, get the total sum of INTEGER values
int get_sum(entry **cache, int cache_size) {
    // Iterate through all the entries
    int sum = 0;
    for (int i = 0; i < cache_size; i++) {
        entry *e = cache[i];
        // Iterate through all the values of that entry
        for (int j = 0; j < e->length; j++) {
            element v = e->values[j];
            // Increment the sum each time an INTEGER element is encountered
            if (INTEGER == v.type) {
                sum += v.value;
            }
        }
    }
    return sum;
}

// Check that an entry and its associated keys already exists within the cache
int check_cache_entries(entry *current_entry, entry ***cache, int cache_size) {
    bool found = false;
    int i = 0;
    for (; i < cache_size; i++) {
        if (0 == strcmp(current_entry->key, cache[i][0]->key)) {
            found = true;
            break;
        }
    }

    if (found) {
        return i;
    } else {
        return -1;
    }
}

// Store the entries in the forward references of a given entry into a cache
entry** get_entries(entry *current_entry, entry **result, int *result_size_ptr,
                        entry ***cache, int *cache_size_ptr, int *lengths) {
    // Base Case
    if (current_entry->is_simple) {
        return result;
    }

    // Recursive Case
    for (int i = 0; i < current_entry->forward_size; i++) {
        entry *e = current_entry->forward[i];

        // Get the index of the entry if it exists in the cache
        int cache_idx = check_cache_entries(e, cache, *cache_size_ptr);
        if (cache_idx >= 0) {
            // Copy cached memory
            // This copies all the other entries associated with the current entry
            int len = lengths[cache_idx];
            result = (entry **) realloc(result, (*result_size_ptr + len) * sizeof(entry *));
            memcpy(result + *result_size_ptr, cache[cache_idx], len * sizeof(entry *));
            (*result_size_ptr) += len;
        } else {
            // Store the current entry's forward references into the cache
            // Recurse on the forward references
            result = (entry **) realloc(result, (*result_size_ptr + 1) * sizeof(entry *));

            int prev = *result_size_ptr;
            result[*result_size_ptr] = e;
            (*result_size_ptr)++;
            result = get_entries(e, result, result_size_ptr, cache,
                                    cache_size_ptr, lengths);

            // Update the cache with new keys
            entry **tmp_cache = (entry **) my_calloc(*result_size_ptr - prev,
                                                        sizeof(entry *));
            // Each entry corresponds to all the keys associated with a key
            // eg. if a[1], b[a], c[a,b], d[a,b,c]
            // then the entry starting with d will contain [d, a, b, a, c, a, b, a]
            // the entry starting with c will contain [c, a, b, a]
            memcpy(tmp_cache, result + prev, (*result_size_ptr - prev) * sizeof(entry *));

            // Update the cache
            cache[*cache_size_ptr] = tmp_cache;
            lengths[*cache_size_ptr] = *result_size_ptr - prev;
            (*cache_size_ptr)++;
        }
    }
    return result;
}

// Calculates the minimum value associated with an entry
void command_min(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "MIN");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    }

    // Allocate a cache to store the references
    entry **cache = (entry **) my_calloc(current_state->num_current_entries,
                                            sizeof(entry *));

    int *cache_size_ptr;
    int cache_size = 1;
    cache_size_ptr = &cache_size;

    cache[0] = current_entry;

    // Get the forward references and add them to the cache
    get_fwd_references(current_entry, cache, cache_size_ptr);

    // Iterate through the cache and find the minmum value
    int min = get_min(cache, *cache_size_ptr);

    // Free memory
    my_free(cache);

    // Print the result
    printf("%d\n\n", min);
}

// Calculates the maximum value associated with an entry
void command_max(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "MAX");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    }

    // Allocate a cache to store the references
    entry **cache = (entry **) my_calloc(current_state->num_current_entries,
                                            sizeof(entry *));

    int *cache_size_ptr;
    int cache_size = 1;
    cache_size_ptr = &cache_size;

    cache[0] = current_entry;

    // Get the forward references and add them to the cache
    get_fwd_references(current_entry, cache, cache_size_ptr);

    // Iterate through the cache and find the maximum value
    int max = get_max(cache, *cache_size_ptr);

    // Free memory
    my_free(cache);

    // Print the result
    printf("%d\n\n", max);
}

// Calculates the maximum value associated with an entry
void command_sum(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "SUM");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    }

    // Initialise a cache to store the forward references
    entry **tmp_result = (entry **) my_calloc(2*current_state->num_current_entries,
                                            sizeof(entry *));

    // Store the result size
    int *result_size_ptr;
    int result_size = 1;
    result_size_ptr = &result_size;

    tmp_result[0] = current_entry;

    // Initialise the cache and associated variables
    entry ***cache = (entry ***) my_calloc(2*current_state->num_current_entries,
                                            sizeof(entry **));

    int *cache_size_ptr;
    int cache_size = 0;
    cache_size_ptr = &cache_size;

    // Stores the length of each cache index
    int *lengths = (int *) my_calloc(2*current_state->num_current_entries,
                                            sizeof(int));

    // Recursively get all the forward references of the current entry
    entry **result = get_entries(current_entry, tmp_result, result_size_ptr,
                                    cache, cache_size_ptr, lengths);

    // Iterate through the cache and calculate the sum of all the INTEGER values
    int sum = get_sum(result, *result_size_ptr);

    // Free memory
    my_free(result);
    my_free(lengths);
    for (int i = 0; i < *cache_size_ptr; i++) {
        my_free(cache[i]);
    }
    my_free(cache);

    // Print the result
    printf("%d\n\n", sum);
}

// Calculates the maximum value associated with an entry
void command_len(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "LEN");

    if (!check_key(key, key_size)) {
        return;
    }

    // Get the corresponding entry with the given key
    entry *current_entry = get_entry(key, current_state);

    if (NULL == current_entry) {
        printf("no such key\n\n");
        return;
    }

    // Initialise a cache to store the forward references
    entry **tmp_result = (entry **) my_calloc(2*current_state->num_current_entries,
                                            sizeof(entry *));

    // Store the result size
    int *result_size_ptr;
    int result_size = 1;
    result_size_ptr = &result_size;

    tmp_result[0] = current_entry;

    // Initialise the cache and associated variables
    entry ***cache = (entry ***) my_calloc(2*current_state->num_current_entries,
                                            sizeof(entry **));

    int *cache_size_ptr;
    int cache_size = 0;
    cache_size_ptr = &cache_size;

    // Stores the length of each cache index
    int *lengths = (int *) my_calloc(2*current_state->num_current_entries,
                                            sizeof(int));

    // Recursively get all the forward references of the current entry
    entry **result = get_entries(current_entry, tmp_result, result_size_ptr,
                                    cache, cache_size_ptr, lengths);

    // Iterate through the cache and calculate the length of all entries
    int len = get_len(result, *result_size_ptr);

    // Free memory
    my_free(result);
    my_free(lengths);
    for (int i = 0; i < *cache_size_ptr; i++) {
        my_free(cache[i]);
    }
    my_free(cache);

    // Print the result
    printf("%d\n\n", len);
}

// Sets the values of a given entry
void command_set(const char *line, main_state *current_state) {
    // Get the alphanumeric key and check that it is valid
    char key[MAX_KEY] = "";
    int key_size = get_key(line, key, "SET");

    if (!check_key(key, key_size)) {
        return;
    }
    // Get the corresponding entry with the given key (if it exists)
    entry *existing_entry = get_entry(key, current_state);

    if (NULL != existing_entry) {
        // Found existing entry, update values instead
        entry_update(line, current_state, existing_entry);
        return;
    }

    // Determine the number of values to be set
    int num_values = get_num_values(line, key_size, "SET");

    // Initialise an array that temporarily stores the elements to be added
    element *tmp_values = (element *) my_calloc(num_values, sizeof(element));

    // Get the number of ENTRY elements
    int num_general_keys = get_values(line, key_size, tmp_values, num_values,
                                        "SET", current_state);

    if (-1 == num_general_keys) {
        my_free(tmp_values);
        return;
    }

    // Initialise a pointer to the new entry
    entry *new_entry_ptr = (entry *) my_calloc(1, sizeof(entry));
    strcpy(new_entry_ptr->key, key);

    // Ensure that the ENTRY elements are valid
    // ie. no self-reference, points to an existing key
    if (!check_general_keys(tmp_values, num_values, num_general_keys,
                                new_entry_ptr)) {
        my_free(tmp_values);
        my_free(new_entry_ptr);
        return;
    }

    // Set the new entry's values
    new_entry_ptr->values = tmp_values;
    new_entry_ptr->length= num_values;

    // Set is_simple
    if (-1 == num_general_keys) {
        return;
    } else if (0 == num_general_keys) {
        new_entry_ptr->is_simple = true;
    } else {
        new_entry_ptr->is_simple = false;
    }

    // Null initialise references
    new_entry_ptr->forward_size = 0;
    new_entry_ptr->forward = NULL;

    new_entry_ptr->backward_size = 0;
    new_entry_ptr->backward = NULL;

    // Add the entry to current state's array of entries
    if (0 == current_state->num_current_entries) {
        // New entry is the head
        new_entry_ptr->prev = NULL;
        new_entry_ptr->next = NULL;

        current_state->current_entries = new_entry_ptr;
    } else {
        // Push the new entry to the head
        entry *head = current_state->current_entries;
        head->prev = new_entry_ptr;
        new_entry_ptr->next = head;
        new_entry_ptr->prev = NULL;

        // Update bi-directional references
        if (num_general_keys > 0) {
            set_references(new_entry_ptr, num_general_keys,
                                tmp_values, num_values);
        }

        // Set the new head
        current_state->current_entries = new_entry_ptr;
    }

    current_state->num_current_entries += 1;
    printf("ok\n\n");
}

// Prints bye and gracefully ends the program
void command_bye() {
    printf("bye\n");
}

// Prints the help string
void command_help() {
    printf("%s\n", HELP_STRING);
}

// Take in the input line and call the respective command
int process_command(const char *line, main_state *current_state) {
    // Commands are case-insensitive
    if (0 == strcasecmp(line, "BYE\n")) {
        command_bye();
        return 0;
    } else if (0 == strcasecmp(line, "HELP\n")) {
        command_help();
        return 1;
    } else if (0 == strcasecmp(line, "LIST KEYS\n")) {
        command_list_keys(current_state);
        return 1;
    } else if (0 == strcasecmp(line, "LIST ENTRIES\n")) {
        command_list_entries(current_state);
        return 1;
    } else if (0 == strcasecmp(line, "LIST SNAPSHOTS\n")) {
        command_list_snapshots(current_state);
        return 1;
    } else if (0 == strcasecmp(line, "SNAPSHOT\n")) {
        command_snapshot(current_state);
        return 1;
    } else if (0 == strncasecmp(line, "GET", 3)) {
        command_get(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "DEL", 3)) {
        command_del(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "PURGE", 5)) {
        command_purge(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "SET", 3)) {
        command_set(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "PUSH", 4)) {
        command_push(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "APPEND", 6)) {
        command_append(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "PICK", 4)) {
        command_pick(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "PLUCK", 5)) {
        command_pluck(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "POP", 3)) {
        command_pop(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "DROP", 4)) {
        command_drop(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "ROLLBACK", 8)) {
        command_rollback(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "CHECKOUT", 8)) {
        command_checkout(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "MIN", 3)) {
        command_min(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "MAX", 3)) {
        command_max(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "SUM", 3)) {
        command_sum(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "LEN", 3)) {
        command_len(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "REV", 3)) {
        command_rev(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "UNIQ", 4)) {
        command_uniq(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "SORT", 4)) {
        command_sort(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "FORWARD", 7)) {
        command_forward(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "BACKWARD", 8)) {
        command_backward(line, current_state);
        return 1;
    } else if (0 == strncasecmp(line, "TYPE", 4)) {
        command_type(line, current_state);
        return 1;
    } else {
        return -1;
    }
}

// Frees all memory associated with the current state and snapshots
void free_all(main_state *current_state) {
    // Free all the memory associated with the entries in the current state
    if (NULL != current_state->current_entries) {
        free_entries(current_state->current_entries);
    }

    // Iterate through all the snapshots withiin the linked list
    // Free all the memory associated with each entry
    snapshot *head = current_state->snapshots;
    while(NULL != head) {
        snapshot *tmp = head;

        head = head->next;

        if (NULL != tmp->entries) {
            free_entries(tmp->entries);
        }

        if (NULL != tmp) {
            tmp->prev = NULL;
            tmp->next = NULL;
            my_free(tmp);
        }
    }
}

// Main Function
int main(void) {

    char line[MAX_LINE];
    memset(line, 0, MAX_LINE);
    int cmd = 0;

    // Declare the main state
    main_state current_state;

    // Initialise struct members
    current_state.snapshots = NULL;
    current_state.num_snapshots = 0;
    current_state.next_snapshot_id = 1;
    current_state.current_entries = NULL;
    current_state.num_current_entries = 0;

    // Main Program Loop
    while (true) {

        printf("> ");

        if (NULL == fgets(line, MAX_LINE, stdin)) {
            printf("\n");
            command_bye();
            break;
        } else {
            cmd = process_command(line, &current_state);
            if (0 == cmd) {
                break;
            } else if (cmd == -1) {
                printf("Error: invalid command.\n\n");
                break;
            }
        }
    }

    // Clean up all memory
    free_all(&current_state);
    return 0;
}
