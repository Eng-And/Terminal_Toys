/* 2025-03-01 Eng-And */

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define DEFAULT_HEIGHT 16
#define DEFAULT_WIDTH 30
#define DEFAULT_MINES_COUNT 99

#define K_LEFT 260
#define K_RIGHT 261
#define K_UP 259
#define K_DOWN 258

#define K_SPACE 32
#define K_Q 113
#define K_Z 122

#define INT_TO_CHAR 48
#define FLAG_TEXT_X 17

#define VECTOR2_COMPARE(a, b) ((a.x == b.x) && (a.y == b.y))
#define FOR_PLUS_MINUS_EQ(var, start)  for (int var = (start) - 1; var < (start) + 2; var++)
#define FOR_EACH_VECTOR2(var, head) for (Vector2List *var = head; var != NULL; var = var->next)

/* used for ordered pairs */
typedef struct Vector2
{
  int x;
  int y;
} Vector2;

/* linked list of Vector2s */
typedef struct Vector2List
{
  Vector2 vec;
  struct Vector2List *next;
} Vector2List;

/* clear the window when done */
static void emergency_finish(int sig)
{
  endwin();
  exit(EXIT_SUCCESS);
}

/* does what it says on the tin */
void set_vector2(Vector2 *vec, int y, int x)
{
  vec->y = y;
  vec->x = x;
  return;
}

/* creates and returns new vec2 */
Vector2 new_vector2(int y, int x)
{
  Vector2 new;
  set_vector2(&new, y, x);
  return new;
}

/* will change the position vector and move cursor based on input */
void update_pos(Vector2 *pos, int key_press, int width, int height)
{
  /* change based on push */
  if (key_press == K_LEFT)
    pos->x--;
  else if (key_press == K_RIGHT)
    pos->x++;
  else if (key_press == K_UP)
    pos->y--;
  else if (key_press == K_DOWN)
    pos->y++;
  
  /* fix cursor pos if its fallen of the screen */
  pos->x = (pos->x + width) % width;
  pos->y = (pos->y + height) % height;

  move(pos->y, pos->x);
  return;
}

/* adds a new vector2 to a list */
void add_vector2(Vector2List **head, int y, int x)
{
  Vector2List *new = malloc(sizeof(Vector2List));
  set_vector2(&(new->vec), y, x);
  new->next = *head;
  *head = new;
  return;
}

/* removes first element of Vector2List and returns the vector associated */
Vector2 pop_vector2(Vector2List **head)
{
  Vector2 output = (*head)->vec;
  Vector2List* to_free = (*head);
  *head = (*head)->next;
  free(to_free);
  return output;
}

/* frees the whole list passed in */
void free_vector2list(Vector2List *head)
{
  Vector2List *cur, *next;
  cur = head;
  while (cur != NULL)
  {
    next = cur->next;
    free(cur);
    cur = next;
  }
  return;
}

/* returns true if vector2 is in a list */
bool in_vector2list(Vector2List *head, Vector2 vec)
{
  FOR_EACH_VECTOR2(foo, head)
    if (VECTOR2_COMPARE(foo->vec, vec))
      return true;
  return false;
}

int len_vector2list(Vector2List *head)
{
  int output = 0;
  FOR_EACH_VECTOR2(cur, head)
    output++;
  return output;
}

/* if finds vec, removes first occurence and returns true, else returns false */
bool remove_from_vector2list(Vector2List **head, Vector2 vec)
{
  if (*head == NULL)
    return false;
  
  if (VECTOR2_COMPARE((*head)->vec, vec))
  {
    Vector2List *new_start = (*head)->next;
    free(*head);
    *head = new_start;
    return true;
  }
  else
  {
    Vector2List *cur = *head;
    while (cur->next != NULL)
    {
      if (VECTOR2_COMPARE(cur->next->vec, vec))
      {
	Vector2List *to_remove = cur->next;
	cur->next = cur->next->next;
	free(to_remove);
	return true;
      }
      else
	cur = cur->next;
    }
  }
  return false;
}

/* tells you if the mine is too close to the cursor */
bool valid_mine_spot(Vector2 cursor_pos, Vector2 mine_pos)
{
  FOR_PLUS_MINUS_EQ(y, cursor_pos.y)
    FOR_PLUS_MINUS_EQ(x, cursor_pos.x)
      if (x == mine_pos.x && y == mine_pos.y)
	return false;
  return true;
}

/* will generate the position for mines, avoiding the cursor position. */
Vector2List *generate_mines(Vector2 cursor_pos, int height, int width, int mine_count)
{
  Vector2List *output = NULL;
  for (int i = 0; i < mine_count; i++)
  {
    /* generate new random spot for next mine */
    Vector2 next = new_vector2(rand() % height, rand() % width);;

    /* shifts mine over if its on top of another or on the cursor */
    while (in_vector2list(output, next) || !valid_mine_spot(cursor_pos, next))
    {
      if (++next.x == width)
      {
	next.x = 0;
	next.y = (next.y + 1) % height;
      }
    }
    /* add new mine to list */
    add_vector2(&output, next.y, next.x);
  }
  return output;
}

/* returns the count for the surrounding mines */
int get_number(Vector2 pos, Vector2List *mines)
{
  int output = 0;
  /* Test all surrounding cells */
  FOR_PLUS_MINUS_EQ(y, pos.y)
    FOR_PLUS_MINUS_EQ(x, pos.x)
      if (in_vector2list(mines, new_vector2(y, x)))
	output++;
  return output;
}

/* takes a vector to search around, and will do the same for everything thats a 0 */
Vector2List *flood_fill(Vector2 start, Vector2List *mines, int width, int height)
{
  Vector2List *to_search = NULL;
  add_vector2(&to_search, start.y, start.x);
  Vector2List *searched = NULL;
  while (to_search != NULL)
  {
    Vector2 next = pop_vector2(&to_search);
    /* search around if number = 0 */
    if (get_number(next, mines) == 0)
    {
      FOR_PLUS_MINUS_EQ(y, next.y)
	FOR_PLUS_MINUS_EQ(x, next.x)
	  /* nasty, but mostly just checks that we're inbounds */
	  if (!(((x == next.x) && (y == next.y)) || (x < 0) || (x >= width) || (y < 0) || (y >= height)))
	  {
	    Vector2 tmp = new_vector2(y, x);
	    if ((! in_vector2list(searched, tmp)) && (! in_vector2list(to_search, tmp)))
	      add_vector2(&to_search, y, x);
	  }
    }
    add_vector2(&searched, next.y, next.x);
  }
  return searched;
}

char *get_flags_text(int num)
{
  char *output = malloc(sizeof(char) * 5);
  if (num > 999)
    strcpy(output, ">999");
  else if (num < -99)
    strcpy(output, "<-99");
  else
    sprintf(output, "%-4d", num);
  return output;
}

void set_flag_text(int num, int y, int x)
{
  char *flag_text = get_flags_text(num);
  attrset(COLOR_PAIR(0));
  mvaddstr(y, x, flag_text);
  free(flag_text);
}

/* takes far too many params, will select a tile and everything around that makes sense */
void select_tile(Vector2 cursor_pos,
		 Vector2 selection_pos,
		 Vector2List *mines,
		 Vector2List *flags,
		 Vector2List **uncovered,
		 int width,
		 int height)
{
  Vector2List *to_uncover = flood_fill(selection_pos, mines, width, height);
  /* fill out spaces */
  //for (Vector2List *cur = to_uncover; cur != NULL; cur = cur->next)
  FOR_EACH_VECTOR2(cur, to_uncover)
  {
    if ((! in_vector2list(*uncovered, cur->vec)) && (! in_vector2list(flags, cur->vec)))
    {
      char num_char = get_number(cur->vec, mines);
      if (num_char == 0)
      {
	attrset(COLOR_PAIR(9));
	num_char = ' ';
      }
      else
      {
	attrset(COLOR_PAIR(num_char));
	num_char += INT_TO_CHAR;
      }

      mvaddch(cur->vec.y, cur->vec.x, num_char);
      add_vector2(uncovered, cur->vec.y, cur->vec.x);
    }
  } 
  free_vector2list(to_uncover);
  move(cursor_pos.y, cursor_pos.x);
}

int main(int argc, char** argv)
{
  int height, width, mine_count;
  if (argc == 1)
  {
    height = DEFAULT_HEIGHT;
    width = DEFAULT_WIDTH;
    mine_count = DEFAULT_MINES_COUNT;      
  }
  else if (argc == 4)
  {
    width = atoi(argv[1]);
    height = atoi(argv[2]);
    mine_count = atoi(argv[3]);

    if (((width * height) - 9) < mine_count)
    {
      printf("Uh oh! That board is too small for that many mines!\n");
      return EXIT_SUCCESS;
    }
  }
  else 
  {
    printf("Bad args! Please try again with the following format: ./minesweeper <width> <height> <mine count>\n");
    return EXIT_SUCCESS;
  }
  
  
  signal(SIGINT, emergency_finish); /* just make it that an interupt won't wreck the terminal */

  initscr(); /* initiate curses library */
  keypad(stdscr, true); /* enable keyboard mapping */
  noecho(); /* don't write keyboard to term */
  cbreak(); /* take input characters one at a time (not at newline) */
  curs_set(1); /* make sure cursor is visible */

  /* if supported, enable colors */
  if (has_colors())
  {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_WHITE);
    init_pair(2, COLOR_GREEN, COLOR_WHITE);
    init_pair(3, COLOR_RED, COLOR_WHITE);
    init_pair(4, COLOR_BLUE, COLOR_WHITE);
    init_pair(5, COLOR_MAGENTA, COLOR_WHITE);
    init_pair(6, COLOR_BLACK, COLOR_WHITE);
    init_pair(7, COLOR_BLUE, COLOR_WHITE);
    init_pair(8, COLOR_MAGENTA, COLOR_WHITE);
    init_pair(9, COLOR_BLACK, COLOR_WHITE); /* un-numbered */
  }

  /* start off whole screen as . */
  attrset(COLOR_PAIR(9));
  for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++)
      mvaddch(y, x, '.');
  attrset(COLOR_PAIR(0));
  /* set flag info text */
  mvaddstr(height, 0, "Flags Remaining: ");
  set_flag_text(mine_count, height, FLAG_TEXT_X);
  refresh();

  /* set cursor to center position */
  Vector2 pos;
  set_vector2(&pos, height / 2, width / 2);
  move(pos.y, pos.x);

  /* gameplay vars */
  bool lost = false;
  bool win = false;
  bool aborted = false;
  Vector2List *flags = NULL;
  Vector2List *mines = NULL;
  Vector2List *uncovered = NULL;
  int flags_remaining = mine_count;
  int to_win = (width * height) - mine_count;
  /* seed the rng with current time */
  time_t cur_time;
  time(&cur_time);
  srand(cur_time);
  
  /* gameplay loop */
  while ((! lost) && (! win) && (! aborted))
  {
    int key_press = getch();

    /* move key pressed */
    if (key_press == K_LEFT || key_press == K_RIGHT || key_press == K_UP || key_press == K_DOWN)
      update_pos(&pos, key_press, width, height);
    /* quit key pressed */
    else if (key_press == K_Q)
      aborted = true;
    /* flag key pressed */
    else if (key_press == K_Z && !(in_vector2list(uncovered, pos)))
    {
      /* already flagged */
      attrset(COLOR_PAIR(9));
      if (remove_from_vector2list(&flags, pos))
      {
	addch('.');
	flags_remaining++;
      }
      /* not flagged */
      else
      {
	add_vector2(&flags, pos.y, pos.x);
	addch('F');
	flags_remaining--;
      }
      set_flag_text(flags_remaining, height, FLAG_TEXT_X);
      move(pos.y, pos.x);
    }
    /* search key pressed */
    else if (key_press == K_SPACE && (!in_vector2list(flags, pos)))
    {
      /* mines havent been generated yet */;
      if (mines == NULL)
	mines = generate_mines(pos, height, width, mine_count);
      /* hit mine */
      if (in_vector2list(mines, pos))
	lost = true;
      /* Selected a cell already uncovered */
      else if (in_vector2list(uncovered, pos))
      {
	/* confirm that amount of flags around is correct */
	int surrounding_flags = 0;
	FOR_PLUS_MINUS_EQ(y, pos.y)
	  FOR_PLUS_MINUS_EQ(x, pos.x)
	    if (in_vector2list(flags, new_vector2(y, x)))
	      surrounding_flags++;
	
	if (surrounding_flags == get_number(pos, mines))
	{
	  /* confirm not hitting a mine */
	  FOR_PLUS_MINUS_EQ(y, pos.y)
	    FOR_PLUS_MINUS_EQ(x, pos.x)
	    {
	      Vector2 to_check = new_vector2(y, x);
	      if ((! in_vector2list(flags, to_check)) && in_vector2list(mines, to_check))
		lost = true;
	    }

	  /* if didn't just die select surrounding tiles */
	  if (! lost)
	  {
	    FOR_PLUS_MINUS_EQ(y, pos.y)
	      FOR_PLUS_MINUS_EQ(x, pos.x)
	      {
		Vector2 to_search = new_vector2(y, x);
		if ((! in_vector2list(flags, to_search)) && (!in_vector2list(uncovered, to_search))  &&
		  (0 <= x) && (0 <= y) && (width > x) && (height > y))
		  select_tile(pos, to_search, mines, flags, &uncovered, width, height);
	      }
	  }
	}
      }
      /* selected a not uncovered cell */
      else
	select_tile(pos, pos, mines, flags, &uncovered, width, height);
    }
    
    /* see if won */
    if (len_vector2list(uncovered) == to_win)
      win = true;

    refresh();
  }

  if (!aborted)
  {
    attrset(COLOR_PAIR(9));
    /* display not found mines */
    FOR_EACH_VECTOR2(mine, mines)
      if (!in_vector2list(flags, mine->vec))
	mvaddch(mine->vec.y, mine->vec.x, 'O');
    /* cross out incorrect flags */
    FOR_EACH_VECTOR2(flag, flags)
      if (!in_vector2list(mines, flag->vec))
	mvaddch(flag->vec.y, flag->vec.x, 'X');

    attrset(COLOR_PAIR(0));
    /* lose message */
    if (lost)
      mvaddstr(height + 2, 0, "Aw unlucky :(");
    /* win message */
    else
      mvaddstr(height + 2, 0, "Good job! :)");
    mvaddstr(height + 3, 0, "Press any key to close");

    refresh();
    move(pos.y, pos.x);
    getch();
  }
  
  /* free garbage */
  free_vector2list(flags);
  free_vector2list(mines);
  free_vector2list(uncovered);
  
  endwin();
  return EXIT_SUCCESS;
}
