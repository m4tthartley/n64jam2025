
#define CORE_IMPL
#include <core/video.h>

int main()
{
	// sys_window_t window;
	// sys_init_window(&window, "Linux Window", 800, 600, 0);

	// sys_init_opengl(&window);

	window_t window = vid_init_window("Linux Window", 800, 600, 0);

	vid_init_opengl(&window);

	for (;;) {
		vid_poll_events(&window);

		if (window.quit) {
			exit(0);
		}

		glClearColor(1, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glXSwapBuffers(window.sysDisplay, window.sysWindow);
	}
}