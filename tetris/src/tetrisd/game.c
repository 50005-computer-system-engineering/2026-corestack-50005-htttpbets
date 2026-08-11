#include <string.h>
#include "game.h"
#include "lib/libtetrisbrain/engine.h"
#include "lib/libtetrisbrain/movement.h"
#include "lib/libtetrisbrain/board_control.h"
#include "lib/libtetrisbrain/hold.h"
#include "lib/libtetrisbrain/targeting.h"

/* ----- SETUP ----- */
void initSession(GameSession *session, const uint32_t *clientIds, uint32_t lobbySize)
{
    memset(session, 0, sizeof(GameSession));

    session->count = (int)lobbySize;
    session->roster.count = (int)lobbySize;

    for (uint32_t i = 0; i < lobbySize && i < MAX_LOBBY_SIZE; i++)
    {
        PlayerSlot *slot = &session->players[i];

        startGame(&slot->state);

        slot->player_id = clientIds[i];
        slot->state.player_id = clientIds[i];
        slot->state.target_player_id = 0; // Resolved below once every ID is known
        slot->state.last_attacker_id = 0;
        slot->active = true;
        slot->dirty = true; // Force an initial broadcast so clients get a first board
        slot->idle_ticks = 0;

        // Mirror into the shared roster used for targeting
        session->roster.ids[i] = clientIds[i];
        session->roster.eliminated[i] = false;
    }

    // Give everyone a sane starting opponent, first player that is not themselves
    for (int i = 0; i < session->count; i++)
    {
        PlayerSlot *slot = &session->players[i];
        for (int j = 0; j < session->roster.count; j++)
        {
            if (session->roster.ids[j] != slot->player_id)
            {
                slot->state.target_player_id = session->roster.ids[j];
                break;
            }
        }
    }
}

PlayerSlot *findPlayer(GameSession *session, uint32_t player_id)
{
    for (int i = 0; i < session->count; i++)
    {
        if (session->players[i].active && session->players[i].player_id == player_id)
        {
            return &session->players[i];
        }
    }
    return NULL; // Unknown ID, caller decides what to do
}

/* ----- INPUT ----- */
// Every key the client used to act on arrives here as a request, and is performed against the authoritative board instead
void applyAction(GameSession *session, PlayerSlot *slot, PlayerAction action)
{
    if (slot == NULL || !slot->active || slot->state.game_over)
    {
        return; // Nothing to do for a dead or unknown player
    }

    GameState *state = &slot->state;

    // Successful moves reset the lock timer
    switch (action)
    {
    case ACTION_MOVE_LEFT:
        if (moveLeft(state))
        {
            state->lock_timer = 0;
        }
        break;

    case ACTION_MOVE_RIGHT:
        if (moveRight(state))
        {
            state->lock_timer = 0;
        }
        break;

    case ACTION_ROTATE_CW:
        rotateCurrentPiece(state);
        state->lock_timer = 0;
        break;

    case ACTION_ROTATE_CCW:
        rotateCounterClockwise(state);
        state->lock_timer = 0;
        break;

    case ACTION_SOFT_DROP:
        if (softDrop(state))
        {
            state->lock_timer = 0;
        }
        break;

    case ACTION_HARD_DROP:
        // Slam the piece then immediately resolve the lock
        hardDrop(state);
        tickGame(state);
        state->gravity_timer = 0;
        state->lock_timer = 0;
        break;

    case ACTION_HOLD:
        // One swap per piece
        if (!state->has_held)
        {
            holdPiece(state);
            state->gravity_timer = 0;
        }
        break;

    case ACTION_CYCLE_TARGET_MODE:
        cycleTargetMode(state);
        break;

    case ACTION_CYCLE_TARGET:
        // Resolved against the server's roster, which is always the real one
        cycleManualTarget(state, &session->roster);
        break;

    case ACTION_QUIT:
        state->game_over = true;
        slot->active = false;

        // Drop them out of targeting so nobody keeps aiming at a player who left
        for (int i = 0; i < session->roster.count; i++)
        {
            if (session->roster.ids[i] == slot->player_id)
            {
                session->roster.eliminated[i] = true;
            }
        }
        break;

    case ACTION_NONE:
    default:
        return; // Unknown action, nothing changed so do not mark dirty
    }

    slot->dirty = true; // An action always warrants pushing fresh state
}

/* ----- SIMULATION ----- */
void tickSession(GameSession *session)
{
    for (int i = 0; i < session->count; i++)
    {
        PlayerSlot *slot = &session->players[i];

        if (!slot->active || slot->state.game_over)
        {
            continue; // Dead players stop simulating
        }

        // updateTimers reports whether the piece actually moved or locked, only mark dirty when the board really changed
        if (updateTimers(&slot->state))
        {
            slot->dirty = true;
        }

        // A player who just topped out needs one final push so clients see it
        if (slot->state.game_over)
        {
            slot->dirty = true;

            for (int j = 0; j < session->roster.count; j++)
            {
                if (session->roster.ids[j] == slot->player_id)
                {
                    session->roster.eliminated[j] = true;
                }
            }
        }
    }
}

/* ----- MATCH END ----- */
// A player counts as alive only while they are still connected AND not topped out
int countAlivePlayers(const GameSession *session)
{
    int alive = 0;
    for (int i = 0; i < session->count; i++)
    {
        if (session->players[i].active && !session->players[i].state.game_over)
        {
            alive++;
        }
    }
    return alive;
}