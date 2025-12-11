#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// --- DEFINIÇÕES GERAIS ---
#define LARGURA_TELA 800
#define ALTURA_TELA 600
#define LARGURA_SPRITE 110
#define ALTURA_SPRITE 75
#define ARQUIVO_RANKING "ranking.txt"

typedef struct No {
    int id_inimigo; 
    struct No* prox;
} No;

typedef struct {
    No* topo;
} Pilha;

void inicializar_pilha(Pilha* p) {
    p->topo = NULL;
}

void push(Pilha* p, int valor) {
    No* novo = (No*)malloc(sizeof(No));
    if (novo) {
        novo->id_inimigo = valor;
        novo->prox = p->topo;
        p->topo = novo;
    }
}

void limpar_pilha(Pilha* p) {
    while (p->topo != NULL) {
        No* temp = p->topo;
        p->topo = temp->prox;
        free(temp);
    }
}

// --- ESTRUTURAS DO MENU ---
enum EstadoGeral {
    MENU,
    CREDITOS,
    JOGO
};

typedef struct {
    int x, y;
    int w, h;
    ALLEGRO_BITMAP* img;
    char id[20];
    int mouse_hover;
} Botao;

typedef struct {
    ALLEGRO_FONT* titulo;
    ALLEGRO_FONT* texto;
    ALLEGRO_FONT* menor;
    ALLEGRO_SAMPLE* musica;
} Assets;

// --- ESTRUTURAS DA LÓGICA DO JOGO ---
enum EstadoPeba { MORTO, AGITADO };

typedef struct {
    int niveis, fase_atual, pontuacao, combo, tempo_intervalo, tempo_restante, contador_inicio, tempo_delay_transicao;
    bool pause, intervalo, contagem_inicio, reiniciar_hitbox, transicao, fim_fases;
    bool ranking_processado; 
} JogoDados;

typedef struct {
    int municao, quantidade_pebas, tempo, pebas_mortos;
} Fase;

typedef struct {
    int x, y, com, alt, vx, vy, sorteio1, sorteio2, contador, lista_estado[2];
    ALLEGRO_COLOR cor;
    bool colisao;
    enum EstadoPeba Estado_peba;
    int frame_atual;
    int tempo_animacao;
    int direcao;
    bool visivel;
} hitbox;

typedef struct {
    int x, y;
    bool click;
} mouse_game;

// --- PROTÓTIPOS ---
int inicializar_allegro();
Botao* criar_botao(const char* path, int x, int y, const char* id_str);
void destruir_botao(Botao* bt);
int verificar_colisao_btn(Botao* bt, int mx, int my);
void desenhar_botao(Botao* bt);

int busca_linear_clique_botao(Botao** botoes, int qtd, int mx, int my);

void processar_ranking(int pontuacao_atual, int* top_scores);
void ordenar_bubble_sort(int* vetor, int tamanho);

void iniciar_hitbox_aleatoria(hitbox* h, int largura_tela, int altura_tela);
void inicializar_fases(Fase fases[], int total_fases);
void peba(hitbox* h, mouse_game* m, JogoDados* Informacao, Fase fases[], int fps, int delay, ALLEGRO_BITMAP* img, Pilha* historico_mortes, int indice_inimigo);
void desenhar_municao(int x_inicial, int y_inicial, int quantidade, int largura, int altura, int espacamento);
void desenhar_mira(int cx, int cy, int tamanho);
void contagem_inicio(JogoDados* jogo, ALLEGRO_FONT* fonte, int w, int h, int fps);
void passar_fase(JogoDados* jogo, Fase fases[], int fps, ALLEGRO_FONT* fonte, int w, int h);
void desenhar_pontuacao(JogoDados* jogo, ALLEGRO_FONT* fonte);
void desenhar_nivel(JogoDados* jogo, ALLEGRO_FONT* fonte);
void mostrar_tela_final(int w, int h, ALLEGRO_FONT* fonte, int* top_scores, int pontuacao_atual);

// --- MAIN ---
int main() {
    srand(time(NULL));

    Pilha historico_mortes;
    inicializar_pilha(&historico_mortes);

    int top_scores[5] = {0, 0, 0, 0, 0};

    if (!inicializar_allegro()) {
        printf("Falha ao inicializar Allegro.\n");
        return -1;
    }

    ALLEGRO_DISPLAY* janela = al_create_display(LARGURA_TELA, ALTURA_TELA);
    if (!janela) { printf("Falha ao criar janela.\n"); return -1; }

    char titulo_janela[50] = "Projeto Peba Hunt";
    char versao[20] = " - v1.0 Final";
    strcat(titulo_janela, versao);
    al_set_window_title(janela, titulo_janela);

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* fila_eventos = al_create_event_queue();

    // Carregando Assets
    Assets assets;
    assets.titulo = al_load_font("fonte.ttf", 20, 0);
    assets.texto = al_load_font("fonte.ttf", 28, 0);
    assets.menor = al_load_font("fonte.ttf", 18, 0);

    if (!assets.texto) {
        printf("Aviso: fonte.ttf nao encontrada. Usando padrao.\n");
        assets.titulo = al_create_builtin_font();
        assets.texto = al_create_builtin_font();
        assets.menor = al_create_builtin_font();
    }

    ALLEGRO_BITMAP* bg_menu = al_load_bitmap("menu_sertao.png");
    ALLEGRO_BITMAP* bg_jogo = al_load_bitmap("backgound_1.png");
    ALLEGRO_BITMAP* logo = al_load_bitmap("peba_hunt.png");
    ALLEGRO_BITMAP* placa = al_load_bitmap("placa.png");
    ALLEGRO_BITMAP* sprite_peba = al_load_bitmap("tatu.png");

    if (!sprite_peba) printf("AVISO: tatu.png nao encontrado.\n");

    ALLEGRO_SAMPLE_ID id_msc;
    assets.musica = al_load_sample("musica.ogg");
    if (assets.musica) al_play_sample(assets.musica, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &id_msc);

    // --- BOTÕES DO MENU ---
    int qtd_btns_menu = 3;
    Botao** btns_menu = (Botao**)malloc(qtd_btns_menu * sizeof(Botao*));

    btns_menu[0] = criar_botao("botao_novoJogo.png", 300, 245, "NOVO");
    btns_menu[1] = criar_botao("botao_creditos.png", 300, 348, "CREDITOS");
    btns_menu[2] = criar_botao("botao_sair.png", 300, 453, "SAIR");

    Botao* btn_continuar = criar_botao("botao_continuar.png", 0, 250, "CONT");
    Botao* btn_voltar = criar_botao("botao_menu.png", 0, 380, "VOLT");

    if (btn_continuar->img) btn_continuar->x = (LARGURA_TELA - btn_continuar->w) / 2;
    if (btn_voltar->img) btn_voltar->x = (LARGURA_TELA - btn_voltar->w) / 2;

    // --- CONFIGURAÇÃO DO JOGO ---
    int fps = 60;
    int delay_anim = 3;
    hitbox inimigos[10];
    JogoDados info_jogo = { 10, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, 0, 0, 0, false };
    Fase fases[10];
    mouse_game mouse_logic;
    mouse_logic.click = false;

    inicializar_fases(fases, info_jogo.niveis);
    info_jogo.tempo_restante = fases[0].tempo;

    for (int i = 0; i < 10; i++) iniciar_hitbox_aleatoria(&inimigos[i], LARGURA_TELA, ALTURA_TELA);

    al_register_event_source(fila_eventos, al_get_display_event_source(janela));
    al_register_event_source(fila_eventos, al_get_timer_event_source(timer));
    al_register_event_source(fila_eventos, al_get_keyboard_event_source());
    al_register_event_source(fila_eventos, al_get_mouse_event_source());

    al_start_timer(timer);

    int rodando = 1;
    int estado_atual = MENU;
    int desenhar = 1;

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila_eventos, &ev);

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            desenhar = 1;
        }
        else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = 0;
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                if (estado_atual == CREDITOS) estado_atual = MENU;
                else if (estado_atual == JOGO) info_jogo.pause = !info_jogo.pause;
                else rodando = 0;
            }
            if (estado_atual == JOGO && ev.keyboard.keycode == ALLEGRO_KEY_SPACE && info_jogo.fim_fases) {
                info_jogo.fim_fases = false;
                info_jogo.fase_atual = 0;
                info_jogo.pontuacao = 0;
                info_jogo.contagem_inicio = false;
                info_jogo.contador_inicio = 0;
                info_jogo.transicao = false;
                info_jogo.pause = false;
                info_jogo.ranking_processado = false; 
                limpar_pilha(&historico_mortes); 
                inicializar_fases(fases, info_jogo.niveis);
                info_jogo.tempo_restante = fases[0].tempo;
                for (int i = 0; i < 10; i++) iniciar_hitbox_aleatoria(&inimigos[i], LARGURA_TELA, ALTURA_TELA);
            }
            else if (estado_atual == JOGO && ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE && info_jogo.fim_fases) {
                estado_atual = MENU;
                limpar_pilha(&historico_mortes);
            }
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
            mouse_logic.x = ev.mouse.x;
            mouse_logic.y = ev.mouse.y;

            if (estado_atual == MENU) {
                for (int i = 0; i < qtd_btns_menu; i++) {
                    btns_menu[i]->mouse_hover = verificar_colisao_btn(btns_menu[i], ev.mouse.x, ev.mouse.y);
                }
            }
            else if (estado_atual == JOGO && info_jogo.pause) {
                btn_continuar->mouse_hover = verificar_colisao_btn(btn_continuar, ev.mouse.x, ev.mouse.y);
                btn_voltar->mouse_hover = verificar_colisao_btn(btn_voltar, ev.mouse.x, ev.mouse.y);
            }
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            if (ev.mouse.button == 1) {
                if (estado_atual == MENU) {
                    
                    int indice_clicado = busca_linear_clique_botao(btns_menu, qtd_btns_menu, ev.mouse.x, ev.mouse.y);

                    if (indice_clicado != -1) {
                        if (strcmp(btns_menu[indice_clicado]->id, "NOVO") == 0) {
                            estado_atual = JOGO;
                            info_jogo.pause = false;
                            info_jogo.fase_atual = 0;
                            info_jogo.pontuacao = 0;
                            info_jogo.contagem_inicio = false;
                            info_jogo.contador_inicio = 0;
                            info_jogo.ranking_processado = false;
                            limpar_pilha(&historico_mortes);
                            inicializar_fases(fases, info_jogo.niveis);
                            info_jogo.tempo_restante = fases[0].tempo;
                            for (int k = 0; k < 10; k++) iniciar_hitbox_aleatoria(&inimigos[k], LARGURA_TELA, ALTURA_TELA);
                        }
                        else if (strcmp(btns_menu[indice_clicado]->id, "CREDITOS") == 0) estado_atual = CREDITOS;
                        else if (strcmp(btns_menu[indice_clicado]->id, "SAIR") == 0) rodando = 0;
                    }
                }
                else if (estado_atual == JOGO) {
                    if (info_jogo.pause && !info_jogo.fim_fases && !info_jogo.transicao) {
                        if (btn_continuar->mouse_hover) info_jogo.pause = false;
                        if (btn_voltar->mouse_hover) { info_jogo.pause = false; estado_atual = MENU; limpar_pilha(&historico_mortes); }
                    }
                    else if (!info_jogo.pause) {
                        if (fases[info_jogo.fase_atual].municao > 0) {
                            fases[info_jogo.fase_atual].municao -= 1;
                            mouse_logic.click = true;
                        }
                    }
                }
            }
        }

        if (desenhar && al_is_event_queue_empty(fila_eventos)) {
            desenhar = 0;
            al_clear_to_color(al_map_rgb(0, 0, 0));

            if (estado_atual == JOGO && !info_jogo.pause && !info_jogo.fim_fases) {
                al_hide_mouse_cursor(janela);
            }
            else {
                al_show_mouse_cursor(janela);
            }

            if (estado_atual == MENU) {
                if (bg_menu) al_draw_bitmap(bg_menu, -115, 0, 0);
                if (logo) al_draw_scaled_bitmap(logo, 0, 0, al_get_bitmap_width(logo), al_get_bitmap_height(logo), (LARGURA_TELA - 450) / 2, 20, 450, 260, 0);
                for (int i = 0; i < qtd_btns_menu; i++) desenhar_botao(btns_menu[i]);
            }
            else if (estado_atual == JOGO) {
                if (bg_jogo) al_draw_scaled_bitmap(bg_jogo, 0, 0, al_get_bitmap_width(bg_jogo), al_get_bitmap_height(bg_jogo), 0, 0, 800, 600, 0);
                else al_clear_to_color(al_map_rgb(255, 255, 255));

                if (info_jogo.fim_fases) {
                    info_jogo.pause = true;
                    if (!info_jogo.ranking_processado) {
                        processar_ranking(info_jogo.pontuacao, top_scores);
                        info_jogo.ranking_processado = true;
                    }
                    mostrar_tela_final(LARGURA_TELA, ALTURA_TELA, assets.texto, top_scores, info_jogo.pontuacao);
                }

                if (info_jogo.reiniciar_hitbox) {
                    for (int i = 0; i < fases[info_jogo.fase_atual].quantidade_pebas; i++) {
                        iniciar_hitbox_aleatoria(&inimigos[i], LARGURA_TELA, ALTURA_TELA);
                    }
                    info_jogo.reiniciar_hitbox = false;
                }

                if (!info_jogo.fim_fases) {
                    for (int cont = 0; cont < fases[info_jogo.fase_atual].quantidade_pebas; cont++) {
                        peba(&inimigos[cont], &mouse_logic, &info_jogo, fases, fps, delay_anim, sprite_peba, &historico_mortes, cont);
                    }
                    passar_fase(&info_jogo, fases, fps, assets.texto, LARGURA_TELA, ALTURA_TELA);
                    desenhar_municao(20, ALTURA_TELA - 50, fases[info_jogo.fase_atual].municao, 10, 30, 20);

                    contagem_inicio(&info_jogo, assets.texto, LARGURA_TELA, ALTURA_TELA, fps);
                    desenhar_pontuacao(&info_jogo, assets.texto);
                    desenhar_nivel(&info_jogo, assets.texto);

                    if (!info_jogo.pause) {
                        desenhar_mira(mouse_logic.x, mouse_logic.y, 20);
                    }
                }

                if (info_jogo.pause && !info_jogo.fim_fases && !info_jogo.transicao && info_jogo.contagem_inicio) {
                    al_draw_filled_rectangle(0, 0, LARGURA_TELA, ALTURA_TELA, al_map_rgba(0, 0, 0, 160));
                    al_draw_text(assets.texto, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 150, ALLEGRO_ALIGN_CENTER, "PAUSADO");
                    desenhar_botao(btn_continuar);
                    desenhar_botao(btn_voltar);
                }
            }
            else if (estado_atual == CREDITOS) {
                al_draw_filled_rectangle(0, 0, LARGURA_TELA, ALTURA_TELA, al_map_rgba(0, 0, 0, 180));
                if (placa) al_draw_scaled_bitmap(placa, 0, 0, al_get_bitmap_width(placa), al_get_bitmap_height(placa), 50, 20, 700, 560, 0);

                char devs[4][50] = { "Gustavo Morais - Sprites", "Wagner Junior - Sprites", "Luan Jefferson - Logica", "Roger Leite - UI" };
                ALLEGRO_COLOR cor_txt = al_map_rgb(255, 255, 255);
                ALLEGRO_COLOR cor_info = al_map_rgb(220, 220, 200);

                int y_inicial = 75; int x_esq = 140; int x_centro = 400;

                int y_devs = y_inicial + 60;
                al_draw_text(assets.texto, cor_txt, x_esq + 65, y_devs + 45, ALLEGRO_ALIGN_LEFT, "Desenvolvedores:");
                for (int i = 0; i < 4; i++) { al_draw_text(assets.menor, cor_txt, x_esq + 75, y_devs + 70 + (i * 25), ALLEGRO_ALIGN_LEFT, devs[i]); }

                int y_cadeira = y_devs + 150;
                al_draw_text(assets.texto, cor_txt, x_esq + 65, y_cadeira + 40, ALLEGRO_ALIGN_LEFT, "Projeto da cadeira:");
                al_draw_text(assets.menor, cor_txt, x_esq + 75, y_cadeira + 75, ALLEGRO_ALIGN_LEFT, "Algoritmos e Estrutura de Dados I");
                al_draw_text(assets.menor, cor_txt, x_esq + 75, y_cadeira + 95, ALLEGRO_ALIGN_LEFT, "Tecnologia da Informacao 2025.1");

                int y_info = y_cadeira + 100;
                al_draw_text(assets.texto, cor_info, x_esq + 65, y_info + 35, ALLEGRO_ALIGN_LEFT, "Info:");
                al_draw_text(assets.menor, cor_info, x_esq + 55, y_info + 75, ALLEGRO_ALIGN_LEFT, "Certificamos que e apenas um projeto educacional e que");
                al_draw_text(assets.menor, cor_info, x_esq + 55, y_info + 85, ALLEGRO_ALIGN_LEFT, "nao influenciamos a caca de devidos animais no jogo.");
                al_draw_text(assets.menor, cor_info, x_esq + 55, y_info + 105, ALLEGRO_ALIGN_LEFT, "Jogo meramente ficticio.");
                al_draw_text(assets.menor, al_map_rgb(200, 200, 200), 400, 550, ALLEGRO_ALIGN_CENTER, "[ ESC para Voltar ]");
            }

            al_flip_display();
            mouse_logic.click = false;
        }
    }

    // --- LIMPEZA ---
    limpar_pilha(&historico_mortes);
    for (int i = 0; i < qtd_btns_menu; i++) destruir_botao(btns_menu[i]);
    free(btns_menu);
    destruir_botao(btn_continuar);
    destruir_botao(btn_voltar);

    if (bg_menu) al_destroy_bitmap(bg_menu);
    if (bg_jogo) al_destroy_bitmap(bg_jogo);
    if (logo) al_destroy_bitmap(logo);
    if (placa) al_destroy_bitmap(placa);
    if (sprite_peba) al_destroy_bitmap(sprite_peba);
    if (assets.musica) al_destroy_sample(assets.musica);

    al_destroy_font(assets.titulo);
    al_destroy_font(assets.texto);
    al_destroy_font(assets.menor);
    al_destroy_display(janela);
    al_destroy_event_queue(fila_eventos);
    al_destroy_timer(timer);

    return 0;
}

// --- FUNÇÕES AUXILIARES ---
int inicializar_allegro() {
    if (!al_init()) return 0;
    al_init_image_addon();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    if (!al_install_audio()) { printf("Falha audio.\n"); return 0; }
    if (!al_init_acodec_addon()) { printf("Falha acodec.\n"); return 0; }
    al_reserve_samples(5);
    al_install_keyboard();
    al_install_mouse();
    return 1;
}

Botao* criar_botao(const char* path, int x, int y, const char* id_str) {
    Botao* bt = (Botao*)malloc(sizeof(Botao));
    if (!bt) return NULL;
    bt->img = al_load_bitmap(path);
    if (bt->img) {
        bt->w = al_get_bitmap_width(bt->img);
        bt->h = al_get_bitmap_height(bt->img);
    }
    else {
        bt->w = 200; bt->h = 50;
    }
    bt->x = x; bt->y = y;
    strcpy(bt->id, id_str);
    bt->mouse_hover = 0;
    return bt;
}

void destruir_botao(Botao* bt) {
    if (bt) {
        if (bt->img) al_destroy_bitmap(bt->img);
        free(bt);
    }
}

int verificar_colisao_btn(Botao* bt, int mx, int my) {
    if (!bt) return 0;
    int margem_w = 15;
    int margem_h = 30;

    if (mx >= bt->x + margem_w && mx <= bt->x + bt->w - margem_w &&
        my >= bt->y + margem_h && my <= bt->y + bt->h - margem_h) {
        return 1;
    }
    return 0;
}

int busca_linear_clique_botao(Botao** botoes, int qtd, int mx, int my) {
    for (int i = 0; i < qtd; i++) {
        if (verificar_colisao_btn(botoes[i], mx, my)) {
            return i; 
        }
    }
    return -1; 
}

void desenhar_botao(Botao* bt) {
    if (!bt || !bt->img) return;
    int offset_y = bt->mouse_hover ? 5 : 0;
    al_draw_bitmap(bt->img, bt->x, bt->y - offset_y, 0);
}

void ordenar_bubble_sort(int* vetor, int tamanho) {
    for (int i = 0; i < tamanho - 1; i++) {
        for (int j = 0; j < tamanho - i - 1; j++) {
            if (vetor[j] < vetor[j + 1]) { 
                int temp = vetor[j];
                vetor[j] = vetor[j + 1];
                vetor[j + 1] = temp;
            }
        }
    }
}

void processar_ranking(int pontuacao_atual, int* top_scores) {
    FILE* arquivo = fopen(ARQUIVO_RANKING, "r");
    int i;
 
    for(i=0; i<5; i++) top_scores[i] = 0;

    if (arquivo) {
        for (i = 0; i < 5; i++) {
            if (fscanf(arquivo, "%d", &top_scores[i]) != 1) {
                break;
            }
        }
        fclose(arquivo);
    }

    if (pontuacao_atual > top_scores[4]) {
        top_scores[4] = pontuacao_atual;

        ordenar_bubble_sort(top_scores, 5);

        arquivo = fopen(ARQUIVO_RANKING, "w");
        if (arquivo) {
            for (i = 0; i < 5; i++) {
                fprintf(arquivo, "%d\n", top_scores[i]);
            }
            fclose(arquivo);
        }
    } else {
        ordenar_bubble_sort(top_scores, 5);
    }
}

// --- LÓGICA DO JOGO ---

void iniciar_hitbox_aleatoria(hitbox* h, int largura_tela, int altura_tela) {
    h->com = LARGURA_SPRITE; h->alt = ALTURA_SPRITE;
    h->x = rand() % (largura_tela - h->com);
    h->y = rand() % (altura_tela - h->alt);
    h->vx = 1; h->vy = 1;
    h->lista_estado[MORTO] = 0;
    h->colisao = false;
    h->cor = al_map_rgb(139, 69, 19);
    h->frame_atual = 0; h->tempo_animacao = 0; h->direcao = 0; h->visivel = true;
}

void desenhar_mira(int cx, int cy, int tamanho) {
    int half = tamanho / 2;
    al_draw_line(cx - half, cy, cx - half / 3, cy, al_map_rgb(255, 0, 0), 3);
    al_draw_line(cx + half / 3, cy, cx + half, cy, al_map_rgb(255, 0, 0), 3);
    al_draw_line(cx, cy - half, cx, cy - half / 3, al_map_rgb(255, 0, 0), 3);
    al_draw_line(cx, cy + half / 3, cx, cy + half, al_map_rgb(255, 0, 0), 3);
    al_draw_filled_circle(cx, cy, tamanho * 0.07, al_map_rgb(0, 0, 0));
}

void peba(hitbox* h, mouse_game* m, JogoDados* Informacao, Fase fases[], int fps, int delay, ALLEGRO_BITMAP* img, Pilha* historico, int indice) {
    if (!Informacao->pause) {
        if (h->lista_estado[MORTO] == 0) {
            h->contador++;
            if ((h->contador) / fps >= delay) {
                int s1 = -10 + rand() % 31; int s2 = -10 + rand() % 31;
                if ((s1 >= -10 && s1 <= -5) || (s1 >= 5 && s1 <= 10)) h->vx = s1;
                if ((s2 >= -10 && s2 <= -5) || (s2 >= 5 && s2 <= 10)) h->vy = s2;
                h->contador = 0;
            }
            if (h->vx > 0) h->direcao = 0;
            if (h->vx < 0) h->direcao = ALLEGRO_FLIP_HORIZONTAL;
            h->tempo_animacao++;
            if (h->tempo_animacao >= 10) {
                h->frame_atual++; if (h->frame_atual >= 3) h->frame_atual = 0;
                h->tempo_animacao = 0;
            }
            if (h->x >= LARGURA_TELA - h->com) h->vx = -h->vx; else if (h->x <= 0) h->vx = -h->vx;
            if (h->y >= ALTURA_TELA - h->alt) h->vy = -h->vy; else if (h->y <= 0) h->vy = -h->vy;
            h->x += h->vx; h->y += h->vy;
            h->colisao = m->x >= h->x && m->x <= h->x + h->com && m->y >= h->y && m->y <= h->y + h->alt;
        }

        if (h->colisao && m->click && h->lista_estado[MORTO] == 0) {
            h->lista_estado[MORTO] = 1; h->contador = 0; h->colisao = false;
            Informacao->pontuacao += 100;
            fases[Informacao->fase_atual].pebas_mortos++;
            
            push(historico, indice);
        }

        if (h->lista_estado[MORTO] == 1) {
            h->contador++;
            if (h->contador % 5 == 0) h->visivel = !h->visivel;
            if (h->contador >= fps * 2) { h->x = -300; h->y = -300; h->visivel = false; }
        }
    }

    if (img) {
        if (h->lista_estado[MORTO] == 0)
            al_draw_bitmap_region(img, h->frame_atual * LARGURA_SPRITE, 0, LARGURA_SPRITE, ALTURA_SPRITE, h->x, h->y, h->direcao);
        else if (h->lista_estado[MORTO] == 1 && h->visivel) {
            int flip = ALLEGRO_FLIP_VERTICAL;
            if (h->direcao == ALLEGRO_FLIP_HORIZONTAL) flip |= ALLEGRO_FLIP_HORIZONTAL;
            al_draw_bitmap_region(img, h->frame_atual * LARGURA_SPRITE, 0, LARGURA_SPRITE, ALTURA_SPRITE, h->x, h->y, flip);
        }
    }
    else {
        if (h->lista_estado[MORTO] == 0) al_draw_rectangle(h->x, h->y, h->x + h->com, h->y + h->alt, h->cor, 2);
        else if (h->lista_estado[MORTO] == 1 && h->visivel) al_draw_filled_rectangle(h->x, h->y, h->x + h->com, h->y + h->alt, al_map_rgb(255, 0, 0));
    }
}

void desenhar_municao(int x_inicial, int y_inicial, int quantidade, int largura, int altura, int espacamento) {
    for (int i = 0; i < quantidade; i++) {
        int x = x_inicial + i * (largura + espacamento); int y = y_inicial;
        al_draw_filled_rounded_rectangle(x, y + altura * 0.2, x + largura, y + altura, largura * 0.2, altura * 0.2, al_map_rgb(200, 0, 0));
        al_draw_filled_circle(x + largura / 2, y + altura * 0.2, largura / 2, al_map_rgb(192, 192, 192));
        al_draw_rounded_rectangle(x, y + altura * 0.2, x + largura, y + altura, largura * 0.2, altura * 0.2, al_map_rgb(0, 0, 0), 2);
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

void contagem_inicio(JogoDados* jogo, ALLEGRO_FONT* fonte, int w, int h, int fps) {
    if (jogo->contagem_inicio) return;
    jogo->pause = true;
    jogo->contador_inicio++;
    int tempo = jogo->contador_inicio / fps;
    const char* txt = "";
    if (tempo == 0) txt = "3"; else if (tempo == 1) txt = "2"; else if (tempo == 2) txt = "1"; else if (tempo == 3) txt = "PEGUE O PEBA DOIDO!!!";
    else { jogo->pause = false; jogo->contagem_inicio = true; }
    if (strlen(txt) > 0) al_draw_text(fonte, al_map_rgb(255, 255, 255), w / 2, h / 2, ALLEGRO_ALIGN_CENTRE, txt);
}

void passar_fase(JogoDados* jogo, Fase fases[], int fps, ALLEGRO_FONT* fonte, int w, int h) {
    if (!jogo->contagem_inicio) return;
    if (!jogo->pause) jogo->tempo_intervalo++;
    if (jogo->fim_fases) return;

    if (jogo->tempo_intervalo >= fps) {
        jogo->tempo_restante--;
        jogo->tempo_intervalo = 0;
    }

    al_draw_textf(fonte, al_map_rgb(255, 255, 255), w - 150, 20, 0, "Tempo: %d", jogo->tempo_restante);

    if (!jogo->transicao && (jogo->tempo_restante <= 0 || fases[jogo->fase_atual].municao <= 0 || fases[jogo->fase_atual].pebas_mortos == fases[jogo->fase_atual].quantidade_pebas)) {
        jogo->transicao = true; jogo->pause = true;
    }

    if (jogo->transicao) {
        jogo->tempo_delay_transicao++;
        if (fases[jogo->fase_atual].pebas_mortos == fases[jogo->fase_atual].quantidade_pebas)
            al_draw_text(fonte, al_map_rgb(255, 255, 255), w / 2, h / 2, ALLEGRO_ALIGN_CENTRE, "CONSEGUI MATAR TODOS OS PEBAS");
        else if (jogo->tempo_restante <= 0)
            al_draw_text(fonte, al_map_rgb(0, 0, 0), w / 2, h / 2, ALLEGRO_ALIGN_CENTRE, "O IBAMA VAI CHEGAR!!!");
        else if (fases[jogo->fase_atual].municao <= 0)
            al_draw_text(fonte, al_map_rgb(0, 0, 0), w / 2, h / 2, ALLEGRO_ALIGN_CENTRE, "ACABOU OS CARTUCHOS");

        if (jogo->tempo_delay_transicao >= fps * 2) {
            if (fases[jogo->fase_atual].pebas_mortos == fases[jogo->fase_atual].quantidade_pebas) jogo->pontuacao += jogo->tempo_restante * 10;
            if (fases[jogo->fase_atual].municao > 0) jogo->pontuacao += fases[jogo->fase_atual].municao * 50;
            jogo->fase_atual++;
            if (jogo->fase_atual >= jogo->niveis) {
                jogo->fim_fases = true; jogo->fase_atual = 0; return;
            }
            if (!jogo->fim_fases) {
                jogo->tempo_restante = fases[jogo->fase_atual].tempo;
                jogo->contagem_inicio = false; jogo->contador_inicio = 0; jogo->tempo_delay_transicao = 0;
                jogo->transicao = false; jogo->reiniciar_hitbox = true;
            }
        }
    }
}

void desenhar_pontuacao(JogoDados* jogo, ALLEGRO_FONT* fonte) {
    al_draw_textf(fonte, al_map_rgb(255, 255, 255), 20, 20, 0, "Pontos: %d", jogo->pontuacao);
}

void desenhar_nivel(JogoDados* jogo, ALLEGRO_FONT* fonte) {
    al_draw_textf(fonte, al_map_rgb(255, 255, 255), 20, 40, 0, "Nivel: %d", jogo->fase_atual + 1);
}

void mostrar_tela_final(int w, int h, ALLEGRO_FONT* fonte, int* top_scores, int pontuacao_atual) {
    al_draw_filled_rectangle(0, 0, w, h, al_map_rgba(0, 0, 0, 150));
    al_draw_text(fonte, al_map_rgb(255, 255, 255), w / 2, h / 2 - 120, ALLEGRO_ALIGN_CENTRE, "FIM DAS FASES!");

    al_draw_textf(fonte, al_map_rgb(255, 255, 0), w / 2, h / 2 - 80, ALLEGRO_ALIGN_CENTRE, "Sua Pontuacao: %d", pontuacao_atual);

    al_draw_text(fonte, al_map_rgb(255, 255, 255), w / 2, h / 2 - 40, ALLEGRO_ALIGN_CENTRE, "--- RANKING (TOP 5) ---");
    for(int i=0; i<5; i++) {
        al_draw_textf(fonte, al_map_rgb(200, 200, 200), w / 2, h / 2 + (i*25), ALLEGRO_ALIGN_CENTRE, "%d. %d pts", i+1, top_scores[i]);
    }

    al_draw_text(fonte, al_map_rgb(255, 255, 255), w / 2, h / 2 + 150, ALLEGRO_ALIGN_CENTRE, "ESPACO: Reiniciar  |  BACKSPACE: Menu");
}
