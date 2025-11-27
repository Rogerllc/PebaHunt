#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

// Estados
enum Estado {
    MENU,
    CREDITOS,
    JOGO
};

typedef struct {
    int x, y;
    int w, h;
    ALLEGRO_BITMAP *img;
    char id[20]; 
    int mouse_hover;
} Botao;
typedef struct {
    ALLEGRO_FONT *titulo;
    ALLEGRO_FONT *texto;
    ALLEGRO_FONT *menor;
    ALLEGRO_SAMPLE *musica;
} Assets;

int inicializar_allegro();
Botao* criar_botao(const char* path, int x, int y, const char* id_str);
void destruir_botao(Botao *bt);
int verificar_colisao(Botao *bt, int mx, int my); 
void desenhar_botao(Botao *bt);

const int LARGURA_TELA = 800;
const int ALTURA_TELA = 600;

int main() {
    if (!inicializar_allegro()) {
        printf("Falha ao inicializar Allegro.\n");
        return -1;
    }

    ALLEGRO_DISPLAY *janela = al_create_display(LARGURA_TELA, ALTURA_TELA);
    if (!janela) {
        printf("Falha ao criar janela.\n");
        return -1;
    }
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE *fila_eventos = al_create_event_queue();

    // Fontes
    Assets assets; 
    assets.titulo = al_load_font("fonte.ttf", 20, 0);
    assets.texto = al_load_font("fonte.ttf", 28, 0);
    assets.menor = al_load_font("fonte.ttf", 18, 0); 
    
    // Debug de fontes
    if (!assets.texto) { 
        printf("Aviso: fonte.ttf nao encontrada. Usando padrao.\n");
        assets.titulo = al_create_builtin_font(); 
        assets.texto = al_create_builtin_font(); 
        assets.menor = al_create_builtin_font(); 
    }

    // Imagens de Fundo
    ALLEGRO_BITMAP *bg_menu = al_load_bitmap("menu_sertao.png");
    ALLEGRO_BITMAP *bg_jogo = al_load_bitmap("backgound_1.png");
    ALLEGRO_BITMAP *logo = al_load_bitmap("peba_hunt.png");
    ALLEGRO_BITMAP *placa = al_load_bitmap("placa.png");

    if(!bg_menu || !bg_jogo || !logo) printf("Imagem de fundo falhou ao carregar.\n");
    
    // Musica
    ALLEGRO_SAMPLE_ID id_msc;
    assets.musica = al_load_sample("musica.ogg");
    if(assets.musica) {
        al_play_sample(assets.musica, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, &id_msc);
    } else {
        printf("musica.ogg nao encontrada.\n");
    }
    
    // Botões do MENU
    int qtd_btns_menu = 3;
    Botao **btns_menu = (Botao**) malloc(qtd_btns_menu * sizeof(Botao*));
    
    btns_menu[0] = criar_botao("botao_novoJogo.png", 300, 245, "NOVO");
    btns_menu[1] = criar_botao("botao_creditos.png", 300, 348, "CREDITOS");
    btns_menu[2] = criar_botao("botao_sair.png", 300, 453, "SAIR");

    Botao *btn_continuar = criar_botao("botao_continuar.png", 0, 250, "CONT"); 
    Botao *btn_voltar = criar_botao("botao_menu.png", 0, 380, "VOLT");

    if(btn_continuar->img) btn_continuar->x = (LARGURA_TELA - btn_continuar->w) / 2;
    if(btn_voltar->img) btn_voltar->x = (LARGURA_TELA - btn_voltar->w) / 2;

    // Eventos
    al_register_event_source(fila_eventos, al_get_display_event_source(janela));
    al_register_event_source(fila_eventos, al_get_timer_event_source(timer));
    al_register_event_source(fila_eventos, al_get_keyboard_event_source());
    al_register_event_source(fila_eventos, al_get_mouse_event_source());

    al_start_timer(timer);

    int rodando = 1;
    int estado = MENU;
    int pausado = 0;
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
                if (estado == CREDITOS) {
                    estado = MENU;
                }
                else if (estado == JOGO) {
                    pausado = !pausado;
                }
                else {
                    rodando = 0;
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
            if (estado == MENU) {
                for(int i=0; i< qtd_btns_menu; i++) {
                    btns_menu[i]->mouse_hover = verificar_colisao(btns_menu[i], ev.mouse.x, ev.mouse.y);
                }
            } else if (estado == JOGO && pausado) {
                btn_continuar->mouse_hover = verificar_colisao(btn_continuar, ev.mouse.x, ev.mouse.y);
                btn_voltar->mouse_hover = verificar_colisao(btn_voltar, ev.mouse.x, ev.mouse.y);
            }
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            if (ev.mouse.button == 1) {
                if (estado == MENU) {
                    for(int i=0; i<qtd_btns_menu; i++) {
                        if (btns_menu[i]->mouse_hover) {
                            if(strcmp(btns_menu[i]->id, "NOVO") == 0) {
                                estado = JOGO; 
                                pausado = 0;
                                printf("Botao NOVO clicado.\n");
                            }
                            else if(strcmp(btns_menu[i]->id, "CREDITOS") == 0) {
                                estado = CREDITOS;
                            }
                            else if(strcmp(btns_menu[i]->id, "SAIR") == 0) {
                                rodando = 0;
                            }
                        }
                    }
                } else if (estado == JOGO && pausado) {
                    if (btn_continuar->mouse_hover) pausado = 0;
                    if (btn_voltar->mouse_hover) { 
                        pausado = 0; 
                        estado = MENU; 
                    }
                }
            }
        }

        if (desenhar && al_is_event_queue_empty(fila_eventos)) {
            desenhar = 0;
            al_clear_to_color(al_map_rgb(0, 0, 0));

            if (estado == MENU) {
                if(bg_menu) al_draw_bitmap(bg_menu, -115, 0, 0);
                if(logo) { 
                    int logo_w = al_get_bitmap_width(logo);
                    int logo_h = al_get_bitmap_height(logo);
                    al_draw_scaled_bitmap(logo, 0, 0, logo_w, logo_h, (LARGURA_TELA - 450)/2, 20, 450, 260, 0);
                }
                for(int i=0; i<qtd_btns_menu; i++) desenhar_botao(btns_menu[i]);
            } else if (estado == JOGO) {
                if(bg_jogo) al_draw_scaled_bitmap(bg_jogo, 0, 0, al_get_bitmap_width(bg_jogo), al_get_bitmap_height(bg_jogo), 0, 0, 800, 600, 0);
                
                if(pausado) {
                    al_draw_filled_rectangle(0, 0, 800, 600, al_map_rgba(0, 0, 0, 160));
                    al_draw_text(assets.texto, al_map_rgb(255,255,255), 400, 150, ALLEGRO_ALIGN_CENTER, "PAUSADO");
                    desenhar_botao(btn_continuar);
                    desenhar_botao(btn_voltar);
                }
            } else if (estado == CREDITOS) {
                 al_draw_filled_rectangle(0, 0, 800, 600, al_map_rgba(0, 0, 0, 100));

                 if(placa) al_draw_scaled_bitmap(placa, 0,0, al_get_bitmap_width(placa), al_get_bitmap_height(placa), 50, 20, 700, 560, 0);
                 
                 char devs[4][50] = {
                     "Gustavo Morais - Sprites", 
                     "Wagner Junior - Sprites", 
                     "Luan Jefferson - Logica", 
                     "Roger Leite - UI"
                 };
                 
                 ALLEGRO_COLOR cor_txt = al_map_rgb(255,255,255);
                 ALLEGRO_COLOR cor_info = al_map_rgb(220,220,200);

                 int y_inicial = 120; 
                 int x_centro = 460;
                 int x_esq = 210; 
                
                 int y_devs = y_inicial + 60;
                 al_draw_text(assets.texto, cor_txt, x_esq, y_devs, ALLEGRO_ALIGN_LEFT, "Desenvolvedores:");
                 for(int i=0; i<4; i++){
                     al_draw_text(assets.menor, cor_txt, x_esq + 20, y_devs + 35 + (i*25), ALLEGRO_ALIGN_LEFT, devs[i]);
                 }
                 int y_cadeira = y_devs + 150;
                 al_draw_text(assets.texto, cor_txt, x_esq, y_cadeira, ALLEGRO_ALIGN_LEFT, "Projeto da cadeira:");
                 al_draw_text(assets.menor, cor_txt, x_esq + 20, y_cadeira + 35, ALLEGRO_ALIGN_LEFT, "Algoritmos e Estrutura de Dados I");
                 al_draw_text(assets.menor, cor_txt, x_esq + 20, y_cadeira + 60, ALLEGRO_ALIGN_LEFT, "Tecnologia da Informacao 2025.1");

                 int y_info = y_cadeira + 100;
                 al_draw_text(assets.texto, cor_info, x_esq, y_info, ALLEGRO_ALIGN_LEFT, "Info:");
                 
                 al_draw_text(assets.menor, cor_info, x_esq + 20, y_info + 35, ALLEGRO_ALIGN_LEFT, "Certificamos que e apenas um projeto educacional e que");
                 al_draw_text(assets.menor, cor_info, x_esq + 20, y_info + 60, ALLEGRO_ALIGN_LEFT, "nao influenciamos a caca de devidos animais no jogo.");
                 al_draw_text(assets.menor, cor_info, x_esq + 20, y_info + 85, ALLEGRO_ALIGN_LEFT, "Jogo meramente ficticio.");
                 
                 al_draw_text(assets.menor, al_map_rgb(200,200,200), 450, 550, ALLEGRO_ALIGN_CENTER, "[ ESC para Voltar ]");
            }

            al_flip_display();
        }
    }
    for(int i=0; i<qtd_btns_menu; i++) destruir_botao(btns_menu[i]);
    free(btns_menu); 
    
    destruir_botao(btn_continuar);
    destruir_botao(btn_voltar);

    if(bg_menu) al_destroy_bitmap(bg_menu);
    if(bg_jogo) al_destroy_bitmap(bg_jogo);
    if(logo) al_destroy_bitmap(logo);
    if(placa) al_destroy_bitmap(placa);
    if(assets.musica) al_destroy_sample(assets.musica);
    
    al_destroy_font(assets.titulo);
    al_destroy_font(assets.texto);
    al_destroy_font(assets.menor);
    
    al_destroy_display(janela);
    al_destroy_event_queue(fila_eventos);
    al_destroy_timer(timer);

    return 0;
}

// Funções
int inicializar_allegro() {
    if (!al_init()) return 0;
    al_init_image_addon();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    
    if(!al_install_audio()){ printf("Falha audio.\n"); return 0; }
    if(!al_init_acodec_addon()){ printf("Falha acodec.\n"); return 0; }
    
    al_reserve_samples(5);
    al_install_keyboard();
    al_install_mouse();
    return 1;
}

Botao* criar_botao(const char* path, int x, int y, const char* id_str) {
    Botao *bt = (Botao*) malloc(sizeof(Botao)); 
    if (!bt) return NULL;

    bt->img = al_load_bitmap(path);
    if (bt->img) {
        bt->w = al_get_bitmap_width(bt->img);
        bt->h = al_get_bitmap_height(bt->img);
    } else {
        printf("Erro ao carregar imagem do botao: %s\n", path);
        bt->w = 200; bt->h = 50;
    }
    bt->x = x;
    bt->y = y;

    strcpy(bt->id, id_str); 
    bt->mouse_hover = 0;
    return bt;
}

void destruir_botao(Botao *bt) {
    if (bt) {
        if(bt->img) al_destroy_bitmap(bt->img);
        free(bt); 
    }
}

int verificar_colisao(Botao *bt, int mx, int my) {
    if (!bt) return 0;
    if (mx >= bt->x && mx <= bt->x + bt->w && my >= bt->y && my <= bt->y + bt->h) {
        return 1;
    }
    return 0;
}

void desenhar_botao(Botao *bt) {
    if (!bt || !bt->img) return;
    int offset_y = bt->mouse_hover ? 5 : 0;
    al_draw_bitmap(bt->img, bt->x, bt->y - offset_y, 0);
}