/* Modified by: Eidmone Tagaca
 Course: CMPS3600 Spring 2022
 original author:  Gordon Griesel
            date:  Jan 24, 2022
         purpose:  C program for students to practice their C programming
		           over the Winter break. Also, introduce students to the
                   X Window System. We will use the X Window protocol or
                   API to generate output in some of our lab and homework
                   assignments.

 Instructions:

      1. If you make changes to this file, put your name at the top of
	     the file. Use one C style multi-line comment to hold your full
		 name. Do not remove the original author's name from this or 
		 other source files please.

      2. Build and run this program by using the provided Makefile.

	     At the command-line enter make.
		 Run the program by entering ./a.out
		 Quit the program by pressing Esc.

         The compile line will look like this:
            gcc xwin89.c -Wall -Wextra -Werror -pedantic -ansi -lX11

		 To run this program on the Odin server, you will have to log in
		 using the -YC option. Example: ssh myname@odin.cs.csub.edu -YC

      3. See the assignment page associated with this program for more
	     instructions.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>




struct SharedMem {
  int flag;
  int a, b, c, d, e, f;
  char myname[60];
  unsigned int parent_screen_color;
  unsigned int child_screen_color;
};

struct Global {
	Display *dpy;
	Window win;
	GC gc;
	int xres, yres;
	int shmid;
	struct SharedMem *shared;
	char message[];
} g;

int set_color = 0;

void drawString(const char *str, const int x, const int y) {
  XDrawString(g.dpy, g.win, g.gc, x, y, str, strlen(str));
}

void x11_cleanup_xwindows(void);
void x11_init_xwindows(void);
void x11_clear_window(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void render(void);

// void drawString(const char *str, const int x, const int y) {
//   XDrawString(g.dpy, g.win, g.gc, x, y, str, strlen(str));
// }

char **myargv;
char **myenv;
int child = 0;

int pipe1[2];
int pipe2[2];

int main(int argc, char *argv[], char *env[])
{
	myargv = argv;
	myenv = env; 

	g.shared = NULL;

	// The child got executed and has an argument 1
	if (argc > 1) {
		if( *argv[1] == '1') {
			printf("I'm the child\n");
			child = 1;
			//read from the pipe
			// char buf[2];
			// read(5, buf, 1);
			// printf("character from pipe: %c\n", *buf);

			// THis is child getting shared mem
			g.shmid = atoi(argv[2]);
    		g.shared = (struct SharedMem *)shmat(g.shmid, (void *)0, 0);
		}
	}

	// char buf = 'b';
	char buf[1025];
	char *data = "Message from parent: Why did you change my color!";
	int n;
	// If its parent then store it to the address of the buf
	if (!child) {
		// I'm the parent

		// Creating the shared memory
		char pathname[128];
		getcwd(pathname, 128);
		strcat(pathname, "/foo");
		int ipckey = ftok(pathname, 21);


		g.shmid = shmget(ipckey, sizeof(g.shmid), IPC_CREAT | 0666);

		g.shared = (struct SharedMem*)shmat(g.shmid, (void *)0, 0);
		// Flag is set to 0 initially
		g.shared->parent_screen_color = 0;
		g.shared->child_screen_color = 0;
		printf("I'm the parent\n");
		pipe(pipe1);
		pipe(pipe2);

		// char buf = 'a';

		// write(pipe2[1], &buf, 1);
	}

	XEvent e;
	int done = 0;
	int done_read = 0;
	int done_write = 0;
	x11_init_xwindows();
	while (!done) {
		/* Check the event queue */
		while (XPending(g.dpy)) {
			XNextEvent(g.dpy, &e);
			check_mouse(&e);
			done = check_keys(&e);
			render();
		}
		if(!child) {
			if (g.shared->parent_screen_color != 0 && done_write == 0) {
				// printf("Hey the shared mem changed");
				// Call render and have them check if the shared mem changed
				// and if it did them simpy change color
				// char buff[]  = "changed parents color";

				// pipe(pipe1);
				// pipe(pipe2);

				// char buf = 'a';

				// write(pipe2[1], &buf, 1);
				// pipe(pipe1);
				// pipe(pipe2);
				// char buff = 'h';
				// char buf = 'a';

				// write(pipe2[1], &buf, 1);
				write(pipe2[1], data, strlen(data));
				
				render();
				done_write =1;
				g.shared->child_screen_color = 1;
			}
		} else {
			if (g.shared->parent_screen_color != 0 && done_read == 0 && g.shared->child_screen_color == 1) {
				// char buf[20];
				// read(24, buf, 19);
				
				// printf("character from pipe: %s\n", buf);
				if ((n = read(5, buf, 1024)) >= 0) {
					buf[n] = 0;	/* terminate the string */
					// printf("read %d bytes from the pipe: \"%s\"\n", n, buf);
					
					 strcpy(g.message, buf);
					// printf("read %d bytes from the pipe done: \"%s\"\n", n, g.message);
					
				}	

				done_read = 1;
			}
		}
		usleep(4000);
	}

	if (g.shared != NULL)
	{
		shmdt(g.shared);
		shmctl(g.shmid, IPC_RMID, 0);
	}
	x11_cleanup_xwindows();
	return 0;
}

void x11_cleanup_xwindows(void)
{
	XDestroyWindow(g.dpy, g.win);
	XCloseDisplay(g.dpy);
}

void x11_init_xwindows(void)
{
	int scr;

	if (!(g.dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "ERROR: could not open display!\n");
		exit(EXIT_FAILURE);
	}
	scr = DefaultScreen(g.dpy);
	g.xres = 400;
	g.yres = 200;
	g.win = XCreateSimpleWindow(g.dpy, RootWindow(g.dpy, scr), 1, 1,
							g.xres, g.yres, 0, 0x00ffffff, 0x00000000);
	XStoreName(g.dpy, g.win, "cs3600 xwin sample");
	g.gc = XCreateGC(g.dpy, g.win, 0, NULL);
	XMapWindow(g.dpy, g.win);
	XSelectInput(g.dpy, g.win, ExposureMask | StructureNotifyMask |
								PointerMotionMask | ButtonPressMask |
								ButtonReleaseMask | KeyPressMask);
}

void make_child() {
	pid_t pid = fork();
	if (pid == 0) {
		// printf("Im the child\n");
		char shmid_temp[32];
    	sprintf(shmid_temp, "%i", g.shmid);
		// Passing the parents shmid as a new arg for new window
		char *a[4] = {"./xwin", "1", shmid_temp, (char *)0}; 

		execve("./xwin", a, myenv);

	} else {
		printf("Im the parent\n");
	}

}

void check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;
	int mx = e->xbutton.x;
	int my = e->xbutton.y;

	if (e->type != ButtonPress
		&& e->type != ButtonRelease
		&& e->type != MotionNotify)
		return;
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) { }
		if (e->xbutton.button==3) { }
	}
	if (e->type == MotionNotify) {
		if (savex != mx || savey != my) {
			/*mouse moved*/
			savex = mx;
			savey = my;
			if (child == 1) {
				printf("c"); fflush(stdout);
			} else {
				printf("p"); fflush(stdout);
			}
		}
	}
}

int check_keys(XEvent *e)
{
	int key;
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
			case XK_1:
				set_color = 1;
				break;
			case XK_2:
			  	make_child();
				break;
			case XK_3:
				if (child) {
					g.shared->parent_screen_color = 1;
				}
				break;
			case XK_Escape:
				return 1;
		}
	}
	return 0;
}




void render(void)
{


	if (set_color == 1 ) {
		XSetForeground(g.dpy, g.gc, 0x0ffaa22);

		XFillRectangle(g.dpy, g.win, g.gc, 0,0, g.xres, g.yres); 
			XSetForeground(g.dpy, g.gc, 0x00000000);

    	drawString("Press 2 to spawn child", 10, 20);
	} 
	if (child == 1) {
		XSetForeground(g.dpy, g.gc, 0x000FF00);

		XFillRectangle(g.dpy, g.win, g.gc, 0,0, g.xres, g.yres); 
  		XSetForeground(g.dpy, g.gc, 0x00000000);

    	drawString("Press 3 to change color of parent", 10, 20);
		if (g.shared->child_screen_color == 1) {
			drawString(g.message, 10, 40);
		}
	}

	if (!child) {
		if(g.shared->parent_screen_color == 1) {
			XSetForeground(g.dpy, g.gc, 0x0ff00ff);
			XFillRectangle(g.dpy, g.win, g.gc, 0,0, g.xres, g.yres); 
		}
	}


	//  const char *msg = "Hello world"; // cyrillic symbols


   	// XDrawString(g.dpy, g.win, g.gc, 16, 16, msg, (int) strlen(msg));
	//     XDrawString(g.dpy, g.win, g.gc,100,100,"hello",5);
   

}
