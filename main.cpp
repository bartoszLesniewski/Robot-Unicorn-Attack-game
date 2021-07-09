#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>


extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH 640        // domyœlna szerokoœæ ekranu
#define SCREEN_HEIGHT 480       // wysokoœæ ekranu
#define UNICORN_WIDTH 90        // szerokoœæ jednoro¿ca
#define UNICORN_HEIGHT 90       // wysokoœæ jednoro¿ca
#define OBSTACLE_WIDTH 40       // szerokoœæ podstawowej przeszkody
#define OBSTACLE_HEIGHT 40      // wysokoœæ podstawowej przeszkody
#define GROUND_WIDTH 200        // szerokoœæ przeszkody w postaci terenu o wyraŸnie wiêkszej wysokoœci
#define GROUND_HEIGHT 90        // wysokoœæ przeszkody w postaci terenu o wyraŸnie wiêkszej wysokoœci
#define STALACTITE_WIDTH 90     // szerokoœæ stalaktytu
#define STALACTITE_HEIGHT 300   // wysokoœæ stalaktytu
#define MAX_OBSTACLES 14        // maksymalna liczba przeszkód w jednej scenie
#define MAX_PLATFORMS 18        // maksymalna liczba platform w jednej scenie
#define MAX_JUMP_HEIGHT 240     // maksymalna wysokoœæ, do której mo¿e wznieœæ siê jednoro¿ec podczas skoku (po przytrzymaniu klawisza)
#define PLATFORM_WIDTH 400      // szerokoœæ platformy
#define PLATFORM_HEIGHT 40      // wysokoœæ platformy
#define LEVEL_HEIGHT 720        // wysokoœæ etapu (etap wy¿szy ni¿ wysokoœæ okna)
#define UNICORN_SPEED 30000     // szybkoœæ jednoro¿ca
#define GRAVITY 50000           // sta³a wartoœæ grawitacji 
#define MOVE_SPEED 160          // szybkoœæ, z jak¹ przesuwa siê plansza
#define DASH_DURATION 0.5       // d³ugoœæ zrywu
#define JUMP_DURATION 1         // d³ugoœæ skoku
#define DASH_LOCK_TIME 3        // blokada na wykonanie kolejnego zrywu (w sekundach)
#define UNICORN_POSITION_CHANGE 10 // wartoœæ zmiany pozycji jednoro¿ca przy manualnym sterowaniu strza³kami
#define MOVE_CHANGE 5000 // zmiana pozycji planszy przy manualnym sterowaniu strza³kami

// status gry
enum gameStatus
{
    undefined = 0, // pocz¹tek gry - menu g³ówne
    play = 1,       // rozgrywka
    collision = 2,  // wykryta kolizja - zapytanie o kontynuacjê
    lost = 3        // przegrana - wyœwietlenie menu
};

// sterowanie 
enum control
{
    basic = 1,      // podstawowe - strza³ki
    extended = 2,   // rozszerzone - z oraz x
    menu = 3,       // sterowanie w menu - strza³ki
    none = 4        // sterowanie wy³¹czone (tylko ESC i n), po kolizji
};

// reprezentacja aktywnej pozcyji w menu
enum menuOption
{
    newGameOpt = 0, // pozycja "NOWA GRA"
    exitGameOpt = 1 // pozycja "WYJŒCIE"
};

// typy przeszkód
enum obstacleType
{
    simple = 1,     // podstawowa przeszkoda
    ground = 2,     // teren o znacznie wiêkszej wysokoœci
    stalactite = 3, // stalaktyt
    star = 4        // gwiazda
};

// pozycja w postaci wspo³rzêdnych o wartoœciach zmiennoprzecinkowych (dla wiêkszej precyzji)
struct position
{
    double x;
    double y;
};

// struktury SDL
struct sdlStructures
{
    SDL_Event event;
    SDL_Surface* screen, * charset;
    SDL_Surface* unicorn, * background, * obstacle, * star, * ground, * stalactite, * gameOver, * platform, * life, * menu1, * menu2;
    SDL_Texture* scrtex;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Rect dest;
    SDL_Rect camera;
};

// zmienne dla gry
struct gameVariables
{
    int t1, t2, quit = 0, frames = 0, posX = 0, speed = MOVE_SPEED;
    double delta, distance = 0, worldTime = 0, fpsTimer = 0, fps = 0, gameTime = 0, speedCounter = 0;
    control cType = menu;
    char text[128];
    int czarny, zielony, czerwony, niebieski;
    bool jumpKeyDown = false;
    int widthChange = 0;
    gameStatus status = undefined;
    menuOption opt = newGameOpt;
};

// parametry jednoro¿ca
struct unicornParam
{
    position unicornPosition;       // pozycja (typ double)  
    int jumpLimit = 2;      // limit skoków
    bool isJumping = false; // flaga reprezentuj¹ca, czy jednoro¿ec w³aœnie wykonuje skok
    bool dashActive = false; // flaga reprezentuj¹ca, czy jednoro¿ec w³aœnie wykonuje zryw
    double dashStart = 0;   // zmienna do przechowywania czasu rozpoczêcia zrywu 
    double lastDashTime = 0;        // czas ostatniego zrywu (zerowany po up³ywie 3s, czyli maksymalnej blokady zrywu)
    double jumpStart = 0;   // czas rozpoczêcia skoku (przy przytrzymanym klawiszu)
    int lifes = 3;  // liczba ¿yæ jednoro¿ca
};

// parametry przeszkody
struct obstacleParam
{
    position pos; // pozcyja (typ double)
    obstacleType type;      // typ przeszkody
    int platformNumber;     // numer platformy, na której wystêpuje przeszkoda
};

// narysowanie napisu txt na powierzchni screen, zaczynaj¹c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj¹ca znaki
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset)
{
    int px, py, c;
    SDL_Rect s, d;
    s.w = 8;
    s.h = 8;
    d.w = 8;
    d.h = 8;
    while (*text)
    {
        c = *text & 255;
        px = (c % 16) * 8;
        py = (c / 16) * 8;
        s.x = px;
        s.y = py;
        d.x = x;
        d.y = y;
        SDL_BlitSurface(charset, &s, screen, &d);
        x += 8;
        text++;
    };
};

// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt œrodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y)
{
    SDL_Rect dest;
    dest.x = x - sprite->w / 2;
    dest.y = y - sprite->h / 2;
    dest.w = sprite->w;
    dest.h = sprite->h;
    SDL_BlitSurface(sprite, NULL, screen, &dest);
};

// rysowanie pojedynczego pixela
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color)
{
    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
    *(Uint32*)p = color;
};

// rysowanie linii o d³ugoœci l w pionie (gdy dx = 0, dy = 1) 
// b¹dŸ poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color)
{
    for (int i = 0; i < l; i++)
    {
        DrawPixel(screen, x, y, color);
        x += dx;
        y += dy;
    };
};

// rysowanie prostok¹ta o d³ugoœci boków l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor)
{
    int i;
    DrawLine(screen, x, y, k, 0, 1, outlineColor);
    DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
    DrawLine(screen, x, y, l, 1, 0, outlineColor);
    DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
    for (i = y + 1; i < y + k - 1; i++)
        DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

// obliczanie iloœci klatek na sekundê
void timer(gameVariables* gVar)
{
    gVar->t2 = SDL_GetTicks();
    gVar->delta = (gVar->t2 - gVar->t1) * 0.001; // czas w sekundach, jaki up³yn¹³ od ostatniego narysowania ekranu
    gVar->t1 = gVar->t2;

    gVar->worldTime += gVar->delta;

    gVar->distance = gVar->delta * UNICORN_SPEED;

    gVar->fpsTimer += gVar->delta;
    if (gVar->fpsTimer > 0.5)
    {
        gVar->fps = gVar->frames * 2;
        gVar->frames = 0;
        gVar->fpsTimer -= 0.5;
    };
}

// zwolnienie powierzchni
void freeSurfaces(sdlStructures* game)
{
    SDL_FreeSurface(game->charset);
    SDL_FreeSurface(game->screen);
    SDL_FreeSurface(game->unicorn);
    SDL_FreeSurface(game->background);
    SDL_FreeSurface(game->obstacle);
    SDL_FreeSurface(game->star);
    SDL_FreeSurface(game->ground);
    SDL_FreeSurface(game->stalactite);
    SDL_FreeSurface(game->gameOver);
    SDL_FreeSurface(game->platform);
    SDL_FreeSurface(game->life);
    SDL_FreeSurface(game->menu1);
    SDL_FreeSurface(game->menu2);
    SDL_DestroyTexture(game->scrtex);
    SDL_DestroyWindow(game->window);
    SDL_DestroyRenderer(game->renderer);
    SDL_Quit();
}

// inicjalizacja SDL
bool initialize(sdlStructures* game)
{
    game->camera.x = 0;
    game->camera.y = 0;
    game->camera.w = SCREEN_WIDTH;
    game->camera.h = SCREEN_HEIGHT;
    int rc;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return false;

    rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &(game->window), &(game->renderer));
    if (rc != 0)
        return false;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(game->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(game->window, "Szablon do zdania drugiego 2017");

    game->screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    game->scrtex = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    // wy³¹czenie widocznoœci kursora myszy
    SDL_ShowCursor(SDL_DISABLE);

    return true;
}

// inicjalizacja kolorów
void initializeColors(sdlStructures game, gameVariables* gVar)
{
    gVar->czarny = SDL_MapRGB(game.screen->format, 0x00, 0x00, 0x00);
    gVar->zielony = SDL_MapRGB(game.screen->format, 0x00, 0xFF, 0x00);
    gVar->czerwony = SDL_MapRGB(game.screen->format, 0xFF, 0x00, 0x00);
    gVar->niebieski = SDL_MapRGB(game.screen->format, 0x11, 0x11, 0xCC);
}

// ³adowanie bitmap
bool loadImages(sdlStructures* game)
{
    game->charset = SDL_LoadBMP("graphics/cs8x8.bmp");
    game->unicorn = SDL_LoadBMP("graphics/unicorn.bmp");
    game->background = SDL_LoadBMP("graphics/tecza.bmp");
    game->obstacle = SDL_LoadBMP("graphics/obstacle.bmp");
    game->star = SDL_LoadBMP("graphics/star.bmp");
    game->ground = SDL_LoadBMP("graphics/ground.bmp");
    game->stalactite = SDL_LoadBMP("graphics/stalactite.bmp");
    game->gameOver = SDL_LoadBMP("graphics/game_over.bmp");
    game->platform = SDL_LoadBMP("graphics/platform.bmp");
    game->life = SDL_LoadBMP("graphics/life.bmp");
    game->menu1 = SDL_LoadBMP("graphics/menu1.bmp");
    game->menu2 = SDL_LoadBMP("graphics/menu2.bmp");

    if (game->charset == NULL || game->unicorn == NULL || game->background == NULL || game->obstacle == NULL || game->star == NULL || game->ground == NULL || game->stalactite == NULL || game->gameOver == NULL || game->platform == NULL || game->life == NULL || game->menu1 == NULL || game->menu2 == NULL)
        return false;

    SDL_SetColorKey(game->charset, true, 0x000000);

    return true;
}

// inicjalizacja domyœlnych pozcyji dla platform
void initializePlatformsPositions(position platformsPositions[])
{
    double defaultY = LEVEL_HEIGHT / 2 + UNICORN_HEIGHT;
    position newPlatformsPositions[] = {
            {0 , defaultY},
            {600, defaultY},
            {1200, defaultY + 100},
            {1800, defaultY + 100},
            {2400, defaultY + 100},
            {3000, defaultY},
            {3200, defaultY - 150},
            {3800, defaultY},
            {4200, defaultY - 150},
            {4400, defaultY + 100},
            {4600, defaultY - 50},
            {4800, defaultY + 100},
            {5400, defaultY},
            {6000, defaultY},
            {6600, defaultY + 100},
            {7200, defaultY},
            {7800, defaultY},
            {8400, defaultY}
    };

    for (int i = 0; i < MAX_PLATFORMS; i++)
        platformsPositions[i] = newPlatformsPositions[i];
}

// inicjalizacja domyœlnych pozycji dla przeszkód
void initializeObstaclePositions(obstacleParam obstacles[], position platformsPositions[])
{
    obstacleParam newObstacles[MAX_OBSTACLES] = {
            {{platformsPositions[1].x + 180, platformsPositions[1].y - GROUND_HEIGHT}, ground, 1},
            {{platformsPositions[2].x + 340, platformsPositions[2].y - OBSTACLE_HEIGHT}, star, 2 },
            {{platformsPositions[2].x + 20, 0}, stalactite, 2 },
            {{platformsPositions[4].x + 180, platformsPositions[4].y - GROUND_HEIGHT}, ground, 4},
            {{platformsPositions[6].x + (rand() % (PLATFORM_WIDTH / 2 - OBSTACLE_WIDTH) + (PLATFORM_WIDTH / 2 - OBSTACLE_WIDTH)), platformsPositions[6].y - OBSTACLE_HEIGHT}, star, 6 },
            {{platformsPositions[8].x + (rand() % (PLATFORM_WIDTH / 2 - OBSTACLE_WIDTH) + (PLATFORM_WIDTH / 2 - OBSTACLE_WIDTH)), platformsPositions[8].y - OBSTACLE_HEIGHT}, simple, 8},
            {{platformsPositions[10].x + (rand() % (PLATFORM_WIDTH / 2 - OBSTACLE_WIDTH) + (PLATFORM_WIDTH / 2 - OBSTACLE_WIDTH)), platformsPositions[10].y - OBSTACLE_HEIGHT }, simple, 10},
            {{platformsPositions[12].x + 100, 0}, stalactite, 12 },
            {{platformsPositions[13].x + (rand() % (PLATFORM_WIDTH / 2 - OBSTACLE_WIDTH) + (PLATFORM_WIDTH / 2 - OBSTACLE_WIDTH)), platformsPositions[13].y - OBSTACLE_HEIGHT }, star, 13},
            {{platformsPositions[14].x + (rand() % (PLATFORM_WIDTH / 2 - OBSTACLE_WIDTH) + (PLATFORM_WIDTH / 2 - OBSTACLE_WIDTH)), platformsPositions[14].y - OBSTACLE_HEIGHT }, simple, 14},
            {{platformsPositions[15].x + 50, platformsPositions[15].y - OBSTACLE_HEIGHT }, simple, 15},
            {{platformsPositions[15].x + 300, 0}, stalactite, 15 },
            {{platformsPositions[16].x + 180, platformsPositions[16].y - GROUND_HEIGHT }, ground, 16},
            {{platformsPositions[17].x + 100, 0}, stalactite, 17 }
    };

    for (int i = 0; i < MAX_OBSTACLES; i++)
        obstacles[i] = newObstacles[i];
}

// aktualizacja ekranu
void updateScreen(sdlStructures game)
{
    SDL_UpdateTexture(game.scrtex, NULL, game.screen->pixels, game.screen->pitch);
    //              SDL_RenderClear(renderer);
    SDL_RenderCopy(game.renderer, game.scrtex, NULL, NULL);
    SDL_RenderPresent(game.renderer);
}

// rysowanie tablic (prosotk¹tów) z informacjami
void drawBoard(sdlStructures* game, gameVariables* gVar, position unicornPosition)
{
    DrawRectangle(game->screen, SCREEN_WIDTH / 2 - 80, 50, SCREEN_WIDTH / 4, 20, gVar->czarny, gVar->zielony);
    sprintf(gVar->text, "Czas gry: %.1lf", gVar->gameTime);
    DrawString(game->screen, game->screen->w / 2 - strlen(gVar->text) * 8 / 2, 55, gVar->text, game->charset);
    DrawRectangle(game->screen, 4, 4, SCREEN_WIDTH - 8, 36, gVar->czerwony, gVar->niebieski);
    sprintf(gVar->text, "Robot Unicorn Attack, czas trwania = %.1lf s  %.0lf klatek / s", gVar->worldTime, gVar->fps);
    DrawString(game->screen, game->screen->w / 2 - strlen(gVar->text) * 8 / 2, 10, gVar->text, game->charset);
    sprintf(gVar->text, "Esc - wyjscie, n - nowa rozgrywka, d - zmiana sterowania");
    DrawString(game->screen, game->screen->w / 2 - strlen(gVar->text) * 8 / 2, 26, gVar->text, game->charset);
}

// wyœwietlanie jednoro¿ca
void displayUnicorn(SDL_Surface* screen, SDL_Surface* unicorn, position unicornPosition, SDL_Rect camera)
{
    SDL_Rect rect;
    rect.x = unicornPosition.x;
    rect.y = unicornPosition.y - camera.y;

    SDL_BlitSurface(unicorn, NULL, screen, &rect);
}

// wyœwietlanie platform
void displayPlatforms(SDL_Surface* screen, SDL_Surface* platform, position platformsPositions[], SDL_Rect camera)
{
    SDL_Rect platformRect;

    for (int i = 0; i < MAX_PLATFORMS; i++)
    {
        platformRect.x = platformsPositions[i].x;
        platformRect.y = platformsPositions[i].y - camera.y;
        platformRect.w = 400;
        platformRect.h = 40;

        SDL_BlitSurface(platform, NULL, screen, &platformRect);
    }
}

// wyœwietlanie przeszkód
void displayObstacles(sdlStructures game, obstacleParam obstacles[], SDL_Rect camera)
{
    SDL_Rect obstacleRect;

    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        obstacleRect.x = obstacles[i].pos.x;
        obstacleRect.y = obstacles[i].pos.y - camera.y;

        if (obstacles[i].type == ground)
            SDL_BlitSurface(game.ground, NULL, game.screen, &obstacleRect);

        else if (obstacles[i].type == stalactite)
            SDL_BlitSurface(game.stalactite, NULL, game.screen, &obstacleRect);
        else if (obstacles[i].type == star)
            SDL_BlitSurface(game.star, NULL, game.screen, &obstacleRect);
        else
            SDL_BlitSurface(game.obstacle, NULL, game.screen, &obstacleRect);
    }
}

// wyœwietlanie pozosta³ych ¿yæ jednoro¿ca
void displayLifes(SDL_Surface* screen, SDL_Surface* life, int lifes)
{
    SDL_Rect lifeRect;
    lifeRect.x = 20;
    lifeRect.y = 50;

    for (int i = 0; i < lifes; i++)
    {
        SDL_BlitSurface(life, NULL, screen, &lifeRect);
        lifeRect.x += 40;
    }
}

// zbiorcza funkcja, wyœwietlaj¹ca wszystkie elementy gry
void displayGameElements(sdlStructures game, unicornParam unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    SDL_BlitSurface(game.background, &(game.camera), game.screen, NULL);
    displayLifes(game.screen, game.life, unicorn.lifes);
    displayUnicorn(game.screen, game.unicorn, unicorn.unicornPosition, game.camera);
    displayPlatforms(game.screen, game.platform, platformsPositions, game.camera);
    displayObstacles(game, obstacles, game.camera);
}

// ustawianie kamery, która œrodkuje pozycjê jednoro¿ca na ekranie
void setCamera(SDL_Rect* camera, unicornParam unicorn)
{
    camera->y = (unicorn.unicornPosition.y + UNICORN_HEIGHT / 2) - SCREEN_HEIGHT / 2;
    if (camera->y < 0)
        camera->y = 0;
    if (camera->y > LEVEL_HEIGHT - camera->h)
        camera->y = LEVEL_HEIGHT - camera->h;
}

// aktualizacja pozycji platform i przeszkód, gdy skoñczy siê scena
void updatePositions(obstacleParam obstacles[], position platformsPositions[], int i)
{
    position beforeLast = platformsPositions[i];
    position last = platformsPositions[i + 1];
    obstacleParam beforeLastObstacle;
    obstacleParam lastObstacle;
    bool checkBeforeLast = false, checkLast = false;

    for (int j = 0; j < MAX_OBSTACLES; j++)
    {
        if (obstacles[j].platformNumber == i)
        {
            checkBeforeLast = true;
            beforeLastObstacle = obstacles[j];
        }
        if (obstacles[j].platformNumber == i + 1)
        {
            checkLast = true;
            lastObstacle = obstacles[j];
        }
    }

    initializePlatformsPositions(platformsPositions);
    platformsPositions[0] = beforeLast;
    platformsPositions[1] = last;

    initializeObstaclePositions(obstacles, platformsPositions);
    if (checkBeforeLast)
        obstacles[0] = beforeLastObstacle;
    if (checkLast)
        obstacles[1] = lastObstacle;

}

// przemierzanie sceny (ruch elementów)
void move(obstacleParam obstacles[], position platformsPositions[], gameVariables gVar, int speed)
{
    for (int i = 0; i < MAX_OBSTACLES; i++)
        obstacles[i].pos.x -= speed * gVar.delta;

    for (int i = 0; i < MAX_PLATFORMS; i++)
    {
        platformsPositions[i].x -= speed * gVar.delta;

        if (platformsPositions[i].x < PLATFORM_WIDTH / 2 && i == MAX_PLATFORMS - 2)
        {
            updatePositions(obstacles, platformsPositions, i);
        }
    }
}

// sprawdzenie pozycji w celu ustalenia, czy jednoro¿ec ma pod sob¹ jakieœ pod³o¿e
bool comparePositions(position unicornPosition, double objectPosX, double objectPosY, int objectWidth, int objectHeight)
{
    SDL_Rect unicornRect, baseRect;
    int topU, topP;

    unicornRect.w = UNICORN_WIDTH;
    unicornRect.h = UNICORN_HEIGHT;
    baseRect.w = objectWidth;
    baseRect.h = objectHeight;

    unicornRect.x = unicornPosition.x;
    unicornRect.y = unicornPosition.y;
    baseRect.x = objectPosX;
    baseRect.y = objectPosY;

    topU = unicornRect.y;
    topP = baseRect.y;

    if (topP == topU + UNICORN_HEIGHT)
    {
        unicornRect.y += 2;
        if (SDL_HasIntersection(&unicornRect, &baseRect))
            return true;
    }

    return false;
}

// sprawdzenie, czy jednor¿ec ma pod sob¹ jakieœ pod³o¿e
bool checkBase(position unicornPosition, position platformsPositions[], obstacleParam obstacles[])
{
    for (int i = 0; i < MAX_PLATFORMS; i++)
    {
        if (comparePositions(unicornPosition, platformsPositions[i].x, platformsPositions[i].y, PLATFORM_WIDTH, PLATFORM_HEIGHT))
            return true;
    }

    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        if (obstacles[i].type == ground)
        {
            if (comparePositions(unicornPosition, obstacles[i].pos.x, obstacles[i].pos.y, GROUND_WIDTH, GROUND_HEIGHT))
                return true;
        }
    }

    return false;
}

// ustalenie wymiarów przeszkody na podstawie jej typu
void defineObstacleDimensions(obstacleType type, SDL_Rect* obstacleRect)
{
    if (type == ground)
    {
        obstacleRect->w = GROUND_WIDTH;
        obstacleRect->h = GROUND_HEIGHT;
    }

    else if (type == stalactite)
    {
        obstacleRect->w = STALACTITE_WIDTH;
        obstacleRect->h = STALACTITE_HEIGHT;
    }
    else
    {
        obstacleRect->w = OBSTACLE_WIDTH;
        obstacleRect->h = OBSTACLE_HEIGHT;
    }
}

// sprawdzenie czy wyst¹pi³a kolizja 
bool checkCollision(unicornParam unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    SDL_Rect rect1, rect2;
    rect1.x = unicorn.unicornPosition.x;
    rect1.y = unicorn.unicornPosition.y;
    rect1.w = UNICORN_WIDTH;
    rect1.h = UNICORN_HEIGHT;

    if (unicorn.unicornPosition.y > LEVEL_HEIGHT - UNICORN_HEIGHT) // jednoro¿ec wypad³ poza doln¹ krawêdŸ ekranu
        return true;

    for (int i = 0; i < MAX_OBSTACLES; i++) // sprawdzenie kolizji z przeszkod¹
    {
        rect2.x = obstacles[i].pos.x;
        rect2.y = obstacles[i].pos.y;
        defineObstacleDimensions(obstacles[i].type, &rect2);

        if (obstacles[i].type != star || !unicorn.dashActive) // brak kolizji z gwiazd¹ podczas zrywu
        {
            if (SDL_HasIntersection(&rect1, &rect2))
                return true;
        }
    }

    for (int i = 0; i < MAX_PLATFORMS; i++) // sprawdzenie kolizji z platform¹
    {
        rect2.x = platformsPositions[i].x;
        rect2.y = platformsPositions[i].y;
        rect2.w = PLATFORM_WIDTH;
        rect2.h = PLATFORM_HEIGHT;

        if (SDL_HasIntersection(&rect1, &rect2) && !checkBase(unicorn.unicornPosition, platformsPositions, obstacles))
            return true;
    }

    return false;
}

// resetowanie parametrów jednoro¿ca (przed rozpoczêciem nowej gry lub kontynuacj¹ gry po kolizji)
void resetUnicornParam(unicornParam* unicorn)
{
    unicorn->unicornPosition.x = 0;
    unicorn->unicornPosition.y = LEVEL_HEIGHT / 2;
    unicorn->jumpLimit = 2;
    unicorn->isJumping = false;
    unicorn->dashActive = false;
    unicorn->dashStart = 0;
    unicorn->lastDashTime = 0;
    unicorn->jumpStart = 0;
}

// ustawienie parametrów dla nowej gry
void newGame(gameVariables* gVar, unicornParam* unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    gVar->gameTime = 0;
    gVar->speed = MOVE_SPEED;
    gVar->cType = extended;
    gVar->status = play;
    gVar->widthChange = 0;

    resetUnicornParam(unicorn);

    initializePlatformsPositions(platformsPositions);
    initializeObstaclePositions(obstacles, platformsPositions);
}

// sprawdzenie czy jednoro¿ec nie ma nad sob¹ platfromy, gdy chce wykonaæ skok
// jeœli tak, to nie ginie, lecz "odbija siê" od niej
double checkIfPlatformAbove(position unicornPosition, position platformsPositions[])
{
    SDL_Rect unicornRect, platformRect;
    unicornRect.x = unicornPosition.x;
    unicornRect.y = unicornPosition.y;
    int leftP, rightP, bottomP;
    int leftU = unicornRect.x, rightU = unicornRect.x + UNICORN_WIDTH, topU = unicornRect.y;

    for (int i = 0; i < MAX_PLATFORMS; i++)
    {
        platformRect.x = platformsPositions[i].x;
        platformRect.y = platformsPositions[i].y;
        leftP = platformRect.x;
        rightP = platformRect.x + PLATFORM_WIDTH;
        bottomP = platformRect.y + PLATFORM_HEIGHT;

        if (bottomP <= topU && ((leftU >= leftP && leftU <= rightP && rightU >= leftP && rightU <= rightP) || (leftU < leftP && leftU < rightP && rightU >= leftP && rightU <= rightP) || (leftU >= leftP && leftU <= rightP && rightU > rightP)))
            return platformsPositions[i].y + PLATFORM_HEIGHT;
    }

    return -1;
}

// wykonanie skoku (jednokrotne wciœniêcie klawisza)
void jump(gameVariables* gVar, unicornParam* unicorn, position platformsPositions[])
{
    if (unicorn->unicornPosition.y - gVar->distance >= 0 && !gVar->jumpKeyDown && unicorn->jumpLimit > 0)
    {
        double result = checkIfPlatformAbove(unicorn->unicornPosition, platformsPositions);
        if (result > 0 && unicorn->unicornPosition.y - gVar->distance < result)
            unicorn->unicornPosition.y -= unicorn->unicornPosition.y - result;

        else
            unicorn->unicornPosition.y -= gVar->distance;

        unicorn->jumpLimit--;
        unicorn->jumpStart = gVar->worldTime;
        unicorn->isJumping = true;
        gVar->jumpKeyDown = true;
    }
}

// ustawienie parametrów dla zrywu (po wciœniêciu klawisza)
void dash(gameVariables* gVar, unicornParam* unicorn)
{
    if (unicorn->lastDashTime == 0)
    {
        unicorn->dashActive = true;
        unicorn->dashStart = gVar->worldTime;
    }
}

// podstawowe sterowanie - manualne poruszanie strza³kami
void basicControl2(sdlStructures* game, gameVariables* gVar, unicornParam* unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    switch (game->event.key.keysym.sym)
    {
    case SDLK_UP:
    {
        if (unicorn->unicornPosition.y - UNICORN_POSITION_CHANGE >= 0)
        {
            double result = checkIfPlatformAbove(unicorn->unicornPosition, platformsPositions);
            if (result > 0 && unicorn->unicornPosition.y - UNICORN_POSITION_CHANGE < result)
                unicorn->unicornPosition.y -= unicorn->unicornPosition.y - result;

            else
                unicorn->unicornPosition.y -= UNICORN_POSITION_CHANGE;
        }
        break;
    }

    case SDLK_DOWN:
    {
        if (unicorn->unicornPosition.y + UNICORN_POSITION_CHANGE <= LEVEL_HEIGHT - UNICORN_HEIGHT && !checkBase(unicorn->unicornPosition, platformsPositions, obstacles))
            unicorn->unicornPosition.y += UNICORN_POSITION_CHANGE;
        break;
    }

    case SDLK_RIGHT:
    {
        gVar->widthChange += MOVE_CHANGE;
        move(obstacles, platformsPositions, *gVar, MOVE_CHANGE);
        break;
    }

    case SDLK_LEFT:
    {
        if (gVar->widthChange - MOVE_CHANGE >= 0)
        {
            move(obstacles, platformsPositions, *gVar, -MOVE_CHANGE);
            gVar->widthChange -= MOVE_CHANGE;
        }

        break;
    }
    }
}

// podstawowe sterowanie - zast¹pienie z i x strza³kami
void basicControl(sdlStructures* game, gameVariables* gVar, unicornParam* unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    if (!unicorn->dashActive) // jeœli jednoro¿ec wykonuje zryw, to nie mo¿e wykonaæ innej akcji
    {
        switch (game->event.key.keysym.sym)
        {
        case SDLK_UP:
            jump(gVar, unicorn, platformsPositions);
            break;

        case SDLK_DOWN:
        {
            if (unicorn->unicornPosition.y + gVar->distance <= LEVEL_HEIGHT - UNICORN_HEIGHT)
                unicorn->unicornPosition.y += gVar->distance;
            break;
        }

        case SDLK_RIGHT:
            dash(gVar, unicorn);
            break;
        }
    }
}

// zaawansowane sterowanie
void extendedControl(sdlStructures* game, gameVariables* gVar, unicornParam* unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    if (!unicorn->dashActive)
    {
        switch (game->event.key.keysym.sym)
        {
        case SDLK_z:
            jump(gVar, unicorn, platformsPositions);
            break;

        case SDLK_x:
            dash(gVar, unicorn);
            break;
        }
    }
}

// sterowanie w menu
void menuControl(sdlStructures* game, gameVariables* gVar, unicornParam* unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    switch (game->event.key.keysym.sym)
    {
    case SDLK_UP:
    case SDLK_DOWN:
    {
        if (gVar->opt == newGameOpt)            // zmiana aktualnie zaznaczonej pozycji w menu
            gVar->opt = exitGameOpt;

        else
            gVar->opt = newGameOpt;

        break;
    }
    case SDLK_RETURN: // enter
    {
        if (gVar->opt == newGameOpt)
        {
            gVar->status = play;
            gVar->cType = extended;
            newGame(gVar, unicorn, obstacles, platformsPositions);
        }
        else
            gVar->quit = true;

        break;
    }
    }
}

// obs³uga zdarzeñ
void handleEvents(sdlStructures* game, gameVariables* gVar, unicornParam* unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    if (game->event.type == SDL_QUIT)
        gVar->quit = true;
    else if (game->event.type == SDL_KEYDOWN)
    {
        if (game->event.key.keysym.sym == SDLK_ESCAPE)
            gVar->quit = true;

        else if (game->event.key.keysym.sym == SDLK_n)
        {
            unicorn->lifes = 3;
            newGame(gVar, unicorn, obstacles, platformsPositions);
        }

        else if (game->event.key.keysym.sym == SDLK_RETURN && gVar->status == collision) // kontynuacja gry po kolizji
            newGame(gVar, unicorn, obstacles, platformsPositions);

        else if (game->event.key.keysym.sym == SDLK_d && gVar->cType != menu && gVar->cType != none) // zmiana sterowania
        {
            if (gVar->cType == extended)
                gVar->cType = basic;
            else
                gVar->cType = extended;
        }

        else if (gVar->cType == basic)
            basicControl2(game, gVar, unicorn, obstacles, platformsPositions);
        //basicControl(game, gVar, unicorn, obstacles, platformsPositions);

        else if (gVar->cType == extended)
            extendedControl(game, gVar, unicorn, obstacles, platformsPositions);

        else if (gVar->cType == menu)
            menuControl(game, gVar, unicorn, obstacles, platformsPositions);
    }
    else if (game->event.type == SDL_KEYUP)
    {
        if (game->event.key.keysym.sym == SDLK_UP || game->event.key.keysym.sym == SDLK_z)
        {
            gVar->jumpKeyDown = false;
            unicorn->jumpStart = 0;
            unicorn->isJumping = false;
        }
    }
}

// zwiêkszenie wysokoœæi i zasiêgu skoku po przytrzymaniu klawisza
void makeJump(gameVariables gVar, unicornParam* unicorn, position platformsPositions[])
{
    if (gVar.worldTime - unicorn->jumpStart <= JUMP_DURATION)
    {
        if (unicorn->unicornPosition.y - (gVar.distance / 1000) >= MAX_JUMP_HEIGHT)
        {
            double result = checkIfPlatformAbove(unicorn->unicornPosition, platformsPositions);
            if (result > 0 && unicorn->unicornPosition.y - (gVar.distance / 1000) < result)
                unicorn->unicornPosition.y -= unicorn->unicornPosition.y - result;

            else
                unicorn->unicornPosition.y -= (gVar.distance / 1000);
        }
    }
    else
        unicorn->isJumping = false;
}

// wykonanie zrywu
void makeDash(gameVariables* gVar, unicornParam* unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    if (gVar->worldTime - unicorn->dashStart <= DASH_DURATION)
        move(obstacles, platformsPositions, *gVar, gVar->speed * 3);
    else
    {
        unicorn->dashActive = false;
        unicorn->lastDashTime = gVar->worldTime;
        if (unicorn->jumpLimit < 2)
            unicorn->jumpLimit++;
    }
}

// aktualizacja parametrów gry i iloœci ¿yæ jednoro¿ca, gdy zosta³a wykryta kolizja
void collisionDetected(gameVariables* gVar, unicornParam* unicorn)
{
    gVar->cType = none;
    unicorn->lifes--;
    if (unicorn->lifes > 0)
        gVar->status = collision;
    else
        gVar->status = lost;
}

// wykonanie akcji, takich jak zryw, skok, automatyczny ruch
void doAction(gameVariables* gVar, unicornParam* unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    if (unicorn->dashActive)
        makeDash(gVar, unicorn, obstacles, platformsPositions);

    else
        move(obstacles, platformsPositions, *gVar, gVar->speed);

    if (checkBase(unicorn->unicornPosition, platformsPositions, obstacles))
        unicorn->jumpLimit = 2;

    if (unicorn->lastDashTime != 0 && gVar->worldTime - unicorn->lastDashTime >= DASH_LOCK_TIME)
        unicorn->lastDashTime = 0;

    if (!unicorn->isJumping && !checkBase(unicorn->unicornPosition, platformsPositions, obstacles) && !unicorn->dashActive && unicorn->unicornPosition.y < LEVEL_HEIGHT - UNICORN_HEIGHT)
        unicorn->unicornPosition.y += (gVar->delta * GRAVITY) / 500;
}

// aktualizacja szybkoœci sceny i czasu gry
void updateSpeedAndGametime(gameVariables* gVar)
{
    gVar->gameTime += gVar->delta;
    gVar->speedCounter += gVar->delta;

    if (gVar->speedCounter >= 10)
    {
        gVar->speed += 5;
        gVar->speedCounter = 0;
    }
}

// obs³uga rozgrywki
void gamePlay(sdlStructures* game, gameVariables* gVar, unicornParam* unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    setCamera(&(game->camera), *unicorn);

    if (unicorn->isJumping == true)
        makeJump(*gVar, unicorn, platformsPositions);

    if (checkCollision(*unicorn, obstacles, platformsPositions))
        collisionDetected(gVar, unicorn);

    else
    {
        updateSpeedAndGametime(gVar);
        displayGameElements(*game, *unicorn, obstacles, platformsPositions);
        if (gVar->cType == extended)
            doAction(gVar, unicorn, obstacles, platformsPositions);
    }
}

// wyœwietlanie menu
void showMenu(sdlStructures game, gameVariables gVar)
{
    if (gVar.opt == newGameOpt)
        SDL_BlitSurface(game.menu1, NULL, game.screen, NULL);
    else
        SDL_BlitSurface(game.menu2, NULL, game.screen, NULL);
}

// sprawdzenie aktualnego stanu gry
void checkGameStatus(sdlStructures* game, gameVariables* gVar, unicornParam* unicorn, obstacleParam obstacles[], position platformsPositions[])
{
    if (gVar->status == undefined)
        showMenu(*game, *gVar);
    else if (gVar->status == play)
        gamePlay(game, gVar, unicorn, obstacles, platformsPositions);
    else if (gVar->status == collision)
        SDL_BlitSurface(game->gameOver, NULL, game->screen, NULL);
    else if (gVar->status == lost)
    {
        unicorn->lifes = 3;
        gVar->cType = menu;
        showMenu(*game, *gVar);
    }
}

// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv)
{
    srand(time(NULL));
    sdlStructures game;
    gameVariables gVar;
    unicornParam unicorn;
    position platformsPositions[MAX_PLATFORMS];
    obstacleParam obstacles[MAX_OBSTACLES];

    unicorn.unicornPosition.x = 0;
    unicorn.unicornPosition.y = LEVEL_HEIGHT / 2;

    if (!initialize(&game) || !loadImages(&game))
    {
        printf("Wystapil blad podczas proby inicjalizacji.");
        freeSurfaces(&game);
        return 1;
    }

    initializeColors(game, &gVar);
    initializePlatformsPositions(platformsPositions);
    initializeObstaclePositions(obstacles, platformsPositions);

    gVar.t1 = SDL_GetTicks();

    while (!gVar.quit)
    {
        timer(&gVar);
        checkGameStatus(&game, &gVar, &unicorn, obstacles, platformsPositions);
        drawBoard(&game, &gVar, unicorn.unicornPosition);
        updateScreen(game);

        while (SDL_PollEvent(&(game.event)))
            handleEvents(&game, &gVar, &unicorn, obstacles, platformsPositions);

        gVar.frames++;
    };

    freeSurfaces(&game);
    return 0;
};