
/* statmbx 0.2
 *
 * Copyright 2007,2010, Jeff Kaufman.  Distributed under the GPL.
 *
 * Reads mbx files and prints out information about unread messages.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STATMBX_BUF_SIZE 256
#define STATMBX_CONFIG_FILE_NAME ".statmbx"
#define STATMBX_DO_BOLD 1

#define SCREEN_WIDTH 80 /* assume we have 80 columns to work with */
#define FROM_HALF 30    /* how many columns to allow from the from
                           line */ 

/* If we're supporting bold text, print with escape characters.
 * Otherwise don't bother*/
void statmbx_printMaybeBold(char* str)
{
#if STATMBX_DO_BOLD
   printf("%c%c%c%c%s%c%c%c%c", 033, '[', '1', 'm',
          str ,033,'[','0','m');
#else
   printf("%s", str);
#endif
}


void statmbx_getRelativeFname(char* base, char* relative, char* fname)
{
   int i, n;

   for(n=0;n<strlen(base) && n<STATMBX_BUF_SIZE-strlen(relative);n++)
      fname[n] = base[n];

   fname[n++] = '/';

   for(i=0;i<strlen(relative) && i< STATMBX_BUF_SIZE;i++)
      fname[n++] = relative[i];

   fname[n] = '\0';
}

FILE* statmbx_getFile(char* homedir, char* suffix)
{
   char fname[STATMBX_BUF_SIZE];
   statmbx_getRelativeFname(homedir, suffix, fname);
   return fopen(fname, "r");
}

void statmbx_printName(char* name, char* subj)
{
   int i, len, offset;

   int seen_at = 0;

   for(i = 0; i<strlen(name) && i<STATMBX_BUF_SIZE; i++)
      if(name[i] == '\n')
	 name[i] = '\0';

   for(i = 0; i<strlen(subj) && i<STATMBX_BUF_SIZE; i++)
      if(subj[i] == '\n')
	 subj[i] = '\0';

   /* prepare "from" */

   offset = 6; /* length of "From: " */
   for (i=offset;i<strlen(name)&&i<STATMBX_BUF_SIZE;i++)
   {
      if (name[i] == '<' || (name[i] == ' ' && seen_at))
	 break;
      if (name[i] == '"')
	 offset++;
      else
      {
	 if (name[i] == '@')
	    seen_at = 1;
	 name[i-offset] = name[i];
      }
   }
   if (i-offset > FROM_HALF-1)
   {
      name[FROM_HALF - 6] = '.';
      name[FROM_HALF - 5] = '.';
      name[FROM_HALF - 4] = '.';
      name[FROM_HALF - 3] = ' ';
      name[FROM_HALF - 2] = ' ';
      name[FROM_HALF - 1] = '\0';
   }
   else 
   {
      while (i - offset < FROM_HALF-1)
         name[(i++) - offset] = ' ';
      name[i-offset] = '\0';
   }


   /* prepare "subj" */

   offset = 9; /* length of "Subject: " */
   for (i=offset;i<strlen(subj)&&i<STATMBX_BUF_SIZE;i++)
      subj[i-offset] = subj[i];

   if (i-offset > SCREEN_WIDTH - FROM_HALF - 2)
   {
      subj[SCREEN_WIDTH - FROM_HALF - 2 + 0] = '.';
      subj[SCREEN_WIDTH - FROM_HALF - 2 + 1] = '.';
      subj[SCREEN_WIDTH - FROM_HALF - 2 + 2] = '.';
      subj[SCREEN_WIDTH - FROM_HALF - 2 + 3] = '\0';
   }
   else 
   {
      while (i - offset < SCREEN_WIDTH - FROM_HALF - 2 - 1)
         subj[(i++) - offset] = ' ';
      subj[i-offset] = '\0';
   }
   
                                                                                           
   if (strlen(name) > 0)
      printf("  %s%s\n", name,subj);
}


void statmbx_splitConfigLine(char* line,
			     char** mailbox_title,
			     char** mailbox_fname)
{
   int i;

   for(i = 0 ; i < STATMBX_BUF_SIZE ; i++)
      if (line[i] == ' ')
      {
	 line[i] = '\0';
	 *mailbox_title = line;
	 *mailbox_fname = line+i+1;
	 for(;i<STATMBX_BUF_SIZE ; i++)
	    if(line[i] == '\n')
	    {
	       line[i] = '\0';
	       break;
	    }
	 return;
      }

   printf("statmbx: error, config line \"%s\" invalid\n",line);
   exit(-1);
}

int main(int argc, char** argv)
{
   FILE *config_file;
   FILE *mail_file;
   char config_line [STATMBX_BUF_SIZE];
   char mail_line [STATMBX_BUF_SIZE];

   char cur_name [STATMBX_BUF_SIZE];
   char cur_subj [STATMBX_BUF_SIZE];

   int n,i;
   char* homedir;
   char* mailbox_title;
   char* mailbox_fname;

   int msgs, new_msgs;
   int inhdr, seen_status_read, printed_title;
   
   int just_print_names;
   

   just_print_names = 0; /*don't print message, just a space separated
                           list of mailboxes */
   if (argc == 2 && strncmp(argv[1], "--only-names", 13) == 0) 
      just_print_names = 1;
   else if (argc != 1) 
   {
      printf("Usage: statmbx or statmbx --only-names\n");
      exit(-1);
   }

   if( (homedir = getenv("HOME")) == NULL)
   {
      printf("statmbx:: error, envrionment varible \"HOME\" not\
              found.\n");
      exit (-1);
   }

   config_file = statmbx_getFile(homedir,STATMBX_CONFIG_FILE_NAME);

   n=0;
   while(fgets(config_line,STATMBX_BUF_SIZE,config_file) != NULL)
   {
      config_line[STATMBX_BUF_SIZE-1] = '\0';
      if (config_line[0] == '\0' || config_line[0] == '#')
         continue;

      statmbx_splitConfigLine(config_line, &mailbox_title, &mailbox_fname);
      mail_file = statmbx_getFile(homedir, mailbox_fname);
      if (mail_file == 0)
      {
	 printf("statmbx: error, couldn't locate ");
	 printf("mailfile %s relative to %s.\n", mailbox_fname, homedir);
      }
      else
      {
	 msgs = new_msgs = seen_status_read = inhdr = printed_title = 0;

         while(fgets(mail_line, STATMBX_BUF_SIZE, mail_file) != NULL)
	 {
	    mail_line[STATMBX_BUF_SIZE-1] = '\0';
	    if (strncmp(mail_line, "From ", 5) == 0)
	    {
	       if (inhdr)
	       {
		  printf("statmbx: error, corrupt mail file \"%s\"",
			 mailbox_fname);
		  exit(-1);
	       }
	       
	       seen_status_read = 0;
	       inhdr = 1;
	       cur_name[0] = '\0';
	       cur_subj[0] = '\0';
	    }
	    else if(inhdr)
	    {
	       if(mail_line[0] == '\n')
	       {
                  if (!seen_status_read)
                  {
                     if (just_print_names) 
                     {
                        printf("%s ", mailbox_title);
                        break; /* next file */                        
                     }

                     new_msgs++;
                     if (!printed_title)
                     {
                        printed_title = 1;
                        printf("\n");
                        statmbx_printMaybeBold(mailbox_title);
                        printf("\n");
                     }
                     
                     statmbx_printName(cur_name, cur_subj);
                     
                  }
                  msgs++;
                  inhdr = 0;
               }
               else /*line in the header */
               {
                  if (strncmp(mail_line, "Status: R", 9) == 0)
                     seen_status_read = 1;
                  else if(strncmp(mail_line, "From: ", 6) == 0)
                     strncpy(cur_name, mail_line, STATMBX_BUF_SIZE);
                  else if(strncasecmp(mail_line, "Subject: ", 9) == 0)
                     strncpy(cur_subj, mail_line, STATMBX_BUF_SIZE);
               }
            }
	 }
      }
      n++;
   }

   if (just_print_names)
      printf("\n");
   

   fclose(config_file);
}
