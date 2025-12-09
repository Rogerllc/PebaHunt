#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>



// Structs

//------------------------------------------------------------------------------------------------------------------------

int fps = 60;

int delay = 3;

bool fim = false;

int quantidade_pebas_total = 10;



typedef struct {

int niveis, fase_atual, pontuacao, combo, tempo_intervalo, tempo_restante, contador_inicio, tempo_delay_transicao;

bool pause, intervalo, contagem_inicio, reiniciar_hitbox, transicao, fim_fases;

} Jogo;



typedef struct {

int municao, quantidade_pebas, tempo, pebas_mortos;

} Fase;



enum Estado { MORTO, AGITADO };



typedef struct {

int x, y, com, alt, vx, vy, sorteio1, sorteio2, contador, lista_estado[2];

ALLEGRO_COLOR cor;

bool colisao;

enum Estado Estado_peba;



// Variaveis para animacao do sprite

int frame_atual;

int tempo_animacao;

int direcao; // 0 para direita, 1 para esquerda



// NOVO: Controle de visibilidade para a animacao de morte (piscar)

bool visivel;

} hitbox;



bool acerto = false;



typedef struct {

int x, y;

bool click;

} mouse;



typedef struct {

int x, y;

ALLEGRO_DISPLAY_MODE mode;

ALLEGRO_FONT* fonte_media;

} tela;

//------------------------------------------------------------------------------------------------------------------------



// Prototipos

//------------------------------------------------------------------------------------------------------------------------



void desenhar_mira(int cx, int cy, int tamanho);

void peba(hitbox* h, tela* t, mouse* m, Jogo* Informacao, Fase fases[], int fps, int delay, ALLEGRO_BITMAP* img);

void iniciar_hitbox_aleatoria(hitbox* h, int largura_tela, int altura_tela);

void desenhar_municao(int x_inicial, int y_inicial, int quantidade, int largura, int altura, int espacamento);

void inicializar_fases(Fase fases[], int total_fases);

void contagem_inicio(Jogo* jogo, tela* t, int fps);

void passar_fase(Jogo* jogo, Fase fases[], int fps, tela* t);

void desenhar_combo(Jogo* jogo, tela* t);

void desenhar_pontuacao(Jogo* jogo, tela* t);

void desenhar_nivel(Jogo* jogo, tela* t);

void mostrar_tela_final(tela* t);



//----------------------------------------------------------------------------------------------------------------------



int main() {



// Inicializa srand

srand(time(NULL));



if (!al_init()) {

printf("Erro de inicializacao");

return -1;

}



// Inicializar propriedades (Incluindo Image Addon!)

al_init_primitives_addon();

al_install_mouse();

al_install_keyboard();

al_init_font_addon();

al_init_image_addon();



// Carrega a imagem

ALLEGRO_BITMAP* sprite_peba = al_load_bitmap("tatu.png");

if (!sprite_peba) {

printf("Erro ao abrir imagem 'tatu.png'. Verifique se o arquivo esta na pasta do projeto.");

}



// Inicializacao padrao

tela tela_main;

mouse mouse_main;



// Vetores com tamanho fixo

hitbox inimigos[10];

Jogo Informacoes_jogo = { 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

Fase fases[10];



// Inicializar tela

al_get_display_mode(al_get_num_display_modes() - 1, &tela_main.mode);



tela_main.x = tela_main.mode.width - 100;

tela_main.y = 640;



for (int i = 0; i < quantidade_pebas_total; i++) {

iniciar_hitbox_aleatoria(&inimigos[i], tela_main.x, tela_main.y);

}



inicializar_fases(fases, Informacoes_jogo.niveis);



ALLEGRO_DISPLAY* display = al_create_display(tela_main.x, tela_main.y);

tela_main.fonte_media = al_create_builtin_font();



ALLEGRO_EVENT_QUEUE* fila_eventos = al_create_event_queue();

ALLEGRO_TIMER* timer = al_create_timer(1.0 / fps);



al_register_event_source(fila_eventos, al_get_display_event_source(display));

al_register_event_source(fila_eventos, al_get_mouse_event_source());

al_register_event_source(fila_eventos, al_get_keyboard_event_source());

al_register_event_source(fila_eventos, al_get_timer_event_source(timer));



al_start_timer(timer);



al_hide_mouse_cursor(display);



// Inicializacao de fases basica

Informacoes_jogo.fase_atual = 0;

Informacoes_jogo.tempo_restante = fases[Informacoes_jogo.fase_atual].tempo;

Informacoes_jogo.tempo_intervalo = 0;



Informacoes_jogo.contagem_inicio = false;



// Loop Principal

bool redraw = false;



while (!fim) {



ALLEGRO_EVENT ev;

al_wait_for_event(fila_eventos, &ev);



// Eventos

if (ev.type == ALLEGRO_EVENT_TIMER) {

redraw = true;

}

else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {

fim = true;

}

else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {

if (ev.mouse.button == 1) {

mouse_main.click = true;

acerto = false;

if (fases[Informacoes_jogo.fase_atual].municao != 0 && !Informacoes_jogo.pause) {

fases[Informacoes_jogo.fase_atual].municao -= 1;

}

}

}

else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {

mouse_main.x = ev.mouse.x;

mouse_main.y = ev.mouse.y;

}

else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {

if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {

Informacoes_jogo.pause = !Informacoes_jogo.pause;

}

if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE && Informacoes_jogo.fim_fases) {

Informacoes_jogo.fim_fases = false;

Informacoes_jogo.fase_atual = 0;

Informacoes_jogo.pontuacao = 0;

Informacoes_jogo.contagem_inicio = false;

Informacoes_jogo.contador_inicio = 0;

Informacoes_jogo.transicao = false;

Informacoes_jogo.pause = false;

inicializar_fases(fases, Informacoes_jogo.niveis);

Informacoes_jogo.tempo_restante = fases[0].tempo;

for (int i = 0; i < quantidade_pebas_total; i++) {

iniciar_hitbox_aleatoria(&inimigos[i], tela_main.x, tela_main.y);

}

redraw = false;

continue;

}

else if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE && Informacoes_jogo.fim_fases) {

al_destroy_display(display);

al_destroy_event_queue(fila_eventos);

al_destroy_timer(timer);

if (sprite_peba) al_destroy_bitmap(sprite_peba);

return 0;

}

}



// Desenho e Logica (Separado para performance)

if (redraw && al_is_event_queue_empty(fila_eventos)) {

redraw = false;

al_clear_to_color(al_map_rgb(255, 255, 255));



if (Informacoes_jogo.fim_fases) {

Informacoes_jogo.pause = true;

mostrar_tela_final(&tela_main);

}



if (Informacoes_jogo.reiniciar_hitbox) {

for (int i = 0; i < fases[Informacoes_jogo.fase_atual].quantidade_pebas; i++) {

iniciar_hitbox_aleatoria(&inimigos[i], tela_main.x, tela_main.y);

}

Informacoes_jogo.reiniciar_hitbox = false;

}



if (!Informacoes_jogo.fim_fases) {

for (int cont = 0; cont < fases[Informacoes_jogo.fase_atual].quantidade_pebas; cont++) {

peba(&inimigos[cont], &tela_main, &mouse_main, &Informacoes_jogo, fases, fps, delay, sprite_peba);

}

passar_fase(&Informacoes_jogo, fases, fps, &tela_main);

desenhar_municao(20, tela_main.y - 50, fases[Informacoes_jogo.fase_atual].municao, 10, 30, 20);

desenhar_mira(mouse_main.x, mouse_main.y, 20);

contagem_inicio(&Informacoes_jogo, &tela_main, fps);

desenhar_pontuacao(&Informacoes_jogo, &tela_main);

desenhar_nivel(&Informacoes_jogo, &tela_main);

}



al_flip_display();

mouse_main.click = false;

}

}



// Limpeza final

al_destroy_display(display);

al_destroy_event_queue(fila_eventos);

al_destroy_timer(timer);

if (sprite_peba) al_destroy_bitmap(sprite_peba);



return 0;

}



// Implementacao das funcoes

void iniciar_hitbox_aleatoria(hitbox* h, int largura_tela, int altura_tela) {

// Tamanho do inimigo (ajuste conforme seu sprite)

h->com = 95;

h->alt = 60;



h->x = rand() % (largura_tela - h->com);

h->y = rand() % (altura_tela - h->alt);



h->vx = 1;

h->vy = 1;

h->sorteio1 = 0;

h->sorteio2 = 0;

h->lista_estado[MORTO] = 0; // Inicializado como 0 (Falso)

h->lista_estado[AGITADO] = 0; // Inicializado como 0 (Falso)

h->colisao = false;

//h->cor = al_map_rgb(0, 0, 0);



h->frame_atual = 0;

h->tempo_animacao = 0;

h->direcao = 0;

h->visivel = true; // NOVO: Inicia visível

}



void desenhar_mira(int cx, int cy, int tamanho) {

int half = tamanho / 2;

al_draw_line(cx - half, cy, cx - half / 3, cy, al_map_rgb(255, 0, 0), 3);

al_draw_line(cx + half / 3, cy, cx + half, cy, al_map_rgb(255, 0, 0), 3);

al_draw_line(cx, cy - half, cx, cy - half / 3, al_map_rgb(255, 0, 0), 3);

al_draw_line(cx, cy + half / 3, cx, cy + half, al_map_rgb(255, 0, 0), 3);

al_draw_filled_circle(cx, cy, tamanho * 0.07, al_map_rgb(0, 0, 0));

}



void peba(hitbox* h, tela* t, mouse* m, Jogo* Informacao, Fase fases[], int fps, int delay, ALLEGRO_BITMAP* img) {

if (!Informacao->pause) {

if (h->lista_estado[MORTO] == 0) {

// Lógica de Movimento e Animação de Peba Vivo

h->contador++;



if ((h->contador) / fps >= delay) {

int s1 = -10 + rand() % 31;

int s2 = -10 + rand() % 31;



if ((s1 >= -10 && s1 <= -5) || (s1 >= 5 && s1 <= 10)) h->vx = s1;

if ((s2 >= -10 && s2 <= -5) || (s2 >= 5 && s2 <= 10)) h->vy = s2;



h->contador = 0;

}



if (h->vx > 0) h->direcao = 0;

if (h->vx < 0) h->direcao = ALLEGRO_FLIP_HORIZONTAL;



h->tempo_animacao++;

if (h->tempo_animacao >= 10) {

h->frame_atual++;

if (h->frame_atual >= 3) h->frame_atual = 0;

h->tempo_animacao = 0;

}



if (h->x >= t->x - h->com) h->vx = -h->vx;

else if (h->x <= 0) h->vx = -h->vx;



if (h->y >= t->y - h->alt) h->vy = -h->vy;

else if (h->y <= 0) h->vy = -h->vy;



h->x += h->vx;

h->y += h->vy;



h->colisao = m->x >= h->x && m->x <= h->x + h->com && m->y >= h->y && m->y <= h->y + h->alt;

}



// Lógica de Hit/Morte

if (h->colisao && m->click && fases[Informacao->fase_atual].municao >= 0 && h->lista_estado[MORTO] == 0) {

h->lista_estado[MORTO] = 1;

h->contador = 0; // Contador reinicia para controle da animacao de morte

h->colisao = false;

Informacao->pontuacao += 100;

fases[Informacao->fase_atual].pebas_mortos++;

}



// Lógica de Animação de Morte (Piscar por 2 segundos)

if (h->lista_estado[MORTO] == 1) {

h->contador++;



// Controle do pisca-pisca (a cada 5 frames, inverte a visibilidade)

if (h->contador % 5 == 0) {

h->visivel = !h->visivel;

}



// Tempo total da animacao de morte (2 segundos * fps)

if (h->contador >= fps * 2) {

// Desaparece ao fim da animacao

h->x = -300;

h->y = -300;

h->visivel = false;

}

}

}



// Desenho

if (h->lista_estado[MORTO] == 0 && img) {

// Desenho do Peba Vivo

int largura_sprite_peba = 110;

int altura_sprite_peba = 75;



al_draw_bitmap_region(

img,

h->frame_atual * largura_sprite_peba, 0,

largura_sprite_peba, altura_sprite_peba,

h->x, h->y,

h->direcao

);

}

else if (h->lista_estado[MORTO] == 1 && img && h->visivel) {

// Desenho do Peba MORTO (de ponta-cabeça e piscando)

int largura_sprite_peba = 110;

int altura_sprite_peba = 75;



// Define a flag para virar de ponta-cabeça (ALLEGRO_FLIP_VERTICAL)

int flip_flags = ALLEGRO_FLIP_VERTICAL;



// Mantém a direção horizontal original (0 ou ALLEGRO_FLIP_HORIZONTAL)

if (h->direcao == ALLEGRO_FLIP_HORIZONTAL) {

flip_flags |= ALLEGRO_FLIP_HORIZONTAL;

}



al_draw_bitmap_region(

img,

h->frame_atual * largura_sprite_peba, 0,

largura_sprite_peba, altura_sprite_peba,

h->x, h->y,

flip_flags

);

}

else if (h->lista_estado[MORTO] == 0) {

// Desenho do Rect se o sprite nao carregar (Peba Vivo)

al_draw_rectangle(h->x, h->y, h->x + h->com, h->y + h->alt, h->cor, 2);

}



if (h->colisao) {

al_draw_filled_rectangle(h->x, h->y, h->x + h->com, h->y + h->alt, h->cor);

}

}



void desenhar_municao(int x_inicial, int y_inicial, int quantidade, int largura, int altura, int espacamento) {

for (int i = 0; i < quantidade; i++) {

int x = x_inicial + i * (largura + espacamento);

int y = y_inicial;

al_draw_filled_rounded_rectangle(x, y + altura * 0.2, x + largura, y + altura, largura * 0.2, altura * 0.2, al_map_rgb(200, 0, 0));

al_draw_filled_circle(x + largura / 2, y + altura * 0.2, largura / 2, al_map_rgb(192, 192, 192));

al_draw_rounded_rectangle(x, y + altura * 0.2, x + largura, y + altura, largura * 0.2, altura * 0.2, al_map_rgb(0, 0, 0), 2);

al_draw_circle(x + largura / 2, y + altura * 0.2, largura / 2, al_map_rgb(0, 0, 0), 2);

}

}



void inicializar_fases(Fase fases[], int total_fases) {

for (int i = 0; i < total_fases; i++) {

int pebas = 2 + (int)(8.0 * i / (total_fases - 1));

fases[i].quantidade_pebas = pebas;

fases[i].municao = pebas + 2;

fases[i].tempo = (pebas / 2) * 15;

fases[i].pebas_mortos = 0;

}

}



void contagem_inicio(Jogo* jogo, tela* t, int fps) {

if (jogo->contagem_inicio) return;

jogo->pause = true;

jogo->contador_inicio++;

int tempo = jogo->contador_inicio / fps;

switch (tempo) {

case 0: al_draw_text(t->fonte_media, al_map_rgb(0, 0, 0), t->x / 2, t->y / 2, ALLEGRO_ALIGN_CENTRE, "3"); break;

case 1: al_draw_text(t->fonte_media, al_map_rgb(0, 0, 0), t->x / 2, t->y / 2, ALLEGRO_ALIGN_CENTRE, "2"); break;

case 2: al_draw_text(t->fonte_media, al_map_rgb(0, 0, 0), t->x / 2, t->y / 2, ALLEGRO_ALIGN_CENTRE, "1"); break;

case 3: al_draw_text(t->fonte_media, al_map_rgb(0, 0, 0), t->x / 2, t->y / 2, ALLEGRO_ALIGN_CENTRE, "PEGUE O PEBA DOIDO!!!"); break;

default: jogo->pause = false; jogo->contagem_inicio = true; break;

}

}



void passar_fase(Jogo* jogo, Fase fases[], int fps, tela* t) {

if (!jogo->contagem_inicio) return;

if (!jogo->pause) jogo->tempo_intervalo++;

if (jogo->fim_fases) return;



if (jogo->tempo_intervalo >= fps) {

jogo->tempo_restante--;

jogo->tempo_intervalo = 0;

}



al_draw_textf(t->fonte_media, al_map_rgb(0, 0, 0), t->x - 100, 20, 0, "Tempo: %d", jogo->tempo_restante);



if (!jogo->transicao && (jogo->tempo_restante <= 0 || fases[jogo->fase_atual].municao <= 0 || fases[jogo->fase_atual].pebas_mortos == fases[jogo->fase_atual].quantidade_pebas)) {

jogo->transicao = true;

jogo->pause = true;

}



if (jogo->transicao) {

jogo->tempo_delay_transicao++;

if (fases[jogo->fase_atual].pebas_mortos == fases[jogo->fase_atual].quantidade_pebas) {

al_draw_text(t->fonte_media, al_map_rgb(0, 0, 0), t->x / 2, t->y / 2, ALLEGRO_ALIGN_CENTRE, "CONSEGUI MATAR TODOS OS PEBAS");

}

else if (jogo->tempo_restante <= 0) {

al_draw_text(t->fonte_media, al_map_rgb(0, 0, 0), t->x / 2, t->y / 2, ALLEGRO_ALIGN_CENTRE, "O IBAMA VAI CHEGAR!!!");

}

else if (fases[jogo->fase_atual].municao <= 0) {

al_draw_text(t->fonte_media, al_map_rgb(0, 0, 0), t->x / 2, t->y / 2, ALLEGRO_ALIGN_CENTRE, "ACABOU OS CARTUCHOS");

}



if (jogo->tempo_delay_transicao >= fps * 2) {

if (fases[jogo->fase_atual].pebas_mortos == fases[jogo->fase_atual].quantidade_pebas) {

jogo->pontuacao += jogo->tempo_restante * 10;

}

if (fases[jogo->fase_atual].municao > 0) {

jogo->pontuacao += fases[jogo->fase_atual].municao * 50;

}

jogo->fase_atual++;

if (jogo->fase_atual >= jogo->niveis) {

jogo->fim_fases = true;

jogo->fase_atual = 0;

return;

}

if (!jogo->fim_fases) {

jogo->tempo_restante = fases[jogo->fase_atual].tempo;

jogo->contagem_inicio = false;

jogo->contador_inicio = 0;

jogo->tempo_delay_transicao = 0;

jogo->transicao = false;

jogo->reiniciar_hitbox = true;

}

}

}

}



void desenhar_combo(Jogo* jogo, tela* t) {

if (jogo->combo <= 1) return;

ALLEGRO_COLOR cor = al_map_rgb(255, 0, 0);

al_draw_textf(t->fonte_media, cor, 10, 10, 0, "COMBO: %d", jogo->combo);

}



void desenhar_pontuacao(Jogo* jogo, tela* t) {

al_draw_textf(t->fonte_media, al_map_rgb(0, 0, 0), 20, 20, 0, "Pontos: %d", jogo->pontuacao);

}



void desenhar_nivel(Jogo* jogo, tela* t) {

al_draw_textf(t->fonte_media, al_map_rgb(0, 0, 0), 20, 40, 0, "Nivel: %d", jogo->fase_atual + 1);

}



void mostrar_tela_final(tela* t) {

al_draw_filled_rectangle(0, 0, t->x, t->y, al_map_rgba(0, 0, 0, 150));

int cx = t->x / 2;

int cy = t->y / 2;

al_draw_text(t->fonte_media, al_map_rgb(255, 255, 255), cx, cy - 40, ALLEGRO_ALIGN_CENTRE, "FIM DAS FAS ES!");

al_draw_text(t->fonte_media, al_map_rgb(255, 255, 255), cx, cy + 10, ALLEGRO_ALIGN_CENTRE, "Pressione ESPACO para jogar de novo");

al_draw_text(t->fonte_media, al_map_rgb(255, 255, 255), cx, cy + 40, ALLEGRO_ALIGN_CENTRE, "Pressione BACKSPACE para sair");

}