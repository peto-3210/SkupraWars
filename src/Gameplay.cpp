#include "Gameplay.hpp"
#include "Graphics.hpp"
#include "Gamestate.hpp"
#include "SoftwareTimer.hpp"
#include "hal/st7735.h"
#include <avr/io.h>

#define MAX_LASERS 7
#define LASER_LENGTH 5
#define SHIP_LENGTH 16
#define DISPLAY_LENGTH 160
#define DISPLAY_WIDTH 128
#define COOLDOWN_NORMAL 600000  // 0.6 sekundy
#define COOLDOWN_RAPID  300000  // 0.3 sekundy 

typedef struct {
    uint8_t x;
    uint8_t y;
    bool active;
} Laser;

// --- Lokální proměnné pro hru ---
static Laser player_lasers[MAX_LASERS];
static Laser enemy_lasers[MAX_LASERS];
static uint8_t x = DISPLAY_WIDTH/2; // Ship is positioned in the middle of the screen
static const uint8_t SHIP_Y = DISPLAY_LENGTH - 36;
static uint8_t last_A;

typedef enum { BTN_IDLE,
	           BTN_WAIT_DEBOUNCE,
	           BTN_WAIT_RELEASE,
	           BTN_RELEASE_DEBOUNCE } ButtonState;
static ButtonState btn_state = BTN_IDLE;

static SoftwareTimer* btnTimer;
static SoftwareTimer* laserTimer;
static SoftwareTimer* fireCooldownTimer;
static bool isRapidFireActive = false; // Zatím false, později se bude měnit po sebrání power-upu

void gameplay_init(void) {
    btnTimer = SoftwareTimerPool::acquireTimer();
    laserTimer = SoftwareTimerPool::acquireTimer();
	fireCooldownTimer = SoftwareTimerPool::acquireTimer();
	fireCooldownTimer->startTimerUs(0);
    
    last_A = (PIND & (1 << PD2)) >> PD2;
    x = DISPLAY_WIDTH/2; // Reset pozice
    
    for (int i = 0; i < MAX_LASERS; i++) {
        player_lasers[i].active = false;
        enemy_lasers[i].active = false;
    }
    
    st7735_fill_screen(0xFFFF);
    draw_ship(x, SHIP_Y, 0x001F);
}

GameState gameplay_tick(void) {
    // --- ČTENÍ TLAČÍTKA (STŘELBA) ---
    switch (btn_state) {
	    case BTN_IDLE:
	    // PŘIDÁNA KONTROLA COOLDOWNU:
	    if (!(PIND & (1 << PD5)) && fireCooldownTimer->isDone()) {
		    btnTimer->startTimerUs(100000);
		    btn_state = BTN_WAIT_DEBOUNCE;
	    }
	    break;

	    case BTN_WAIT_DEBOUNCE:
	    if (btnTimer->isDone()) {
		    if (!(PIND & (1 << PD5))) {
			    // Najdeme první volný laser v zásobníku a vystřelíme
			    for (int i = 0; i < MAX_LASERS; i++) {
				    if (!player_lasers[i].active) {
					    player_lasers[i].active = true;
					    player_lasers[i].x = x + 7;
					    player_lasers[i].y = SHIP_Y - 1;
					    
					    // Timer sdílí všechny střely, takže ho pustíme, pokud ještě neběží
					    laserTimer->startTimerUs(80000);
						
						// Nastartování cooldownu podle toho, jestli máme Power-up
						uint32_t current_cd = isRapidFireActive ? COOLDOWN_RAPID : COOLDOWN_NORMAL;
						fireCooldownTimer->startTimerUs(current_cd);
						
					    break; // Vystřelí jen jeden na jedno zmáčknutí
				    }
			    }
			    btn_state = BTN_WAIT_RELEASE;
			    } else {
			    btn_state = BTN_IDLE;
		    }
	    }
	    break;

	    case BTN_WAIT_RELEASE:
	    if (PIND & (1 << PD5)) {
		    btnTimer->startTimerUs(50000);
		    btn_state = BTN_RELEASE_DEBOUNCE;
	    }
	    break;

	    case BTN_RELEASE_DEBOUNCE:
	    if (btnTimer->isDone()) {
		    btn_state = BTN_IDLE;
	    }
	    break;
    }

    // --- FYZIKA A VYKRESLOVÁNÍ LASERU ---
    if (laserTimer->isDone()) {
	    bool any_laser_active = false;

	    // --- A) HRÁČOVY STŘELY (Letí NAHORU, Y roste) ---
	    for (int i = 0; i < MAX_LASERS; i++) {
		    if (player_lasers[i].active) {
			    any_laser_active = true;
			    
			    // Smažeme ocas střely (pokud je dostatečně daleko od lodi)
			    if (player_lasers[i].y + LASER_LENGTH < SHIP_Y) {
				    st7735_draw_pixel(player_lasers[i].x, player_lasers[i].y + LASER_LENGTH, 0xFFFF);
			    }

			    // Stará špička ztmavne
			    st7735_draw_pixel(player_lasers[i].x, player_lasers[i].y, 0xF800); // Červená stopa

			    // Posun nahoru
			    player_lasers[i].y--;

			    // Nová zářivá špička (např. žlutá pro tvé střely)
			    st7735_draw_pixel(player_lasers[i].x, player_lasers[i].y, 0xFFE0);

			    // Kontrola okrajů 
			    if (player_lasers[i].y <= 16) {
				    player_lasers[i].active = false;
				    
				    // Odeslání střely přes UART (tady se později zavolá funkce)
					/**********************************************/
				    /* uart_send_projectile(player_lasers[i].x);  */
				    /**********************************************/
					
				    // Smazání celého laseru z obrazovky
				    for(uint8_t j = 0; j <= LASER_LENGTH; j++) {
					    st7735_draw_pixel(player_lasers[i].x, player_lasers[i].y + j, 0xFFFF);
				    }
			    }
		    }
	    }

	    // --- B) NEPŘÁTELSKÉ STŘELY (Letí DOLŮ, Y roste) ---
	    for (int i = 0; i < MAX_LASERS; i++) {
		    if (enemy_lasers[i].active) {
			    any_laser_active = true;
			    
			    // Smažeme ocas 
			    if (enemy_lasers[i].y > 16 + LASER_LENGTH) {
				    st7735_draw_pixel(enemy_lasers[i].x, enemy_lasers[i].y - LASER_LENGTH, 0xFFFF);
			    }

			    // Stará špička ztmavne
			    st7735_draw_pixel(enemy_lasers[i].x, enemy_lasers[i].y, 0x001F); // Modrá stopa nepřítele

			    // Posun dolů
			    enemy_lasers[i].y++;

			    // Nová zářivá špička (např. světle modrá pro nepřátele)
			    st7735_draw_pixel(enemy_lasers[i].x, enemy_lasers[i].y, 0x07FF);

			    // Kontrola okrajů a kolizí
			    if (enemy_lasers[i].y >= SHIP_Y) {
				    enemy_lasers[i].active = false;
				    
				    // Smazání zbytku nepřátelského laseru
				    for(uint8_t j = 0; j <= LASER_LENGTH; j++) {
					    st7735_draw_pixel(enemy_lasers[i].x, enemy_lasers[i].y - j, 0xFFFF);
				    }
			    }
				
			    // Zde později přidáme kontrolu: if (kolize s mojí lodí) -> uber život a smaž laser
				/**********************************************/
				/**********************************************/
		    }
	    }

	    // Pokud letí alespoň jeden laser (můj nebo cizí), restartujeme časovač
	    if (any_laser_active) {
		    laserTimer->startTimerUs(25000);
	    }
    }

    // --- ČTENÍ ENKODÉRU (POHYB LODĚ) ---
    uint8_t current_A = (PIND & (1 << PD3)) >> PD3;

    if (current_A != last_A) {
	    if (current_A == 0) {
		    uint8_t current_B = (PIND & (1 << PD2)) >> PD2;
		    uint8_t old_x = x; // Zapamatujeme si starou pozici

		    // --- Výpočet nové pozice s wrappingem ---
		    if (current_B != current_A) {
			    if (x >= 3) {
				    x -= 3; // Doleva
				    } else {
				    x = DISPLAY_WIDTH - SHIP_LENGTH; // Skok na pravý okraj (128 - 16 = 112)
			    }
			    } else {
			    if (x <= (DISPLAY_WIDTH - SHIP_LENGTH - 3)) {
				    x += 3; // Doprava
				    } else {
				    x = 0; // Skok na levý okraj
			    }
		    }

		    // Pokud jsme se pohnuli, překreslíme
		    if (x != old_x) {
			    // 1. Uklidíme "odpad"
			    
			    // A) Došlo k teleportu přes celou obrazovku? (Skok z 0 na 112 nebo naopak)
			    if ((old_x < 3 && x > 100) || (old_x > 100 && x < 3)) {
				    // Musíme smazat celou starou loď (16x16)
				    for (int dx = 0; dx < SHIP_LENGTH; dx++) {
					    for (int dy = 0; dy < SHIP_LENGTH; dy++) {
						    st7735_draw_pixel(old_x + dx, SHIP_Y + dy, 0xFFFF);
					    }
				    }
			    }
			    // B) Normální posun doleva (mažeme pravý okraj)
			    else if (x < old_x) {
				    for (int i = 0; i < SHIP_LENGTH; i++) {
					    st7735_draw_pixel(old_x + 13, SHIP_Y + i, 0xFFFF);
					    st7735_draw_pixel(old_x + 14, SHIP_Y + i, 0xFFFF);
					    st7735_draw_pixel(old_x + 15, SHIP_Y + i, 0xFFFF);
				    }
			    }
			    // C) Normální posun doprava (mažeme levý okraj)
			    else {
				    for (int i = 0; i < SHIP_LENGTH; i++) {
					    st7735_draw_pixel(old_x, SHIP_Y + i, 0xFFFF);
					    st7735_draw_pixel(old_x + 1, SHIP_Y + i, 0xFFFF);
					    st7735_draw_pixel(old_x + 2, SHIP_Y + i, 0xFFFF);
				    }
			    }
			    
			    // 2. Vykreslíme loď na nové pozici
			    draw_ship(x, SHIP_Y, 0x001F);
		    }
	    }
	    last_A = current_A;
    }	
	
	return STATE_GAMEPLAY;
}

void gameplay_spawn_enemy_laser(uint8_t received_x) {
	uint8_t mirrored_x = 127 - received_x;
	for (int i = 0; i < MAX_LASERS; i++) {
		if (!enemy_lasers[i].active) {
			enemy_lasers[i].active = true;
			enemy_lasers[i].x = mirrored_x;
			enemy_lasers[i].y = 0; // Začíná NAHOŘE
			break;
		}
	}
}

