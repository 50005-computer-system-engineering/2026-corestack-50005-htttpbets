#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

// Movement/hitbox constants shared by the authoritative server (bombd) and the
// renderer (bombapp), so the two never drift apart on how a player moves.
// Prefixed SIM_ so these tokens never collide with struct field names that
// happen to share the same word (eg: Physics.PLAYER_SPEED in config.h)
#define SIM_PLAYER_SPEED 3.0f
#define SIM_PLAYER_SPRINT_SPEED 5.5f

// Bounding box offset/size, expressed as a ratio of one tile (0f - 1f)
#define SIM_PLAYER_BOX_OFFSET_X 0.35f
#define SIM_PLAYER_BOX_OFFSET_Y 0.10f
#define SIM_PLAYER_BOX_SIZE_X 0.42f
#define SIM_PLAYER_BOX_SIZE_Y 0.85f

#endif
