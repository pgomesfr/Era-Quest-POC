#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

#define LARGURA_TELA 1920
#define ALTURA_TELA  1080

typedef struct {
    float x, y, w, h;
}   Plataforma;

typedef struct {
    float x, y, raio;
    bool coletado;
} Coletavel;

#define COR_CORPO   al_map_rgb(70, 50, 35)
#define COR_TOPO    al_map_rgb(110, 160, 70)
#define COR_BORDA   al_map_rgb(30, 20, 15)

Plataforma plataformas[] = {
    { 0, 980, 1920, 100 },
    { 300, 840,  260,  30 },
    { 680, 700,  260,  30 },
    { 1060, 570,  260,  30 },
    { 1420, 700,  240,  30 },
    { 1700, 840,  220,  30 },
};
const int NUM_PLATAFORMAS = sizeof(plataformas) / sizeof(plataformas[0]);

Coletavel coletaveis[] = {
{ 1190, 546, 14, false },
};
const int NUM_COLETAVEIS = sizeof(coletaveis) / sizeof(coletaveis[0]);

void desenhar_plataforma(Plataforma p) {
    al_draw_filled_rectangle(p.x, p.y, p.x + p.w, p.y + p.h, COR_CORPO);
    al_draw_filled_rectangle(p.x, p.y, p.x + p.w, p.y + 10, COR_TOPO);
    al_draw_rectangle(p.x, p.y, p.x + p.w, p.y + p.h, COR_BORDA, 3);
}

void desenhar_coletavel(Coletavel c) {
    float oscilacao = sinf(al_get_time() * 4) * 4;
    al_draw_filled_circle(c.x, c.y + oscilacao, c.raio, al_map_rgb(255, 210, 60));
    al_draw_circle(c.x, c.y + oscilacao, c.raio, al_map_rgb(200, 140, 20), 3);
}

bool tocou_coletavel(Coletavel c, float x, float y, float largura, float altura) {
    float mais_perto_x = fmaxf(x, fminf(c.x, x + largura));
    float mais_perto_y = fmaxf(y, fminf(c.y, y + altura));
    float dx = c.x - mais_perto_x;
    float dy = c.y - mais_perto_y;
    return (dx * dx + dy * dy) <= (c.raio * c.raio);
}

void calcular_hitbox(ALLEGRO_BITMAP* bmp, int* off_x, int* off_y, int* hit_w, int* hit_h) {
    int largura = al_get_bitmap_width(bmp);
    int altura = al_get_bitmap_height(bmp);
    int min_x = largura, min_y = altura, max_x = -1, max_y = -1;

    al_lock_bitmap(bmp, al_get_bitmap_format(bmp), ALLEGRO_LOCK_READONLY);

    for (int py = 0; py < altura; py++) {
        for (int px = 0; px < largura; px++) {
            unsigned char r, g, b, a;
            al_unmap_rgba(al_get_pixel(bmp, px, py), &r, &g, &b, &a);
            if (a > 10) {
                if (px < min_x) min_x = px;
                if (px > max_x) max_x = px;
                if (py < min_y) min_y = py;
                if (py > max_y) max_y = py;
            }
        }
    }

    al_unlock_bitmap(bmp);

    if (max_x < 0) {
        *off_x = 0;
        *off_y = 0;
        *hit_w = largura;
        *hit_h = altura;
        return;
    }

    *off_x = min_x;
    *off_y = min_y;
    *hit_w = max_x - min_x + 1;
    *hit_h = max_y - min_y + 1;
}

int main() {
    al_init();
    al_init_primitives_addon();
    al_init_image_addon();
    al_install_keyboard();

    ALLEGRO_DISPLAY* display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    al_set_window_title(display, "Era Quest");

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);

    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();
    al_register_event_source(fila, al_get_display_event_source(display));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_timer_event_source(timer));

    ALLEGRO_BITMAP* cenario = al_load_bitmap("cenario.png");
    ALLEGRO_BITMAP* nox = al_load_bitmap("nox.png");

    float sprite_largura = al_get_bitmap_width(nox);
    int off_x, off_y, hit_w, hit_h;
    calcular_hitbox(nox, &off_x, &off_y, &hit_w, &hit_h);

    float largura = hit_w;
    float altura = hit_h;

    float x = 200;
    float y = 0;
    float velocidadex = 0;
    float velocidadey = 0;

    float gravidade = 0.7;
    float velocidade = 6;
    float forcadopulo = -16;

    int coletados = 0;
    char titulo[64];

    bool pisandonochao = false;
    bool olharparaesquerda = false;
    bool isRodando = true;
    bool redesenhar = false;

    al_start_timer(timer);

    while (isRodando) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(fila, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            isRodando = false;
        }

        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN && evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            isRodando = false;
        }

        else if (evento.type == ALLEGRO_EVENT_TIMER) {
            ALLEGRO_KEYBOARD_STATE teclado;
            al_get_keyboard_state(&teclado);

            velocidadex = 0;
            if (al_key_down(&teclado, ALLEGRO_KEY_LEFT)) {
                velocidadex = -velocidade;
                olharparaesquerda = true;
            }

            if (al_key_down(&teclado, ALLEGRO_KEY_RIGHT)) {
                velocidadex = velocidade;
                olharparaesquerda = false;
            }

            if (pisandonochao && al_key_down(&teclado, ALLEGRO_KEY_UP)) {
                velocidadey = forcadopulo;
                pisandonochao = false;
            }

            velocidadey += gravidade;

            x += velocidadex;

            for (int i = 0; i < NUM_PLATAFORMAS; i++) {
                Plataforma p = plataformas[i];

                bool colide = (x < p.x + p.w) && (x + largura > p.x) &&
                    (y < p.y + p.h) && (y + altura > p.y);

                if (colide) {
                    if (velocidadex > 0) {
                        x = p.x - largura;
                    }
                    else if (velocidadex < 0) {
                        x = p.x + p.w;
                    }
                }
            }

            if (x < 0) x = 0;
            if (x + largura > LARGURA_TELA) x = LARGURA_TELA - largura;

            pisandonochao = false;
            y += velocidadey;

            for (int i = 0; i < NUM_PLATAFORMAS; i++) {
                Plataforma p = plataformas[i];

                bool colide = (x < p.x + p.w) && (x + largura > p.x) &&
                    (y < p.y + p.h) && (y + altura > p.y);

                if (colide) {
                    if (velocidadey > 0) {
                        y = p.y - altura;
                        pisandonochao = true;
                    }

                    else if (velocidadey < 0) {
                        y = p.y + p.h;
                    }

                    velocidadey = 0;
                }
            }

            for (int i = 0; i < NUM_COLETAVEIS; i++) {
                if (!coletaveis[i].coletado && tocou_coletavel(coletaveis[i], x, y, largura, altura)) {
                    coletaveis[i].coletado = true;

                    coletados++;

                    snprintf(titulo, sizeof(titulo), "Era Quest - Coletaveis: %d", coletados);
                    al_set_window_title(display, titulo);
                }
            }

            redesenhar = true;
        }

        if (redesenhar && al_is_event_queue_empty(fila)) {
            redesenhar = false;

            al_clear_to_color(al_map_rgb(0, 0, 0));

            if (cenario) {
                al_draw_scaled_bitmap(cenario,
                    0, 0, al_get_bitmap_width(cenario), al_get_bitmap_height(cenario),
                    0, 0, LARGURA_TELA, ALTURA_TELA, 0);
            }

            for (int i = 0; i < NUM_PLATAFORMAS; i++) {
                desenhar_plataforma(plataformas[i]);
            }

            for (int i = 0; i < NUM_COLETAVEIS; i++) {
                if (!coletaveis[i].coletado) {
                    desenhar_coletavel(coletaveis[i]);
                }
            }

            if (nox) {
                float desenho_x;
                if (olharparaesquerda) {
                    desenho_x = x - (sprite_largura - off_x - hit_w);
                }
                else {
                    desenho_x = x - off_x;
                }
                float desenho_y = y - off_y;
                al_draw_bitmap(nox, desenho_x, desenho_y, olharparaesquerda ? ALLEGRO_FLIP_HORIZONTAL : 0);
            }

            al_flip_display();
        }
    }

    if (cenario) al_destroy_bitmap(cenario);
    if (nox) al_destroy_bitmap(nox);
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);
    al_destroy_display(display);

    return 0;
}