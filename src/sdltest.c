/* Prueba de SDL2 para Trimui Smart Pro: dibuja en pantalla y mide fluidez.
   Escribe el resultado en sdltest_log.txt dentro de la carpeta indicada. */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *lg;
#define LOG(...) do { fprintf(lg, __VA_ARGS__); fflush(lg); } while (0)

static int quit_flag = 0;

static void pump(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) quit_flag = 1;
    }
}

/* modo 0 = barras de color, 1 = textura ARGB8888, 2 = textura YUV (IYUV) */
static void run_phase(SDL_Renderer *r, SDL_Texture *tex, int mode, int W, int H,
                      Uint32 dur_ms, const char *name,
                      unsigned char *argb, unsigned char *yp, unsigned char *up, unsigned char *vp) {
    Uint32 t0 = SDL_GetTicks();
    Uint32 frames = 0;
    while (!quit_flag && SDL_GetTicks() - t0 < dur_ms) {
        pump();
        if (mode == 0) {
            static const Uint8 cols[8][3] = {
                {255,255,255},{255,255,0},{0,255,255},{0,255,0},
                {255,0,255},{255,0,0},{0,0,255},{40,40,40}};
            int sw = 1280, sh = 720, i;
            SDL_GetRendererOutputSize(r, &sw, &sh);
            for (i = 0; i < 8; i++) {
                SDL_Rect rc;
                rc.x = i * (sw / 8); rc.y = 0; rc.w = sw / 8; rc.h = sh;
                SDL_SetRenderDrawColor(r, cols[i][0], cols[i][1], cols[i][2], 255);
                SDL_RenderFillRect(r, &rc);
            }
            {
                SDL_Rect sq;
                sq.w = 80; sq.h = 80;
                sq.x = (int)((frames * 7) % (Uint32)(sw - 80));
                sq.y = sh / 2;
                SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
                SDL_RenderFillRect(r, &sq);
            }
        } else if (mode == 1) {
            int row = (int)((frames * 8) % (Uint32)(H - 64));
            memset(argb + (size_t)row * W * 4, (int)(frames & 0xFF), (size_t)64 * W * 4);
            SDL_UpdateTexture(tex, NULL, argb, W * 4);
            SDL_RenderClear(r);
            SDL_RenderCopy(r, tex, NULL, NULL);
        } else {
            int row = (int)((frames * 8) % (Uint32)(H - 64));
            memset(yp + (size_t)row * W, (int)(frames & 0xFF), (size_t)64 * W);
            SDL_UpdateYUVTexture(tex, NULL, yp, W, up, W / 2, vp, W / 2);
            SDL_RenderClear(r);
            SDL_RenderCopy(r, tex, NULL, NULL);
        }
        SDL_RenderPresent(r);
        frames++;
    }
    {
        Uint32 el = SDL_GetTicks() - t0;
        if (el == 0) el = 1;
        LOG("[%s] %u frames en %u ms = %.1f fps\n", name, frames, el, (double)frames * 1000.0 / (double)el);
    }
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : ".";
    char path[600];
    SDL_version v;
    SDL_DisplayMode dm;
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_RendererInfo ri;
    SDL_Texture *tex_argb = NULL, *tex_yuv = NULL;
    const int W = 1280, H = 720;
    unsigned char *argb, *yp, *up, *vp;
    Uint32 i;

    snprintf(path, sizeof path, "%s/sdltest_log.txt", dir);
    lg = fopen(path, "w");
    if (!lg) return 1;

    SDL_GetVersion(&v);
    LOG("SDL en ejecucion: %d.%d.%d\n", v.major, v.minor, v.patch);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG("SDL_Init fallo: %s\n", SDL_GetError());
        return 2;
    }
    LOG("Driver de video: %s\n", SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "(ninguno)");
    LOG("Pantallas: %d\n", SDL_GetNumVideoDisplays());
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        LOG("No pude leer el modo de pantalla: %s\n", SDL_GetError());
        dm.w = W; dm.h = H;
    }
    LOG("Modo de pantalla: %dx%d a %d Hz\n", dm.w, dm.h, dm.refresh_rate);

    win = SDL_CreateWindow("test", 0, 0, dm.w, dm.h, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);
    if (!win) {
        LOG("SDL_CreateWindow fallo: %s\n", SDL_GetError());
        SDL_Quit();
        return 3;
    }
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) {
        LOG("Renderer acelerado fallo: %s. Pruebo por software.\n", SDL_GetError());
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!ren) {
        LOG("Renderer fallo: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 4;
    }
    if (SDL_GetRendererInfo(ren, &ri) == 0) {
        LOG("Renderer: %s (flags=%u, max textura %dx%d)\n", ri.name, (unsigned)ri.flags, ri.max_texture_width, ri.max_texture_height);
        for (i = 0; i < ri.num_texture_formats; i++)
            LOG("  formato: %s\n", SDL_GetPixelFormatName(ri.texture_formats[i]));
    }

    tex_argb = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, W, H);
    if (!tex_argb) LOG("Textura ARGB8888 fallo: %s\n", SDL_GetError());
    tex_yuv = SDL_CreateTexture(ren, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, W, H);
    if (!tex_yuv) LOG("Textura IYUV fallo: %s\n", SDL_GetError());

    argb = (unsigned char *)calloc((size_t)W * H * 4, 1);
    yp = (unsigned char *)calloc((size_t)W * H, 1);
    up = (unsigned char *)malloc((size_t)W * H / 4);
    vp = (unsigned char *)malloc((size_t)W * H / 4);
    if (!argb || !yp || !up || !vp) {
        LOG("Sin memoria\n");
        return 5;
    }
    memset(up, 128, (size_t)W * H / 4);
    memset(vp, 128, (size_t)W * H / 4);

    run_phase(ren, NULL, 0, W, H, 4000, "barras de color", argb, yp, up, vp);
    if (tex_argb) run_phase(ren, tex_argb, 1, W, H, 5000, "video ARGB 1280x720", argb, yp, up, vp);
    if (tex_yuv) run_phase(ren, tex_yuv, 2, W, H, 5000, "video YUV 1280x720", argb, yp, up, vp);

    LOG("FIN\n");
    fclose(lg);
    if (tex_argb) SDL_DestroyTexture(tex_argb);
    if (tex_yuv) SDL_DestroyTexture(tex_yuv);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    free(argb); free(yp); free(up); free(vp);
    return 0;
}
