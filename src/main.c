// DenCoder.EXE

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>

#ifdef WINDOWS
    #define SDL_main WinMain
#elif LINUX
    #define SDL_main main
#endif

#include "constants.h"
#include "colors.h"
#include "tetrominoes.h"
#include "../assets/textures/icon.h"
#include "../assets/fonts/Jersey_10/jersey10.h"
#include "../assets/audio/background_music.h"
#include "../assets/audio/scoring.h"
#include "../assets/audio/youwin.h"
#include "../assets/audio/gameover.h"

typedef struct Window {
    int window_height;
    int window_width;
} Window;

typedef struct Tetromino {
    int x;
    int y;
    int tetromino_id;
    int rotations;
} Tetromino;

typedef struct Game {
    int game_is_running;
    int game_over;
    int score;
    int **field;
    Tetromino tetromino;
} Game;

Window w;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;
Mix_Music *bg_music = NULL;
Mix_Chunk *scoring_sound = NULL;
Mix_Chunk *youwin_sound = NULL;
Mix_Chunk *gameover_sound = NULL;
Game g;
int last_frame_time = 0;

void resize_window(void) {
    float current_aspect_ratio = (float)(w.window_height / w.window_width);

    if (current_aspect_ratio > ASPECT_RATIO) {
        w.window_width = (int)(w.window_height * ASPECT_RATIO);
        SDL_SetWindowSize(window, w.window_width, w.window_height);
    }
    else if (current_aspect_ratio < ASPECT_RATIO) {
        w.window_height = (int)(w.window_width * ASPECT_RATIO);
        SDL_SetWindowSize(window, w.window_width, w.window_height);
    }
}

void load_sounds(void) {
    SDL_RWops *rw = SDL_RWFromMem(background_music, background_music_len);
    if (!rw) {
        SDL_Log("Error creating RWops for bg_music: %s\n", SDL_GetError());
        return;
    }

    bg_music = Mix_LoadMUS_RW(rw, 1);

    rw = SDL_RWFromMem(scoring, scoring_len);
    if (!rw) {
        SDL_Log("Error creating RWops for scoring_sound: %s\n", SDL_GetError());
        return;
    }

    scoring_sound = Mix_LoadWAV_RW(rw, 1);

    rw = SDL_RWFromMem(gameover, gameover_len);
    if (!rw) {
        SDL_Log("Error creating RWops for gameover_sound: %s\n", SDL_GetError());
        return;
    }

    gameover_sound = Mix_LoadWAV_RW(rw, 1);

    rw = SDL_RWFromMem(youwin, youwin_len);
    if (!rw) {
        SDL_Log("Error creating RWops for youwin_sound: %s\n", SDL_GetError());
        return;
    }

    youwin_sound = Mix_LoadWAV_RW(rw, 1);
}

void load_font(void) {
    SDL_RWops *rw = SDL_RWFromMem(Jersey10_Regular_ttf, Jersey10_Regular_ttf_len);
    if (!rw) {
        SDL_Log("Error creating RWops for font: %s\n", SDL_GetError());
        return;
    }

    font = TTF_OpenFontRW(rw, 1, FONT_SIZE);
}

void load_icon(void) {
    SDL_RWops *rw = SDL_RWFromMem(icon_png, icon_png_len);
    if (!rw) {
        SDL_Log("Error creating RWops for icon: %s\n", SDL_GetError());
        return;
    }

    SDL_Surface *icon_surface = IMG_Load_RW(rw, 1);

    SDL_SetWindowIcon(window, icon_surface);
}

int initialize_window(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Error initializing SDL: %s", SDL_GetError());
        return FALSE;
    }

    if (TTF_Init() != 0) {
        SDL_Log("Error initializing TTF: %s\n", TTF_GetError());
        return FALSE;
    }

    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        SDL_Log("Error initializing Mixer: %s\n", Mix_GetError());
        return FALSE;
    }

    window = SDL_CreateWindow(
        "Tetrominoes",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        SDL_Log("Error creating SDL Window: %s", SDL_GetError());
        return FALSE;
    }

    w.window_height = WINDOW_HEIGHT;
    w.window_width = WINDOW_WIDTH;

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        SDL_Log("Error creating SDL Renderer: %s", SDL_GetError());
        return FALSE;
    }
    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    load_font();
    if (!font) {
        SDL_Log("Error opening the font: %s.\n", TTF_GetError());
        return FALSE;
    }

    load_icon();

    load_sounds();
    if (!bg_music || !scoring_sound || !gameover_sound || !youwin_sound) {
        SDL_Log("Error loading the sounds: %s.\n", Mix_GetError());
        return FALSE;
    }

    return TRUE;
}

void destroy_window(void) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    Mix_HaltMusic();
    Mix_FreeMusic(bg_music);
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
}

void allocate_memory_for_game_field(void) {
    g.field = (int**)calloc(FIELD_HEIGHT, sizeof(int*));
    if (g.field == NULL) {
        SDL_Log("Error allocating memory for rows of the game field.\n");
        g.game_is_running = FALSE;
        return;
    }
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        g.field[i] = (int*)calloc(FIELD_WIDTH, sizeof(int));
        if (g.field[i] == NULL) {
            SDL_Log("Error allocating memory for %d. column of the game field.\n", i);
            g.game_is_running = FALSE;
            return;
        }
    }
}

void free_memory(void) {
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        free(g.field[i]);
        g.field[i] = NULL;
    }
    free(g.field);
    g.field = NULL;
}

void mark_the_borders(void) {
    for (int i = 0; i < FIELD_HEIGHT; i++) {
        for (int j = 0; j < FIELD_WIDTH; j++) {
            if (i == FIELD_HEIGHT - HEIGHT_BIAS || j == 0 || j == FIELD_WIDTH - WIDTH_BIAS) {
                g.field[i][j] = BORDER_BLOCK;
            }
        }
    }
}

int rotate(int x, int y, int r, int tetromino_id) {
    int pointer = 0;
    switch (r) {
    case 0:
        pointer = (y * 4 + x);
        break;
    case 1:
        pointer = (12 + y - (x * 4));
        break;
    case 2:
        pointer = (15 - (y * 4) - x);
        break;
    case 3:
        pointer = (3 - y + (x * 4));
        break;    
    default:
        break;
    }
    return *(TETROMINOES[tetromino_id] + pointer);
}

int collision(int x, int y, int r) {
    for (int j = 0; j < TETROMINO_SIZE; j++) {
        for (int i = 0; i < TETROMINO_SIZE; i++) {
            if (x + i >= 0 && x + i < FIELD_WIDTH &&
                y + j >= 0 && y + j < FIELD_HEIGHT) {
                int block_id = rotate(i, j, r, g.tetromino.tetromino_id);
                int field_block_id = g.field[y + j][x + i];
                if (block_id != EMPTY && field_block_id != EMPTY) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

void spawn_tetromino(void) {
    g.tetromino.x = TETROMINO_SPAWN_X;
    g.tetromino.y = TETROMINO_SPAWN_Y;
    g.tetromino.rotations = 0;
    g.tetromino.tetromino_id++;
    if (g.tetromino.tetromino_id >= TETROMINO_COUNT) {
        g.tetromino.tetromino_id = 0;
    }
    if (collision(g.tetromino.x, g.tetromino.y, g.tetromino.rotations)) {
        g.game_over = TRUE;
        Mix_HaltMusic();
        Mix_PlayChannel(-1, gameover_sound, 0);
    }    
}

void setup(void) {
    g.game_over = FALSE;
    g.score = 0;
    g.tetromino.tetromino_id = -1;
    
    if (g.field != NULL) {
        free_memory();
    }
    allocate_memory_for_game_field();
    mark_the_borders();
    spawn_tetromino();
    Mix_PlayMusic(bg_music, -1);
}

void process_key(SDL_Event event) {
    int tmp_rotations = g.tetromino.rotations;
    switch (event.key.keysym.sym) {
    case SDLK_ESCAPE:
        g.game_is_running = FALSE;
        break;
    case SDLK_RIGHT:
        if (!collision(g.tetromino.x + 1, g.tetromino.y, g.tetromino.rotations)) {
            g.tetromino.x++;
        }
        break;
    case SDLK_LEFT:
        if (!collision(g.tetromino.x - 1, g.tetromino.y, g.tetromino.rotations)) {
            g.tetromino.x--;
        }
        break;
    case SDLK_UP:
        tmp_rotations += 1;
        if (tmp_rotations == 4) {
            tmp_rotations = 0;
        }
        if (!collision(g.tetromino.x, g.tetromino.y, tmp_rotations)) {
            g.tetromino.rotations = tmp_rotations;
        }
        break;
    case SDLK_DOWN:
        if (!collision(g.tetromino.x, g.tetromino.y + 1, g.tetromino.rotations)) {
            g.tetromino.y++;
        }
        break;
    default:
        break;
    }
}

void process_input(void) {
    SDL_Event event;
    SDL_PollEvent(&event);

    if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        w.window_width = event.window.data1;
        w.window_height = event.window.data2;
        resize_window();
    }

    if (g.game_over) {
        if (event.type == SDL_KEYDOWN) {
            setup();
        }
        else if (event.type == SDL_QUIT) {
            g.game_is_running = FALSE;
        }
    }
    else {
        switch (event.type) {
        case SDL_QUIT:
            g.game_is_running = FALSE;
            break;

        case SDL_KEYDOWN:
            process_key(event);           
            break;
        
        default:
            break;
        }
    }
}

void drop_lines_above(int lines_completed, int lowest_lines[]) {
    for (int i = 0, k = 0; i < 4; i++, k++) {
        if (lowest_lines[i] == -1) {
            return;
        }
        for (int j = lowest_lines[i] - 1 + k; j > 0; j--) {
            for (int l = 1; l < FIELD_WIDTH - WIDTH_BIAS; l++) {
                g.field[j+1][l] = g.field[j][l];
                g.field[j][l] = EMPTY;
            }
        }
    }
}

void calculate_score(int lines_completed) {
    int tmp_score = g.score;
    switch (lines_completed) {
    case 1:
        tmp_score += SINGLE;
        break;
    case 2:
        tmp_score += DOUBLE;
        break;
    case 3:
        tmp_score += TRIPLE;
        break;
    case 4:
        tmp_score += TETRIS;
        break;
    default:
        break;
    }
    if (tmp_score >= MAX_SCORE) {
        g.score = MAX_SCORE;
        g.game_over = TRUE;
        Mix_HaltMusic();
        Mix_PlayChannel(-1, youwin_sound, 0);
    }
    else {
        g.score = tmp_score;
    }
}

void check_lines(void) {
    int lines_completed = 0;
    int lowest_lines[4] = {-1}; // The highest number of layers that can be completed is 4
    int k = 0;
    for (int i = FIELD_HEIGHT - HEIGHT_BIAS - 1; i > 1; i--) {
        int line_completed = TRUE;
        for (int j = 1; j < FIELD_WIDTH - WIDTH_BIAS; j++) {
            if (g.field[i][j] == EMPTY) {
                line_completed = FALSE;
            }
        }
        if (line_completed) {
            lines_completed++;
            lowest_lines[k] = i;
            k++;
        }
    }
    if (lines_completed > 0) {
        Mix_PlayChannel(-1, scoring_sound, 0);
        calculate_score(lines_completed);
        drop_lines_above(lines_completed, lowest_lines);
    }
}

void lock_tetromino(void) {
    for (int j = 0; j < TETROMINO_SIZE; j++) {
        for (int i = 0; i < TETROMINO_SIZE; i++) {
            int block_id = rotate(i, j, g.tetromino.rotations, g.tetromino.tetromino_id);
            if (block_id != EMPTY) {
                g.field[g.tetromino.y + j][g.tetromino.x + i] = block_id;
            }
        }
    }
}

void update(void) {
    if (g.game_over) {
        return;
    }

    if (collision(g.tetromino.x, g.tetromino.y + 1, g.tetromino.rotations)) {
        lock_tetromino();
        check_lines();
        spawn_tetromino();
    }
    
    if (SDL_GetTicks() - last_frame_time >= BLOCK_SPEED) {
        g.tetromino.y += 1;
        last_frame_time = SDL_GetTicks();
    }
}

void render_text(char text[32], int text_x, int text_y) {
    SDL_Color text_color = WHITE;
    
    SDL_Surface *text_surface = TTF_RenderText_Solid(font, text, text_color);
    if (!text_surface) {
        SDL_Log("Error creating text surface: %s.\n", TTF_GetError());
        g.game_is_running = FALSE;
        return;
    }

    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_FreeSurface(text_surface);

    int text_width, text_height;
    SDL_QueryTexture(text_texture, NULL, NULL, &text_width, &text_height);

    if (text_x == -1) {
        text_x = (WINDOW_WIDTH - text_width) / 2;
    }

    SDL_Rect text_rect = {text_x, text_y, text_width, text_height};
    SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
}

void render_block(int i, int j, SDL_Color color, int block_size, int anchor_x, int anchor_y) {
    int x = i;
    int y = j;

    // Draw block body
    SDL_Rect block = {y * block_size + anchor_x, x * block_size + anchor_y, block_size, block_size};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &block);
    // Draw block outline
    SDL_Rect block_outline = {y * block_size + anchor_x, x * block_size + anchor_y, block_size, block_size};
    SDL_SetRenderDrawColor(renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
    SDL_RenderDrawRect(renderer, &block_outline);
}

void render_score(void) {
    char text[32];
    snprintf(text, sizeof(text), "SCORE: %06d", g.score);
    render_text(text, SCORE_ANCHOR_X, SCORE_ANCHOR_Y);
}

void render_game_over(void) {
    char text[32];
    if (g.score == MAX_SCORE) {
        snprintf(text, sizeof(text), "YOU WIN!");
    }
    else {
        snprintf(text, sizeof(text), "GAME OVER");
    }
    render_text(text, -1, (WINDOW_HEIGHT - FONT_SIZE) / 2);
}

void render_background(void) {
    SDL_Rect background = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_SetRenderDrawColor(renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
    SDL_RenderFillRect(renderer, &background);
}

void block_switch(int block_id, int i, int j, int block_size, int anchor_x, int anchor_y) {
    switch (block_id) {
    case BORDER_BLOCK:
        render_block(i, j, GRAY, block_size, anchor_x, anchor_y);
        break;
    case RED_BLOCK:
        render_block(i, j, RED, block_size, anchor_x, anchor_y);
        break;
    case GREEN_BLOCK:
        render_block(i, j, GREEN, block_size, anchor_x, anchor_y);
        break;
    case BLUE_BLOCK:
        render_block(i, j, BLUE, block_size, anchor_x, anchor_y);
        break;
    case YELLOW_BLOCK:
        render_block(i, j, YELLOW, block_size, anchor_x, anchor_y);
        break;
    case PINK_BLOCK:
        render_block(i, j, PINK, block_size, anchor_x, anchor_y);
        break;
    case CYAN_BLOCK:
        render_block(i, j, CYAN, block_size, anchor_x, anchor_y);
        break;
    case ORANGE_BLOCK:
        render_block(i, j, ORANGE, block_size, anchor_x, anchor_y);
        break;
    default:
        break;
    }
}

void render_next_tetromino(void) {
    char text[32];
    snprintf(text, sizeof(text), "NEXT:");
    render_text(text, NEXT_ANCHOR_X, NEXT_ANCHOR_Y);
    int next_tetromino_id = g.tetromino.tetromino_id + 1;
    if (g.game_over) {
        next_tetromino_id = g.tetromino.tetromino_id;
    }
    else if (next_tetromino_id >= TETROMINO_COUNT) {
        next_tetromino_id = 0;
    }
    
    for (int y = 0; y < TETROMINO_SIZE; y++) {
        for (int x = 0; x < TETROMINO_SIZE; x++) {
            int block_id = rotate(x, y, 0, next_tetromino_id);
            if (block_id != BORDER_BLOCK) {
                block_switch(block_id, y, x, SMALL_BLOCK_SIZE, NEXT_TETROMINO_ANCHOR_X, NEXT_TETROMINO_ANCHOR_Y);
            }
        }
    }
}

void render(void) {
    SDL_RenderClear(renderer);

    render_background();

    render_score();

    render_next_tetromino();

    // Render scene
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
            int block_id = g.field[y][x];
            block_switch(block_id, y, x, BLOCK_SIZE, 0, FIELD_ANCHOR_Y);
        }
    }

    // Render active tetromino
    if (!g.game_over) {
        for (int y = 0; y < TETROMINO_SIZE; y++) {
            for (int x = 0; x < TETROMINO_SIZE; x++) {
                int block_id = rotate(x, y, g.tetromino.rotations, g.tetromino.tetromino_id);
                if (block_id != BORDER_BLOCK) {
                    block_switch(block_id, y + g.tetromino.y, x + g.tetromino.x, BLOCK_SIZE, 0, FIELD_ANCHOR_Y);
                }
            }
        }
    }

    if (g.game_over) {
        render_game_over();
    }

    SDL_RenderPresent(renderer);
}

int SDL_main(int argc, char *argv[]) {
    g.game_is_running = initialize_window();

    if (g.game_is_running) {
        setup();
    }

    while (g.game_is_running)
    {
        process_input();
        update();
        render();
        SDL_Delay(FRAME_TARGET_TIME);
    }
    
    destroy_window();
    free_memory();

    return 0;
}
