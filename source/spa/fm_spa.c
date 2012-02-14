/* =====================================================================
 *
 *  Copyright 2009, 2010, Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  Author    : Hendricks Vince
 *
 * ===================================================================*/

/* ---------------------------------------------------------------------------
 *
 * Stand alone Soft Parser Assembler
 *
 * This assembler uses the Freescale Soft Parser library.
 *
 * ---------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


#include "fm_sp.h"

#define DEFAULT_OUTPUT_FILE_NAME "soft_parse_instructions.c"

#define FMSP_MAX_INSTR_WORDS 992

/* Help function */
void usage(void);

/* --------------------------------------------------------------------------
 * The assembler main
 * --------------------------------------------------------------------------*/

int main (int argc, char **argv)
{
   int                       option_index               = 0;
   int                       option_val                 = 0;
   int                       use_default_output         = 1;
   int                       input_file_given           = 0;
   int                       file_name_size             = 0;
   char                     *output_file_name_p         = NULL;
   char                     *input_file_name_p          = NULL;
   char                      default_output_file_name[] = DEFAULT_OUTPUT_FILE_NAME;
   int                       result;
   int                       mem_result;
   fmsp_assembler_options_t  assembler_options;
   char                     *assemble_msg_p             = NULL;
   uint32_t                  parse_program_base_address = FMSP_PARSE_PROGRAM_DEFAULT_BASE_ADDRESS;

   fmsp_label_list_t        *label_list_p               = NULL;
   uint8_t                   buffer_p[FMSP_MAX_INSTR_WORDS * 2];
   char                     *input_string_p;
   fmsp_label_list_t        *cur_label_p                = NULL;
   FILE   *fp;
   char   *str;
   struct  stat buf;
   int     fd;


   /* Clear buffer */
   memset(buffer_p, 0, FMSP_MAX_INSTR_WORDS * 2);

   /* Set default assembler options */

   memset (&assembler_options, 0, sizeof(fmsp_assembler_options_t));

   assembler_options.warnings_are_errors        = false;
   assembler_options.suppress_warnings          = false;
   assembler_options.program_space_base_address = FMSP_PARSE_PROGRAM_DEFAULT_BASE_ADDRESS;

   /* Uncomment to turn debug on */
   //_fmsp_assembler_yydebug = 1;

   static struct option long_options[] =
                {
                  {"help",           no_argument,       0, 'h'},
                  {"base",           required_argument, 0, 'b'},
                  {"input",          required_argument, 0, 'i'},
                  {"output",         required_argument, 0, 'o'},
                  {"Werror",         no_argument,       0, 'W'},
                  {"w",              no_argument,       0, 'w'},
#ifdef FMSP_DEBUG
                  {"debug_summary",  no_argument,       0, 's'},
                  {"debug_detailed", no_argument,       0, 'd'},
                  {"debug_test",     no_argument,       0, 't'},
#endif /* FMSP_DEBUG */
                  {0, 0, 0, 0}
                };

   /* Process options */
   while (1)
   {
      option_val = getopt_long (argc,
                                argv,
#ifdef FMSP_DEBUG
                                "hb:i:o:Wwsdt",
#else
                                "hb:i:o:Ww",
#endif
                                long_options,
                                &option_index);

      /* End of options */
      if (option_val == -1)
      {
         break;
      }

      /* Deal with each option */
      switch (option_val)
      {
         case 'h':
            usage();
            exit (0);
            break;

         case 'b':
            errno = 0;
            parse_program_base_address = (uint32_t)strtoul(optarg, NULL, 0);

            if (errno != 0)
            {
               printf("PE_SPA: Failed to convert base address.\n");
               return(fmsp_invalid_parse_program_base_address_e);
            }

            assembler_options.program_space_base_address = parse_program_base_address;
            
            break;

         case 'i':
            file_name_size = strlen(optarg) + 1;

            input_file_name_p = (char *)calloc(1, file_name_size);

            if (input_file_name_p == NULL)
            {
               printf("PE_SPA: ERROR: Malloc failure while parsing input file option.\n");
               return (fmsp_malloc_failure_e);
            }

            memset(input_file_name_p, 0, file_name_size);

            strncpy(input_file_name_p,
                    optarg,
                    file_name_size);

            input_file_given = 1;

            break;

         case 'o':
            file_name_size = strlen(optarg) + 1;

            output_file_name_p = (char *)calloc(1, file_name_size);

            if (output_file_name_p == NULL)
            {
               printf("PE_SPA: ERROR: Malloc failure while parsing output file option.\n");
               return (fmsp_malloc_failure_e);
            }

            memset(output_file_name_p, 0, file_name_size);

            strncpy(output_file_name_p,
                    optarg,
                    file_name_size);

            use_default_output = 0;
 
            break;

#ifdef FMSP_DEBUG
         case 's':
            assembler_options.debug_level = assembler_options.debug_level | fmsp_debug_summary_e;
            break;

         case 'd':
            assembler_options.debug_level = assembler_options.debug_level | fmsp_debug_detailed_e;
            break;

         case 't':
            assembler_options.debug_level = assembler_options.debug_level | fmsp_debug_test_e;
            break;
#endif /* FMSP_DEBUG */

         case 'W':
            assembler_options.warnings_are_errors = true;
            break;

         case 'w':
            assembler_options.suppress_warnings = true;
            break;

         default:
            printf("Unrecognized option =>%s<=\n", *argv);
            usage();
            exit (0);
            break;
      }
   }

   if (!input_file_given)
   {
      printf("PE_SPA: ERROR: No input file given.\n");
      return(-1);
   }

   if (use_default_output)
   {
      file_name_size = strlen(default_output_file_name) + 1;

      output_file_name_p = (char *)calloc(1, file_name_size);

      if (output_file_name_p == NULL)
      {
         printf("PE_SPA: ERROR: Malloc failure while creating output file string.\n");
         return (fmsp_malloc_failure_e);
      }

      memset(output_file_name_p, 0, file_name_size);

      strncpy(output_file_name_p,
              default_output_file_name,
              file_name_size);
   }

   fflush(NULL);

#if 0
   /* Assemble the code */
   result = fmsp_assemble(input_file_name_p,
                          output_file_name_p,
                          &assembler_options,
                          &assemble_msg_p);

   if (assembler_options.warnings_are_errors == 1 &&
       result == fmsp_warnings_e)
   {
      result = fmsp_assemble_failed_e;
   }

   if (result != fmsp_ok_e)
   {
      if (assemble_msg_p != NULL)
      {
         printf("%s\n", assemble_msg_p);
      }

      printf("\n%s\n", fmsp_get_error_string(result));
   }

   if (assemble_msg_p != NULL)
   {
      /* Assemble message was allocated in assembler lib. Must free here. */
      free (assemble_msg_p);
   }

   if (input_file_name_p != NULL)
   {
      free(input_file_name_p);
   }

   if (output_file_name_p != NULL)
   {
      free(output_file_name_p);
   }
#endif

   // Read file into string
   fd = open(input_file_name_p, O_RDONLY);

   if (fd < 0)
   {
      perror("Could not open file:");
      return(errno);
   }

   result = fstat(fd, &buf);
   close (fd);

   input_string_p = calloc(buf.st_size + 1, 1);

   if (input_string_p == NULL)
   {
      perror("Could not allocate memory:");
      return(errno);
   }

   fp = fopen(input_file_name_p, "r");

   if (fp == NULL)
   {
      perror("Could not open file:");
      return(errno);
   }

   result = fread(input_string_p,
                  buf.st_size,
                  1,
                  fp);

//   printf("DEBUG: Read string:\n%s\n", input_string_p);

   fclose(fp);

   // Assemble code
   result = fmsp_assemble(input_string_p,
                          buffer_p,
                          &label_list_p,
                          &assembler_options,
                          &assemble_msg_p);

   if (assembler_options.warnings_are_errors == 1 &&
       result == fmsp_warnings_e)
   {
      result = fmsp_assemble_failed_e;
   }

   if (result != fmsp_ok_e)
   {
      if (assemble_msg_p != NULL)
      {
         printf("%s\n", assemble_msg_p);
      }

      printf("\n%s\n", fmsp_get_error_string(result));
   }

   if (assemble_msg_p != NULL)
   {
      /* Assemble message was allocated in assembler lib. Must free here. */
      free (assemble_msg_p);
   }

   printf("\nBuffer:\n");

   int byte;
   for (byte = 0; byte < FMSP_MAX_INSTR_WORDS * 2; byte++)
   {
      if (byte > 0 && (byte % 40 == 0)) {printf("\n");}

      printf("%02x", buffer_p[byte]);
   }

   printf("\n");

   printf("\nLabels:\n");
   cur_label_p =  label_list_p;

   while (cur_label_p != NULL)
   {
      printf("  Name  : %s\n", cur_label_p->name);
      printf("  Pos   : %d\n", cur_label_p->byte_position);
      printf("  Size  : %d\n", cur_label_p->bit_size);
      printf("  Offset: %d\n", cur_label_p->bit_offset);
      printf("\n");

      cur_label_p = cur_label_p->next_p;
   }


   return (result);
}

/*
 * Display the help text.
 */
void usage (void)
{
   printf ("\n");
   printf ("   Description:\n");
   printf ("      The soft parser assembler takes input from a file or STDIN and\n");
   printf ("      converts the instructions into an internal format.\n");
   printf ("\n");
   printf ("   Options:\n");
   printf ("      -h, --help           This help.\n");
   printf ("\n");
   printf ("      -b, --base           The base address of the parse program space.\n");
   printf ("                           Defaults to 0x%03x\n", FMSP_PARSE_PROGRAM_DEFAULT_BASE_ADDRESS);
   printf ("                           Min: 0x%03x\n", FMSP_PARSE_PROGRAM_MIN_BASE_ADDRESS);
   printf ("                           Max: 0x%03x\n", FMSP_PARSE_PROGRAM_MAX_BASE_ADDRESS);
   printf ("\n");
   printf ("      -i, --input          The name of the file containing the user's code.\n");
   printf ("                           Defaults to STDIN.\n");
   printf ("\n");
   printf ("      -o, --output         The name of the file where the output will be placed.\n");
   printf ("                           Defaults to '%s'.\n", DEFAULT_OUTPUT_FILE_NAME);
   printf ("\n");
   printf ("      -W, --Werror         Warnings are errors.\n");
   printf ("\n");
   printf ("      -w, --w              Suppress warning messages. Note: Ignored if --Werror used.\n");
   printf ("\n");
#ifdef FMSP_DEBUG
   printf ("   Debug:\n");
   printf ("      -s, --debug_summary  Summary level debug.\n");
   printf ("\n");
   printf ("      -d, --debug_detailed Show lower level debug (must use with --debug_summary).\n");
   printf ("\n");
   printf ("      -t, --debug_test     Show test debug.\n");
   printf ("\n");
#endif /* FMSP_DEBUG */
   printf ("   Example:\n");
   printf ("      fm_spa --input my_expressions --output my_expressions.out\n");
   printf ("\n");
}
