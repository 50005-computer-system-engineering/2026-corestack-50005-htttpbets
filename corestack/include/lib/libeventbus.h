#ifndef EVENT_BUS_H
#define EVENT_BUS_H

/**
 *  Event Bus System to allow events to be listened to/broadcast.
 * 
 *  Example Usage:
 *  // Initialise Event Bus
 *  EventBus bus;
 *  event_bus_init(&bus, EVENT_COUNT);
 *
 *  // Listen to events
 *  void on_player_move(void *args) {
 *      // Cast void* to the expected args struct
 *      PlayerMoveArgs *a = (PlayerMoveArgs *)args;
 *      printf("Player %d moved to (%d, %d)\n", a->player_id, a->x, a->y);
 *  }
 *  event_bus_listen(&bus, EVENT_PLAYER_MOVE, on_player_move);
 *
 *  // Trigger Event
 *  PlayerMoveArgs move = { .player_id = 1, .x = 3, .y = 5 };
 *  event_bus_trigger(&bus, EVENT_PLAYER_MOVE, &move);
 *
 *  // Stop listening to events
 *  event_bus_stop_listening(&bus, EVENT_PLAYER_MOVE, on_player_move);
 *  
 *  // Free event bus
 *  event_bus_free(&bus);
 */

// Listener function signature
// args is cast to whatever struct the event uses
typedef void (*EventListener)(void *args);

// Linked list node for one registered listener
typedef struct ListenerNode {
    EventListener fn;
    struct ListenerNode *next;
} ListenerNode;

// One slot per event type
// Holds the head of its listener list
typedef struct {
    ListenerNode *head;
} EventSlot;

// The event bus itself!
typedef struct {
    EventSlot *slots;
    int        num_events;
} EventBus;

/**
 * @brief Initialise event bus with number of events required.
 * Call this before using the event bus!
 *
 * @param bus event bus pointer
 * @param num_events number of events, usually just dedicate the last entry in your enum of events, such as EVENT_COUNT
 */
void event_bus_init          (EventBus *bus, int num_events);

/**
 * @brief Free event bus memory.
 * Call this at the end of your program!
 *
 * @param bus event bus pointer
 */
void event_bus_free          (EventBus *bus);

/**
 * @brief Start listening for an event
 * 
 * @param bus event bus pointer
 * @param event_type enum of event type
 * @param listener listener function
 */
void event_bus_listen        (EventBus *bus, int event_type, EventListener listener);

/**
 * @brief Stop listening to event
 * 
 * @param bus event bus pointer
 * @param event_type enum of event type
 * @param listener listener function
 */
void event_bus_stop_listening(EventBus *bus, int event_type, EventListener listener);

/**
 * @brief Broadcast trigger of event to all listeners
 * 
 * @param bus event bus pointer
 * @param event_type enum of event type
 * @param args arguments to provide to listeners
 */
void event_bus_trigger       (EventBus *bus, int event_type, void *args);
#endif