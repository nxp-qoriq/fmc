/* =====================================================================
 *
 *  Copyright 2009,2010 Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  Packet Engine Soft Parse Assembler parser definition.
 *
 * --------------------------------------------------------------------------*/

%{
#ifndef _MSC_VER
#include <inttypes.h>  /* For standard int types and print format macros */
#include <stdbool.h>   /* For the bool type */
#endif

#include <errno.h>     /* For errno */
#include <stdlib.h>    /* For strtoull */
#ifdef _MSC_VER
#define _STDLIB_H
#endif
#include <string.h>    /* For strdup */

#include "fm_sp_private.h"

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

/* ---------------------------------------------------------------------------
 * Some defines that will make the grammar easier to read
 * ---------------------------------------------------------------------------*/

/* An action that take no parameters other than the context. */
#define _FMSP_PROC_TOK_P0(action_ok)                \
   if (!action_ok(_fmsp_assembler_context_p))       \
   {                                                \
      return (1);                                   \
   }

/* An action that takes the context and 1 parameter that must be freed
 * (lexical analyzer allocated the memory while collecting tokens).
 */
#define _FMSP_PROC_TOK_P1(action_ok, p1)            \
   if (!action_ok(_fmsp_assembler_context_p, p1))   \
   {                                                \
      free p1;                                      \
      return (1);                                   \
   }                                                \
   free p1;

/* An action that takes the context and 2 parameters that must be freed
 * (lexical analyzer allocated the memory while collecting tokens).
 */
#define _FMSP_PROC_TOK_P2(action_ok, p1, p2)            \
   if (!action_ok(_fmsp_assembler_context_p, p1, p2))   \
   {                                                    \
      free p1;                                          \
      free p2;                                          \
      return (1);                                       \
   }                                                    \
   free p1;                                             \
   free p2;

/* An action that takes the context and 3 parameters that must be freed
 * (lexical analyzer allocated the memory while collecting tokens).
 */
#define _FMSP_PROC_TOK_P3(action_ok, p1, p2, p3)            \
   if (!action_ok(_fmsp_assembler_context_p, p1, p2, p3))   \
   {                                                        \
      free p1;                                              \
      free p2;                                              \
      free p3;                                              \
      return (1);                                           \
   }                                                        \
   free p1;                                                 \
   free p2;                                                 \
   free p3;

/* An action that takes the context and 4 parameters that must be freed
 * (lexical analyzer allocated the memory while collecting tokens).
 */
#define _FMSP_PROC_TOK_P4(action_ok, p1, p2, p3, p4)            \
   if (!action_ok(_fmsp_assembler_context_p, p1, p2, p3, p4))   \
   {                                                            \
      free p1;                                                  \
      free p2;                                                  \
      free p3;                                                  \
      free p4;                                                  \
      return (1);                                               \
   }                                                            \
   free p1;                                                     \
   free p2;                                                     \
   free p3;                                                     \
   free p4;

/* An action that takes the context and 5 parameters that must be freed
 * (lexical analyzer allocated the memory while collecting tokens).
 */
#define _FMSP_PROC_TOK_P5(action_ok, p1, p2, p3, p4, p5)            \
   if (!action_ok(_fmsp_assembler_context_p, p1, p2, p3, p4, p5))   \
   {                                                                \
      free p1;                                                      \
      free p2;                                                      \
      free p3;                                                      \
      free p4;                                                      \
      free p5;                                                      \
      return (1);                                                   \
   }                                                                \
   free p1;                                                         \
   free p2;                                                         \
   free p3;                                                         \
   free p4;                                                         \
   free p5;

/* ---------------------------------------------------------------------------
 * Constants 
 * ---------------------------------------------------------------------------*/

#define MAX_RA_DECIMAL_INDEX_SIZE      3
#define MAX_RESULT_ARRAY_BYTE_INDEX    0x7f 
#define MAX_PARAMETER_ARRAY_BYTE_INDEX 0x3f 
#define MAX_FRAME_WINDOW_BYTE_INDEX    0x0f 
#define MAX_FRAME_WINDOW_BIT_INDEX     0x7f 

/* ---------------------------------------------------------------------------
 * Lexical Analyzer declarations
 * ---------------------------------------------------------------------------*/

int _fmsp_assembler_yylex (void                             *lval_p,
                           _fmsp_assembler_parser_context_t *context_p);

/* ---------------------------------------------------------------------------
 * Action declarations
 * ---------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------
 * Action for label
 *
 * Parameters:
 *    ctx_p - Parse context
 *    str_p - String containing label name.
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_label_action(_fmsp_assembler_parser_context_t *ctx_p,
                               char                             *str_p)
{
   _fmsp_label_t                 *label_p;
   _fmsp_assembler_instruction_t *instr_p;
   char                          *full_label_p;

   /* The context holds a hash table for all labels.  Each label object
    * in the hash table holds the program counter for the instruction
    * which follows the label.
    */

   /* Build the label name */
   full_label_p = (char *)calloc(strlen(str_p) + 1, 1);

   if (full_label_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");

      free (full_label_p); /* ROEE fixed mem leak*/
      return (false);
   }

   strcat(full_label_p, str_p);

   /* Store label in a hash table */
   label_p = _fmsp_new_label(full_label_p,
                             ctx_p->next_program_counter);

   if (label_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");

      free (full_label_p); /* ROEE fixed mem leak*/
      return (false);
   }

   if (!htbl_insert(ctx_p->label_table_p,
                    &(label_p->htbl_entry)))
   {
      _fmsp_destroy_label(label_p);

      _fmsp_assembler_yyerror(ctx_p,
                              "Label already exists.");
      free (full_label_p); /* ROEE fixed mem leak*/
      return (false);
   }

   instr_p = _fmsp_new_instruction(_fmsp_assembler_label_e,
                                   ctx_p);

   if (instr_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      free (full_label_p); /* ROEE fixed mem leak*/
      return (false);
   }

   /* Label is just a place holder, not an actual instruction. */
   instr_p->num_valid_hw_words = 0;

   instr_p->jump_label1_p = strdup(str_p);

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(instr_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   free (full_label_p); /* ROEE fixed mem leak*/
   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for five words
 *
 * Parameters:
 *    ctx_p  - Parse context
 *    str1_p - String containing hex value
 *    str2_p - String containing hex value
 *    str3_p - String containing hex value
 *    str4_p - String containing hex value
 *    str5_p - String containing hex value
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_five_hex_words_action(_fmsp_assembler_parser_context_t *ctx_p,
                                        char                             *str1_p,
                                        char                             *str2_p,
                                        char                             *str3_p,
                                        char                             *str4_p,
                                        char                             *str5_p)
{
   ctx_p->raw_str_p[0] = strdup(str1_p);
   ctx_p->raw_str_p[1] = strdup(str2_p);
   ctx_p->raw_str_p[2] = strdup(str3_p);
   ctx_p->raw_str_p[3] = strdup(str4_p);
   ctx_p->raw_str_p[4] = strdup(str5_p);

   if (ctx_p->raw_str_p[0] == NULL ||
       ctx_p->raw_str_p[1] == NULL ||
       ctx_p->raw_str_p[2] == NULL ||
       ctx_p->raw_str_p[3] == NULL ||
       ctx_p->raw_str_p[4] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      return (false);
   }

   ctx_p->num_immediate_values = 4;

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for four words
 *
 * Parameters:
 *    ctx_p  - Parse context
 *    str1_p - String containing hex value
 *    str2_p - String containing hex value
 *    str3_p - String containing hex value
 *    str4_p - String containing hex value
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_four_hex_words_action(_fmsp_assembler_parser_context_t *ctx_p,
                                        char                             *str1_p,
                                        char                             *str2_p,
                                        char                             *str3_p,
                                        char                             *str4_p)
{
   ctx_p->raw_str_p[0] = strdup(str1_p);
   ctx_p->raw_str_p[1] = strdup(str2_p);
   ctx_p->raw_str_p[2] = strdup(str3_p);
   ctx_p->raw_str_p[3] = strdup(str4_p);

   if (ctx_p->raw_str_p[0] == NULL ||
       ctx_p->raw_str_p[1] == NULL ||
       ctx_p->raw_str_p[2] == NULL ||
       ctx_p->raw_str_p[3] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      return (false);
   }

   ctx_p->num_immediate_values = 3;

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for three words
 *
 * Parameters:
 *    ctx_p  - Parse context
 *    str1_p - String containing hex value
 *    str2_p - String containing hex value
 *    str3_p - String containing hex value
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_three_hex_words_action(_fmsp_assembler_parser_context_t *ctx_p,
                                         char                             *str1_p,
                                         char                             *str2_p,
                                         char                             *str3_p)
{
   ctx_p->raw_str_p[0] = strdup(str1_p);
   ctx_p->raw_str_p[1] = strdup(str2_p);
   ctx_p->raw_str_p[2] = strdup(str3_p);

   if (ctx_p->raw_str_p[0] == NULL ||
       ctx_p->raw_str_p[1] == NULL ||
       ctx_p->raw_str_p[2] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      return (false);
   }

   ctx_p->num_immediate_values = 2;

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for two words
 *
 * Parameters:
 *    ctx_p  - Parse context
 *    str1_p - String containing hex value
 *    str2_p - String containing hex value
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_two_hex_words_action(_fmsp_assembler_parser_context_t *ctx_p,
                                       char                             *str1_p,
                                       char                             *str2_p)
{
   ctx_p->raw_str_p[0] = strdup(str1_p);
   ctx_p->raw_str_p[1] = strdup(str2_p);

   if (ctx_p->raw_str_p[0] == NULL ||
       ctx_p->raw_str_p[1] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      return (false);
   }

   ctx_p->num_immediate_values = 1;

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for one word
 *
 * Parameters:
 *    ctx_p  - Parse context
 *    str1_p - String containing hex value
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_one_hex_word_action(_fmsp_assembler_parser_context_t *ctx_p,
                                      char                             *str1_p)
{
   ctx_p->raw_str_p[0] = strdup(str1_p);

   if (ctx_p->raw_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      return (false);
   }

   ctx_p->num_immediate_values = 0;

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for raw instruction
 *
 * Parameters:
 *    ctx_p - Parse context
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_raw_instruction_action(_fmsp_assembler_parser_context_t *ctx_p)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;
   int32_t                        count;

   object_p = _fmsp_new_instruction(_fmsp_assembler_raw_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   for (count = 0; count < ctx_p->num_immediate_values + 1; count++)
   {
      len = strlen(ctx_p->raw_str_p[count]);

      if (len > 0)
      {
         object_p->hw_words_str_p[count] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[count] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            return (false);
         }

         strncpy(object_p->hw_words_str_p[count],
                 ctx_p->raw_str_p[count],
                 len);

         object_p->hw_words_p[count] = (uint16_t)strtol(ctx_p->raw_str_p[count],
                                                        NULL,
                                                        0);
      }
   }

   object_p->num_valid_hw_words = ctx_p->num_immediate_values + 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for jmp label
 *
 * Parameters:
 *    ctx_p - Parse context
 *    str_p - String containing label name.
 *    hxs   - Header examination sequence?
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_label_action(_fmsp_assembler_parser_context_t *ctx_p,
                                   char                             *str_p,
                                   bool                              hxs)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;
   char                           hxs_bit_p[] = " | 0x400";

   /* No immediate values for jump instruction */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_jump_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   if (hxs)
   {
      len = strlen(_FMSP_INSTR_CODE_JUMP_STR) +
            strlen(hxs_bit_p)                 +
            1;

      object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

      if (object_p->hw_words_str_p[0] == NULL)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to allocate string.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      snprintf(object_p->hw_words_str_p[0],
               len,
               "%s%s",
               _FMSP_INSTR_CODE_JUMP_STR,
               hxs_bit_p);

      object_p->hw_words_p[0] = _FMSP_INSTR_CODE_JUMP | _FMSP_INSTR_MOD_JMP_HXS;
   }
   else
   {
      len = strlen(_FMSP_INSTR_CODE_JUMP_STR) + 1;

      object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

      if (object_p->hw_words_str_p[0] == NULL)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to allocate string.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      snprintf(object_p->hw_words_str_p[0],
               len,
               "%s",
               _FMSP_INSTR_CODE_JUMP_STR);

      object_p->hw_words_p[0] = _FMSP_INSTR_CODE_JUMP;
   }

   object_p->hxs                = hxs;
   object_p->num_valid_hw_words = 1;

   object_p->jump_label1_p      = strdup(str_p);

   if (object_p->jump_label1_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for jmp label if wr0 <operator> wr1
 *
 * Parameters:
 *    ctx_p      - Parse context
 *    str_p      - String containing label name.
 *    operator_p - Operator. (==, !=, <=, >=, <, >)
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_label_if_wr0_wr1_action(_fmsp_assembler_parser_context_t *ctx_p,
                                              char                             *str_p,
                                              uint16_t                          operator)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;
   char                           or_str_p[] = " | ";

   /* 1 immediate value for jump destination. */
   ctx_p->num_immediate_values = 1;

   object_p = _fmsp_new_instruction(_fmsp_assembler_compare_working_regs_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   len = strlen(_FMSP_INSTR_CODE_COMPARE_WORKING_REGS_STR) +
         strlen(or_str_p)                                  +
         6                                                 + /* 0xNNNN */
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s0x%04x",
            _FMSP_INSTR_CODE_COMPARE_WORKING_REGS_STR,
            or_str_p,
            operator);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_COMPARE_WORKING_REGS | operator;

   object_p->num_valid_hw_words = 2;
   object_p->function           = (uint8_t)operator;

   object_p->jump_label1_p      = strdup(str_p);

   if (object_p->jump_label1_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for jmp label if wr0 <operator> immediate value
 *
 * Parameters:
 *    ctx_p      - Parse context
 *    str_p      - String containing label name.
 *    operator_p - Operator. (==, !=, <, >)
 *    iv_p       - Immediate value.
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_label_if_wr0_iv_action(_fmsp_assembler_parser_context_t *ctx_p,
                                             char                             *str_p,
                                             uint16_t                          operator,
                                             char                             *iv_p)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           start_shift_str_p[]  = " | (";
   char                           end_shift_12_str_p[] = " << 12)";
   char                           end_shift_10_str_p[] = " << 10)";
   uint32_t                       len;
   uint64_t                       immediate_value      = 0;
   uint16_t                       word_1;
   uint16_t                       word_2;
   uint16_t                       word_3;
   uint16_t                       word_4;
   uint8_t                        iv_word_size         = 0;

   /* Check if the immediate value starts with "0x". If it does then
    * convert the value and base the number of immediate value words
    * on that.  Otherwise, this is a plan id enum which is 1 byte so
    * only use 1 immediate value word.
    */
   if (strncmp(iv_p, "0x", 2) == 0)
   {
      immediate_value = (uint64_t)strtoull(iv_p, NULL, 0);      

      if (immediate_value < 0x10000)
      {
         ctx_p->num_immediate_values = 1;
      }
      else if (immediate_value < 0x100000000ULL)
      {
         ctx_p->num_immediate_values = 2;
      }
      else if (immediate_value < 0x1000000000000ULL)
      {
         ctx_p->num_immediate_values = 3;
      }
      else
      {
         ctx_p->num_immediate_values = 4;
      }
   }

   object_p = _fmsp_new_instruction(_fmsp_assembler_compare_wr0_to_iv_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Set immediate values. */
   if (strncmp(iv_p, "0x", 2) == 0)
   {
      len = 6 + /* 0xNNNN */
            1;

      word_1 =  immediate_value        & 0xffff;
      word_2 = (immediate_value >> 16) & 0xffff;
      word_3 = (immediate_value >> 32) & 0xffff;
      word_4 = (immediate_value >> 48) & 0xffff;

      if (immediate_value < 0x10000)
      {
         object_p->num_valid_hw_words = 2;
         iv_word_size                 = 0;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         object_p->hw_words_p[1] = word_1;
      }
      else if (immediate_value < 0x100000000ULL)
      {
         object_p->num_valid_hw_words = 3;
         iv_word_size                 = 1;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL ||
             object_p->hw_words_str_p[2] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x%04x",
                  word_2);

         object_p->hw_words_p[1] = word_1;
         object_p->hw_words_p[2] = word_2;
      }
      else if (immediate_value < 0x1000000000000ULL)
      {
         object_p->num_valid_hw_words = 4;
         iv_word_size                 = 2;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[3] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL ||
             object_p->hw_words_str_p[2] == NULL ||
             object_p->hw_words_str_p[3] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x%04x",
                  word_2);

         snprintf(object_p->hw_words_str_p[3],
                  len,
                  "0x%04x",
                  word_3);

         object_p->hw_words_p[1] = word_1;
         object_p->hw_words_p[2] = word_2;
         object_p->hw_words_p[3] = word_3;
      }
      else
      {
         object_p->num_valid_hw_words = 5;
         iv_word_size                 = 3;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[3] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[4] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL ||
             object_p->hw_words_str_p[2] == NULL ||
             object_p->hw_words_str_p[3] == NULL ||
             object_p->hw_words_str_p[4] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x%04x",
                  word_2);

         snprintf(object_p->hw_words_str_p[3],
                  len,
                  "0x%04x",
                  word_3);

         snprintf(object_p->hw_words_str_p[4],
                  len,
                  "0x%04x",
                  word_4);

         object_p->hw_words_p[1] = word_1;
         object_p->hw_words_p[2] = word_2;
         object_p->hw_words_p[3] = word_3;
         object_p->hw_words_p[4] = word_4;
      }
   }
   else
   {
      object_p->num_valid_hw_words = 2;
      iv_word_size                 = 0;

      len = strlen(iv_p) + 1;

      object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);

      if (object_p->hw_words_str_p[1] == NULL)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to allocate string.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      snprintf(object_p->hw_words_str_p[1],
               len,
               "%s",
               iv_p);

      object_p->hw_words_p[1] = 0; /* Variable that must be filled in later */
   }

   /* Bit layout of this instruction:
    *
    * 01CC IIJJ JJJJ JJJJ
    *
    * C - Comparison operator
    * I - Immediate value size (0, 1, 2, 3)
    * J - Jump destination
    */

   object_p->function = (uint8_t)operator;

   len = strlen(_FMSP_INSTR_CODE_COMPARE_WR0_TO_IV_STR) +
         strlen(start_shift_str_p)                      +
         6                                              + /* 0xNNNN */
         strlen(end_shift_12_str_p)                     +
         strlen(start_shift_str_p)                      +
         6                                              + /* 0xNNNN */
         strlen(end_shift_10_str_p)                     +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s0x%04x%s%s0x%04x%s",
            _FMSP_INSTR_CODE_COMPARE_WR0_TO_IV_STR,
            start_shift_str_p,
            operator,
            end_shift_12_str_p,
            start_shift_str_p,
            iv_word_size,
            end_shift_10_str_p
           );

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_COMPARE_WR0_TO_IV |
                             (operator << 12)                   |
                             (iv_word_size << 10);

   object_p->jump_label1_p = strdup(str_p);

   if (object_p->jump_label1_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for case 1 default jump
 *
 * Parameters:
 *    ctx_p    - Parse context
 *    str1_p   - String containing label 1 name.
 *    str2_p   - String containing label 2 name.
 *    hxs      - Header examination sequence?
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_label_case1dj_action(_fmsp_assembler_parser_context_t *ctx_p,
                                           char                             *str1_p,
                                           char                             *str2_p,
                                           bool                              hxs_1,
                                           bool                              hxs_2)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           or_str_p[]            = " | ";
   char                           hxs_1_not_set_str_p[] = " 0";
   char                           hxs_2_not_set_str_p[] = " (0 << 1)";
   char                           hxs_1_set_str_p[]     = " 1";
   char                           hxs_2_set_str_p[]     = " (1 << 1)";
   char                          *hxs_1_str_p;
   char                          *hxs_2_str_p;
   uint32_t                       len;

   /* Set the number of immediate values this instruction will take. */
   ctx_p->num_immediate_values = 2;

   object_p = _fmsp_new_instruction(_fmsp_assembler_case1_dj_wr_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0000 10AB <label 1> <label 2>
    *
    * A - HXS for label 2
    * B - HXS for label 1
    */

   if (hxs_1)
   {
      hxs_1_str_p = hxs_1_set_str_p;
   }
   else
   {
      hxs_1_str_p = hxs_1_not_set_str_p;
   }

   if (hxs_2)
   {
      hxs_2_str_p = hxs_2_set_str_p;
   }
   else
   {
      hxs_2_str_p = hxs_2_not_set_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_CASE1_DJ_WR_TO_WR_STR) +
         strlen(or_str_p)                               +
         strlen(hxs_1_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_2_str_p)                            +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s%s",
            _FMSP_INSTR_CODE_CASE1_DJ_WR_TO_WR_STR,
            or_str_p,
            hxs_2_str_p,
            or_str_p,
            hxs_1_str_p
           );

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_CASE1_DJ_WR_TO_WR |
                             (hxs_2 << 1)                       |
                             hxs_1;

   object_p->hxs_1              = hxs_1;
   object_p->hxs_2              = hxs_2;
   object_p->num_valid_hw_words = 3;

   object_p->jump_label1_p = strdup(str1_p);

   if (object_p->jump_label1_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label2_p = strdup(str2_p);

   if (object_p->jump_label2_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for case 2 default continue
 *
 * Parameters:
 *    ctx_p    - Parse context
 *    str1_p   - String containing label 1 name.
 *    str2_p   - String containing label 2 name.
 *    hxs      - Header examination sequence?
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_label_case2dc_action(_fmsp_assembler_parser_context_t *ctx_p,
                                           char                             *str1_p,
                                           char                             *str2_p,
                                           bool                              hxs_1,
                                           bool                              hxs_2)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           or_str_p[]            = " | ";
   char                           hxs_1_not_set_str_p[] = " 0";
   char                           hxs_2_not_set_str_p[] = " (0 << 1)";
   char                           hxs_1_set_str_p[]     = " 1";
   char                           hxs_2_set_str_p[]     = " (1 << 1)";
   char                          *hxs_1_str_p;
   char                          *hxs_2_str_p;
   uint32_t                       len;

   /* Set the number of immediate values this instruction will take. */
   ctx_p->num_immediate_values = 2;

   object_p = _fmsp_new_instruction(_fmsp_assembler_case2_dc_wr_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0000 11AB <label 1> <label 2>
    *
    * A - HXS for label 2
    * B - HXS for label 1
    */

   if (hxs_1)
   {
      hxs_1_str_p = hxs_1_set_str_p;
   }
   else
   {
      hxs_1_str_p = hxs_1_not_set_str_p;
   }

   if (hxs_2)
   {
      hxs_2_str_p = hxs_2_set_str_p;
   }
   else
   {
      hxs_2_str_p = hxs_2_not_set_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_CASE2_DC_WR_TO_WR_STR) +
         strlen(or_str_p)                               +
         strlen(hxs_1_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_2_str_p)                            +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s%s",
            _FMSP_INSTR_CODE_CASE2_DC_WR_TO_WR_STR,
            or_str_p,
            hxs_2_str_p,
            or_str_p,
            hxs_1_str_p
           );

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_CASE2_DC_WR_TO_WR |
                             (hxs_2 << 1)                       |
                             hxs_1;

   object_p->hxs_1              = hxs_1;
   object_p->hxs_2              = hxs_2;
   object_p->num_valid_hw_words = 3;

   object_p->jump_label1_p = strdup(str1_p);

   if (object_p->jump_label1_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label2_p = strdup(str2_p);

   if (object_p->jump_label2_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for case 2 default jump
 *
 * Parameters:
 *    ctx_p    - Parse context
 *    str1_p   - String containing label 1 name.
 *    str2_p   - String containing label 2 name.
 *    str3_p   - String containing label 3 name.
 *    hxs      - Header examination sequence?
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_label_case2dj_action(_fmsp_assembler_parser_context_t *ctx_p,
                                           char                             *str1_p,
                                           char                             *str2_p,
                                           char                             *str3_p,
                                           bool                              hxs_1,
                                           bool                              hxs_2,
                                           bool                              hxs_3)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           or_str_p[]            = " | ";
   char                           hxs_1_not_set_str_p[] = " 0";
   char                           hxs_2_not_set_str_p[] = " (0 << 1)";
   char                           hxs_3_not_set_str_p[] = " (0 << 2)";
   char                           hxs_1_set_str_p[]     = " 1";
   char                           hxs_2_set_str_p[]     = " (1 << 1)";
   char                           hxs_3_set_str_p[]     = " (1 << 2)";
   char                          *hxs_1_str_p;
   char                          *hxs_2_str_p;
   char                          *hxs_3_str_p;
   uint32_t                       len;

   /* Set the number of immediate values this instruction will take. */
   ctx_p->num_immediate_values = 3;

   object_p = _fmsp_new_instruction(_fmsp_assembler_case2_dj_wr_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0001 0ABC <label 1> <label 2> <label 3>
    *
    * A - HXS for label 3
    * B - HXS for label 2
    * C - HXS for label 1
    */

   if (hxs_1)
   {
      hxs_1_str_p = hxs_1_set_str_p;
   }
   else
   {
      hxs_1_str_p = hxs_1_not_set_str_p;
   }

   if (hxs_2)
   {
      hxs_2_str_p = hxs_2_set_str_p;
   }
   else
   {
      hxs_2_str_p = hxs_2_not_set_str_p;
   }

   if (hxs_3)
   {
      hxs_3_str_p = hxs_3_set_str_p;
   }
   else
   {
      hxs_3_str_p = hxs_3_not_set_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_CASE2_DJ_WR_TO_WR_STR) +
         strlen(or_str_p)                               +
         strlen(hxs_1_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_2_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_3_str_p)                            +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s%s%s%s",
            _FMSP_INSTR_CODE_CASE2_DJ_WR_TO_WR_STR,
            or_str_p,
            hxs_3_str_p,
            or_str_p,
            hxs_2_str_p,
            or_str_p,
            hxs_1_str_p
           );

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_CASE2_DJ_WR_TO_WR |
                             (hxs_3 << 2)                       |
                             (hxs_2 << 1)                       |
                             hxs_1;

   object_p->hxs_1              = hxs_1;
   object_p->hxs_2              = hxs_2;
   object_p->hxs_3              = hxs_3;
   object_p->num_valid_hw_words = 4;

   object_p->jump_label1_p = strdup(str1_p);

   if (object_p->jump_label1_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label2_p = strdup(str2_p);

   if (object_p->jump_label2_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label3_p = strdup(str3_p);

   if (object_p->jump_label3_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for case 3 default continue
 *
 * Parameters:
 *    ctx_p    - Parse context
 *    str1_p   - String containing label 1 name.
 *    str2_p   - String containing label 2 name.
 *    str3_p   - String containing label 3 name.
 *    hxs      - Header examination sequence?
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_label_case3dc_action(_fmsp_assembler_parser_context_t *ctx_p,
                                           char                             *str1_p,
                                           char                             *str2_p,
                                           char                             *str3_p,
                                           bool                              hxs_1,
                                           bool                              hxs_2,
                                           bool                              hxs_3)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           or_str_p[]            = " | ";
   char                           hxs_1_not_set_str_p[] = " 0";
   char                           hxs_2_not_set_str_p[] = " (0 << 1)";
   char                           hxs_3_not_set_str_p[] = " (0 << 2)";
   char                           hxs_1_set_str_p[]     = " 1";
   char                           hxs_2_set_str_p[]     = " (1 << 1)";
   char                           hxs_3_set_str_p[]     = " (1 << 2)";
   char                          *hxs_1_str_p;
   char                          *hxs_2_str_p;
   char                          *hxs_3_str_p;
   uint32_t                       len;

   /* Set the number of immediate values this instruction will take. */
   ctx_p->num_immediate_values = 3;

   object_p = _fmsp_new_instruction(_fmsp_assembler_case3_dc_wr_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0001 1ABC <label 1> <label 2> <label 3>
    *
    * A - HXS for label 3
    * B - HXS for label 2
    * C - HXS for label 1
    */

   if (hxs_1)
   {
      hxs_1_str_p = hxs_1_set_str_p;
   }
   else
   {
      hxs_1_str_p = hxs_1_not_set_str_p;
   }

   if (hxs_2)
   {
      hxs_2_str_p = hxs_2_set_str_p;
   }
   else
   {
      hxs_2_str_p = hxs_2_not_set_str_p;
   }

   if (hxs_3)
   {
      hxs_3_str_p = hxs_3_set_str_p;
   }
   else
   {
      hxs_3_str_p = hxs_3_not_set_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_CASE3_DC_WR_TO_WR_STR) +
         strlen(or_str_p)                               +
         strlen(hxs_1_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_2_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_3_str_p)                            +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s%s%s%s",
            _FMSP_INSTR_CODE_CASE3_DC_WR_TO_WR_STR,
            or_str_p,
            hxs_3_str_p,
            or_str_p,
            hxs_2_str_p,
            or_str_p,
            hxs_1_str_p
           );

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_CASE3_DC_WR_TO_WR |
                             (hxs_3 << 2)                       |
                             (hxs_2 << 1)                       |
                             hxs_1;

   object_p->hxs_1              = hxs_1;
   object_p->hxs_2              = hxs_2;
   object_p->hxs_3              = hxs_3;
   object_p->num_valid_hw_words = 4;

   object_p->jump_label1_p = strdup(str1_p);

   if (object_p->jump_label1_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label2_p = strdup(str2_p);

   if (object_p->jump_label2_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label3_p = strdup(str3_p);

   if (object_p->jump_label3_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for case 3 default jump
 *
 * Parameters:
 *    ctx_p    - Parse context
 *    str1_p   - String containing label 1 name.
 *    str2_p   - String containing label 2 name.
 *    str3_p   - String containing label 3 name.
 *    hxs      - Header examination sequence?
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_label_case3dj_action(_fmsp_assembler_parser_context_t *ctx_p,
                                           char                             *str1_p,
                                           char                             *str2_p,
                                           char                             *str3_p,
                                           char                             *str4_p,
                                           bool                              hxs_1,
                                           bool                              hxs_2,
                                           bool                              hxs_3,
                                           bool                              hxs_4)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           or_str_p[]            = " | ";
   char                           hxs_1_not_set_str_p[] = " 0";
   char                           hxs_2_not_set_str_p[] = " (0 << 1)";
   char                           hxs_3_not_set_str_p[] = " (0 << 2)";
   char                           hxs_4_not_set_str_p[] = " (0 << 3)";
   char                           hxs_1_set_str_p[]     = " 1";
   char                           hxs_2_set_str_p[]     = " (1 << 1)";
   char                           hxs_3_set_str_p[]     = " (1 << 2)";
   char                           hxs_4_set_str_p[]     = " (1 << 3)";
   char                          *hxs_1_str_p;
   char                          *hxs_2_str_p;
   char                          *hxs_3_str_p;
   char                          *hxs_4_str_p;
   uint32_t                       len;

   /* Set the number of immediate values this instruction will take. */
   ctx_p->num_immediate_values = 4;

   object_p = _fmsp_new_instruction(_fmsp_assembler_case3_dj_wr_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0010 ABCD <label 1> <label 2> <label 3> <label 4>
    *
    * A - HXS for label 4
    * B - HXS for label 3
    * C - HXS for label 2
    * D - HXS for label 1
    */

   if (hxs_1)
   {
      hxs_1_str_p = hxs_1_set_str_p;
   }
   else
   {
      hxs_1_str_p = hxs_1_not_set_str_p;
   }

   if (hxs_2)
   {
      hxs_2_str_p = hxs_2_set_str_p;
   }
   else
   {
      hxs_2_str_p = hxs_2_not_set_str_p;
   }

   if (hxs_3)
   {
      hxs_3_str_p = hxs_3_set_str_p;
   }
   else
   {
      hxs_3_str_p = hxs_3_not_set_str_p;
   }

   if (hxs_4)
   {
      hxs_4_str_p = hxs_4_set_str_p;
   }
   else
   {
      hxs_4_str_p = hxs_4_not_set_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_CASE3_DJ_WR_TO_WR_STR) +
         strlen(or_str_p)                               +
         strlen(hxs_1_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_2_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_3_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_4_str_p)                            +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s%s%s%s%s%s",
            _FMSP_INSTR_CODE_CASE3_DJ_WR_TO_WR_STR,
            or_str_p,
            hxs_4_str_p,
            or_str_p,
            hxs_3_str_p,
            or_str_p,
            hxs_2_str_p,
            or_str_p,
            hxs_1_str_p
           );

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_CASE3_DJ_WR_TO_WR |
                             (hxs_4 << 3)                       |
                             (hxs_3 << 2)                       |
                             (hxs_2 << 1)                       |
                             hxs_1;

   object_p->hxs_1              = hxs_1;
   object_p->hxs_2              = hxs_2;
   object_p->hxs_3              = hxs_3;
   object_p->hxs_4              = hxs_4;
   object_p->num_valid_hw_words = 5;

   object_p->jump_label1_p = strdup(str1_p);

   if (object_p->jump_label1_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label2_p = strdup(str2_p);

   if (object_p->jump_label2_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label3_p = strdup(str3_p);

   if (object_p->jump_label3_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label4_p = strdup(str4_p);

   if (object_p->jump_label4_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for case 4 default continue
 *
 * Parameters:
 *    ctx_p    - Parse context
 *    str1_p   - String containing label 1 name.
 *    str2_p   - String containing label 2 name.
 *    str3_p   - String containing label 3 name.
 *    hxs      - Header examination sequence?
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_label_case4dc_action(_fmsp_assembler_parser_context_t *ctx_p,
                                           char                             *str1_p,
                                           char                             *str2_p,
                                           char                             *str3_p,
                                           char                             *str4_p,
                                           bool                              hxs_1,
                                           bool                              hxs_2,
                                           bool                              hxs_3,
                                           bool                              hxs_4)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           or_str_p[]            = " | ";
   char                           hxs_1_not_set_str_p[] = " 0";
   char                           hxs_2_not_set_str_p[] = " (0 << 1)";
   char                           hxs_3_not_set_str_p[] = " (0 << 2)";
   char                           hxs_4_not_set_str_p[] = " (0 << 3)";
   char                           hxs_1_set_str_p[]     = " 1";
   char                           hxs_2_set_str_p[]     = " (1 << 1)";
   char                           hxs_3_set_str_p[]     = " (1 << 2)";
   char                           hxs_4_set_str_p[]     = " (1 << 3)";
   char                          *hxs_1_str_p;
   char                          *hxs_2_str_p;
   char                          *hxs_3_str_p;
   char                          *hxs_4_str_p;
   uint32_t                       len;

   /* Set the number of immediate values this instruction will take. */
   ctx_p->num_immediate_values = 4;

   object_p = _fmsp_new_instruction(_fmsp_assembler_case4_dc_wr_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0011 ABCD <label 1> <label 2> <label 3> <label 4>
    *
    * A - HXS for label 4
    * B - HXS for label 3
    * C - HXS for label 2
    * D - HXS for label 1
    */

   if (hxs_1)
   {
      hxs_1_str_p = hxs_1_set_str_p;
   }
   else
   {
      hxs_1_str_p = hxs_1_not_set_str_p;
   }

   if (hxs_2)
   {
      hxs_2_str_p = hxs_2_set_str_p;
   }
   else
   {
      hxs_2_str_p = hxs_2_not_set_str_p;
   }

   if (hxs_3)
   {
      hxs_3_str_p = hxs_3_set_str_p;
   }
   else
   {
      hxs_3_str_p = hxs_3_not_set_str_p;
   }

   if (hxs_4)
   {
      hxs_4_str_p = hxs_4_set_str_p;
   }
   else
   {
      hxs_4_str_p = hxs_4_not_set_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_CASE4_DC_WR_TO_WR_STR) +
         strlen(or_str_p)                               +
         strlen(hxs_1_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_2_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_3_str_p)                            +
         strlen(or_str_p)                               +
         strlen(hxs_4_str_p)                            +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s%s%s%s%s%s",
            _FMSP_INSTR_CODE_CASE4_DC_WR_TO_WR_STR,
            or_str_p,
            hxs_4_str_p,
            or_str_p,
            hxs_3_str_p,
            or_str_p,
            hxs_2_str_p,
            or_str_p,
            hxs_1_str_p
           );

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_CASE4_DC_WR_TO_WR |
                             (hxs_4 << 3)                       |
                             (hxs_3 << 2)                       |
                             (hxs_2 << 1)                       |
                             hxs_1;

   object_p->hxs_1              = hxs_1;
   object_p->hxs_2              = hxs_2;
   object_p->hxs_3              = hxs_3;
   object_p->hxs_4              = hxs_4;
   object_p->num_valid_hw_words = 5;

   object_p->jump_label1_p = strdup(str1_p);

   if (object_p->jump_label1_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label2_p = strdup(str2_p);

   if (object_p->jump_label2_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label3_p = strdup(str3_p);

   if (object_p->jump_label3_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   object_p->jump_label4_p = strdup(str4_p);

   if (object_p->jump_label4_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate label object.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for nop
 *
 * Parameters:
 *    ctx_p - Parse context
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_nop_action(_fmsp_assembler_parser_context_t *ctx_p)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;

   /* No immediate values */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_nop_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   len = strlen(_FMSP_INSTR_CODE_NOP_STR) + 1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s",
            _FMSP_INSTR_CODE_NOP_STR);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_NOP;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for clm
 *
 * Parameters:
 *    ctx_p - Parse context
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_clm_action(_fmsp_assembler_parser_context_t *ctx_p)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;

   /* No immediate values */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_clm_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   len = strlen(_FMSP_INSTR_CODE_CLM_STR) + 1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s",
            _FMSP_INSTR_CODE_CLM_STR);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_CLM;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for Advance Header Base by Window Offset
 *
 * Parameters:
 *    ctx_p - Parse context
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_advance_hb_by_wo_action(_fmsp_assembler_parser_context_t *ctx_p)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;

   /* No immediate values */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_advance_hb_by_wo_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   len = strlen(_FMSP_INSTR_CODE_ADVANCE_HB_BY_WO_STR) + 1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s",
            _FMSP_INSTR_CODE_ADVANCE_HB_BY_WO_STR);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_ADVANCE_HB_BY_WO;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for set LCV bits.
 *
 * Parameters:
 *    ctx_p - Parse context
 *    str_p - String containing bit mask string.
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_set_lcv_bits_action(_fmsp_assembler_parser_context_t *ctx_p)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           start_lsb_str_p[] = "(PE_H2NL(";
   char                           end_lsb_str_p[]   = ")) & 0xffff";
   char                           start_msb_str_p[] = "((PE_H2NL(";
   char                           end_msb_str_p[]   = ")) >> 16) & 0xffff";
   uint32_t                       len;
   uint32_t                       immediate_value;

   /* Always 2 immediate value words */
   ctx_p->num_immediate_values = 2;

   object_p = _fmsp_new_instruction(_fmsp_assembler_or_iv_lcv_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* The bit mask string must be used to create both immediate value words
    * which are part of this instruction.  The words are in little endian.
    *
    * Example:
    *   ctx_p->value_str_p = "l3_row_id | l4_row_id"
    *
    *   =>
    *
    *   1st word after instruction = (l3_row_id | l4_row_id) & 0xffff
    *   2nd word after instruction = ((l3_row_id | l4_row_id) >> 16) & 0xffff
    */

   /* Instruction word */
   len = strlen(_FMSP_INSTR_CODE_OR_IV_LCV_STR) + 1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s",
            _FMSP_INSTR_CODE_OR_IV_LCV_STR);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_OR_IV_LCV;

   /* Attempt to convert immediate value string to a number. */
   errno = 0;
   immediate_value = (uint32_t)strtoll(ctx_p->value_str_p, NULL, 0);
   
   if (errno == 0)
   {
      object_p->hw_words_p[1] = immediate_value & 0xffff;
      object_p->hw_words_p[2] = (immediate_value >> 16) & 0xffff;
   }
   else
   {
      // TODO
      // Variable substitution must be done.
   }

   /* Immediate value word 1 */
   len = strlen(start_lsb_str_p)    +
         strlen(ctx_p->value_str_p) +
         strlen(end_lsb_str_p)      +
         1;

   object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[1] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");

      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[1],
            len,
            "%s%s%s",
            start_lsb_str_p,
            ctx_p->value_str_p,
            end_lsb_str_p);

   /* Immediate value word 2 */
   len = strlen(start_msb_str_p)    +
         strlen(ctx_p->value_str_p) +
         strlen(end_msb_str_p)      +
         1;

   object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[2] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");

      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[2],
            len,
            "%s%s%s",
            start_msb_str_p,
            ctx_p->value_str_p,
            end_msb_str_p);

   object_p->num_valid_hw_words = 3;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for result array range
 *
 * Parameters:
 *    ctx_p     - Parse context
 *    start_idx - Start index of result array range.
 *    end_idx   - End index of result array range.
 *
 * Returns:
 *   string - NULL means error, otherwise returns a string of form xxx:yyy
 * ---------------------------------------------------------------------------*/
static bool _fmsp_result_array_range_action(_fmsp_assembler_parser_context_t *ctx_p,
                                            int32_t                           start_idx,
                                            int32_t                           end_idx,
                                            bool                              is_gprv)
{
   if (is_gprv)
   {
      if (start_idx > 15)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "General purpose register start index out of bounds");

         return (false);
      }

      if (end_idx > 15)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "General purpose register end index out of bounds");

         return (false);
      }
   }

   if (start_idx < 0)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Result Array start index out of bounds");

      return (false);
   }

   if (end_idx < 0)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Result Array end index out of bounds");

      return (false);
   }

   if (start_idx > MAX_RESULT_ARRAY_BYTE_INDEX)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Result Array start index out of bounds");

      return (false);
   }

   if (end_idx   > MAX_RESULT_ARRAY_BYTE_INDEX)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Result Array end index out of bounds");

      return (false);
   }

   if (start_idx > end_idx)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Result Array start index > end index");

      return (false);
   }

   if (end_idx - start_idx > 7)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Result Array (end index - start index) > 7");

      return (false);
   }

   ctx_p->range_start = start_idx;
   ctx_p->range_end   = end_idx;

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for load bytes from result array to working register.
 *
 * Parameters:
 *    ctx_p      - Parse context.
 *    wr         - Working register.
 *    is_assign  - Boolean value that is true if assign, false if concat.
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_load_bytes_ra_to_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                             uint8_t                           wr,
                                             bool                              is_assign)
{
   char                           wr0_str_p[]           = " | 0";
   char                           wr1_str_p[]           = " | 1";
   char                          *wr_str_p;
   char                           assign_str_p[]        = " | 0x0000";
   char                           concat_str_p[]        = " | 0x0800";
   char                          *funcion_str_p;
   char                           range_end_str_p[]     = " | (0x0000 << 4)";
   char                           num_bytes_str_p[]     = " | (0x0000 << 1)";
   uint32_t                       len;
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       shift = 0;

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_load_bytes_ra_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0011 SJJJ JJJJ KKKW
    *
    * S - Shift
    * J - End of register range
    * K - Number of bytes - 1 to load
    * W - Working register (0 or 1)
    */

   /* Set working register */
   if (wr == 0)
   {
      object_p->working_register = 0;
      wr_str_p = wr0_str_p;
   }
   else
   {
      object_p->working_register = 1;
      wr_str_p = wr1_str_p;
   }

   /* Set function */
   if (is_assign)
   {
      object_p->is_assign = true;
      funcion_str_p = assign_str_p;
      shift = 0;
   }
   else
   {
      object_p->is_assign = false;
      funcion_str_p = concat_str_p;
      shift = 1;
   }

   /* Set range */
   object_p->range_start = ctx_p->range_start;
   object_p->range_end   = ctx_p->range_end;

   len = strlen(range_end_str_p) + 1;

   /* Range end */
   snprintf(range_end_str_p,
            len,
            " | (0x%04x << 4)",
            object_p->range_end);

   len = strlen(num_bytes_str_p) + 1;

   /* Num bytes - 1 */
   snprintf(num_bytes_str_p,
            len,
            " | (0x%04x << 1)",
            (object_p->range_end - object_p->range_start));

   len = strlen(_FMSP_INSTR_CODE_LOAD_BYTES_RA_TO_WR_STR) +
         strlen(funcion_str_p)                            +
         strlen(range_end_str_p)                          +
         strlen(num_bytes_str_p)                          +
         strlen(wr_str_p)                                 +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");

      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s%s",
            _FMSP_INSTR_CODE_LOAD_BYTES_RA_TO_WR_STR,
            funcion_str_p,
            range_end_str_p,
            num_bytes_str_p,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_LOAD_BYTES_RA_TO_WR           |
                             (shift << 11)                                  |
                             (ctx_p->range_end << 4)                        |
                             ((ctx_p->range_end - ctx_p->range_start) << 1) |
                             wr;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for store working register to result array.
 *
 * Parameters:
 *    ctx_p - Parse context.
 *    wr    - Working register.
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_store_wr_to_ra_action(_fmsp_assembler_parser_context_t *ctx_p,
                                        uint8_t                           wr)
{
   char                           wr0_str_p[]           = " | 0";
   char                           wr1_str_p[]           = " | 1";
   char                          *wr_str_p;
   char                           range_end_str_p[]     = " | (0x0000 << 1)";
   char                           num_bytes_str_p[]     = " | (0x0000 << 8)";
   uint32_t                       len;
   _fmsp_assembler_instruction_t *object_p;

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_store_wr_to_ra_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0010 1SSS TTTT TTTW
    *
    * S - Number of bytes - 1 to store
    * T - End of register range
    * W - Working register (0 or 1)
    */

   /* Set working register */
   if (wr == 0)
   {
      object_p->working_register = 0;
      wr_str_p = wr0_str_p;
   }
   else
   {
      object_p->working_register = 1;
      wr_str_p = wr1_str_p;
   }

   /* Set range */
   object_p->range_start = ctx_p->range_start;
   object_p->range_end   = ctx_p->range_end;

   len = strlen(range_end_str_p) + 1;

   /* Range end */
   snprintf(range_end_str_p,
            len,
            " | (0x%04x << 1)",
            object_p->range_end);

   len = strlen(num_bytes_str_p) + 1;

   /* Num bytes - 1 */
   snprintf(num_bytes_str_p,
            len,
            " | (0x%04x << 8)",
            (object_p->range_end - object_p->range_start));

   len = strlen(_FMSP_INSTR_CODE_STORE_WR_TO_RA_STR) +
         strlen(num_bytes_str_p)                     +
         strlen(range_end_str_p)                     +
         strlen(wr_str_p)                            +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");

      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s",
            _FMSP_INSTR_CODE_STORE_WR_TO_RA_STR,
            num_bytes_str_p,
            range_end_str_p,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_STORE_WR_TO_RA                |
                             ((ctx_p->range_end - ctx_p->range_start) << 8) |
                             (ctx_p->range_end << 1)                        |
                             wr;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for store immediate value to result array.
 *
 * Parameters:
 *    ctx_p      - Parse context.
 *    iv_p       - String containing immediate value
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_store_iv_to_ra_action(_fmsp_assembler_parser_context_t *ctx_p,
                                        char                             *iv_p)
{
   char                           or_str_p[]           = " | ";
   char                           start_shift_str_p[]  = " | (";
   char                           end_shift_7_str_p[] = " << 7)";
   uint32_t                       len;
   uint64_t                       immediate_value      = 0;
   uint16_t                       word_1;
   uint16_t                       word_2;
   uint16_t                       word_3;
   uint16_t                       word_4;
   uint8_t                        iv_word_size         = 0;
   uint8_t                        num_iv_words         = 0;
   uint8_t                        num_iv_bytes         = 0;

   _fmsp_assembler_instruction_t *object_p;

   num_iv_bytes = ctx_p->range_end - ctx_p->range_start + 1;

   switch (num_iv_bytes)
   {
      case 1:
      case 2:
         num_iv_words = 1;
         ctx_p->num_immediate_values = 1;

         break;

      case 3:
      case 4:
         num_iv_words = 2;
         ctx_p->num_immediate_values = 2;
         break;

      case 5:
      case 6:
         num_iv_words = 3;
         ctx_p->num_immediate_values = 3;
         break;

      case 7:
      case 8:
         num_iv_words = 4;
         ctx_p->num_immediate_values = 4;
         break;

      default:
         _fmsp_assembler_yyerror(ctx_p,
                                 "INTERNAL ERROR: Default case in _fmsp_store_iv_to_ra_action");
         return (false);
         break;
   }

   object_p = _fmsp_new_instruction(_fmsp_assembler_store_iv_to_ra_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Set range */
   object_p->range_start = ctx_p->range_start;
   object_p->range_end   = ctx_p->range_end;

   /* Check if the immediate value starts with "0x". If it does then
    * convert the value and fill in the appropriate number of immediate
    * value words based on the range requested.
    * Otherwise, this is a plan id enum.
    */
   if (strncmp(iv_p, "0x", 2) == 0)
   {
      immediate_value = (uint64_t)strtoull(iv_p, NULL, 0);

      len = 6 + /* 0xNNNN */
            1;

      word_1 =  immediate_value        & 0xffff;
      word_2 = (immediate_value >> 16) & 0xffff;
      word_3 = (immediate_value >> 32) & 0xffff;
      word_4 = (immediate_value >> 48) & 0xffff;

      if (num_iv_words == 1)
      {
         object_p->num_valid_hw_words = 2;
         iv_word_size                 = 0;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         object_p->hw_words_p[1] = word_1;
      }
      else if (num_iv_words == 2)
      {
         object_p->num_valid_hw_words = 3;
         iv_word_size                 = 1;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL ||
             object_p->hw_words_str_p[2] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x%04x",
                  word_2);

         object_p->hw_words_p[1] = word_1;
         object_p->hw_words_p[2] = word_2;
      }
      else if (num_iv_words == 3)
      {
         object_p->num_valid_hw_words = 4;
         iv_word_size                 = 2;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[3] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL ||
             object_p->hw_words_str_p[2] == NULL ||
             object_p->hw_words_str_p[3] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x%04x",
                  word_2);

         snprintf(object_p->hw_words_str_p[3],
                  len,
                  "0x%04x",
                  word_3);

         object_p->hw_words_p[1] = word_1;
         object_p->hw_words_p[2] = word_2;
         object_p->hw_words_p[3] = word_3;
      }
      else
      {
         object_p->num_valid_hw_words = 5;
         iv_word_size                 = 3;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[3] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[4] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL ||
             object_p->hw_words_str_p[2] == NULL ||
             object_p->hw_words_str_p[3] == NULL ||
             object_p->hw_words_str_p[4] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x%04x",
                  word_2);

         snprintf(object_p->hw_words_str_p[3],
                  len,
                  "0x%04x",
                  word_3);

         snprintf(object_p->hw_words_str_p[4],
                  len,
                  "0x%04x",
                  word_4);

         object_p->hw_words_p[1] = word_1;
         object_p->hw_words_p[2] = word_2;
         object_p->hw_words_p[3] = word_3;
         object_p->hw_words_p[4] = word_4;
      }
   }
   else
   {
      /* There will always be 1 word with the enum in it.
       * All subsequent words will be 0x0000.
       */
      len = strlen(iv_p) + 1;

      object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);

      if (object_p->hw_words_str_p[1] == NULL)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to allocate string.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      snprintf(object_p->hw_words_str_p[1],
               len,
               "%s",
               iv_p);

      object_p->hw_words_p[1] = 0;

      if (num_iv_words == 1)
      {
         object_p->num_valid_hw_words = 2;
      }
      else if (num_iv_words == 2)
      {
         len = 6 + /* 0xNNNN */
               1;

         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[2] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x0000");

         object_p->hw_words_p[1] = 0;
         object_p->hw_words_p[2] = 0;

         object_p->num_valid_hw_words = 3;
      }
      else if (num_iv_words == 3)
      {
         len = 6 + /* 0xNNNN */
               1;

         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[3] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[2] == NULL ||
             object_p->hw_words_str_p[3] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x0000");

         snprintf(object_p->hw_words_str_p[3],
                  len,
                  "0x0000");

         object_p->hw_words_p[1] = 0;
         object_p->hw_words_p[2] = 0;
         object_p->hw_words_p[3] = 0;

         object_p->num_valid_hw_words = 4;
      }
      else
      {
         len = 6 + /* 0xNNNN */
               1;

         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[3] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[4] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[2] == NULL ||
             object_p->hw_words_str_p[3] == NULL ||
             object_p->hw_words_str_p[4] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x0000");

         snprintf(object_p->hw_words_str_p[3],
                  len,
                  "0x0000");

         snprintf(object_p->hw_words_str_p[4],
                  len,
                  "0x0000");

         object_p->hw_words_p[1] = 0;
         object_p->hw_words_p[2] = 0;
         object_p->hw_words_p[3] = 0;
         object_p->hw_words_p[4] = 0;

         object_p->num_valid_hw_words = 5;
      }
   }

   /* Bit layout of this instruction:
    *
    * 0000 10SS STTT TTTT
    *
    * S - Number of bytes - 1 to store
    * T - End of register range
    */
   len = strlen(_FMSP_INSTR_CODE_STORE_IV_TO_RA_STR) +
         strlen(start_shift_str_p)                   +
         6                                           + /* 0xNNNN */
         strlen(end_shift_7_str_p)                   +
         strlen(or_str_p)                            +
         6                                           + /* 0xNNNN */
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s0x%04x%s%s0x%04x",
            _FMSP_INSTR_CODE_STORE_IV_TO_RA_STR,
            start_shift_str_p,
            num_iv_bytes - 1,
            end_shift_7_str_p,
            or_str_p,
            object_p->range_end
           );

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_STORE_IV_TO_RA |
                             ((num_iv_bytes - 1) << 7)       |
                             object_p->range_end;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for load specific value to window offset
 *
 * Parameters:
 *    ctx_p      - Parse context.
 *    str_p      - String containing hex value.
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_load_sv_to_wo_action(_fmsp_assembler_parser_context_t *ctx_p,
                                       char                             *str_p)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           or_str_p[] = " | ";
   uint32_t                       len;
   uint32_t                       specific_value;

   specific_value = strtol(str_p, NULL, 0);

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_load_sv_to_wo_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0110 VVVV VVVV
    *
    * V - Specific value.
    */

   len = strlen(_FMSP_INSTR_CODE_LOAD_SV_TO_WO_STR) +
         strlen(or_str_p)                           +
         strlen(str_p)                              +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s",
            _FMSP_INSTR_CODE_LOAD_SV_TO_WO_STR,
            or_str_p,
            str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_LOAD_SV_TO_WO |
                             specific_value;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for add specific value to window offset
 *
 * Parameters:
 *    ctx_p      - Parse context.
 *    str_p      - String containing hex value.
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_add_sv_to_wo_action(_fmsp_assembler_parser_context_t *ctx_p,
                                      char                             *str_p)
{
   char                           or_str_p[] = " | ";
   uint32_t                       len;
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       specific_value;

   specific_value = strtol(str_p, NULL, 0);


   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_add_sv_to_wo_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0111 VVVV VVVV
    *
    * V - Specific value.
    */
   len = strlen(_FMSP_INSTR_CODE_ADD_SV_TO_WO_STR) +
         strlen(or_str_p)                          +
         strlen(str_p)                             +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s",
            _FMSP_INSTR_CODE_ADD_SV_TO_WO_STR,
            or_str_p,
            str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_ADD_SV_TO_WO |
                             specific_value;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for modifying window offset by a working register
 *
 * Parameters:
 *    ctx_p      - Parse context.
 *    add        - True for add, false for set
 *    wr         - Working register number (0/1)
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_modify_wo_by_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                         bool                              add,
                                         uint8_t                           wr)
{
   char                           wr0_str_p[] = " | 0x0";
   char                           wr1_str_p[] = " | 0x1";
   char                          *wr_str_p;
   char                           add_str_p[] = " | 0x2";
   char                           set_str_p[] = " | 0x0";
   char                          *mod_str_p;
   uint32_t                       len;
   _fmsp_assembler_instruction_t *object_p;

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_modify_wo_by_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   if (add)
   {
      mod_str_p = add_str_p;
   }
   else
   {
      mod_str_p = set_str_p;
   }

   if (wr == 0)
   {
      wr_str_p = wr0_str_p;
   }
   else
   {
      wr_str_p = wr1_str_p;
   }

   object_p->working_register = wr;

   /* Bit layout of this instruction:
    *
    * 0000 0000 1000 00AW
    *
    * A - Add or set
    * W - Working register
    */
   len = strlen(_FMSP_INSTR_CODE_MODIFY_WO_BY_WR_STR) +
         strlen(mod_str_p)                            +
         strlen(wr_str_p)                             +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s",
            _FMSP_INSTR_CODE_MODIFY_WO_BY_WR_STR,
            mod_str_p,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_MODIFY_WO_BY_WR |
                             (add << 1)                       |
                             wr;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for clear working register.
 *
 * Parameters:
 *    ctx_p - Parse context.
 *    wr    - Working register.
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_zero_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                 uint8_t                           wr)
{
   _fmsp_assembler_instruction_t *object_p;
   char                          *wr_str_p;
   char                           wr_0_str_p[] = " | 0x0";
   char                           wr_1_str_p[] = " | 0x1";
   uint32_t                       len;

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_zero_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0000 010W
    *
    * W - Working register (0 or 1)
    */

   if (wr == 0)
   {
      wr_str_p = wr_0_str_p;
   }
   else
   {
      wr_str_p = wr_1_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_ZERO_WR_STR) +
         strlen(wr_str_p)                     +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s",
            _FMSP_INSTR_CODE_ZERO_WR_STR,
            wr_str_p);

   object_p->working_register = wr;

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_ZERO_WR | wr;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for onescomp
 *
 * Parameters:
 *    ctx_p - Parse context
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_ones_comp_wr1_to_wr0_action(_fmsp_assembler_parser_context_t *ctx_p)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_ones_comp_wr1_to_wr0_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   len = strlen(_FMSP_INSTR_CODE_ONES_COMP_WR1_TO_WR0_STR) + 1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s",
            _FMSP_INSTR_CODE_ONES_COMP_WR1_TO_WR0_STR);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_ONES_COMP_WR1_TO_WR0;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for bitwise working register to working register operation
 *
 * Parameters:
 *    ctx_p    - Parse context
 *    wr       - Working register to place result in.
 *    function - Function to be used
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_bitwise_wr_wr_to_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                             uint8_t                           wr,
                                             uint8_t                           function)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;
   char                           or_str_p[]          = " | ";
   char                          *wr_str_p;
   char                           wr_0_str_p[]        = " | 0x0";
   char                           wr_1_str_p[]        = " | 0x1";
   char                           start_shift_str_p[] = " (";
   char                           end_shift_1_str_p[] = " << 1)";

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_bitwise_wr_wr_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0111 0FFW
    *
    * F - Function (AND/OR/XOR)
    * W - Working register (0 or 1)
    */

   if (wr == 0)
   {
      wr_str_p = wr_0_str_p;
   }
   else
   {
      wr_str_p = wr_1_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_BITWISE_WR_WR_TO_WR_STR) +
         strlen(or_str_p)                                 +
         strlen(start_shift_str_p)                        +
         6                                                + /* 0xNNNN */
         strlen(end_shift_1_str_p)                        +
         strlen(wr_str_p)                                 +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s0x%04x%s%s",
            _FMSP_INSTR_CODE_BITWISE_WR_WR_TO_WR_STR,
            or_str_p,
            start_shift_str_p,
            function,
            end_shift_1_str_p,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_BITWISE_WR_WR_TO_WR |
                             (function << 1)                      |
                             wr;

   object_p->working_register = wr;
   object_p->function         = function;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for bitwise immediate value to working register operation
 *
 * Parameters:
 *    ctx_p     - Parse context
 *    target_wr - Working register to place result in
 *    source_wr - Working register to operate on with immediate value
 *    function  - Function to be used
 *    iv_p      - Immediate value
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_bitwise_wr_iv_to_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                             uint8_t                           target_wr,
                                             uint8_t                           source_wr,
                                             uint8_t                           function,
                                             char                             *iv_p)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           start_shift_str_p[] = " | (";
   char                           end_shift_3_str_p[] = " << 3)";
   char                           end_shift_1_str_p[] = " << 1)";
   char                           tgt_wr_0_str_p[]    = " | (0x0 << 5)";
   char                           tgt_wr_1_str_p[]    = " | (0x1 << 5)";
   char                           src_wr_0_str_p[]    = " | 0x0";
   char                           src_wr_1_str_p[]    = " | 0x1";
   char                          *src_wr_str_p;
   char                          *tgt_wr_str_p;
   uint32_t                       len;
   uint64_t                       immediate_value      = 0;
   uint16_t                       word_1;
   uint16_t                       word_2;
   uint16_t                       word_3;
   uint16_t                       word_4;
   uint8_t                        iv_word_size         = 0;

   if (source_wr == 0)
   {
      src_wr_str_p = src_wr_0_str_p;
   }
   else
   {
      src_wr_str_p = src_wr_1_str_p;
   }

   if (target_wr == 0)
   {
      tgt_wr_str_p = tgt_wr_0_str_p;
   }
   else
   {
      tgt_wr_str_p = tgt_wr_1_str_p;
   }

   /* This instruction is always 4 immediate value words */
   ctx_p->num_immediate_values   = 4;

   object_p = _fmsp_new_instruction(_fmsp_assembler_bitwise_wr_iv_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Assume immediate value is always 64 bits as the user
    * will be performing the operation over the whole register.
    * This means all immediate values are padded on the left with 0
    * and the function is performed on the entire source WR. */
   object_p->num_valid_hw_words  = 5;
   iv_word_size                  = 3;

   /* Check if the immediate value starts with "0x". If it does then
    * convert the value. Otherwise, this is a plan id enum.
    */
   if (strncmp(iv_p, "0x", 2) == 0)
   {
      errno = 0;

      immediate_value = (uint64_t)strtoull(iv_p, NULL, 0);

      if (errno != 0)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to convert immediate value.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      len = 6 + /* 0xNNNN */
            1;

      word_1 =  immediate_value        & 0xffff;
      word_2 = (immediate_value >> 16) & 0xffff;
      word_3 = (immediate_value >> 32) & 0xffff;
      word_4 = (immediate_value >> 48) & 0xffff;

      object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
      object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);
      object_p->hw_words_str_p[3] = (char *)calloc(len + 1, 1);
      object_p->hw_words_str_p[4] = (char *)calloc(len + 1, 1);

      object_p->hw_words_p[1] = word_1;
      object_p->hw_words_p[2] = word_2;
      object_p->hw_words_p[3] = word_3;
      object_p->hw_words_p[4] = word_4;

      if (object_p->hw_words_str_p[1] == NULL ||
          object_p->hw_words_str_p[2] == NULL ||
          object_p->hw_words_str_p[3] == NULL ||
          object_p->hw_words_str_p[4] == NULL)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to allocate string.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      snprintf(object_p->hw_words_str_p[1],
               len,
               "0x%04x",
               word_1);

      snprintf(object_p->hw_words_str_p[2],
               len,
               "0x%04x",
               word_2);

      snprintf(object_p->hw_words_str_p[3],
               len,
               "0x%04x",
               word_3);

      snprintf(object_p->hw_words_str_p[4],
               len,
               "0x%04x",
               word_4);
   }
   else
   {
      len = strlen(iv_p) + 1;

      object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);

      if (object_p->hw_words_str_p[1] == NULL)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to allocate string.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      snprintf(object_p->hw_words_str_p[1],
               len,
               "%s",
               iv_p);

      /* Pad other immediate value words with 0 */

      len = 6 + /* 0xNNNN */
            1;

      object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);
      object_p->hw_words_str_p[3] = (char *)calloc(len + 1, 1);
      object_p->hw_words_str_p[4] = (char *)calloc(len + 1, 1);

      /* HW opcodes */
      object_p->hw_words_p[1] = 0;
      object_p->hw_words_p[2] = 0;
      object_p->hw_words_p[3] = 0;
      object_p->hw_words_p[4] = 0;

      if (object_p->hw_words_str_p[1] == NULL ||
          object_p->hw_words_str_p[2] == NULL ||
          object_p->hw_words_str_p[3] == NULL)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to allocate string.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      snprintf(object_p->hw_words_str_p[2],
               len,
               "0x0000");

      snprintf(object_p->hw_words_str_p[3],
               len,
               "0x0000");

      snprintf(object_p->hw_words_str_p[4],
               len,
               "0x0000");
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 11VI IFFW
    *
    * V - Target working register (0 or 1)
    * I - Immediate value size
    * F - Function (OR/AND/XOR)
    * W - Source working register (0 or 1)
    */

   len = strlen(_FMSP_INSTR_CODE_BITWISE_WR_IV_TO_WR_STR) +
         strlen(tgt_wr_str_p)                             +
         strlen(start_shift_str_p)                        +
         6                                                + /* 0xNNNN */
         strlen(end_shift_3_str_p)                        +
         strlen(start_shift_str_p)                        +
         6                                                + /* 0xNNNN */
         strlen(end_shift_1_str_p)                        +
         strlen(src_wr_str_p)                             +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s0x%04x%s%s0x%04x%s%s",
            _FMSP_INSTR_CODE_BITWISE_WR_IV_TO_WR_STR,
            tgt_wr_str_p,
            start_shift_str_p,
            iv_word_size,
            end_shift_3_str_p,
            start_shift_str_p,
            function,
            end_shift_1_str_p,
            src_wr_str_p);

   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_BITWISE_WR_IV_TO_WR |
                             (target_wr << 5)                     |
                             (iv_word_size << 3)                  |
                             (function << 1)                      |
                             source_wr;

   object_p->target_working_register = target_wr;
   object_p->source_working_register = source_wr;
   object_p->function                = function;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));


   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for addsub working register to working register operation
 *
 * Parameters:
 *    ctx_p         - Parse context
 *    target        - Working register to place result
 *    left_operand  - Left operand of operation
 *    function      - Function to be used
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_addsub_wr_wr_to_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                            uint8_t                           target,
                                            uint8_t                           left_operand,
                                            uint8_t                           function)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;
   char                          *target_str_p;
   char                           target_0_str_p[]    = " | 0x0";
   char                           target_1_str_p[]    = " | 0x4";
   char                          *function_str_p;
   char                           add_str_p[]         = " | 0x0";
   char                           sub_str_p[]         = " | 0x2";
   char                          *left_op_str_p;
   char                           left_0_str_p[]      = " | 0x0";
   char                           left_1_str_p[]      = " | 0x1";

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_addsub_wr_wr_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0100 1VOL
    *
    * V - Target working register
    * O - Operation (0 add, 1 subtract)
    * L - Left operand (other working register is the right operand)
    */

   if (target == 0)
   {
      target_str_p = target_0_str_p;
   }
   else
   {
      target_str_p = target_1_str_p;
   }

   if (function == 0)
   {
      function_str_p = add_str_p;
   }
   else
   {
      function_str_p = sub_str_p;
   }

   if (left_operand == 0)
   {
      left_op_str_p = left_0_str_p;
   }
   else
   {
      left_op_str_p = left_1_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_ADDSUB_WR_WR_TO_WR_STR) +
         strlen(target_str_p)                            +
         strlen(function_str_p)                          +
         strlen(left_op_str_p)                           +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s",
            _FMSP_INSTR_CODE_ADDSUB_WR_WR_TO_WR_STR,
            target_str_p,
            function_str_p,
            left_op_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_ADDSUB_WR_WR_TO_WR |
                             (target << 2)                       |
                             (function << 1)                     |
                             left_operand;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for addsub immediate value to working register operation
 *
 * Parameters:
 *    ctx_p         - Parse context
 *    target        - Working register to place result
 *    source        - Working register to use as source operand
 *    function      - Function to be used
 *    iv_p          - Immediate value
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_addsub_wr_iv_to_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                            uint8_t                           target,
                                            uint8_t                           source,
                                            uint8_t                           function,
                                            char                             *iv_p)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;
   char                          *size_str_p;
   char                           size16_str_p[]      = " | 0x0";
   char                           size32_str_p[]      = " | 0x8";
   char                          *target_str_p;
   char                           target_0_str_p[]    = " | 0x0";
   char                           target_1_str_p[]    = " | 0x4";
   char                          *function_str_p;
   char                           add_str_p[]         = " | 0x0";
   char                           sub_str_p[]         = " | 0x2";
   char                          *source_str_p;
   char                           source_0_str_p[]    = " | 0x0";
   char                           source_1_str_p[]    = " | 0x1";
   uint64_t                       immediate_value     = 0;
   uint16_t                       word_1;
   uint16_t                       word_2;
   uint32_t                       size = 0;

   /* Check if the immediate value starts with "0x". If it does then
    * convert the value and base the number of immediate value words
    * on that.  Otherwise, this is a plan id enum which is 1 byte so
    * only use 1 immediate value word.
    */
   if (strncmp(iv_p, "0x", 2) == 0)
   {
      errno = 0;
   
      immediate_value = (uint64_t)strtoull(iv_p, NULL, 0);

      if (errno != 0)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to convert immediate value.");
         return (false);
      }

      if (immediate_value < 0x10000)
      {
         size_str_p = size16_str_p;
         ctx_p->num_immediate_values = 1;
         size = 0;
      }
      else if (immediate_value < 0x100000000ULL)
      {
         size_str_p = size32_str_p;
         ctx_p->num_immediate_values = 2;
         size = 1;
      }
      else
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Immediate value > 32 bits. Not allowed for this instruction.");
         return (false);
      }
   }
   else
   {
      size_str_p = size16_str_p;
      ctx_p->num_immediate_values = 1;
      size = 0;
   }

   if (target == 0)
   {
      target_str_p = target_0_str_p;
   }
   else
   {
      target_str_p = target_1_str_p;
   }

   if (source == 0)
   {
      source_str_p = source_0_str_p;
   }
   else
   {
      source_str_p = source_1_str_p;
   }

   object_p = _fmsp_new_instruction(_fmsp_assembler_addsub_wr_wr_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0101 SVOW
    *
    * S - Size of immediate value (0 -> 16 bytes, 1 -> 32 bytes)
    * V - Target working register
    * O - Operation (0 add, 1 subtract)
    * W - Working register to use as operand
    */

   if (target == 0)
   {
      target_str_p = target_0_str_p;
   }
   else
   {
      target_str_p = target_1_str_p;
   }

   if (function == 0)
   {
      function_str_p = add_str_p;
   }
   else
   {
      function_str_p = sub_str_p;
   }

   if (source == 0)
   {
      source_str_p = source_0_str_p;
   }
   else
   {
      source_str_p = source_1_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_ADDSUB_WR_IV_TO_WR_STR) +
         strlen(size_str_p)                              +
         strlen(target_str_p)                            +
         strlen(function_str_p)                          +
         strlen(source_str_p)                            +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s%s",
            _FMSP_INSTR_CODE_ADDSUB_WR_IV_TO_WR_STR,
            size_str_p,
            target_str_p,
            function_str_p,
            source_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_ADDSUB_WR_IV_TO_WR |
                             (size << 3)                         |
                             (target << 2)                       |
                             (function << 1)                     |
                             source;

   if (strncmp(iv_p, "0x", 2) == 0)
   {
      len = 6 + /* 0xNNNN */
            1;

      word_1 =  immediate_value        & 0xffff;
      word_2 = (immediate_value >> 16) & 0xffff;

      if (ctx_p->num_immediate_values == 1)
      {
         object_p->num_valid_hw_words = 2;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         object_p->hw_words_p[1] = word_1;
      }
      else
      {
         object_p->num_valid_hw_words = 3;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL ||
             object_p->hw_words_str_p[2] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x%04x",
                  word_2);

         object_p->hw_words_p[1] = word_1;
         object_p->hw_words_p[2] = word_2;
      }
   }
   else
   {
      object_p->num_valid_hw_words = 2;

      len = strlen(iv_p) + 1;

      object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);

      if (object_p->hw_words_str_p[1] == NULL)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to allocate string.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      snprintf(object_p->hw_words_str_p[1],
               len,
               "%s",
               iv_p);

         object_p->hw_words_p[1] = 0;
   }

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   free (iv_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for shift left working register by specific value.
 *
 * Parameters:
 *    ctx_p - Parse context
 *    wr    - Working register
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_bitwise_shift_left_wr_by_sv_action(_fmsp_assembler_parser_context_t *ctx_p,
                                                     uint8_t                           wr,
                                                     char                             *sv_p)
{
   _fmsp_assembler_instruction_t *object_p;
   uint8_t                        specific_value;
   char                           start_shift_str_p[] = " | (";
   char                           end_shift_1_str_p[] = " << 1)";
   char                           wr_0_str_p[]        = " | 0x0";
   char                           wr_1_str_p[]        = " | 0x1";
   char                          *wr_str_p;
   uint32_t                       len;

   errno = 0;

   specific_value = (uint8_t)strtoul(sv_p, NULL, 0);

   if (errno != 0)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to convert immediate value.");
      return (false);
   }

   if (specific_value == 0)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Shift left value of 0 is not allowed.");
      return (false);
   }

   if (specific_value > 64)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Shift left value too large.");
      return (false);
   }

   /* Specific value is stored as val - 1 in instruction */
   specific_value--;

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_shift_left_wr_by_sv_e,
                                    ctx_p);
   
   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }
   
   /* Bit layout of this instruction:
    *
    * 0000 0001 0ZZZ ZZZW
    *
    * W - Working register (0 or 1)
    * Z - Specific value - 1
    */
   
   object_p->working_register = wr;
   
   if (wr == 0)
   {
      wr_str_p = wr_0_str_p;
   }
   else
   {
      wr_str_p = wr_1_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_SHIFT_LEFT_WR_BY_SV_STR) +
         strlen(start_shift_str_p)                        +
         6                                                + /* 0xNNNN */
         strlen(end_shift_1_str_p)                        +
         strlen(wr_str_p)                                 +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s0x%04x%s%s",
            _FMSP_INSTR_CODE_SHIFT_LEFT_WR_BY_SV_STR,
            start_shift_str_p,
            specific_value,
            end_shift_1_str_p,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_SHIFT_LEFT_WR_BY_SV |
                             (specific_value << 1)                |
                             wr;

   object_p->num_valid_hw_words = 1;
   
   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for shift right working register by specific value.
 *
 * Parameters:
 *    ctx_p - Parse context
 *    wr    - Working register
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_bitwise_shift_right_wr_by_sv_action(_fmsp_assembler_parser_context_t *ctx_p,
                                                      uint8_t                           wr,
                                                      char                             *sv_p)
{
   _fmsp_assembler_instruction_t *object_p;
   uint8_t                        specific_value;
   char                           start_shift_str_p[] = " | (";
   char                           end_shift_1_str_p[] = " << 1)";
   char                           wr_0_str_p[]        = " | 0x0";
   char                           wr_1_str_p[]        = " | 0x1";
   char                          *wr_str_p;
   uint32_t                       len;

   errno = 0;

   specific_value = (uint8_t)strtoul(sv_p, NULL, 0);

   if (errno != 0)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to convert immediate value.");
      return (false);
   }

   if (specific_value == 0)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Shift right value 0 is not allowed.");
      return (false);
   }

   if (specific_value > 64)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Shift right value too large.");
      return (false);
   }

   /* Specific value is stored as val - 1 in instruction */
   specific_value--;

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_shift_right_wr_by_sv_e,
                                    ctx_p);
   
   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }
   
   /* Bit layout of this instruction:
    *
    * 0000 0001 1ZZZ ZZZW
    *
    * W - Working register (0 or 1)
    * Z - Specific value - 1
    */
   
   object_p->working_register = wr;
   
   if (wr == 0)
   {
      wr_str_p = wr_0_str_p;
   }
   else
   {
      wr_str_p = wr_1_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_SHIFT_RIGHT_WR_BY_SV_STR) +
         strlen(start_shift_str_p)                         +
         6                                                 + /* 0xNNNN */
         strlen(end_shift_1_str_p)                         +
         strlen(wr_str_p)                                  +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s0x%04x%s%s",
            _FMSP_INSTR_CODE_SHIFT_RIGHT_WR_BY_SV_STR,
            start_shift_str_p,
            specific_value,
            end_shift_1_str_p,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_SHIFT_RIGHT_WR_BY_SV |
                             (specific_value << 1)                |
                             wr;

   object_p->num_valid_hw_words = 1;
   
   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for load bits immediate value to working register.
 *
 * Parameters:
 *    ctx_p      - Parse context.
 *    wr         - Working register.
 *    iv_p       - String containing immediate value.
 *    num_bits   - Number of bits to load.
 *    is_assign  - Assignment (true) or concat (false).
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_load_bits_iv_to_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                            uint8_t                           wr,
                                            char                             *iv_p,
                                            uint8_t                           num_bits,
                                            bool                              is_assign)
{
   uint32_t                       auto_num_bits         = 0;
   _fmsp_assembler_instruction_t *object_p;
   char                           start_shift_str_p[]  = " | (";
   char                           end_shift_1_str_p[]  = " << 1)";
   char                           assign_str_p[]       = " | (0x0 << 7)";
   char                           concat_str_p[]       = " | (0x1 << 7)";
   char                          *func_str_p;
   char                           wr_0_str_p[]         = " | 0x0";
   char                           wr_1_str_p[]         = " | 0x1";
   char                          *wr_str_p;
   uint32_t                       len;
   uint64_t                       immediate_value      = 0;
   uint16_t                       word_1;
   uint16_t                       word_2;
   uint16_t                       word_3;
   uint16_t                       word_4;
   uint32_t                       shift = 0;

   /* Check if the immediate value starts with "0x". If it does then
    * convert the value and base the number of immediate value words
    * on that.  Otherwise, this is a plan id enum which is 1 byte so
    * only use 1 immediate value word.
    */
   if (strncmp(iv_p, "0x", 2) == 0)
   {
      errno = 0;
   
      immediate_value = (uint64_t)strtoull(iv_p, NULL, 0);

      if (errno != 0)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to convert immediate value.");
         return (false);
      }

      if (immediate_value < 0x10000)
      {
         auto_num_bits = 16;
      }
      else if (immediate_value < 0x100000000ULL)
      {
         auto_num_bits = 32;
      }
      else if (immediate_value < 0x1000000000000ULL)
      {
         auto_num_bits = 48;
      }
      else
      {
         auto_num_bits = 64;
      }
   }
   else
   {
      auto_num_bits = 16;
   }

   if (wr == 0)
   {
      wr_str_p = wr_0_str_p;
   }
   else
   {
      wr_str_p = wr_1_str_p;
   }

   /* Use automatic value for number of bits if none supplied. */
   if (num_bits == 0)
   {
      num_bits = auto_num_bits;
   }

   /* Immediate value words */
   if (num_bits < 17)
   {
      ctx_p->num_immediate_values = 1;
   }
   else if (num_bits < 33)
   {
      ctx_p->num_immediate_values = 2;
   }
   else if (num_bits < 49)
   {
      ctx_p->num_immediate_values = 3;
   }
   else
   {
      ctx_p->num_immediate_values = 4;
   }

   object_p = _fmsp_new_instruction(_fmsp_assembler_load_bits_iv_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   if (is_assign)
   {
      func_str_p = assign_str_p;
      object_p->is_assign = true;
      shift = 0;
   }
   else
   {
      func_str_p = concat_str_p;
      object_p->is_assign = false;
      shift = 1;
   }

   /* Check if the immediate value starts with "0x". If it does then
    * convert the value and base the number of immediate value words
    * on that.  Otherwise, this is a plan id enum which is 1 byte so
    * only use 1 immediate value word.
    */
   if (strncmp(iv_p, "0x", 2) == 0)
   {
      len = 6 + /* 0xNNNN */
            1;

      word_1 =  immediate_value        & 0xffff;
      word_2 = (immediate_value >> 16) & 0xffff;
      word_3 = (immediate_value >> 32) & 0xffff;
      word_4 = (immediate_value >> 48) & 0xffff;

      if (num_bits <= 16)
      {
         object_p->num_valid_hw_words = 2;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         object_p->hw_words_p[1] = word_1;
      }
      else if (num_bits <= 32)
      {
         object_p->num_valid_hw_words = 3;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL ||
             object_p->hw_words_str_p[2] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x%04x",
                  word_2);

         object_p->hw_words_p[1] = word_1;
         object_p->hw_words_p[2] = word_2;
      }
      else if (num_bits <= 48)
      {
         object_p->num_valid_hw_words = 4;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[3] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL ||
             object_p->hw_words_str_p[2] == NULL ||
             object_p->hw_words_str_p[3] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x%04x",
                  word_2);

         snprintf(object_p->hw_words_str_p[3],
                  len,
                  "0x%04x",
                  word_3);

         object_p->hw_words_p[1] = word_1;
         object_p->hw_words_p[2] = word_2;
         object_p->hw_words_p[3] = word_3;
      }
      else
      {
         object_p->num_valid_hw_words = 5;

         object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[2] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[3] = (char *)calloc(len + 1, 1);
         object_p->hw_words_str_p[4] = (char *)calloc(len + 1, 1);

         if (object_p->hw_words_str_p[1] == NULL ||
             object_p->hw_words_str_p[2] == NULL ||
             object_p->hw_words_str_p[3] == NULL ||
             object_p->hw_words_str_p[4] == NULL)
         {
            _fmsp_assembler_yyerror(ctx_p,
                                    "Failed to allocate string.");
            _fmsp_destroy_instruction(object_p);
            return (false);
         }

         snprintf(object_p->hw_words_str_p[1],
                  len,
                  "0x%04x",
                  word_1);

         snprintf(object_p->hw_words_str_p[2],
                  len,
                  "0x%04x",
                  word_2);

         snprintf(object_p->hw_words_str_p[3],
                  len,
                  "0x%04x",
                  word_3);

         snprintf(object_p->hw_words_str_p[4],
                  len,
                  "0x%04x",
                  word_4);

         object_p->hw_words_p[1] = word_1;
         object_p->hw_words_p[2] = word_2;
         object_p->hw_words_p[3] = word_3;
         object_p->hw_words_p[4] = word_4;
      }
   }
   else
   {
      if (num_bits > 16)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Bit size too large. Can only load 16 bits from an id.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      object_p->num_valid_hw_words = 2;

      len = strlen(iv_p) + 1;

      object_p->hw_words_str_p[1] = (char *)calloc(len + 1, 1);

      if (object_p->hw_words_str_p[1] == NULL)
      {
         _fmsp_assembler_yyerror(ctx_p,
                                 "Failed to allocate string.");
         _fmsp_destroy_instruction(object_p);
         return (false);
      }

      snprintf(object_p->hw_words_str_p[1],
               len,
               "%s",
               iv_p);

      object_p->hw_words_p[1] = 0;
   }

   /* Bit layout of this instruction:
    *
    * 0000 0010 SNNN NNNW
    *
    * S - Assign or concat
    * N - Number of bits
    * W - Working register
    */

   len = strlen(_FMSP_INSTR_CODE_LOAD_BITS_IV_TO_WR_STR) +
         strlen(func_str_p)                              +
         strlen(start_shift_str_p)                       +
         6                                               + /* 0xNNNN */
         strlen(end_shift_1_str_p)                       +
         strlen(wr_str_p)                                +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s0x%04x%s%s",
            _FMSP_INSTR_CODE_LOAD_BITS_IV_TO_WR_STR,
            func_str_p,
            start_shift_str_p,
            num_bits - 1,          /* coded as 0 -> 63 for 1 -> 64 */
            end_shift_1_str_p,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_LOAD_BITS_IV_TO_WR |
                             (shift << 7)                        |
                             ((num_bits - 1) << 1)               |
                             wr;

   object_p->num_bits = num_bits;
   object_p->working_register = wr;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for load bytes parameter array to working register.
 *
 * Parameters:
 *    ctx_p       - Parse context.
 *    wr          - Working register.
 *    start_range - Start of parameter array range.
 *    end_range   - End of parameter array range.
 *    is_assign   - Assignment (true) or concat (false).
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_load_bytes_pa_to_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                             uint8_t                           wr,
                                             uint32_t                          range_start,
                                             uint32_t                          range_end,
                                             bool                              is_assign)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           wr0_str_p[]           = " | 0x0";
   char                           wr1_str_p[]           = " | 0x1";
   char                           assign_str_p[]        = " | (0x0 << 10)";
   char                           concat_str_p[]        = " | (0x1 << 10)";
   char                          *wr_str_p;
   char                          *func_str_p;
   char                           range_end_str_p[]     = " | (0x0000 << 4)";
   char                           num_bytes_str_p[]     = " | (0x0000 << 1)";
   uint32_t                       len;
   uint32_t                       shift = 0;

   if (range_start > MAX_PARAMETER_ARRAY_BYTE_INDEX)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Parameter Array start index out of bounds");

      return (false);
   }

   if (range_end   > MAX_PARAMETER_ARRAY_BYTE_INDEX)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Parameter Array end index out of bounds");

      return (false);
   }

   if (range_start > range_end)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Parameter Array start index > end index");

      return (false);
   }

   if (range_end - range_start > 7)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Parameter Array (end index - start index) > 7");

      return (false);
   }

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_load_bytes_pa_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0001 0SJJ JJJJ KKKW
    *
    * S - Assign or concat
    * J - End of range
    * K - Number of bytes - 1
    * W - Working register
    */

   /* Assign or concat */
   if (is_assign)
   {
      func_str_p = assign_str_p;
      object_p->is_assign = true;
      shift = 0;
   }
   else
   {
      func_str_p = concat_str_p;
      object_p->is_assign = false;
      shift = 1;
   }

   /* Set working register */
   if (wr == 0)
   {
      object_p->working_register = 0;
      wr_str_p = wr0_str_p;
   }
   else
   {
      object_p->working_register = 1;
      wr_str_p = wr1_str_p;
   }

   /* Set range */
   object_p->range_start = range_start;
   object_p->range_end   = range_end;

   len = strlen(range_end_str_p) + 1;

   /* Range end */
   snprintf(range_end_str_p,
            len,
            " | (0x%04x << 4)",
            object_p->range_end);

   len = strlen(num_bytes_str_p) + 1;

   /* Num bytes - 1 */
   snprintf(num_bytes_str_p,
            len,
            " | (0x%04x << 1)",
            (object_p->range_end - object_p->range_start));

   len = strlen(_FMSP_INSTR_CODE_LOAD_BYTES_PA_TO_WR_STR) +
         strlen(func_str_p)                               +
         strlen(range_end_str_p)                          +
         strlen(num_bytes_str_p)                          +
         strlen(wr_str_p)                                 +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");

      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s%s",
            _FMSP_INSTR_CODE_LOAD_BYTES_PA_TO_WR_STR,
            func_str_p,
            range_end_str_p,
            num_bytes_str_p,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_LOAD_BYTES_PA_TO_WR |
                             (shift << 10)                        |
                             (range_end << 4)                     |
                             ((range_end - range_start) << 1)     |
                             wr;

   object_p->working_register   = wr;
   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for load bits frame window to working register.
 *
 * Parameters:
 *    ctx_p       - Parse context.
 *    wr          - Working register.
 *    range_start - Start of frame window range.
 *    range_end   - End of frame window range.
 *    is_assign   - Assignment (true) or concat (false).
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_load_bits_fw_to_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                            uint8_t                           wr,
                                            uint32_t                          range_start,
                                            uint32_t                          range_end,
                                            bool                              is_assign)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           wr0_str_p[]           = " | 0x0";
   char                           wr1_str_p[]           = " | 0x1";
   char                           assign_str_p[]        = " | (0x0 << 14)";
   char                           concat_str_p[]        = " | (0x1 << 14)";
   char                          *wr_str_p;
   char                          *func_str_p;
   char                           range_end_str_p[]     = " | (0x0000 << 7)";
   char                           num_bytes_str_p[]     = " | (0x0000 << 1)";
   uint32_t                       len;
   uint32_t                       shift = 0;

   if (range_start > MAX_FRAME_WINDOW_BIT_INDEX)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Frame Window start index out of bounds");

      return (false);
   }

   if (range_end   > MAX_FRAME_WINDOW_BIT_INDEX)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Frame Window end index out of bounds");

      return (false);
   }

   if (range_start > range_end)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Frame Window start index > end index");

      return (false);
   }

   if (range_end - range_start > 63)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Frame Window (end index - start index) > 63");

      return (false);
   }

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_load_bits_fw_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 1SMM MMMM MNNN NNNW
    *
    * S - Assign or concat
    * M - End of range
    * N - Number of bytes - 1
    * W - Working register
    */

   /* Assign or concat */
   if (is_assign)
   {
      func_str_p = assign_str_p;
      object_p->is_assign = true;
      shift = 0;
   }
   else
   {
      func_str_p = concat_str_p;
      object_p->is_assign = false;
      shift = 1;
   }

   /* Set working register */
   if (wr == 0)
   {
      object_p->working_register = 0;
      wr_str_p = wr0_str_p;
   }
   else
   {
      object_p->working_register = 1;
      wr_str_p = wr1_str_p;
   }

   /* Set range */
   object_p->range_start = range_start;
   object_p->range_end   = range_end;

   len = strlen(range_end_str_p) + 1;

   /* Range end */
   snprintf(range_end_str_p,
            len,
            " | (0x%04x << 7)",
            object_p->range_end);

   len = strlen(num_bytes_str_p) + 1;

   /* Num bytes - 1 */
   snprintf(num_bytes_str_p,
            len,
            " | (0x%04x << 1)",
            (object_p->range_end - object_p->range_start));

   len = strlen(_FMSP_INSTR_CODE_LOAD_BITS_FW_TO_WR_STR) +
         strlen(func_str_p)                              +
         strlen(range_end_str_p)                         +
         strlen(num_bytes_str_p)                         +
         strlen(wr_str_p)                                +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");

      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s%s%s%s",
            _FMSP_INSTR_CODE_LOAD_BITS_FW_TO_WR_STR,
            func_str_p,
            range_end_str_p,
            num_bytes_str_p,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_LOAD_BITS_FW_TO_WR |
                             (shift << 14)                       |
                             (range_end << 7)                    |
                             ((range_end - range_start) << 1)    |
                             wr;

   object_p->working_register   = wr;
   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for value
 *
 * Parameters:
 *    ctx_p - Parse context
 *    str_p - Value string
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_value_action(_fmsp_assembler_parser_context_t *ctx_p,
                               char                             *str_p)
{
   int   len;
   char *old_str_p = ctx_p->value_str_p;
   char *value_str_p;

   value_str_p = strdup(str_p);

   if (value_str_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      return (false);
   }

   if (old_str_p == NULL)
   {
      ctx_p->value_str_p = value_str_p;
   }
   else
   {
      len = strlen(ctx_p->value_str_p) +
                   strlen(" | ")       +
                   strlen(value_str_p) +
                   1;

      ctx_p->value_str_p = (char *)calloc(len + 1, 1);

      snprintf(ctx_p->value_str_p,
               len,
               "%s | %s",
               old_str_p,
               value_str_p);

      free (value_str_p);
      free (old_str_p);
   }

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for store working register to lineup confirmation vector.
 *
 * Parameters:
 *    ctx_p       - Parse context.
 *    wr          - Working register.
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_store_wr_to_lcv_action(_fmsp_assembler_parser_context_t *ctx_p,
                                         uint8_t                           wr)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           wr0_str_p[]   = " | 0x0";
   char                           wr1_str_p[]   = " | 0x1";
   char                          *wr_str_p;
   uint32_t                       len;

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_store_wr_to_lcv_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0100 001W
    *
    * W - Working register
    */

   /* Set working register */
   if (wr == 0)
   {
      wr_str_p = wr0_str_p;
   }
   else
   {
      wr_str_p = wr1_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_STORE_WR_TO_LCV_STR) +
         strlen(wr_str_p)                             +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");

      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s",
            _FMSP_INSTR_CODE_STORE_WR_TO_LCV_STR,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_STORE_WR_TO_LCV | wr;

   object_p->working_register   = wr;
   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for load lineup confirmation vector to working register.
 *
 * Parameters:
 *    ctx_p       - Parse context.
 *    wr          - Working register.
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_load_lcv_to_wr_action(_fmsp_assembler_parser_context_t *ctx_p,
                                        uint8_t                           wr)
{
   _fmsp_assembler_instruction_t *object_p;
   char                           wr0_str_p[]   = " | 0x0";
   char                           wr1_str_p[]   = " | 0x1";
   char                          *wr_str_p;
   uint32_t                       len;

   /* No immediate value words */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_load_lcv_to_wr_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   /* Bit layout of this instruction:
    *
    * 0000 0000 0100 000W
    *
    * W - Working register
    */

   /* Set working register */
   if (wr == 0)
   {
      wr_str_p = wr0_str_p;
   }
   else
   {
      wr_str_p = wr1_str_p;
   }

   len = strlen(_FMSP_INSTR_CODE_LOAD_LCV_TO_WR_STR) +
         strlen(wr_str_p)                            +
         1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");

      _fmsp_destroy_instruction(object_p);
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s%s",
            _FMSP_INSTR_CODE_LOAD_LCV_TO_WR_STR,
            wr_str_p);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_LOAD_LCV_TO_WR | wr;

   object_p->working_register   = wr;
   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for jmp_nxt_eth_type
 *
 * Parameters:
 *    ctx_p - Parse context
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_nxt_eth_type_action(_fmsp_assembler_parser_context_t *ctx_p)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;

   /* No immediate values */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_jmp_nxt_eth_type_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   len = strlen(_FMSP_INSTR_CODE_JMP_NXT_ETH_TYPE_STR) + 1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s",
            _FMSP_INSTR_CODE_JMP_NXT_ETH_TYPE_STR);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_JMP_NXT_ETH_TYPE;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for jmp_nxt_ip_proto
 *
 * Parameters:
 *    ctx_p - Parse context
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_jmp_nxt_ip_proto_action(_fmsp_assembler_parser_context_t *ctx_p)
{
   _fmsp_assembler_instruction_t *object_p;
   uint32_t                       len;

   /* No immediate values */
   ctx_p->num_immediate_values = 0;

   object_p = _fmsp_new_instruction(_fmsp_assembler_jmp_nxt_ip_proto_e,
                                    ctx_p);

   if (object_p == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate instruction object.");
      return (false);
   }

   len = strlen(_FMSP_INSTR_CODE_JMP_NXT_IP_PROTO_STR) + 1;

   object_p->hw_words_str_p[0] = (char *)calloc(len + 1, 1);

   if (object_p->hw_words_str_p[0] == NULL)
   {
      _fmsp_assembler_yyerror(ctx_p,
                              "Failed to allocate string.");
      return (false);
   }

   snprintf(object_p->hw_words_str_p[0],
            len,
            "%s",
            _FMSP_INSTR_CODE_JMP_NXT_IP_PROTO_STR);

   /* HW opcode */
   object_p->hw_words_p[0] = _FMSP_INSTR_CODE_JMP_NXT_IP_PROTO;

   object_p->num_valid_hw_words = 1;

   dll_add_to_back(&(ctx_p->instruction_list),
                   &(object_p->dll_node));

   /* Clear context values */
   _fmsp_clear_ctx_instruction_values(ctx_p);

   return (true);
}

/* ---------------------------------------------------------------------------
 * Action for define_variable
 *
 * Parameters:
 *    ctx_p      - Parse context
 *    var_type   - Variable type
 *    var_name_p - Variable name
 *
 * Returns:
 *   Bool - True means action succeeded.
 *          False means action did not succeed.
 * ---------------------------------------------------------------------------*/
static bool _fmsp_define_variable_action(_fmsp_assembler_parser_context_t *ctx_p,
                                         _fmsp_assembler_variable_type_t   var_type,
                                         char                             *var_name_p)
{
   return (true);
}

%}

/* ---------------------------------------------------------------------------
 * Begin Parser definition
 * ---------------------------------------------------------------------------*/

/* Bison will report shift/reduce conflicts unless we tell it to expect
 * a particular number.  The following explains where the conflicts arise.
 * See Bison documentation for more explanation. */

%expect 0

/* Use verbose errors */
/* %error-verbose */

/* Prefix the standard functions with "_fmsp_assembler_yy" instead of just "yy".
   If we don't do this, then we will possibly conflict with other
   compiler libraries. */
%name-prefix="_fmsp_assembler_yy"

/* Pure re-entrant parser */
%pure-parser

/* Pass parser context to lex */
%lex-param {_fmsp_assembler_parser_context_t *_fmsp_assembler_context_p}

/* Pass parser context to _fmsp_yyparse */
%parse-param {_fmsp_assembler_parser_context_t *_fmsp_assembler_context_p}

/* Allow enabling of debug information */
/* %debug */

%union {
          int   integer;
          char *string;
       }

/* Non terminals that return a value during the parse. */
%type  <integer> working_register
%type  <integer> bitwise_operator
%type  <integer> operator_set1
%type  <integer> operator_set2
%type  <string>  immediate_value
%type  <string>  hard_examination
%type  <integer> hxs
%type  <string>  value
%type  <integer> addsub_operator
%type  <integer> variable_type

/* Terminals that return a value from the lexical analyzer. */
%token <string> ALPHA_NUMERIC_WORD
%token <string> JMP
//%token <string> END_OF_LINE
%token <string> COLON
%token <string> NOP
%token <string> CONFIRM_LAYER_MASK
%token <string> START_RAW_INSTRUCTION
%token <string> HEX_WORD
%token <string> HEX_VALUE
%token <string> ASSIGN
//%token <string> BLANK_LINE
%token <string> INCREMENT
%token <string> WINDOW_OFFSET
%token <string> HEADER_BASE
%token <string> RETURN
%token <string> SET_BITS
%token <string> LCV
%token <string> START_RESULT_ARRAY
%token <string> RIGHT_BRACKET
%token <string> ETH_HXS
%token <string> IPV4_HXS
%token <string> IPV6_HXS
%token <string> OTH_L3_HXS
%token <string> TCP_HXS
%token <string> UDP_HXS
%token <string> OTH_L4_HXS
%token <string> RETURN_HXS
%token <string> END_PARSE
%token <string> CONCAT
%token <string> WR0
%token <string> WR1
%token <string> CLEAR
%token <string> ONESCOMP
%token <string> AND
%token <string> OR
%token <string> XOR
%token <string> SHIFT_LEFT
%token <string> SHIFT_RIGHT
%token <string> IF
%token <string> START_FRAME_WINDOW
%token <string> QUESTION
%token <string> PIPE
%token <string> AMPERSAND
%token <string> HXS
%token <string> BRIDGE
%token <string> START_PARAMETER_ARRAY
%token <string> EQUAL
%token <string> NOT_EQUAL
%token <string> GREATER_THAN
%token <string> LESS_THAN
%token <string> GREATER_THAN_EQUAL
%token <string> LESS_THAN_EQUAL
%token <string> OPEN_PAREN
%token <string> CLOSE_PAREN
%token <string> COMMA
%token <string> CONTEXT_LIST
%token <string> PLUS
%token <string> MINUS
%token <string> NXT_ETH_TYPE
%token <string> NXT_IP_PROTO
%token <string> UINT6_TYPE
%token <string> UINT7_TYPE
%token <string> UINT8_TYPE
%token <string> UINT16_TYPE
%token <string> UINT32_TYPE
%token <string> UINT48_TYPE
%token <string> UINT64_TYPE

%token <integer> DEC_VALUE

%%

/* -------------------------------------------------------------------------
 * Grammar rules and actions follow.
 * ------------------------------------------------------------------------- */

input                      :   /* empty */
//                             | input instruction_line
                             | input instruction
                             ;

//instruction_line           :   blank_line
//                               {_fmsp_assembler_context_p->line_number++;}
//                             | instruction END_OF_LINE
//                               {_fmsp_assembler_context_p->line_number++;}
//                             ;

instruction                :   label
                             | raw_instruction
                             | jmp
                             | nop
                             | clm
                             | advance_hb_by_wo
                             | set_lcv_bits
                             | load_lcv_to_wr
                             | store_wr_to_lcv
                             | load_bytes_ra_to_wr
                             | store_wr_to_ra
                             | store_iv_to_ra
                             | load_sv_to_wo
                             | add_sv_to_wo
                             | modify_wo_by_wr
                             | zero_wr
                             | ones_comp_wr1_to_wr0
                             | bitwise_wr_wr_to_wr
                             | bitwise_wr_iv_to_wr
                             | addsub_wr_wr_to_wr
                             | addsub_wr_iv_to_wr
                             | shift_left_wr_by_sv
                             | shift_right_wr_by_sv
                             | load_bits_iv_to_wr
                             | load_bytes_pa_to_wr
                             | load_bits_fw_to_wr
                             | define_variable
                             ;

variable_type              :   UINT6_TYPE
                               {$$ = _fmsp_uint6_type_e;}
                             | UINT7_TYPE
                               {$$ = _fmsp_uint7_type_e;}
                             | UINT8_TYPE
                               {$$ = _fmsp_uint8_type_e;}
                             | UINT16_TYPE
                               {$$ = _fmsp_uint16_type_e;}
                             | UINT32_TYPE
                               {$$ = _fmsp_uint32_type_e;}
                             | UINT48_TYPE
                               {$$ = _fmsp_uint48_type_e;}
                             | UINT64_TYPE
                               {$$ = _fmsp_uint64_type_e;}
                             ;
 
define_variable            : variable_type ALPHA_NUMERIC_WORD
                               {if (!_fmsp_define_variable_action(_fmsp_assembler_context_p, $1, $2)) {return (1);}
                                free $2;
                               }
                             ;

value                      :   HEX_WORD
                               {$$ = $1;}
                             | HEX_VALUE
                               {$$ = $1;}
                             | ALPHA_NUMERIC_WORD
                               {$$ = $1;}
                             ;

values                     :   value
                               {_FMSP_PROC_TOK_P1(_fmsp_value_action, $1)}
                             | values PIPE value
                               {_FMSP_PROC_TOK_P1(_fmsp_value_action, $3)}
                             ;

//blank_line                 :   BLANK_LINE END_OF_LINE
//                             | END_OF_LINE
//                             ;

label                      :   ALPHA_NUMERIC_WORD COLON
                               {_FMSP_PROC_TOK_P1(_fmsp_label_action, $1)}
                             ;

five_hex_words             :   HEX_WORD HEX_WORD HEX_WORD HEX_WORD HEX_WORD
                               {_FMSP_PROC_TOK_P5(_fmsp_five_hex_words_action, $1, $2, $3, $4, $5)}
                             ;

four_hex_words             :   HEX_WORD HEX_WORD HEX_WORD HEX_WORD
                               {_FMSP_PROC_TOK_P4(_fmsp_four_hex_words_action, $1, $2, $3, $4)}
                             ;

three_hex_words            :   HEX_WORD HEX_WORD HEX_WORD
                               {_FMSP_PROC_TOK_P3(_fmsp_three_hex_words_action, $1, $2, $3)}
                             ;

two_hex_words              :   HEX_WORD HEX_WORD
                               {_FMSP_PROC_TOK_P2(_fmsp_two_hex_words_action, $1, $2)}
                             ;

one_hex_word               :   HEX_WORD
                               {_FMSP_PROC_TOK_P1(_fmsp_one_hex_word_action, $1)}
                             ;

raw_instruction            :   START_RAW_INSTRUCTION five_hex_words
                               {_FMSP_PROC_TOK_P0(_fmsp_raw_instruction_action)}
                             | START_RAW_INSTRUCTION four_hex_words
                               {_FMSP_PROC_TOK_P0(_fmsp_raw_instruction_action)}
                             | START_RAW_INSTRUCTION three_hex_words
                               {_FMSP_PROC_TOK_P0(_fmsp_raw_instruction_action)}
                             | START_RAW_INSTRUCTION two_hex_words
                               {_FMSP_PROC_TOK_P0(_fmsp_raw_instruction_action)}
                             | START_RAW_INSTRUCTION one_hex_word
                               {_FMSP_PROC_TOK_P0(_fmsp_raw_instruction_action)}
                             ;

hard_examination           :   ETH_HXS
                               {$$ = _FMSP_ETH_HXS_LABEL;}
                             | IPV4_HXS
                               {$$ = _FMSP_IPV4_HXS_LABEL;}
                             | IPV6_HXS
                               {$$ = _FMSP_IPV6_HXS_LABEL;}
                             | OTH_L3_HXS
                               {$$ = _FMSP_OTH_L3_HXS_LABEL;}
                             | TCP_HXS
                               {$$ = _FMSP_TCP_HXS_LABEL;}
                             | UDP_HXS
                               {$$ = _FMSP_UDP_HXS_LABEL;}
                             | OTH_L4_HXS
                               {$$ = _FMSP_OTH_L4_HXS_LABEL;}
                             | RETURN_HXS
                               {$$ = _FMSP_RETURN_HXS_LABEL;}
                             | END_PARSE
                               {$$ = _FMSP_END_PARSE_LABEL;}

hxs                       :    HXS
                               {$$ = true;}
                             | /* empty */
                               {$$ = false;}
                             ;

jmp                        :   JMP hard_examination
                               {if (!_fmsp_jmp_label_action(_fmsp_assembler_context_p, $2, false)) {return (1);}
                               }
                             | JMP HXS hard_examination
                               {if (!_fmsp_jmp_label_action(_fmsp_assembler_context_p, $3, true)) {return (1);}
                               }
                             | JMP ALPHA_NUMERIC_WORD
                               {if (!_fmsp_jmp_label_action(_fmsp_assembler_context_p, $2, false)) {return (1);}
                                free $2;
                               }
                             | JMP HXS ALPHA_NUMERIC_WORD
                               {if (!_fmsp_jmp_label_action(_fmsp_assembler_context_p, $3, true)) {return (1);}
                                free $3;
                               }
                             | JMP ALPHA_NUMERIC_WORD IF WR0 operator_set1 WR1
                               {if (!_fmsp_jmp_label_if_wr0_wr1_action(_fmsp_assembler_context_p, $2, $5)) {return (1);}
                                free $2;
                               }
                             | JMP ALPHA_NUMERIC_WORD IF WR0 operator_set2 immediate_value
                               {if (!_fmsp_jmp_label_if_wr0_iv_action(_fmsp_assembler_context_p, $2, $5, $6)) {return (1);}
                                free $2;
                                free $6;
                               }

                             | JMP QUESTION hxs ALPHA_NUMERIC_WORD COLON COLON hxs ALPHA_NUMERIC_WORD
                               {if (!_fmsp_jmp_label_case1dj_action(_fmsp_assembler_context_p, $4, $8, $3, $7)) {return (1);}
                                free $4; free $8;
                               }

                             | JMP QUESTION hxs ALPHA_NUMERIC_WORD PIPE hxs ALPHA_NUMERIC_WORD
                               {if (!_fmsp_jmp_label_case2dc_action(_fmsp_assembler_context_p, $4, $7, $3, $6)) {return (1);}
                                free $4; free $7;
                               }

                             | JMP QUESTION hxs ALPHA_NUMERIC_WORD PIPE hxs ALPHA_NUMERIC_WORD COLON COLON hxs ALPHA_NUMERIC_WORD
                               {if (!_fmsp_jmp_label_case2dj_action(_fmsp_assembler_context_p, $4, $7, $11, $3, $6, $10)) {return (1);}
                                free $4; free $7; free $11;
                               }

                             | JMP QUESTION hxs ALPHA_NUMERIC_WORD PIPE hxs ALPHA_NUMERIC_WORD PIPE hxs ALPHA_NUMERIC_WORD
                               {if (!_fmsp_jmp_label_case3dc_action(_fmsp_assembler_context_p, $4, $7, $10, $3, $6, $9)) {return (1);}
                                free $4; free $7; free $10;
                               }

                             | JMP QUESTION hxs ALPHA_NUMERIC_WORD PIPE hxs ALPHA_NUMERIC_WORD PIPE hxs ALPHA_NUMERIC_WORD COLON COLON hxs ALPHA_NUMERIC_WORD
                               {if (!_fmsp_jmp_label_case3dj_action(_fmsp_assembler_context_p, $4, $7, $10, $14, $3, $6, $9, $13)) {return (1);}
                                free $4; free $7; free $10; free $14;
                               }

                             | JMP QUESTION hxs ALPHA_NUMERIC_WORD PIPE hxs ALPHA_NUMERIC_WORD PIPE hxs ALPHA_NUMERIC_WORD PIPE hxs ALPHA_NUMERIC_WORD
                               {if (!_fmsp_jmp_label_case4dc_action(_fmsp_assembler_context_p, $4, $7, $10, $13, $3, $6, $9, $12)) {return (1);}
                                free $4; free $7; free $10; free $13;
                               }
                             | JMP NXT_ETH_TYPE
                               {_FMSP_PROC_TOK_P0(_fmsp_jmp_nxt_eth_type_action)}
                             | JMP NXT_IP_PROTO
                               {_FMSP_PROC_TOK_P0(_fmsp_jmp_nxt_ip_proto_action)}
                             ;

nop                        :   NOP
                               {_FMSP_PROC_TOK_P0(_fmsp_nop_action)}
                             ;

clm                        :   CONFIRM_LAYER_MASK
                               {_FMSP_PROC_TOK_P0(_fmsp_clm_action)}
                             ;

advance_hb_by_wo           :   HEADER_BASE INCREMENT WINDOW_OFFSET
                               {_FMSP_PROC_TOK_P0(_fmsp_advance_hb_by_wo_action)}
                             ;

store_wr_to_lcv            :   LCV ASSIGN working_register
                               {if (!_fmsp_store_wr_to_lcv_action(_fmsp_assembler_context_p, $3)) {return (1);}}
                             ;

load_lcv_to_wr             :   working_register ASSIGN LCV
                               {if (!_fmsp_load_lcv_to_wr_action(_fmsp_assembler_context_p, $1)) {return (1);}}
                             ;

set_lcv_bits               :   LCV SET_BITS values
                               {_FMSP_PROC_TOK_P0(_fmsp_set_lcv_bits_action)}
                             ;

working_register           :   WR0
                               {$$ = 0;}
                             | WR1
                               {$$ = 1;}
                             ;

result_array_range         :   START_RESULT_ARRAY DEC_VALUE COLON DEC_VALUE RIGHT_BRACKET
                               {if (!_fmsp_result_array_range_action(_fmsp_assembler_context_p, $2, $4, false)) {return (1);}}
                             | START_RESULT_ARRAY DEC_VALUE RIGHT_BRACKET
                               {if (!_fmsp_result_array_range_action(_fmsp_assembler_context_p, $2, $2, false)) {return (1);}}
                             ;

immediate_value            :   HEX_VALUE
                               {$$ = $1;}
                             | HEX_WORD
                               {$$ = $1;}
                             ;

load_bytes_ra_to_wr        :   working_register ASSIGN result_array_range
                               {if (!_fmsp_load_bytes_ra_to_wr_action(_fmsp_assembler_context_p, $1, true)) {return (1);}}
                             | working_register CONCAT result_array_range
                               {if (!_fmsp_load_bytes_ra_to_wr_action(_fmsp_assembler_context_p, $1, false)) {return (1);}}
                             ;

store_wr_to_ra             :   result_array_range ASSIGN working_register
                               {if (!_fmsp_store_wr_to_ra_action(_fmsp_assembler_context_p, $3)) {return (1);}}
                             ;

store_iv_to_ra             :   result_array_range ASSIGN immediate_value
                               {_FMSP_PROC_TOK_P1(_fmsp_store_iv_to_ra_action, $3)}
                             ;

load_sv_to_wo              :   WINDOW_OFFSET ASSIGN immediate_value
                               {_FMSP_PROC_TOK_P1(_fmsp_load_sv_to_wo_action, $3)}
                             ;

add_sv_to_wo               :   WINDOW_OFFSET INCREMENT immediate_value
                               {_FMSP_PROC_TOK_P1(_fmsp_add_sv_to_wo_action, $3)}
                             ;

modify_wo_by_wr            :   WINDOW_OFFSET ASSIGN working_register
                               {if (!_fmsp_modify_wo_by_wr_action(_fmsp_assembler_context_p, false, $3)) {return (1);}}
                             | WINDOW_OFFSET INCREMENT working_register
                               {if (!_fmsp_modify_wo_by_wr_action(_fmsp_assembler_context_p, true, $3)) {return (1);}}
                             ;

zero_wr                    :   CLEAR working_register
                               {if (!_fmsp_zero_wr_action(_fmsp_assembler_context_p, $2)) {return (1);}}
                             ;

ones_comp_wr1_to_wr0       :   ONESCOMP
                               {_FMSP_PROC_TOK_P0(_fmsp_ones_comp_wr1_to_wr0_action)}
                             ;

bitwise_operator           :   AND
                               {$$ = _FMSP_OPERATOR_AND;}
                             | AMPERSAND
                               {$$ = _FMSP_OPERATOR_AND;}
                             | OR
                               {$$ = _FMSP_OPERATOR_OR;}
                             | PIPE
                               {$$ = _FMSP_OPERATOR_OR;}
                             | XOR
                               {$$ = _FMSP_OPERATOR_XOR;}
                             ;

operator_set1              :   EQUAL
                               {$$ = _FMSP_OPERATOR_EQUAL;}
                             | NOT_EQUAL
                               {$$ = _FMSP_OPERATOR_NOT_EQUAL;}
                             | LESS_THAN
                               {$$ = _FMSP_OPERATOR_LESS_THAN;}
                             | GREATER_THAN
                               {$$ = _FMSP_OPERATOR_GREATER_THAN;}
                             | LESS_THAN_EQUAL
                               {$$ = _FMSP_OPERATOR_LESS_THAN_EQUAL;}
                             | GREATER_THAN_EQUAL
                               {$$ = _FMSP_OPERATOR_GREATER_THAN_EQUAL;}

operator_set2              :   EQUAL
                               {$$ = _FMSP_OPERATOR_EQUAL;}
                             | NOT_EQUAL
                               {$$ = _FMSP_OPERATOR_NOT_EQUAL;}
                             | LESS_THAN
                               {$$ = _FMSP_OPERATOR_LESS_THAN;}
                             | GREATER_THAN
                               {$$ = _FMSP_OPERATOR_GREATER_THAN;}

addsub_operator            :   PLUS
                               {$$ = _FMSP_OPERATOR_PLUS;}
                             | MINUS
                               {$$ = _FMSP_OPERATOR_MINUS;}
                             ;
bitwise_wr_wr_to_wr        :   working_register ASSIGN working_register bitwise_operator working_register
                               {if (!_fmsp_bitwise_wr_wr_to_wr_action(_fmsp_assembler_context_p, $1, $4)) {return (1);}}
                             ;

bitwise_wr_iv_to_wr        :   working_register ASSIGN working_register bitwise_operator immediate_value
                               {if (!_fmsp_bitwise_wr_iv_to_wr_action(_fmsp_assembler_context_p, $1, $3, $4, $5)) {return (1);} free $5;}
                             ;

addsub_wr_wr_to_wr         :   working_register ASSIGN WR0 addsub_operator WR1
                               {if (!_fmsp_addsub_wr_wr_to_wr_action(_fmsp_assembler_context_p, $1, 0, $4)) {return (1);}}
                             | working_register ASSIGN WR1 addsub_operator WR0
                               {if (!_fmsp_addsub_wr_wr_to_wr_action(_fmsp_assembler_context_p, $1, 1, $4)) {return (1);}}
                             ;

addsub_wr_iv_to_wr         :   working_register ASSIGN WR0 addsub_operator immediate_value
                               {if (!_fmsp_addsub_wr_iv_to_wr_action(_fmsp_assembler_context_p, $1, 0, $4, $5)) {return (1);}}
                             | working_register ASSIGN WR1 addsub_operator immediate_value
                               {if (!_fmsp_addsub_wr_iv_to_wr_action(_fmsp_assembler_context_p, $1, 1, $4, $5)) {return (1);}}
                             ;

shift_left_wr_by_sv        :   working_register SHIFT_LEFT immediate_value
                               {if (!_fmsp_bitwise_shift_left_wr_by_sv_action(_fmsp_assembler_context_p, $1, $3)) {return (1);} free $3;}
                             ;

shift_right_wr_by_sv       :   working_register SHIFT_RIGHT immediate_value
                               {if (!_fmsp_bitwise_shift_right_wr_by_sv_action(_fmsp_assembler_context_p, $1, $3)) {return (1);} free $3;}
                             ;

load_bits_iv_to_wr         :   working_register ASSIGN immediate_value COLON DEC_VALUE
                               {if (!_fmsp_load_bits_iv_to_wr_action(_fmsp_assembler_context_p, $1, $3, $5, true)) {return (1);} free $3;}
                             | working_register ASSIGN immediate_value
                               {if (!_fmsp_load_bits_iv_to_wr_action(_fmsp_assembler_context_p, $1, $3, 0, true)) {return (1);} free $3;}
                             | working_register CONCAT immediate_value COLON DEC_VALUE
                               {if (!_fmsp_load_bits_iv_to_wr_action(_fmsp_assembler_context_p, $1, $3, $5, false)) {return (1);} free $3;}
                             | working_register CONCAT immediate_value
                               {if (!_fmsp_load_bits_iv_to_wr_action(_fmsp_assembler_context_p, $1, $3, 0, false)) {return (1);} free $3;}
                             ;

load_bytes_pa_to_wr        :   working_register ASSIGN START_PARAMETER_ARRAY DEC_VALUE COLON DEC_VALUE RIGHT_BRACKET
                               {if (!_fmsp_load_bytes_pa_to_wr_action(_fmsp_assembler_context_p, $1, $4, $6, true)) {return (1);}}
                             | working_register ASSIGN START_PARAMETER_ARRAY DEC_VALUE RIGHT_BRACKET
                               {if (!_fmsp_load_bytes_pa_to_wr_action(_fmsp_assembler_context_p, $1, $4, $4, true)) {return (1);}}
                             | working_register CONCAT START_PARAMETER_ARRAY DEC_VALUE COLON DEC_VALUE RIGHT_BRACKET
                               {if (!_fmsp_load_bytes_pa_to_wr_action(_fmsp_assembler_context_p, $1, $4, $6, false)) {return (1);}}
                             | working_register CONCAT START_PARAMETER_ARRAY DEC_VALUE RIGHT_BRACKET
                               {if (!_fmsp_load_bytes_pa_to_wr_action(_fmsp_assembler_context_p, $1, $4, $4, false)) {return (1);}}
                             ;

load_bits_fw_to_wr         :   working_register ASSIGN START_FRAME_WINDOW DEC_VALUE BRIDGE DEC_VALUE RIGHT_BRACKET
                               {if (!_fmsp_load_bits_fw_to_wr_action(_fmsp_assembler_context_p, $1, $4, $6, true)) {return (1);}}
                             | working_register CONCAT START_FRAME_WINDOW DEC_VALUE BRIDGE DEC_VALUE RIGHT_BRACKET
                               {if (!_fmsp_load_bits_fw_to_wr_action(_fmsp_assembler_context_p, $1, $4, $6, false)) {return (1);}}
                             ;


/* -------------------------------------------------------------------------
 * End of parser grammar
 * ------------------------------------------------------------------------- */
