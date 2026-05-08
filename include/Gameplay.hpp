#pragma once
#include "Gamestate.hpp"
#include "Graphics/Graphics.hpp"

// --- ENUMS ---

// State machine states for button input processing (e.g., handling long/short presses)
typedef enum { BTN_IDLE,
	BTN_WAIT_PUSHED,
	BTN_WAIT_RELEASE,
	BTN_WAIT_FULL_RELEASE,
 } ButtonState;

 // Available power-up buffs in the game
enum PowerUpType {
	PU_RAPID_FIRE,
	PU_SHIELD,
	PU_SENTRY
};

// Available weapon types, explicitly numbered
enum WeaponType {
	WEP_RAILGUN = 0,
	WEP_BURST = 1,
	WEP_ROCKET = 2,
	WEP_LASER = 3
};

// --- DATA STRUCTURES ---

// Represents a deployable automated turret
struct SentryGun {
    bool active;              // Is the turret currently deployed?
    uint8_t x;                // X coordinate
    uint8_t y;                // Y coordinate
    uint32_t spawn_time;      // Timestamp for calculating turret lifetime/despawn
    uint32_t last_shot_time;  // Timestamp for calculating fire rate
};

// Represents a flying projectile (player or enemy)
struct Projectile {
    bool active;              // Is the projectile currently in flight?
    uint8_t x, y;             // Current coordinates
    WeaponType type;          // Visual and behavioral type of the projectile
    uint32_t spawn_time;      // Used for lifetime limits
};

// Represents a power-up dropped on the battlefield
struct PowerUp {
    bool active;              // Is the power-up currently on the screen?
    uint8_t x;                // X coordinate
    uint8_t y;                // Y coordinate
    PowerUpType type;         // The specific buff it grants
    uint8_t health;           // Health (can be destroyed by weapons)
    uint32_t spawn_time;      // Timestamp for despawning after a while
};

// --- GAMEPLAY CORE FUNCTIONS ---

/**
 * @brief Updates the ammo count display on the HUD for a specific weapon.
 * * Calculates the screen position based on the weapon type, clears the previous 
 * value, and renders the new ammo count. Note that the Railgun is skipped 
 * as it has infinite ammo.
 * * @param wep The type of weapon to update.
 * @param new_ammo The current ammo count to be displayed (00-99).
 */
void update_hud_ammo(WeaponType wep, uint8_t new_ammo);

/**
 * @brief Increases the player's ammo count for a given weapon.
 * * Adds the specified amount to the inventory and enforces a maximum 
 * cap of 99 units. Automatically triggers a HUD refresh to reflect 
 * the change visually.
 * * @param wep The type of weapon for which to add ammo.
 * @param amount The quantity of ammo to be added.
 */
void add_ammo(WeaponType wep, uint8_t amount);


/**
 * @brief Adds a power-up to the player's inventory stack and grants random ammo.
 * * If there is space in the inventory, the power-up is stored and its icon is 
 * rendered on the UI. Regardless of inventory space, the player also receives 
 * a random amount of ammo for a random weapon (excluding the Railgun).
 * * @param type The type of power-up to be collected.
 */
void add_powerup_to_inventory(PowerUpType type);

/**
 * @brief Spawns a new sentry gun at the ship's current horizontal position.
 * * Searches for an available (inactive) slot in the sentry array. If found, 
 * initializes the sentry 10 pixels above the ship and records the current 
 * system time (in microseconds) to manage its lifetime and firing rate.
 * * @param ship_x The current X-coordinate of the player's ship.
 */
void spawn_sentry_gun(uint8_t ship_x);

/**
 * @brief Activates and removes the most recently acquired power-up from the inventory.
 * * Uses a stack-based (LIFO) logic: the last item collected is the first one used. 
 * The function clears the power-up's icon from the HUD, triggers its specific 
 * effect (e.g., starting timers for Rapid Fire or Shield, or spawning a Sentry), 
 * updates the UI, and plays the activation sound.
 * * @note If the inventory is empty, the function exits silently without any effect.
 */
void use_powerup();

/**
 * @brief Spawns a new power-up at a random location within the play area.
 * * Searches for an available slot in the power-up array. If a slot is free, 
 * it initializes a power-up with a random type, sets its position within 
 * defined boundaries (X: 14-113, Y: 20-99), and sets its initial health and 
 * spawn time. The power-up is then rendered to the screen.
 * * @return true If the power-up was successfully spawned.
 * @return false If all power-up slots are currently occupied (MAX_POWERUPS reached).
 */
bool powerup_spawn_random();

/**
 * @brief Retrieves the damage value for a specific weapon type.
 * * Centralizes the combat balancing. Each weapon has a unique damage profile:
 * - Railgun: 4 DMG (destroys 11 HP objects in 3 hits).
 * - Burst: 2 DMG per projectile (6 DMG total per burst).
 * - Rocket: 11 DMG (one-shot capability for standard 11 HP objects).
 * - Laser: 1 DMG (high frequency, interacts with invincibility frames).
 * * @param wep The type of weapon to check.
 * @return uint8_t The damage value inflicted by a single projectile/hit.
 */
uint8_t get_weapon_damage(WeaponType wep);

/**
 * @brief Performs an Axis-Aligned Bounding Box (AABB) collision detection.
 * * Checks if two rectangular areas overlap in 2D space. This is a standard 
 * non-rotated collision check used for projectiles, entities, and power-ups.
 * * @param x1,y1 Top-left coordinates of the first rectangle.
 * @param w1,h1 Width and height of the first rectangle.
 * @param x2,y2 Top-left coordinates of the second rectangle.
 * @param w2,h2 Width and height of the second rectangle.
 * @return true If the two rectangles are intercepting.
 * @return false If there is no overlap between the rectangles.
 */
bool check_collision(int x1, int y1, int w1, int h1,
                        int x2, int y2, int w2, int h2);

/**
 * @brief Processes interactions between a single projectile and all active power-ups.
 * * This function calculates the effective collision box based on the projectile type 
 * (Rocket, Laser, or Railgun/Burst) and checks for overlaps with power-up entities. 
 * If a hit occurs:
 * - Damage is applied based on the weapon type.
 * - If health drops to zero, the power-up is collected and added to the inventory.
 * - Visuals are updated (redrawn or cleared).
 * - Projectiles are destroyed upon impact, except for the Laser, which penetrates.
 * * @param p Reference to the Projectile object to be checked.
 * @return true If a collision occurred and the projectile was consumed (destroyed).
 * @return false If no collision occurred or if the projectile persists (e.g., Laser).
 */
bool check_powerup_collisions(Projectile& p);

/**
 * @brief Updates the player's health points (HP) display on the HUD.
 * * Formats the HP value as a two-digit string (e.g., "05") to ensure 
 * consistent UI alignment and renders it at a fixed position using 
 * the player's signature red color.
 * * @param hp The current health value of the player to be displayed.
 */
void update_player_hp_ui(uint8_t hp);

/**
 * @brief Updates the health indicator for a specific enemy on the HUD.
 * * Calculates the UI position and color based on the enemy index. 
 * - Index 0: Positioned on the left, rendered in green.
 * - Index 1: Positioned on the right, rendered in blue.
 * The HP is formatted as a two-digit string to maintain fixed-width layout.
 * * @param enemy_index The identifier of the enemy (0 for primary, 1 for secondary).
 * @param hp The current health value of the specified enemy.
 */
void update_enemy_hp_ui(uint8_t enemy_index, uint8_t hp);

/**
 * @brief Evaluates interactions between enemy projectiles and the player's ship.
 * * Handles the complete lifecycle of a player hit:
 * - Hitbox Calculation: Adjusts projectile dimensions based on weapon type.
 * - Shield Logic: Negates all damage if the shield power-up is active.
 * - I-Frames (Invincibility): Implements an 80ms throttle for Laser weapons to 
 * prevent instantaneous health depletion.
 * - State Management: Updates player HP, triggers HUD refreshes, plays sound 
 * effects, and synchronizes the new state via Messenger (UART).
 * - Game Over: Sets the `endGame` flag if health reaches zero.
 * * @param p Reference to the enemy Projectile being checked.
 * @return true If a collision occurred and the projectile was consumed.
 * @return false If no collision occurred or the projectile is persistent (Laser).
 */
bool check_player_collision(Projectile& p);

/**
 * @brief Draws a single pixel with protection for the inventory UI zone.
 * * Acts as a wrapper for the standard pixel drawing function. It implements 
 * a hardware-level clipping mask that prevents any drawing within the 
 * inventory bounds (X <= 13, Y between 77 and 111), ensuring the inventory 
 * icons remain visible and "above" the game world.
 * * @param x The horizontal coordinate.
 * @param y The vertical coordinate.
 * @param color The 16-bit color value (RGB565).
 */
void safe_draw_pixel(int x, int y, uint16_t color);

/**
 * @brief Renders a filled rectangle with per-line clipping against the HUD.
 * * Provides a safe way to draw larger shapes without overdrawing the inventory area.
 * - Fast path: If the rectangle is completely to the right of the inventory, 
 * it uses a single hardware-accelerated fill call.
 * - Slow path: If the rectangle overlaps the inventory's X-axis, it renders 
 * line-by-line (scanlines), skipping those that fall within the vertical 
 * protected range (Y: 77-111).
 * * @param x,y Top-left coordinates of the rectangle.
 * @param w,h Width and height of the rectangle.
 * @param color The 16-bit color value.
 */
void safe_fill_rect(int x, int y, int w, int h, uint16_t color);

/**
 * @brief Restores power-up sprites that were visually corrupted by projectile cleanup.
 * * When a projectile's trail is erased from the screen, it may leave "holes" in 
 * static power-up sprites it passed over. This function performs an intersection 
 * check between the erased area and all active power-ups. If an overlap is detected, 
 * the affected power-up is immediately redrawn to maintain visual integrity.
 * * @param erase_x,erase_y Top-left coordinates of the recently erased area.
 * @param erase_w,erase_h Width and height of the erased area.
 */
void redraw_powerups_under_tail(int erase_x, int erase_y, int erase_w, int erase_h);

/**
 * @brief Main update routine for all projectile types. 
 * * Orchestrates movement, collision detection, and optimized rendering for the 
 * entire arsenal. The function implements three distinct rendering strategies:
 * - **Railgun/Burst:** Trail-based rendering that erases only the last pixel 
 * (the "tail") and updates the "tip" to save CPU cycles.
 * - **Rocket:** Sprite-based movement with incremental updates (drawing only 
 * the leading edge) after the initial spawn phase.
 * - **Laser:** A persistent, time-limited beam with dynamic height 
 * calculation to prevent overdrawing the player's ship or HUD.
 * * It also handles automatic background restoration for power-ups corrupted 
 * by moving projectiles and synchronizes state upon projectile expiration.
 * * @param p Reference to the Projectile object to be processed.
 * @param is_enemy Boolean flag to determine direction, colors, and target logic.
 */
void process_projectile(Projectile& p, bool is_enemy);

/**
 * @brief Instantiates a projectile fired by the network opponent.
 * * Coordinates the arrival of enemy fire by mirroring the received X-coordinate 
 * to match the local screen orientation. The projectile is spawned at the 
 * upper HUD boundary and assigned a weapon type. If the projectile update 
 * timer is currently idle, it is automatically reactivated with a 25ms duty cycle.
 * * @param received_x The raw X-coordinate sent by the peer.
 * @param wep_type The type of weapon being fired by the enemy.
 */
void spawn_enemy_projectile(uint8_t received_x, WeaponType wep_type);

/**
 * @brief Spawns a local player projectile and synchronizes it with the network peer.
 * * Initializes a projectile at the ship's current location, adjusting the 
 * vertical offset based on the weapon's geometry (e.g., Rockets spawn further 
 * out). It triggers the corresponding SFX, increments the shot counter, 
 * and broadcasts the event via the Messenger system to the remote opponent.
 * * @param wep The type of weapon to be fired.
 * @param ship_x,ship_y Current coordinates of the player's ship.
 */
void spawn_projectile(WeaponType wep, uint8_t ship_x, uint8_t ship_y);

/**
 * @brief Evaluates firing conditions and executes a shot if all requirements are met.
 * * Checks multiple constraints before allowing a projectile to spawn:
 * - **Projectile Cap:** Enforces a limit on simultaneous projectiles (6 by default, 
 * increased to 12 if Rapid Fire is active).
 * - **Ammo Check:** Verifies if the player has enough ammunition (Railgun is 
 * exempted as it is infinite).
 * - **Fire Modes:** Handles specific weapon logic, such as initializing the 
 * burst sequence for `WEP_BURST`.
 * * If conditions are met, it spawns the projectile, decrements ammo, and 
 * updates the HUD. If not, it triggers an "out of ammo" sound effect.
 * * @param wep The type of weapon the player is attempting to fire.
 * @param ship_x,ship_y Current coordinates of the player's ship for spawn positioning.
 */
void try_shoot(WeaponType wep, uint8_t ship_x, uint8_t ship_y);

/**
 * @brief Initializes the gameplay environment and resets the game state.
 * * Prepares the system for a new game session through several phases:
 * - **Resource Allocation:** Acquires necessary software timers from the pool 
 * (only if they haven't been allocated yet).
 * - **State Reset:** Sets player and enemy health, resets position, clears 
 * inventory, and disables active power-ups and projectiles.
 * - **Hardware Prep:** Performs an initial read of the rotary encoder state.
 * - **Network Handshake:** Executes the `initTopology` to synchronize with the 
 * remote peer; triggers a fatal error if connection fails.
 * - **UI & Audio Bootstrap:** Clears the display, renders the static HUD, 
 * plays the opening melody, and draws the player's ship.
 * * @note This function performs a "soft reset" of all gameplay variables, 
 * ensuring no stale data remains from previous sessions.
 */
void gameplay_init(void);

/**
 * @brief Main gameplay update loop, executed once per frame.
 * * This core function orchestrates the entire game logic in a non-blocking 
 * manner. It manages the following subsystems:
 * - **Terminal States:** Checks for fatal errors, victory (via network 
 * announcements), or defeat.
 * - **Network Dispatching:** Processes incoming packets for enemy projectiles, 
 * HP updates, and hit visualizations.
 * - **Input Handling:** Implements a Finite State Machine for the fire 
 * button (supporting short-press to shoot and long-press to use power-ups) 
 * and handles weapon cycling via the encoder button.
 * - **Physics & Logic:** Updates autonomous Sentry guns, manages projectile 
 * trajectories, and schedules random power-up spawns/despawns.
 * - **Movement:** Processes rotary encoder input for ship movement with 
 * integrated screen-wrapping and "dirty region" rendering optimization.
 * - **Power-up Lifecycle:** Monitors timers for active buffs (Shield, Rapid Fire) 
 * and reverts player state upon expiration.
 * * @return GameState The next state of the game engine (e.g., CONTINUE, VICTORY, DEFEAT).
 */
GameState gameplay_tick(void);


// --- UI & GRAPHICS EXTERN FUNCTIONS ---

/**
 * @brief Renders the complete inventory UI on the left side of the HUD.
 * * Provides a full refresh of the power-up storage display:
 * - **Clearing:** Wipes the dedicated inventory area (X: 0-12, Y: 78-110) to 
 * remove stale icons.
 * - **Framing:** Re-draws the magenta dotted boundary for visual consistency.
 * - **Icon Stack:** Iterates through the current inventory array and renders 
 * each power-up icon at 10-pixel vertical intervals.
 * * The rendering follows a bottom-up approach, visually representing the 
 * Last-In-First-Out (LIFO) stack logic.
 * * @param count The current number of power-ups held in the inventory.
 * @param inventory Pointer to the array of PowerUpType items currently owned.
 */
extern void update_inventory_ui(uint8_t count, PowerUpType* inventory);

/**
 * @brief Renders a rectangular outline using a dotted line pattern. 
 * It is primarily used for the inventory slot outlines to provide 
 * a "holographic" or lightweight HUD aesthetic.
 * * @param x,y Top-left coordinates of the rectangle.
 * @param w,h Width and height of the rectangle.
 * @param color The 16-bit color value for the dots.
 */
extern void draw_dotted_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * @brief Renders a 1-bit power-up icon based on its type.
 * * This function maps a PowerUpType to its corresponding bitmap and color 
 * profile. It then delegates the rendering to the lower-level PROGMEM 
 * drawing routine. Supported types include:
 * - **PU_RAPID_FIRE:** Rendered in orange.
 * - **PU_SHIELD:** Rendered in cyan.
 * - **PU_SENTRY:** Rendered in white.
 * * @param x,y Top-left coordinates where the icon should be rendered.
 * @param type The specific PowerUpType enumeration to draw.
 * @param bg_color The background color used for transparency/erasure.
 */
extern void draw_powerup8x8(uint8_t x, uint8_t y, PowerUpType type, uint16_t bg_color);



// --- HUD rendering functions ---
/**
 * @brief Renders the complete bottom HUD, including health and weapon systems.
 * * This function assembles the player's dashboard at the bottom of the screen:
 * - **Health Section:** Draws a red heart icon and the player's current HP 
 * (formatted as 2 digits).
 * - **Separator:** Draws a solid red horizontal line and a thick white 
 * vertical bar to compartmentalize the UI.
 * - **Weapon Arsenal:** Iterates through all 4 weapon slots, rendering 
 * procedural icons for each:
 * - *Railgun:* Infinite ammo indicator ("--") and blue trace.
 * - *Burst:* Triple-shot icon with ammo count.
 * - *Rocket:* Multicolored sprite (Green/Orange) with ammo count.
 * - *Laser:* Cyan beam block with ammo count.
 * - **Selection:** Highlights the currently active weapon with a white border.
 * * @param your_health Current HP of the player.
 * @param active_weapon The weapon type currently selected by the player.
 * @param ammo_counts Array containing the remaining ammunition for each weapon.
 */
extern void gameplay_draw_bottom_hud(uint8_t your_health, WeaponType active_weapon, uint8_t ammo_counts[]);

/**
 * @brief Renders a selection highlight box around a specific weapon slot.
 * * Provides visual feedback for weapon selection by drawing a rectangular 
 * border around the designated HUD slot. The function uses the weapon index 
 * to calculate the precise horizontal position on the bottom HUD.
 * * It is typically used in two ways:
 * - To **highlight** a newly selected weapon (using a visible color like COLOR_WHITE).
 * - To **clear** a previous selection (using the background color COLOR_BG).
 * * @param wep The weapon type/slot index to be enclosed in the box.
 * @param color The 16-bit color value (RGB565) for the selection border.
 */
extern void draw_weapon_selection_box(WeaponType wep, uint16_t color);

/**
 * @brief Updates the dynamic health indicators on the top HUD.
 * * Formats and renders the current health points for two entities (e.g., Enemy 1 
 * and Enemy 2) at the top of the screen. 
 * - **Formatting:** Converts numeric HP values to 2-digit strings using manual 
 * character conversion for maximum efficiency.
 * - **Visuals:** Entity 1 is rendered in green on the left, while Entity 2 is 
 * rendered in blue on the right, maintaining consistent color-coding with 
 * their respective projectile effects.
 * * @param p1_health Current health of the first entity (index 0).
 * @param p2_health Current health of the second entity (index 1).
 */
extern void gameplay_draw_top_hud_dynamic(uint8_t p1_health, uint8_t p2_health);

/**
 * @brief Renders the non-changing (static) visual elements of the top HUD.
 * * This function initializes the upper interface layout:
 * - **Separator:** Draws a solid green horizontal line that delimits the 
 * gameplay area from the HUD.
 * - **Icons:** Renders two heart icons (Green for the left entity, Blue for 
 * the right entity) using the `draw_heart8x8` routine.
 * - **Center Divider:** Creates a vertical dotted magenta line in the exact 
 * center of the HUD to visually separate the two health indicators.
 * * @note This is called once during initialization to minimize redundant 
 * draw calls in the main game loop.
 */
extern void gameplay_draw_top_hud_static(void);

/**
 * @brief Renders a hit indicator icon near the health bar of the affected enemy.
 * * Displays a temporary emote (e.g., an "X" or flash icon) to provide immediate 
 * visual feedback when a remote peer is successfully hit. 
 * - If **direction** is false: The icon is drawn on the left side (Enemy 1).
 * - If **direction** is true: The icon is drawn on the right side (Enemy 2).
 * * @param direction Boolean flag indicating which enemy was hit (matches network direction).
 */
extern void draw_enemy_hit(bool direction);

/**
 * @brief Removes all enemy hit indicators from the top HUD.
 * * Clears the 8x8 pixel regions at both possible hit marker locations 
 * by filling them with the background color. This is typically called 
 * after a short timer expires to reset the HUD state.
 */
extern void clear_enemy_hit();