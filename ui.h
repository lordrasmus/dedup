

#ifndef _DEDUP_UI_H_
#define _DEDUP_UI_H_


#include <string>
#include <string.h>
#include <form.h>
#include <stdlib.h>
#include <ctype.h>
#include <curses.h>
#include <sys/ioctl.h>



//#define printf aa



char * intprtkey(int ch);
	
class UI{
	
private:

	struct winsize w;
	
	
	int cur_line_progress_line = 0;
	int cur_log_line = 0;
	
	double proz_last = 0;
	
public:

	WINDOW * mainwin, *childwin, *my_form_win, *log_win;

	char data_path[1000];

	UI(){
		ioctl(0, TIOCGWINSZ, &this->w);
		 
		if ( (this->mainwin = initscr()) == NULL ) {
			fprintf(stderr, "Error initialising ncurses.\n");
			exit(EXIT_FAILURE);
		}

		start_color();                    /*  Initialize colours  */
		cbreak();
		noecho();
		keypad(this->mainwin, TRUE);
		box(this->mainwin, 0, 0);
		
		mvwaddstr(this->mainwin, 1 , 3, "Status : " );
		
		mvwaddstr(this->mainwin, 1 , w.ws_col - 13, "F3 : hash" );
		mvwaddstr(this->mainwin, 2 , w.ws_col - 13, "F4 : merge" );
		mvwaddstr(this->mainwin, 3 , w.ws_col - 13, "F5 : dedup" );
		
		
		log_win = subwin(mainwin, 8, w.ws_col,  w.ws_row - 8, 0);
		box(log_win, 0, 0);
		
		touchwin( log_win );
		wrefresh( log_win );
		
		touchwin( mainwin );
		wrefresh( mainwin );
		refresh();
	
	}
	
	void close(){
		delwin(this->mainwin);
		endwin();
	}
	
	void show_progress_win(){
		
		//int ch;
		
		int      width = 63, height = 17;
		int      rows  = w.ws_row, cols   = w.ws_col;
		int      x = (cols - width)  / 2;
		int      y = (rows - height) / 2;
    
		childwin = subwin(mainwin, height, width, y, x);
		box(childwin, 0, 0);
		
		cur_line_progress_line = 0;
		
		wrefresh( childwin );
		refresh();
		
		
	}
	
	void update_hash_progress( uint64_t total, uint64_t finished ){
	
		double tmp = finished;
		tmp /= total;
		tmp *= 100;
		

		if (( tmp - proz_last ) > 0.01 ) {
			
			mvwprintw(mainwin, 3, 3, "Files Hashed : %d / %d ( %.2f %% )",finished,total,tmp );
			wrefresh( mainwin );
			
			proz_last = tmp;
		}
		
		if ( total == finished){
			mvwprintw(mainwin, 3, 3, "Files Hashed : %d / %d ( %.2f %% )",finished,total,100.0 );
			wrefresh( mainwin );
		}
		
	}
	
	void update_merge_progress( uint64_t matches, uint64_t merged ,char*  reduction ){
			mvwprintw(mainwin, 5, 3, "                                                                       " );
			mvwprintw(mainwin, 5, 3, "Matches : %d  Merged: %d  Reduction : %s",matches,merged,reduction );
			wrefresh( mainwin );
	}
	
	void add_progress_win_line( std::string line ){
		mvwaddstr(childwin, 3 + cur_line_progress_line++, 3, line.c_str() );
		wrefresh( childwin );
	}
	
	void set_status( const char* format,...) {
		va_list arg;

		va_start(arg, format);
		char buffer[vsnprintf(0, 0, format, arg) + 1];
		va_end(arg);

		va_start(arg, format);
		vsprintf(buffer, format, arg);
		va_end(arg);
		
		mvwaddstr(mainwin, 1 , 12, "                                             " );
		mvwaddstr(mainwin, 1 , 12, buffer );
		wrefresh( mainwin );
	}
	
	void add_log( const char* format,...) {
		va_list arg;

		va_start(arg, format);
		char buffer[vsnprintf(0, 0, format, arg) + 1];
		va_end(arg);

		va_start(arg, format);
		vsprintf(buffer, format, arg);
		va_end(arg);
		
		//printf(buffer);
		
		mvwaddstr(log_win, 1 + cur_log_line++, 3, buffer );
		wrefresh( log_win );
	}
	
	void add_progress_win_line( const char* format,...) {
		va_list arg;

		va_start(arg, format);
		char buffer[vsnprintf(0, 0, format, arg) + 1];
		va_end(arg);

		va_start(arg, format);
		vsprintf(buffer, format, arg);
		va_end(arg);
		
		//printf(buffer);
		
		mvwaddstr(childwin, 3 + cur_line_progress_line++, 3, buffer );
		wrefresh( childwin );
	}
	
	void hide_progress_win(){
		wclear(childwin);
		wborder(childwin, ' ', ' ', ' ',' ',' ',' ',' ',' ');
		wrefresh(childwin);
		delwin(childwin);
		touchwin( mainwin );
		wrefresh( mainwin );
		refresh();
	}
	
	
	void show_data_path_dialog( std::string data_path_p ){
		
		FIELD *field[3];
		FORM  *my_form;
		
		int ch;
		
		/* Initialize the fields */
		field[0] = new_field(1, 40, 1, 13, 0, 0);
		field[1] = new_field(1, 10, 3, 13, 0, 0);
		field[2] = NULL;

		/* Set field options */
		set_field_buffer(field[0], 0, data_path_p.c_str()); 
		set_field_back(field[0], A_UNDERLINE); 	/* Print a line for the option 	*/
		field_opts_off(field[0], O_AUTOSKIP);  	/* Don't go to next field when this */
												/* Field is filled up 		*/
		
		//set_field_buffer(field[1], 0, "OK"); 
		//set_field_back(field[1], A_UNDERLINE); 
		field_opts_off(field[1], O_AUTOSKIP);
		field_opts_off(field[1], O_EDIT );

		/* Create the form and post it */
		my_form = new_form(field);
		int f_rows,f_cols;
		scale_form(my_form, &f_rows, &f_cols);
		
		/* Create the window to be associated with the form */
		my_form_win = subwin(mainwin, f_rows + 4, f_cols + 4, 4, 4);
		keypad(my_form_win, TRUE);
		
		/* Set main window and sub window */
		set_form_win(my_form, my_form_win);
		set_form_sub(my_form, derwin(my_form_win, f_rows, f_cols, 2, 2));

		/* Print a border around the main window and print a title */
		box(my_form_win, 0, 0);
		
		post_form(my_form);
		refresh();
		
		mvwprintw(my_form_win,5, 15, "OK");
		mvwprintw(my_form_win,3, 3, "Data Path : ");
		set_current_field(my_form, field[0]); /* Set focus to the colored field */
		
		refresh();
		
		/* Loop through to get user requests */
		while((ch = wgetch(my_form_win)) != KEY_F(2)){
			if ( ch == 0xa )
				break;
				
			switch(ch)
			{	case KEY_DOWN:
					/* Go to next field */
					form_driver(my_form, REQ_NEXT_FIELD);
					form_driver(my_form, REQ_END_LINE);
					break;
				case KEY_UP:
					/* Go to previous field */
					form_driver(my_form, REQ_PREV_FIELD);
					form_driver(my_form, REQ_END_LINE);
					break;
				case KEY_BACKSPACE:
					form_driver(my_form, REQ_DEL_PREV);
					break;
				
				default:
					/* If this is a normal character, it gets */
					/* Printed				  */	
					form_driver(my_form, ch);
					break;
			}
		}
		
		
		strcpy( data_path, field_buffer( field[0], 0 ) );
		
		unpost_form(my_form);
		free_form(my_form);
		free_field(field[0]);
		free_field(field[1]); 
		
		wborder(my_form_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
		wrefresh(my_form_win);
		delwin(my_form_win);
		touchwin( mainwin );
		wrefresh( mainwin );
		refresh();
	}
	
};

#endif
