/**************************************************************************
*
* Tint Task Manager
* 
* Copyright (C) 2008 thil7 (lorthiois@bbsoft.fr)
* 
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Soft-
* ware"), to deal in the Software without restriction, including without
* limitation the rights to use, copy, modify, merge, publish, distribute,
* sublicense, and/or sell copies of the Software, and to permit persons to
* whom the Software is furnished to do so, subject to the following condi-
* tions:
* 
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
* ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
* SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
* OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
* 
**************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "launcher.h"

// voir lxpanel/src/plugins/launchbar.c
// malloc(): memory corruption si on inclue glibc.h

void launcher_add_exec (const char *exec)
{
   void *ptr;
   int  len = strlen(exec);

   if (len == 0) return;

   if ((launcher.launcher_count % 5) == 0) {
      if (launcher.launcher_count == 0)
         ptr = malloc(5 * sizeof(Launcher));
      else
         ptr = realloc(launcher.launcherlist, (launcher.launcher_count+5) * sizeof(Launcher));
      if (ptr == NULL) {
         fprintf(stderr, "malloc failed on launcher\n");
         return;
      }
      launcher.launcherlist = ptr;
      //printf("ici1 alloc memory\n");
   }
   
   if ((ptr = malloc(len+1)) == NULL) {
      fprintf(stderr, "malloc failed\n");
      return;
   }

   strcpy(ptr, exec);
   launcher.launcherlist[launcher.launcher_count].exec = ptr;
   launcher.launcherlist[launcher.launcher_count].icon = 0;
   launcher.launcherlist[launcher.launcher_count].text = 0;
   launcher.launcherlist[launcher.launcher_count].icon_data = 0;
   launcher.launcher_count++;
   
}

void launcher_add_text (const char *txt)
{
   void *ptr;
   int  len = strlen(txt);

   if ((launcher.launcher_count == 0) || (len == 0)) return;

   if ((ptr = malloc(len+1)) == NULL) {
      fprintf(stderr, "malloc failed\n");
      return;
   }
   strcpy(ptr, txt);
   launcher.launcherlist[launcher.launcher_count-1].text = ptr;

}

void launcher_add_icon (const char *ic)
{
   void *ptr;
   int  len = strlen(ic);

   if ((launcher.launcher_count == 0) || (len == 0)) return;

   if ((ptr = malloc(len+1)) == NULL) {
      fprintf(stderr, "malloc failed\n");
      return;
   }
   strcpy(ptr, ic);
   launcher.launcherlist[launcher.launcher_count-1].icon = ptr;
}


