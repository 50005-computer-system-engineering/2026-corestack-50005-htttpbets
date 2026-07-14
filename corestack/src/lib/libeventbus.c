#include "libeventbus.h"
#include <stdlib.h>

// Allocate one slot per event type
void event_bus_init(EventBus *bus, int num_events) {
    bus->num_events = num_events;
    bus->slots      = calloc(num_events, sizeof(EventSlot)); // zero memory so all heads start NULL
}

// Walk every slot and free its listener list, then free the slots array.
void event_bus_free(EventBus *bus) {
    for (int i = 0; i < bus->num_events; i++) {
        ListenerNode *node = bus->slots[i].head;
        while (node) {
            ListenerNode *next = node->next;
            free(node);
            node = next;
        }
    }
    free(bus->slots);
    bus->slots      = NULL;
    bus->num_events = 0;
}

// Prepend a new listener node to the front of the slot's list.
void event_bus_listen(EventBus *bus, int event_type, EventListener listener) {
    ListenerNode *node = malloc(sizeof(ListenerNode));
    node->fn           = listener;
    node->next         = bus->slots[event_type].head;
    bus->slots[event_type].head = node;
}

// Walk the list with a pointer-to-pointer so we can unlink without a separate
// "previous" variable. Function pointers compare equal iff they point to the
// same function, so this correctly identifies the right node to remove.
void event_bus_stop_listening(EventBus *bus, int event_type, EventListener listener) {
    ListenerNode **curr = &bus->slots[event_type].head;
    while (*curr) {
        if ((*curr)->fn == listener) {
            ListenerNode *to_free = *curr;
            *curr = (*curr)->next;
            free(to_free);
            return;
        }
        curr = &(*curr)->next;
    }
}

// Call every listener registered for this event type in order.
void event_bus_trigger(EventBus *bus, int event_type, void *args) {
    ListenerNode *node = bus->slots[event_type].head;
    while (node) {
        node->fn(args);
        node = node->next;
    }
}