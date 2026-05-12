/******************************************************************************
 ** ISCTE-IUL: Sistemas Operativos
 **
 ** Nome do Módulo: so_utils.h
 ** @author Carlos Coutinho
 ** Descrição/Explicação do Módulo:
 **     Definição de funções utilitárias genéricas
 **
 ** SYNTAX: #include "/home/so/utils/include/so_utils.h"
 ******************************************************************************/
 #ifndef __SO_UTILS_H__
    #define __SO_UTILS_H__

    #include <stdio.h>      // Header para as constantes e a funções de Standard I/O (https://en.wikipedia.org/wiki/C_file_input/output)
    #include <stdlib.h>     // Header para as funções exit(), malloc(), rand(), srand()
    #include <string.h>     // Header para as funções str*(), mem*(), atoi(), atof()
    #include <errno.h>      // Header para as constantes que descrevem os vários erros E* (https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html)
    #include <sys/time.h>   // Header para os tipos time_t e timeval, e para a função gettimeofday()
    #include <stdarg.h>     // Header para as funções va_start(), va_end()
    #include <limits.h>     // Header to the constant INT_MAX
    #include <ctype.h>      // Header to the function isspace()
    #include "define_colours.h"

    #define TRUE    1
    #define FALSE   0
    #define SUCCESS 0       // This means returning from a function with no errors
    #define ERROR  -1       // This means returning from a function with error

    /******************************************************************************
     * Descrição/Explicação do Módulo:
     *     Macros para impressão de mensagens de DEBUG, SUCESSO e ERRO
     *     Se a variável de ambiente SO_HIDE_COLOURS estiver definida, não mostra cores
     *     Se a variável de ambiente SO_HIDE_DEBUG estiver definida, não mostra mensagens de debug
     ******************************************************************************/

    #include "define_colours.h"
    // #define SO_HIDE_COLOURS  // Uncomment this line to hide all colours
    // #define SO_HIDE_DEBUG    // Uncomment this line to hide all @DEBUG statements

    /**
     * Escreve uma mensagem de DEBUG (parâmetros iguais ao printf) mas apenas se SO_HIDE_DEBUG não estiver definida.
     * Escreve mensagens de DEBUG incluindo o módulo e a linha de código.
     * SYNTAX: so_debug(format, [<opcional 1>], [<opcional 2>], ... [<opcional n>])
     * Esta macro NÃO CAUSA TERMINAÇÃO do programa que o invoca.
     *
     * @param fmt format string, para ser usada de forma análoga a printf()
     * @param 2..n: Informações que se pretende que sejam apresentadas
     */
    #ifndef SO_HIDE_DEBUG
        static inline void so_debug(const char file[], const int line, const char func[], const char *restrict format, ...) {
            fprintf(stderr, COLOUR_BACK_FAINT_GRAY "@DEBUG:%s:%d:%s():" COLOUR_GRAY " [", file, line, func );
            va_list __ap__;
            va_start(__ap__, format);
            vfprintf(stderr, format, __ap__);
            va_end(__ap__);
            fprintf(stderr, "]" COLOUR_NONE "\n");
        }
        // Define a macro to file, line and func values
        #define so_debug(...) so_debug( __FILE__, __LINE__, __func__, __VA_ARGS__ )
    #else
        // Dummy function that does nothing, so the compilation does not return warnings
        static inline void so_debug (const char *restrict format, ...) {
            va_list __ap__;
            va_start(__ap__, format);
            va_end(__ap__);
        }
        // Define a macro to file, line and func values
        #define so_debug(...) so_debug( __VA_ARGS__ )
    #endif

    /**
     * Escreve uma mensagem de SUCESSO (parâmetros iguais ao printf), deve ser usado
     * em todas as mensagens "positivas" (de SUCESSO) que a aplicação mostra.
     * SYNTAX: so_success(<step>, [<opcional 1>], [<opcional 2>], ... [<opcional n>])
     *
     * @param step - Passo do enunciado que está a ser avaliado
     * @param fmt format string, para ser usada de forma análoga a printf()
     * @param 3..n: Informações adicionais que sejam pedidas no step avaliado
     */
    static inline void so_success(const char *restrict step, const char *restrict format, ...) {
        va_list __ap__;
        va_start(__ap__, format);
        fprintf(stderr, COLOUR_BACK_GREEN "@SUCCESS" );
        if (*step) fprintf(stderr," {%s}", step);
        fprintf(stderr, COLOUR_GREEN " [");
        vfprintf(stderr, format, __ap__);
        fprintf(stderr, "]" COLOUR_NONE "\n");
        va_end(__ap__);
    }

    /**
     * Escreve uma mensagem de ERRO (parâmetros iguais ao printf), deve ser usado
     * em todas as mensagens "negativas" (de falha) que a aplicação mostra.
     * SYNTAX: so_error(<step>, [<opcional 1>], [<opcional 2>], ... [<opcional n>])
     *
     * @param step - Passo do enunciado que está a ser avaliado
     * @param fmt format string, para ser usada de forma análoga a printf()
     * @param 3..n: Informações adicionais que sejam pedidas no step avaliado
     */
    static inline void so_error(const char *restrict step, const char *restrict format, ...) {
        va_list __ap__;
        va_start(__ap__, format);
        fprintf(stderr, COLOUR_BACK_BOLD_RED "@ERROR");
        if (*step) fprintf(stderr," {%s}", step);
        fprintf(stderr, COLOUR_RED " [" );
        vfprintf(stderr, format, __ap__);
        fprintf(stderr, "]" COLOUR_NONE "\n");
        if (errno) perror("(SO_Erro)");
        va_end(__ap__);
    }

    /******************************************************************************
     *  Macros para tratamento de erros
     ******************************************************************************/

    /**
     *  Escreve uma mensagem de erro e sai com erro ERROR, e mostrando a mensagem de erro,
     *  @param format mensagem de erro a apresentar em caso de erro (com a síntaxe do printf)
     */
    static inline void so_fatal_error (const char file[], const int line, const char *restrict format, ...) {
        fprintf(stderr, COLOUR_BACK_BOLD_RED "Fatal Error: %s:%d" COLOUR_RED " [", file, line);
        va_list __ap__;
        va_start(__ap__, format);
        vfprintf(stderr, format, __ap__);
        va_end(__ap__);
        fprintf(stderr, "]");
        if (errno) perror(" SO");
        fprintf(stderr, COLOUR_NONE "\n");
        exit(ERROR);
    }
    // Define a macro to file, line and func values
    #define so_exit_on_error(status, ...) do { if (status < 0) so_fatal_error( __FILE__, __LINE__, __VA_ARGS__ ); } while (0)
    // Define a macro to file, line and func values
    #define so_exit_on_error_so_error(status, step, ...) do { if (status < 0) { so_error(step, __VA_ARGS__); so_fatal_error( __FILE__, __LINE__, __VA_ARGS__ ); } } while (0)
    // Define a macro to file, line and func values
    #define so_exit_on_null(status, ...) do { if (NULL == status) so_fatal_error( __FILE__, __LINE__, __VA_ARGS__ ); } while (0)
    // Define a macro to file, line and func values
    #define so_exit_on_null_so_error(status, step, ...) do { if (NULL == status) { so_error(step, __VA_ARGS__); so_fatal_error( __FILE__, __LINE__, __VA_ARGS__ ); } } while (0)

    /******************************************************************************
     *  Macros para leitura de Strings de um ficheiro ou do STDIN
     ******************************************************************************/

    /**
     *  Macro para leitura de uma string de um ficheiro já aberto
     *  Esta macro basicamente efetua a leitura de uma string de um ficheiro aberto, definido um buffer_size. Semelhante a fgets(), mas removendo o '\n' e os caracteres extra.
     *  @param buffer       (char*) buffer onde vai ser depositada a informação
     *  @param buffer_size  (int) tamanho do buffer acima (em bytes)
     *  @param file         (FILE*) handler do ficheiro já previamente aberto, de onde deve ler
     *  @return             Um apontador (char *) para o buffer passado (a string lida), ou NULL em caso de erro
     */
    static inline char *so_fgets (char *buffer, int buffer_size, FILE *file) {
        char *__result__ = fgets(buffer, buffer_size, file);
        if (NULL != __result__) {
            if ('\n' == buffer[strlen(buffer) - 1])
                buffer[strlen(buffer) - 1] = '\0';
            else {
                int c;
                do
                    c = getc(file);
                while ('\n' != c && EOF != c);
            }
        }
        return __result__;
    }

    /**
     *  Macro para leitura de uma string do STDIN
     *  Esta macro basicamente efetua a leitura de uma string do STDIN, definido um buffer_size. Semelhante a fgets(), mas removendo o '\n' e os caracteres extra.
     *  @param buffer        (char*) buffer onde vai ser depositada a informação
     *  @param buffer_size   (int) tamanho do buffer acima (em bytes)
     *  @return             Um __ap__ontador (char *) para o buffer passado (a string lida), ou NULL em caso de erro
     */
    static inline char *so_gets (char *buffer, int buffer_size) {
        return so_fgets(buffer, buffer_size, stdin);
    }

    char __so_utils_temporary_buffer__[99];

    /**
     * @brief Returns a pointer to a substring of the original string. If the given
     * string was allocated dynamically, the caller must not overwrite that pointer
     * with the returned value, since the original pointer must be deallocated using
     * the same allocator with which it was allocated. The return value must NOT be
     * deallocated using free() etc.
     * @param stringWithSpaces Input String, which can include heading and traling spaces
     * @return char *   a pointer to the substring without heading or trailing spaces
     */
    static inline char *so_trim (char *stringWithSpaces) {
        char *__stringEnd__;
        while (isspace(*stringWithSpaces)) ++stringWithSpaces;
        if ('\0' == *stringWithSpaces)  // The string only contained spaces?
            return stringWithSpaces;
        __stringEnd__ = stringWithSpaces + strlen(stringWithSpaces) - 1;
        while (__stringEnd__ > stringWithSpaces && isspace(*__stringEnd__)) {
            __stringEnd__--;}
        __stringEnd__[1] = '\0';
        return stringWithSpaces;
    }

    /**
     *  Macro para leitura de um caracter de um ficheiro já aberto
     *  Esta macro basicamente efetua a leitura de uma string de um ficheiro já aberto, e depois extrai o primeiro caracter da string, retornando-o.
     *  @param file (FILE*) handler do ficheiro já previamente aberto, de onde deve ler
     *  @return char O caracter lido
     */
    static inline int so_fgetc (FILE *file) {
        char __result__ = 0;
        if (NULL == so_fgets(__so_utils_temporary_buffer__, sizeof(__so_utils_temporary_buffer__), file))
            errno = EOVERFLOW;
        else
            __result__ = *so_trim(__so_utils_temporary_buffer__);
        return __result__;
    }

    /**
     *  Macro para leitura de um caracter do STDIN
     *  Esta macro basicamente efetua a leitura de uma string do STDIN, e depois extrai o primeiro caracter da string, retornando-o.
     *  @return char O caracter lido
     */
    static inline char so_getc (void) {
        return so_fgetc(stdin);
    }

    /**
     *  @brief  Macro para conversão de uma string para um inteiro
     *          Esta função basicamente converte a string num inteiro, retornando-o.
     *          O comportamento dessa conversão é o descrito em https://www.ibm.com/docs/en/i/7.4?topic=functions-atoi-convert-character-string-integer
     *          PROBLEMA:   O comportamento de atoi é retornar 0 em caso de erro, o que não diferencia o caso de input == "0\n" de input = "ABC\n".
     *                      Para ajudar, esta macro verifica, de forma muito básica, no caso particular do resultado ser 0, se o primeiro caracter da
     *                      string de input foi "0". Caso não tenha sido, retorna 0 na mesma, mas coloca errno = EOVERFLOW
     *  @param number (char*) string a converter
     *  @return (long) o long lido. Se o valor returnado for 0 e errno == EOVERFLOW, então é porque houve erro na conversão.
     */
    static inline long so_strtol (char *number, int base) {
        long __result__ = 0;
        char *__end_conversion__;
        __result__ = strtol(number, &__end_conversion__, base);
        if (__end_conversion__ == number || __result__ > INT_MAX || __result__ < INT_MIN)
            errno = EOVERFLOW;
        return __result__;
    }

    static inline long so_atol (char *number) {
        return so_strtol(number, 10);
    }

    static inline int so_atoi (char *number) {
        return (int) so_strtol(number, 10);
    }

    /**
     *  @brief  Função para conversão de uma string para um float
     *          Esta função basicamente converte a string num float, retornando-o.
     *          O comportamento dessa conversão é o descrito em https://www.ibm.com/docs/en/i/7.4?topic=functions-atof-convert-character-string-float
     *          PROBLEMA:   O comportamento de atof é retornar 0 em caso de erro, o que não diferencia o caso de input == "0\n" de input = "ABC\n".
     *                      Para ajudar, esta função verifica, de forma muito básica, no caso particular do resultado ser 0, se o primeiro caracter da
     *                      string de input foi "0". Caso não tenha sido, retorna 0 na mesma, mas coloca errno = EOVERFLOW
     *  @param number (char*) string a converter
     *  @return (double) o double lido. Se o valor returnado for 0 e errno == EOVERFLOW, então é porque houve erro na conversão.
     */
    static inline double so_strtod(char *number) {
        double __result__ = 0;
        char *__end_conversion__;
        __result__ = strtof(number, &__end_conversion__);
        if (__end_conversion__ == number)
            errno = EOVERFLOW;
        return __result__;
    }

    static inline float so_atof(char *number) {
        return so_strtod(number);
    }

    /**
     *  Macro para leitura de um inteiro de um ficheiro já aberto
     *  Esta macro basicamente efetua a leitura de uma string de um ficheiro já aberto, e depois converte a string num inteiro, retornando-o.
     *  O comportamento dessa conversão é o descrito em https://www.ibm.com/docs/en/i/7.4?topic=functions-atoi-convert-character-string-integer
     *  PROBLEMA: O comportamento de atoi é retornar 0 em caso de erro, o que não diferencia o caso de input == "0\n" de input = "ABC\n".
     *            Para ajudar, esta macro verifica, de forma muito básica, no caso particular do resultado ser 0, se o primeiro caracter da
     *            string de input foi "0". Caso não tenha sido, retorna 0 na mesma, mas coloca errno = EOVERFLOW
     *  @param file (FILE*) handler do ficheiro já previamente aberto, de onde deve ler
     *  @return int O inteiro lido
     */
    static inline int so_fgeti (FILE *file) {
        if (NULL == so_fgets(__so_utils_temporary_buffer__, sizeof(__so_utils_temporary_buffer__), file))
            errno = EOVERFLOW;
        else
            return so_atoi(__so_utils_temporary_buffer__);
        return 0;
    }

    /**
     *  Macro para leitura de um inteiro do STDIN
     *  Esta macro basicamente efetua a leitura de uma string do STDIN, e depois converte a string num inteiro, retornando-o.
     *  @return int O inteiro lido
     */
    static inline int so_geti (void) {
        return so_fgeti(stdin);
    }

    /**
     *  Macro para leitura de um float de um ficheiro já aberto
     *  Esta macro basicamente efetua a leitura de uma string de um ficheiro já aberto, e depois converte a string num float, retornando-o.
     *  O comportamento dessa conversão é o descrito em https://www.ibm.com/docs/en/i/7.4?topic=functions-atof-convert-character-string-float
     *  PROBLEMA: O comportamento de atof é retornar 0 em caso de erro, o que não diferencia o caso de input == "0\n" de input = "ABC\n".
     *            Para ajudar, esta macro verifica, de forma muito básica, no caso particular do resultado ser 0, se o primeiro caracter da
     *            string de input foi "0". Caso não tenha sido, retorna 0 na mesma, mas coloca errno = EOVERFLOW
     *  @param file (FILE*) handler do ficheiro já previamente aberto, de onde deve ler
     *  @return float O float lido
     */
    static inline float so_fgetf (FILE *file) {
        if (NULL == so_fgets(__so_utils_temporary_buffer__, sizeof(__so_utils_temporary_buffer__), file)) 
            errno = EOVERFLOW;
        else
            return so_strtod(__so_utils_temporary_buffer__);
        return 0;
    }

    /**
     *  Macro para leitura de um float do STDIN
     *  Esta macro basicamente efetua a leitura de uma string do STDIN, e depois converte a string num float, retornando-o.
     *  @return float O float lido
     */
    static inline float so_getf (void) {
        return so_fgetf(stdin);
    }

    /******************************************************************************
     *  Macros utilitárias
     ******************************************************************************/

    /**
     *  Macro para validação se as macros so_geti() ou so_fgeti() retornaram um número
     *  @param number o valor acabado de ser lido por so_geti(), so_getf(), so_fgeti() ou so_fgetf()
     *  @return ERROR se number NÃO for um número introduzido pelo utilizador
     */
    static inline int so_isint(int number) {
        return (number || EOVERFLOW != errno) ? SUCCESS : ERROR;
    }
    #define so_isnumber(number) (so_isint(number) != ERROR)

    /**
     *  Macro para validação se as macros so_getf() ou so_fgetf() retornaram um número
     *  @param number o valor acabado de ser lido por so_geti(), so_getf(), so_fgeti() ou so_fgetf()
     *  @return ERROR se number NÃO for um número introduzido pelo utilizador
     */
    static inline int so_isfloat(float number) {
        return (number || EOVERFLOW != errno) ? SUCCESS : ERROR;
    }

    /**
     * @brief Returns a pseudo-random integer in the range 0 to 2^31-1 inclusive
     *
     * The function uses the libc `random()` function and the current microsseconds
     * (from gettimeofday) as a seed.
     *
     * @return long  random integer
     */
    static inline long so_random (void) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        srandom(tv.tv_usec);
        return random();
    }

    /**
     * @brief Returns a pseudo-random integer in the specified range
     *
     * The function relies on `so_random()` for generating the random number
     *
     * @param min   Minimum value
     * @param max   Maximum value
     * @return int  random integer
     */
    static inline int so_random_between_values (int min, int max) {
        return min + (int) ((max - min + 1) * (so_random() / (float) INT_MAX));
    }

#endif // __SO_UTILS_H__