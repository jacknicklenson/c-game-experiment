// Copyright (c) 2023 CODESOLE
// main.c: entry point
// SPDX-License-Identifier: MIT

#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "chipmunk/chipmunk.h"
#include "enet/enet.h"
#include "flecs.h"
#include <stdio.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

#define i_key ecs_entity_t
#define i_tag entity
#define i_use_cmp
#include "stc/cvec.h"

#if defined(_MSC_VER) && defined(NDEBUG)
#pragma comment(linker, "/ENTRY:mainCRTStartup")
#endif

struct game_state {
  uint32_t score;
  uint64_t passed_time_seconds;
};

struct editor_context {
  SDL_Window *win;
  SDL_Event event;
  SDL_Renderer *ren;
  struct nk_context *nkctx;
  cpSpace *space;
  cpBody *static_body;
  ecs_world_t *world;
  cvec_entity entities;
  ecs_entity_t last_selected_entity;
  uint32_t fps;
  uint32_t is_running;
};

enum component_type {
  COMPONENT_TYPE_POSITION = 0,
  COMPONENT_TYPE_ROTATION,
  COMPONENT_TYPE_SCALE
};

/* COMPONENTS */
typedef struct {
  float x, y;
} ecs_comp_position;
ECS_COMPONENT_DECLARE(ecs_comp_position);
#define DEFAULT_INIT_ECS_COMP_POSITION                                         \
  { 0.f, 0.f }

typedef struct {
  float angle;
} ecs_comp_rotation;
ECS_COMPONENT_DECLARE(ecs_comp_rotation);
#define DEFAULT_INIT_ECS_COMP_ROTATION                                         \
  { 0.f }

typedef struct {
  float x, y;
} ecs_comp_scale;
ECS_COMPONENT_DECLARE(ecs_comp_scale);
#define DEFAULT_INIT_ECS_COMP_SCALE                                            \
  { 1.f, 1.f }

enum { GIZMO_TYPE_TRANSLATE = 0, GIZMO_TYPE_SCALE, GIZMO_TYPE_ROTATE };
int LAST_SELECTED_GIZMO = GIZMO_TYPE_TRANSLATE;

/* FUNCTIONS */
void editor_ui(struct editor_context *ctx);
void draw_gizmos(struct editor_context *ctx, int gizmo_type);
void game_ui(struct editor_context *ctx);
void render(struct editor_context *ctx);
void parse_commandline_args(struct editor_context *ctx, int argc, char **argv);
void editor_init(struct editor_context *ctx);
void input_handle(struct editor_context *ctx);
void physics_init(struct editor_context *ctx, int iteration, cpVect gravity,
                  float sleeptime_thresh, float collision_slop);
void create_entity(struct editor_context *ctx);
void add_component(struct editor_context *ctx, ecs_entity_t eid,
                   enum component_type ct);
void load_font(struct editor_context *ctx, const char *path, float height);
SDL_Texture *load_bmp_image(struct editor_context *ctx, const char *path);
void cleanup(struct editor_context *ctx);

int
main(int argc, char **argv) {
  struct editor_context ctx = {0};
  parse_commandline_args(&ctx, argc, argv);

  physics_init(&ctx, 90, cpv(0, 9.0), 0.5f, 0.5f);

  cpBody *body;
  cpShape *shape;

  shape = cpSegmentShapeNew(ctx.static_body, cpv(0.0f, 100.0f),
                            cpv(320.f, 100.f), 0);
  cpShapeSetFriction(shape, 1);
  cpSpaceAddShape(ctx.space, shape);

  body = cpSpaceAddBody(ctx.space,
                        cpBodyNew(1.0f, cpMomentForBox(1.0f, 30.0f, 30.0f)));
  cpBodySetPosition(body, cpv(0.0f, 0.0f));

  shape = cpSpaceAddShape(ctx.space, cpBoxShapeNew(body, 30.0f, 30.0f, 0.5f));
  cpShapeSetElasticity(shape, 0.0f);
  cpShapeSetFriction(shape, 0.4f);

  editor_init(&ctx);
  uint64_t new, old = SDL_GetTicks64();

  load_font(&ctx, "assets/hack.ttf", 14.f);

  /* SDL_Texture *tex = load_bmp_image(&ctx, "assets/wolox_logo.bmp"); */

  while (ctx.is_running) {
    input_handle(&ctx);
    /* cpVect pos = cpBodyGetPosition(body); */
    cpSpaceStep(ctx.space, 0.01);
    render(&ctx);
    SDL_Delay(10);
    new = SDL_GetTicks64();
    ctx.fps = (uint32_t)(1.f / ((float)(new - old) / 1000.f));
    old = new;
  }

  /* SDL_DestroyTexture(tex); */
  cpShapeFree(shape);
  cpBodyFree(body);
  cleanup(&ctx);
  return 0;
}

void
render(struct editor_context *ctx) {
  SDL_RenderClear(ctx->ren);

  game_ui(ctx);
#ifndef NDEBUG
  editor_ui(ctx);
#endif

  SDL_SetRenderDrawColor(ctx->ren, 0, 0, 0, 255);
  nk_sdl_render(NK_ANTI_ALIASING_ON);
  SDL_RenderPresent(ctx->ren);
}

#define VERSION "0.1.0"
#include "stc/coption.h"
void
parse_commandline_args(struct editor_context *ctx, int argc, char **argv) {
  (void)ctx;
  coption_long longopts[] = {{"version", coption_no_argument, 'v'},
                             {"help", coption_no_argument, 'h'},
                             {"debug-output", coption_optional_argument, 'd'},
                             /* {"file", coption_required_argument, 'f'}, */
                             {0}};
  const char *shortopts = "xy:";
  coption opt = coption_init();
  int c;
  while ((c = coption_get(&opt, argc, argv, shortopts, longopts)) != -1) {
    switch (c) {
    case '?':
      printf("error: unknown option: %s\n", opt.optstr);
      break;
    case ':':
      printf("error: missing argument for %s.\n", opt.optstr);
      goto help;
    case 'v':
      printf("VERSION: %s\n", VERSION);
      exit(0);
    case 'h':
    help:
      printf("Usage: %s \n", argv[0]);
      puts("-x");
      puts("-y ARG");
      /* puts("--file <config file>"); */
      puts("--version");
      puts("--debug-output [<directory>]");
      puts("--help");
      exit(0);
    default:
      printf("option: %c [%s]\n", opt.opt, opt.arg ? opt.arg : "");
      break;
    }
  }
  printf("\nNon-option arguments:");
  for (int i = opt.ind; i < argc; ++i)
    printf(" %s", argv[i]);
  putchar('\n');
}

void
physics_init(struct editor_context *ctx, int iteration, cpVect gravity,
             float sleeptime_thresh, float collision_slop) {
  ctx->space = cpSpaceNew();
  cpSpaceSetIterations(ctx->space, iteration);
  cpSpaceSetGravity(ctx->space, gravity);
  cpSpaceSetSleepTimeThreshold(ctx->space, sleeptime_thresh);
  cpSpaceSetCollisionSlop(ctx->space, collision_slop);
  ctx->static_body = cpSpaceGetStaticBody(ctx->space);
}

void
editor_init(struct editor_context *ctx) {
  ctx->last_selected_entity = 0;
  ctx->is_running = 1;
  ctx->entities = cvec_entity_init();
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }
  ctx->fps = 0;
  ctx->win = SDL_CreateWindow("wolox", 100, 100, 620, 400, SDL_WINDOW_SHOWN);
  if (ctx->win == NULL) {
    fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }
  SDL_SetWindowResizable(ctx->win, SDL_TRUE);

  ctx->ren = SDL_CreateRenderer(
      ctx->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (ctx->ren == NULL) {
    fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(ctx->win);
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  ctx->world = ecs_init();
}

void
load_font(struct editor_context *ctx, const char *path, float height) {
  ctx->nkctx = nk_sdl_init(ctx->win, ctx->ren);
  {
    struct nk_font_atlas *atlas;
    struct nk_font_config config = nk_font_config(0);
    struct nk_font *font;

    /* set up the font atlas and add desired font; note that font sizes are
     * multiplied by font_scale to produce better results at higher DPIs */
    nk_sdl_font_stash_begin(&atlas);
    /* font = nk_font_atlas_add_default(atlas, 13 * 1.0f, &config); */
    font = nk_font_atlas_add_from_file(atlas, path, height, &config);
    nk_sdl_font_stash_end();

    /* this hack makes the font appear to be scaled down to the desired
     * size and is only necessary when font_scale > 1 */
    /* font->handle.height /= font_scale; */
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    nk_style_set_font(ctx->nkctx, &font->handle);
  }
}

SDL_Texture *
load_bmp_image(struct editor_context *ctx, const char *path) {
  SDL_Surface *bmp = SDL_LoadBMP(path);
  if (bmp == NULL) {
    fprintf(stderr, "SDL_LoadBMP Error: %s\n", SDL_GetError());
    SDL_DestroyRenderer(ctx->ren);
    SDL_DestroyWindow(ctx->win);
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  printf("PIXEL_FORMAT: %s\n", SDL_GetPixelFormatName(bmp->format->format));
  SDL_Texture *tex = SDL_CreateTextureFromSurface(ctx->ren, bmp);
  if (tex == NULL) {
    fprintf(stderr, "SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
    SDL_FreeSurface(bmp);
    SDL_DestroyRenderer(ctx->ren);
    SDL_DestroyWindow(ctx->win);
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  SDL_FreeSurface(bmp);
  return tex;
}

void
input_handle(struct editor_context *ctx) {
  nk_input_begin(ctx->nkctx);
  while (SDL_PollEvent(&ctx->event)) {
    switch (ctx->event.type) {
    case SDL_QUIT:
      ctx->is_running = 0;
      break;
    case SDL_KEYDOWN:
      switch (ctx->event.key.keysym.sym) {
      case SDLK_ESCAPE:
        ctx->is_running = 0;
        break;
        /*   case SDLK_d: */
        /*     x += 5; */
        /*     break; */
        /*   case SDLK_a: */
        /*     x -= 5; */
        /*     break; */
        /*   case SDLK_w: */
        /*     y -= 5; */
        /*     break; */
        /*   case SDLK_s: */
        /*     y += 5; */
        /*     break; */
      default:
        break;
      }
      break;
    default:
      break;
    }
    nk_sdl_handle_event(&ctx->event);
  }
  nk_input_end(ctx->nkctx);
}

void
editor_ui(struct editor_context *ctx) {
  if (ctx->last_selected_entity) {
    draw_gizmos(ctx, LAST_SELECTED_GIZMO);
  }
  struct nk_style_window win_style = ctx->nkctx->style.window;
  ctx->nkctx->style.window.padding = nk_vec2(0.f, 0.f);
  int w, h;
  SDL_GetWindowSize(ctx->win, &w, &h);

  if (nk_begin(ctx->nkctx, "ToolBar",
               nk_rect(0.f, (float)h - 50.f, (float)w, 50.f),
               NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(ctx->nkctx, 50, 4);
    if (nk_button_label(ctx->nkctx, "Add Entity")) {
      ecs_entity_t e = ecs_new_id(ctx->world);
      ECS_COMPONENT_DEFINE(ctx->world, ecs_comp_position);
      ecs_set(ctx->world, e, ecs_comp_position, {200.f, 250.f});
      cvec_entity_push_back(&ctx->entities, e);
      ctx->last_selected_entity = e;
    }
    if (nk_button_label(ctx->nkctx, "Translate")) {
      LAST_SELECTED_GIZMO = GIZMO_TYPE_TRANSLATE;
    }
    if (nk_button_label(ctx->nkctx, "Scale")) {
      LAST_SELECTED_GIZMO = GIZMO_TYPE_SCALE;
    }
    if (nk_button_label(ctx->nkctx, "Rotate")) {
      LAST_SELECTED_GIZMO = GIZMO_TYPE_ROTATE;
    }
  }
  nk_end(ctx->nkctx);
  ctx->nkctx->style.window = win_style;
}

void
game_ui(struct editor_context *ctx) {
  struct nk_style_window win_style = ctx->nkctx->style.window;
  ctx->nkctx->style.window.padding = nk_vec2(0.f, 0.f);
  ctx->nkctx->style.window.background = nk_rgb(0, 0, 0);
  if (nk_begin(ctx->nkctx, "GameUI", nk_rect(0, 0, 99, 25),
               NK_WINDOW_NOT_INTERACTIVE | NK_WINDOW_NO_SCROLLBAR)) {
    nk_layout_row_dynamic(ctx->nkctx, 25, 1);
    nk_labelf_colored(ctx->nkctx, NK_TEXT_LEFT | NK_TEXT_ALIGN_LEFT,
                      (struct nk_color){255, 0, 0, 255}, "FPS: %d", ctx->fps);
  }
  nk_end(ctx->nkctx);
  ctx->nkctx->style.window = win_style;
}

void
cleanup(struct editor_context *ctx) {
  nk_sdl_shutdown();
  SDL_DestroyRenderer(ctx->ren);
  SDL_DestroyWindow(ctx->win);
  SDL_Quit();

  // Clean up our objects and exit!
  cpSpaceFree(ctx->space);

  ecs_fini(ctx->world);
}

void
draw_gizmos(struct editor_context *ctx, int gizmo_type) {
  const ecs_comp_position *p =
      ecs_get(ctx->world, ctx->last_selected_entity, ecs_comp_position);
  float x = p->x;
  float y = p->y;

  if (p == NULL) {
    puts("Selected Entity doesnt have Position Component!");
    return;
  }

  SDL_Vertex y_triangle[3] = {
      {{x - 10.f, y - 100.f}, {0, 255, 0, 255}, {0}},
      {{x, y - 100.f - 10.f}, {0, 255, 0, 255}, {0}},
      {{x + 10.f, y - 100.f}, {0, 255, 0, 255}, {0}},
  };
  SDL_Vertex x_triangle[3] = {
      {{x + 100.f, y - 10.f}, {255, 0, 0, 255}, {0}},
      {{x + 100.f + 10.f, y}, {255, 0, 0, 255}, {0}},
      {{x + 100.f, y + 10.f}, {255, 0, 0, 255}, {0}},
  };

  uint8_t r, g, b, a;
  SDL_GetRenderDrawColor(ctx->ren, &r, &g, &b, &a);
  int mx, my;
  uint32_t mevent = SDL_GetMouseState(&mx, &my);
  SDL_FPoint mp = {(float)mx, (float)my};
  bool m1click = mevent == SDL_BUTTON(1) ? 1 : 0;
  switch (gizmo_type) {
  case GIZMO_TYPE_TRANSLATE: {
    // y direction
    SDL_FRect gizmo_rect_y = {x - 5.f, y - 100.f, 10.f, 100.f};
    if (SDL_PointInFRect(&mp, &gizmo_rect_y) && m1click) {
      int mx2, my2;
      SDL_GetRelativeMouseState(&mx2, &my2);
      ecs_set(ctx->world, ctx->last_selected_entity, ecs_comp_position,
              {p->x, p->y + (float)(my2 - my)});
    }
    SDL_SetRenderDrawColor(ctx->ren, 0, 255, 0, 255);
    SDL_RenderFillRectF(ctx->ren, &gizmo_rect_y);
    SDL_RenderGeometry(ctx->ren, NULL, &y_triangle[0], 3, NULL, 0);

    // x direction
    SDL_FRect gizmo_rect_x = {x, y - 5.f, 100.f, 10.f};
    if (SDL_PointInFRect(&mp, &gizmo_rect_x) && m1click) {
      int mx2, my2;
      SDL_GetRelativeMouseState(&mx2, &my2);
      printf("%f\n", (float)(mx2 - mx));
      ecs_set(ctx->world, ctx->last_selected_entity, ecs_comp_position,
              {p->x + (float)(mx2 - mx), p->y});
    }
    SDL_SetRenderDrawColor(ctx->ren, 255, 0, 0, 255);
    SDL_RenderFillRectF(ctx->ren, &gizmo_rect_x);
    SDL_RenderGeometry(ctx->ren, NULL, &x_triangle[0], 3, NULL, 0);
  } break;
  case GIZMO_TYPE_SCALE: {
    // y direction
    SDL_FRect gizmo_rect_y = {x - 5.f, y - 100.f, 10.f, 100.f};
    SDL_SetRenderDrawColor(ctx->ren, 0, 255, 0, 255);
    SDL_RenderFillRectF(ctx->ren, &gizmo_rect_y);
    SDL_RenderGeometry(ctx->ren, NULL, &y_triangle[0], 3, NULL, 0);

    // x direction
    SDL_SetRenderDrawColor(ctx->ren, 255, 0, 0, 255);
    SDL_FRect gizmo_rect_x = {x, y - 5.f, 100.f, 10.f};
    SDL_RenderFillRectF(ctx->ren, &gizmo_rect_x);
    SDL_RenderGeometry(ctx->ren, NULL, &x_triangle[0], 3, NULL, 0);
  } break;
  case GIZMO_TYPE_ROTATE: {
    // y direction
    SDL_FRect gizmo_rect_y = {x - 5.f, y - 100.f, 10.f, 100.f};
    SDL_SetRenderDrawColor(ctx->ren, 0, 255, 0, 255);
    SDL_RenderFillRectF(ctx->ren, &gizmo_rect_y);
    SDL_RenderGeometry(ctx->ren, NULL, &y_triangle[0], 3, NULL, 0);

    // x direction
    SDL_SetRenderDrawColor(ctx->ren, 255, 0, 0, 255);
    SDL_FRect gizmo_rect_x = {x, y - 5.f, 100.f, 10.f};
    SDL_RenderFillRectF(ctx->ren, &gizmo_rect_x);
    SDL_RenderGeometry(ctx->ren, NULL, &x_triangle[0], 3, NULL, 0);
  } break;
  default:
    perror("INVALID GIZMO TYPE");
    abort();
  }
  SDL_SetRenderDrawColor(ctx->ren, r, g, b, a);
}
