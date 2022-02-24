#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>


extern "C" {
#include"./sdl-2.0.7/include/SDL.h"
#include"./sdl-2.0.7/include/SDL_main.h"
}

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define INFO_HEIGHT     36
#define MAXDIGITS 7

struct addxy {     //struktura przechowuj¹ca dane na temat p³ytki która ma byæ dodana
	int addx;
	int addy;
	bool fromBackup=0;
};
 
enum direction_t
{
	LEFT,
	RIGHT,
	DOWN,
	UP
};

enum move_t
{
	NONE,
	MOVE,
	MERGE
};

// narysowanie napisu txt na powierzchni screen, zaczynaj¹c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj¹ca znaki
void DrawString(SDL_Surface *screen, int x, int y, const char *text,
	SDL_Surface *charset);
	
// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt œrodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y);


// rysowanie pojedynczego pixela
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color);


// rysowanie linii o d³ugoœci l w pionie (gdy dx = 0, dy = 1) 
// b¹dŸ poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color);


// rysowanie prostok¹ta o d³ugoœci boków l i k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor);

//rysowanie kwadratowej planszy o rozmiarze boardsize
void DrawBoard(SDL_Surface *screen, int **board, int boardsize, int boardtiles,
	Uint32 outlineColor, Uint32 boardColor, Uint32 tileColor, Uint32 tileBack, SDL_Surface *charset);


//dodaje kafelek 2 w losowym miejscu planszy
void addTile(int **board, int size);


void cleanBoard(int **board, int size);


void newGame(double *time, int **board, int size, int *score);


void setDirection(direction_t where, int *startx, int *starty, int *stepx, int *stepy, int x, int y, int size);


//jeœli mo¿na poruszyæ p³ytkê o wspó³rzêdnych x y w kierunku where, porusza j¹ i zwraca true
bool moveTile(int **board, int**lastboard, int x, int y, int size, direction_t where, int *score, int *lastscore);


//porusza w kieunku where wszystkie p³ytki mo¿liwe do poruszenia
void moveAll(int **board, int**lastboard, int size, direction_t where, int* score, int *lastscore);


//zapisuje stan gry na potzreby cofania ruchu
void backUp(int **board, int **lastboard, int score, int *lastscore, int size);


//cofa ostatni ruch
void loadBackup(int **board, int **lastboard, int *score, int lastscore, int size);

void saveGame(int **board, int score, int size)
{
	FILE *file;
	time_t seconds;
	struct tm *realtime;

	seconds = time(NULL);
	realtime = localtime(&seconds);

	file = fopen("saves.txt", "w");
	if (file == NULL) printf("Can't open File");	
	else
	{
		fprintf(file, "!\n%s", asctime(realtime));
		for (int j = 0; j < size; j++)
		{
			for (int i = 0; i < size; i++)
			{
				fprintf(file, "%i ", board[j][i]);
			}
			fprintf(file, "\n");
		}
		fprintf(file, "%i\n", score);			
	}
	fclose(file);
}

void loadGame(int **board, int *score, int position, int size)
{
	FILE *file;
	int a;
	file = fopen("saves.txt", "r");
	if (file == NULL) printf("Can't open File");
	else
	{
		for (int i = 0; i < position; i++)
			fscanf(file, "!\n");
		fscanf(file, "\n");
		fscanf(file, "\n%i", &a);
		for (int j=0; j<size; j++)
			for (int i = 0; i < size; i++)
			{
				fscanf(file, "%d", &board[j][i]);
			}
		fscanf(file, "%d", score);
	}
	fclose(file);

}

// main 
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	int t1, t2, frames, rc, score = 0, lastscore = 0, boardwidth = SCREEN_WIDTH / 2, size = 0;
	double delta, worldTime, fpsTimer, fps, distance, etiSpeed;
	int **board, **lastboard;
	bool quit, chosen;
	SDL_Event event;
	SDL_Surface *screen, *charset;
	SDL_Surface *eti;
	SDL_Texture *scrtex;
	SDL_Window *window;
	SDL_Renderer *renderer;

	// okno konsoli nie jest widoczne, je¿eli chcemy zobaczyæ
	// komunikaty wypisywane printf-em trzeba w opcjach:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// zmieniæ na "Console"

	printf("wyjscie printfa trafia do tego okienka\n");
	printf("printf output goes here\n");

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
		}

	// tryb pe³noekranowy

	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
	                                 &window, &renderer);
	if(rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
		};
	
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Piotr Wojciechowski 175757");

	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
	                              0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
	                           SDL_TEXTUREACCESS_STREAMING,
	                           SCREEN_WIDTH, SCREEN_HEIGHT);


	// wy³¹czenie widocznoœci kursora myszy
	SDL_ShowCursor(SDL_DISABLE);

	// wczytanie obrazka cs8x8.bmp
	charset = SDL_LoadBMP("./cs8x8.bmp");
	if(charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};
	SDL_SetColorKey(charset, true, 0x000000);

	/*eti = SDL_LoadBMP("./eti.bmp");
	if(eti == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
		};*/

	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int zolty = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0x00);

	t1 = SDL_GetTicks();

	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	chosen = 0;
	worldTime = 0;
	distance = 0;
	etiSpeed = 1;

	
	//okienko wyboru wielkoœci planszy
	while (!chosen)
	{
		
		int width = SCREEN_WIDTH / 3 + 40, key;
		char text[128];

		DrawRectangle(screen, (SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - 0.3*width) / 2, width, 0.3*width, czerwony, niebieski);
		sprintf(text, "Wybierz z klawiatury rozmiar");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 - 20, text, charset);
		sprintf(text, "planszy z zakresu od 3 do 9 :");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2, text, charset);

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
				key = event.key.keysym.sym;
				if (key >= SDLK_3 && key <= SDLK_9)
				{
					size = key - 48;
					chosen = 1;
				}
				else if (key == SDLK_ESCAPE)
				{
					SDL_Quit();
					return 0;
				}
				break;
			}
		}
		
		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);		
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_RenderClear(renderer);
	}
	
	
	board = (int**)malloc(size * sizeof(int*));
	for (int i = 0; i < size; i++)
	{
		board[i] = (int*)malloc(size * sizeof(int));
	}

	lastboard = (int**)malloc(size * sizeof(int*));
	for (int i = 0; i < size; i++)
	{
		lastboard[i] = (int*)malloc(size * sizeof(int));
	}

	if (size > 5) boardwidth = SCREEN_HEIGHT * 0.7;

	newGame(&worldTime, board, size, &score);

	while(!quit) {
		t2 = SDL_GetTicks();

		// w tym momencie t2-t1 to czas w milisekundach,
		// jaki uplyna³ od ostatniego narysowania ekranu
		// delta to ten sam czas w sekundach
		delta = (t2 - t1) * 0.001;
		t1 = t2;

		worldTime += delta;

		distance += etiSpeed * delta;

		SDL_FillRect(screen, NULL, czarny);

		/*DrawSurface(screen, eti,
		            SCREEN_WIDTH / 2 + sin(distance) * SCREEN_HEIGHT / 3,
			    SCREEN_HEIGHT / 2 + cos(distance) * SCREEN_HEIGHT / 3);*/

		fpsTimer += delta;
		if(fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
			};

		// tekst informacyjny / info text
		DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, INFO_HEIGHT, czerwony, niebieski);
		sprintf(text, "Piotr Wojciechowski 175757, czas trwania = %.1lf s  %.0lf klatek / s", worldTime, fps);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
		sprintf(text, "Esc - wyjscie, n - nowa gra");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);
		
		

		//liczba punktow
		DrawRectangle(screen, SCREEN_WIDTH/3, SCREEN_HEIGHT-35, SCREEN_WIDTH/3, 30, czerwony, niebieski);
		sprintf(text, "Your score: %d", score);		
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT - 25, text, charset);

		DrawBoard(screen, board, boardwidth, size, niebieski, czerwony, niebieski, zolty, charset);

		
		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
//		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		// obs³uga zdarzeñ (o ile jakieœ zasz³y) / handling of events (if there were any)
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					if(event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
					else if(event.key.keysym.sym == SDLK_UP) moveAll(board, lastboard, size, UP, &score, &lastscore);
					else if(event.key.keysym.sym == SDLK_DOWN) moveAll(board, lastboard, size, DOWN, &score, &lastscore);
					else if (event.key.keysym.sym == SDLK_LEFT)  moveAll(board, lastboard, size, LEFT, &score, &lastscore);
					else if (event.key.keysym.sym == SDLK_RIGHT)  moveAll(board, lastboard, size, RIGHT, &score, &lastscore);
					else if (event.key.keysym.sym == 'n') newGame(&worldTime, board, size, &score);
					else if (event.key.keysym.sym == 'u') loadBackup(board, lastboard, &score, lastscore, size);
					else if (event.key.keysym.sym == 's') saveGame(board, score, size);
					else if (event.key.keysym.sym == 'l') loadGame(board, &score, 1, size);
					break;
				case SDL_KEYUP:
					etiSpeed = 1.0;
					break;
				case SDL_QUIT:
					quit = 1;
					break;
				};
			};
		frames++;
		};

	// zwolnienie powierzchni / freeing all surfaces
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
	};



	void DrawBoard(SDL_Surface *screen, int** board, int boardsize, int boardtiles,
		Uint32 outlineColor, Uint32 boardColor, Uint32 tileColor, Uint32 tileBack, SDL_Surface *charset)
	{	 
		int value = 0;
		char temp[MAXDIGITS];
		char digit[MAXDIGITS];
		int length = 0;
		Uint32 color;
		int spacesize = boardsize / (11 * boardtiles + 1);
		int tilesize = boardsize / (11 * boardtiles + 1) * 10;
		boardsize = boardtiles * tilesize + (boardtiles + 1)*spacesize;
		int boardx = (SCREEN_WIDTH - boardsize) / 2;
		int boardy = (SCREEN_HEIGHT - boardsize) / 2;
		DrawRectangle(screen, boardx, boardy, boardsize, boardsize,
			outlineColor, boardColor);
		for (int i = 0; i < boardtiles; i++)
			for (int j = 0; j < boardtiles; j++)
			{
				if (board[j][i]  == 0)
				{
					color = tileBack;
					digit[0] = ' \0';
				}
					
				else
				{
					color = tileColor;
					value = board[j][i] ;
					length = 0;
					for (; length < MAXDIGITS && value>0; length++)
					{
						temp[length] = '0' + value % 10;
						value /= 10;
					}
					for (int b = 0; b <length; b++)
					{
						digit[b] = temp[length - b - 1];
					}

					digit[length] = '\0';
				}
					
				DrawRectangle(screen, boardx + spacesize + j * (tilesize + spacesize),
					boardy + spacesize + i * (tilesize + spacesize), tilesize,
					tilesize, outlineColor, color);
				
				int stringx = boardx + j * (tilesize + spacesize) + tilesize / 2 - length*2;
				int stringy = boardy + i * (tilesize + spacesize) + tilesize / 2;

				DrawString(screen, stringx, stringy, digit, charset);

				
			}
	}


	void addTile(int **board, int size)
	{
		srand(time(NULL));
		int i, j;
		do
		{
			i = rand() % size;
			j = rand() % size;
		} while (board[i][j]  != 0);
		board[i][j]  = 2;
	}


	bool moveTile(int **board, int **lastboard, int x, int y, int size, direction_t where, int *score, int *lastscore)
	{
		int startx, starty, stepx, stepy, destx = -1, desty = -1, addx, addy;    //zmienne definiuj¹ce punkt pocz¹tkowy i kierunek sprawdzania pól
		move_t move = NONE;

		setDirection(where, &startx, &starty, &stepx, &stepy, x, y, size);

		for (int j = starty, i = startx; j != y || i != x; j += stepy, i += stepx)
		{
			if (move == NONE && board[i][j] == 0)
			{
				destx = i;
				desty = j;
				move = MOVE;
			}
			else if (move == NONE && board[i][j] == board[x][y])
			{
				destx = i;
				desty = j;
				move = MERGE;
			}
		}
			
		if (move == MOVE)
		{
			board[destx][desty] = board[x][y];
			board[x][y] = 0;
		}
		else if (move == MERGE)
		{
			board[destx][desty] *= 2;
			(*score) += board[destx][desty];
			board[x][y] = 0;
		}
		return (move != NONE);
	}


	void moveAll(int **board, int **lastboard, int size, direction_t where, int* score, int* lastscore)
	{
		int startx, starty, stepx, stepy;    //zmienne definiuj¹ce punkt pocz¹tkowy i kierunek sprawdzania pól
		move_t move = NONE;
		bool moved = 0;

		setDirection(where, &startx, &starty, &stepx, &stepy, 0, 0, size);

		backUp(board, lastboard, *score, lastscore, size);

		for (int k = 0; k < size; k++)
		{
			for (int j = starty, i = startx; abs(starty - j) < size && abs(startx - i) < size; i += stepx, j += stepy)
			{
				if (board[i][j] != 0)
					if (moveTile(board, lastboard, i, j, size, where, score, lastscore)) moved = 1;
			}
			startx += abs(stepy);
			starty += abs(stepx);
		}
		if (moved) addTile(board, size);	
	}
	

	void backUp(int **board, int **lastboard, int score, int *lastscore, int size)
	{
		for (int j = 0; j < size; j++)
			for (int i = 0; i < size; i++)
			{
				lastboard[i][j] = board[i][j];
			}
		*lastscore = score;
	}


	void loadBackup(int **board, int **lastboard, int *score, int lastscore, int size)
	{
		for (int j = 0; j < size; j++)
			for (int i = 0; i < size; i++)
			{
				board[i][j] = lastboard[i][j];
			}
		*score = lastscore;
	}


	void cleanBoard(int **board, int size)
	{
		for (int i = 0; i < size; i++)
			for (int j = 0; j < size; j++)			
				board[j][i]  = 0;			
	}


	void newGame(double *time, int **board, int size, int *score)
	{
		cleanBoard(board, size);
		*time = 0;
		*score = 0;
		addTile(board, size);	
	}


	void setDirection(direction_t where, int *startx, int *starty, int *stepx, int *stepy, int x, int y, int size)
	{
		switch (where)
		{
		case RIGHT:
			*startx = size - 1;
			*starty = y;
			*stepx = -1;
			*stepy = 0;
			break;
		case LEFT:
			*startx = 0;
			*starty = y;
			*stepx = 1;
			*stepy = 0;
			break;
		case DOWN:
			*startx = x;
			*starty = size - 1;
			*stepx = 0;
			*stepy = -1;
			break;
		case UP:
			*startx = x;
			*starty = 0;
			*stepx = 0;
			*stepy = 1;
			break;
		}
	}


	void DrawString(SDL_Surface *screen, int x, int y, const char *text,
		SDL_Surface *charset) {
		int px, py, c;
		SDL_Rect s, d;
		s.w = 8;
		s.h = 8;
		d.w = 8;
		d.h = 8;
		while (*text) {
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

	void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
		SDL_Rect dest;
		dest.x = x - sprite->w / 2;
		dest.y = y - sprite->h / 2;
		dest.w = sprite->w;
		dest.h = sprite->h;
		SDL_BlitSurface(sprite, NULL, screen, &dest);
	};

	void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
		int bpp = surface->format->BytesPerPixel;
		Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
		*(Uint32 *)p = color;
	};

	void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
		for (int i = 0; i < l; i++) {
			DrawPixel(screen, x, y, color);
			x += dx;
			y += dy;
		};
	};

	void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
		Uint32 outlineColor, Uint32 fillColor) {
		int i;
		DrawLine(screen, x, y, k, 0, 1, outlineColor);
		DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
		DrawLine(screen, x, y, l, 1, 0, outlineColor);
		DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
		for (i = y + 1; i < y + k - 1; i++)
			DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	};

