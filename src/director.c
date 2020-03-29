/******************************************************************************
  This file is part of b3.

  Copyright 2020 Richard Paul Baeck <richard.baeck@mailbox.org>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*******************************************************************************/

/**
 * @author Richard B�ck <richard.baeck@mailbox.org>
 * @date 2020-02-26
 * @brief File contains the director class implementation and private methods
 */

#include "director.h"

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wbkbase/logger.h>

#include "monitor.h"
#include "ws.h"

static wbk_logger_t logger = { "director" };

static int
b3_director_free_monitor_arr(b3_director_t *director);

static const b3_monitor_t *
b3_director_get_monitor_by_monitor_name(b3_director_t *director, const char *monitor_name);

static BOOL CALLBACK
b3_director_enum_monitors(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM data);

b3_director_t *
b3_director_new(b3_monitor_factory_t *monitor_factory)
{
	b3_director_t *director;

	director = NULL;
	director = malloc(sizeof(b3_director_t));

	director->focused_monitor = NULL;

	array_new(&(director->monitor_arr));

	director->monitor_factory = monitor_factory;

	return director;
}

int
b3_director_free(b3_director_t *director)
{
	director->focused_monitor = NULL;

	b3_director_free_monitor_arr(director);

	director->monitor_factory = NULL;

	free(director);
	return 0;
}

int
b3_director_free_monitor_arr(b3_director_t *director)
{
	ArrayIter monitor_iter;
	b3_monitor_t *monitor;

	while (array_iter_next(&monitor_iter, (void *) &monitor) != CC_ITER_END) {
		b3_monitor_free(monitor);
	}

	array_destroy_cb(director->monitor_arr, NULL);

	director->monitor_arr = NULL;

	return 0;
}

int
b3_director_refresh(b3_director_t *director)
{
	b3_monitor_t *monitor;

	char found;

	EnumDisplayMonitors(NULL, NULL, b3_director_enum_monitors, (LPARAM) director);

   	SendMessage(HWND_BROADCAST, WM_NCPAINT, (void *) NULL, (void *) NULL);

	return 0;
}

BOOL CALLBACK
b3_director_enum_monitors(HMONITOR wmonitor, HDC hdc, LPRECT rect, LPARAM data)
{
    b3_director_t *director;
    MONITORINFOEX monitor_info;
    b3_monitor_t *monitor;

    director = (b3_director_t *) data;

    monitor_info.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(wmonitor, &monitor_info);

    wbk_logger_log(&logger, INFO, "Found monitor: %s - %dx%d\n",
    			   monitor_info.szDevice,
    			   monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
				   monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top);

    monitor = b3_monitor_factory_create(director->monitor_factory,
		    		 	 	 	 	   	monitor_info.szDevice,
									    monitor_info.rcMonitor);
    array_add(director->monitor_arr,
	 	      monitor);
    if (director->focused_monitor == NULL) {
    	director->focused_monitor = monitor;
    }

    return TRUE;
}

const Array *
b3_director_get_monitor_arr(b3_director_t *director)
{
	return director->monitor_arr;
}

const b3_monitor_t *
b3_director_get_monitor_by_monitor_name(b3_director_t *director, const char *monitor_name)
{
	ArrayIter monitor_iter;
	b3_monitor_t *monitor;
	char found;

	monitor = NULL;
	found = 0;
	array_iter_init(&monitor_iter, director->monitor_arr);
	while (array_iter_next(&monitor_iter, (void *) &monitor) != CC_ITER_END
		   && !found) {
		if (strcmp(b3_monitor_get_monitor_name(monitor), monitor_name) == 0) {
			found = 1;
		}
	}

	return monitor;
}

int
b3_director_set_focused_monitor(b3_director_t *director, const char *monitor_name)
{
	char found;
	ArrayIter iter;
	b3_monitor_t *monitor;
	int ret;

	found = 0;
	array_iter_init(&iter, director->monitor_arr);
    while (!found && array_iter_next(&iter, (void*) &monitor) != CC_ITER_END) {
    	if (strcmp(monitor_name, b3_monitor_get_monitor_name(monitor)) == 0)	{
    		found = 1;
    	}
    }

    if (found) {
    	director->focused_monitor = monitor;
    	wbk_logger_log(&logger, INFO, "Switching to monitor %s.\n", monitor_name);
    	ret = 0;
    } else {
    	wbk_logger_log(&logger, SEVERE, "Cannot switch monitor %s. Monitor not available.\n", monitor_name);
    	ret = 1;
    }

   	SendMessage(HWND_BROADCAST, WM_NCPAINT, (void *) NULL, (void *) NULL);

   	return ret;
}

int
b3_director_switch_to_ws(b3_director_t *director, const char *ws_id)
{
	char found;
	ArrayIter iter;
	b3_monitor_t *monitor;

	found = 0;
	array_iter_init(&iter, director->monitor_arr);
    while (!found && array_iter_next(&iter, (void*) &monitor) != CC_ITER_END) {
    	if (b3_monitor_contains_ws(monitor, ws_id))	{
    		found = 1;
    	}
    }

    if (found) {
    	director->focused_monitor = monitor;
    	wbk_logger_log(&logger, INFO, "Switching to monitor %s.\n", b3_monitor_get_monitor_name(monitor));
    }
    b3_monitor_set_focused_ws(director->focused_monitor, ws_id);

    wbk_logger_log(&logger, INFO, "Switching to workspace %s.\n", ws_id);

   	SendMessage(HWND_BROADCAST, WM_NCPAINT, (void *) NULL, (void *) NULL);

	return 0;
}

int
b3_director_add_win(b3_director_t *director, const char *monitor_name, b3_win_t *win)
{
	ArrayIter iter;
	b3_monitor_t *monitor;
	char found;
	int ret;

	found = 0;
	array_iter_init(&iter, director->monitor_arr);
    while (!found && array_iter_next(&iter, (void*) &monitor) != CC_ITER_END) {
    	if (strcmp(b3_monitor_get_monitor_name(monitor), monitor_name) == 0) {
    		found = 1;
    	}
    }

    ret = 1;
    if (found) {
    	ret = b3_monitor_add_win(monitor, win);
    }

    if (ret == 0) {
    	wbk_logger_log(&logger, DEBUG, "Added window %d\n", win->window_handler);
    }

	return ret;
}

int
b3_director_remove_win(b3_director_t *director, b3_win_t *win)
{
	ArrayIter iter;
	b3_monitor_t *monitor;
	int ret;

	ret = 1;
	array_iter_init(&iter, director->monitor_arr);
    while (ret && array_iter_next(&iter, (void*) &monitor) != CC_ITER_END) {
    	ret = b3_monitor_remove_win(monitor, win);
    }

    if (ret == 0) {
    	wbk_logger_log(&logger, DEBUG, "Removed window %d\n", win->window_handler);
    }

    return ret;
}

int
b3_director_show(b3_director_t *director)
{
	ArrayIter monitor_iter;
	b3_monitor_t *monitor;

	array_iter_init(&monitor_iter, director->monitor_arr);
	while (array_iter_next(&monitor_iter, (void *) &monitor) != CC_ITER_END) {
		b3_monitor_show(monitor);
	}

	return 0;
}

int
b3_director_draw(b3_director_t *director, HWND window_handler)
{
	ArrayIter monitor_iter;
	b3_monitor_t *monitor;

	array_iter_init(&monitor_iter, director->monitor_arr);
	while (array_iter_next(&monitor_iter, (void *) &monitor) != CC_ITER_END) {
		b3_monitor_draw(monitor, window_handler);
	}

	return 0;
}