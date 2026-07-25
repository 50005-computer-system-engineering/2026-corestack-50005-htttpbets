#include "movement.h"
#include "board_control.h"

bool moveLeft(GameState *state)
{
    if (isValidPos(state, state->current.type, state->current.rot, state->current.x - 1, state->current.y))
    {
        state->current.x--;
        state->last_action_rotation = false;
        return true;
    }
    return false;
}

bool moveRight(GameState *state)
{
    if (isValidPos(state, state->current.type, state->current.rot, state->current.x + 1, state->current.y))
    {
        state->current.x++;
        state->last_action_rotation = false;
        return true;
    }
    return false;
}

bool softDrop(GameState *state)
{
    if (isValidPos(state, state->current.type, state->current.rot, state->current.x, state->current.y + 1))
    {
        state->current.y++;
        state->last_action_rotation = false;
        return true;
    }
    return false;
}

bool hardDrop(GameState *state)
{
    while (isValidPos(state, state->current.type, state->current.rot, state->current.x, state->current.y + 1))
    {
        state->current.y++;
        state->last_action_rotation = false;
        return true;
    }
    return false;
}