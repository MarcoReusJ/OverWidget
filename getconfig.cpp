#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include "getconfig.h"
#if defined(__BORLANDC__)
#include <alloc.h>
#include <mem.h>
#elif defined(_MSC_VER)
#define strcmpi _stricmp
#else
#define strcmpi strcasecmp
#endif

using namespace std;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifndef MAX_LINE_LEN
#define MAX_LINE_LEN 20000
#endif

/* constants for copy file */
#define BLOCKSIZE 512
typedef char DATA;
/******************************************************************
* DESCRIPTION:  Binary copy of one file to another
* RETURN:       none
* ALGORITHM:    none
* NOTES:        none
******************************************************************/
void filecopy(
        FILE *pDest, // destination file handle
        FILE *pSource) // source file handle
{
    DATA block[BLOCKSIZE] = {0}; /* holds data for file copy */
    int num_read = 0;  /* number of bytes read when copying file */

    if (pDest && pSource)
    {
        while ((num_read = fread(block,sizeof(DATA),
                                 BLOCKSIZE,pSource)) == BLOCKSIZE)
        {
            fwrite(block,sizeof(DATA),num_read,pDest);
        }
        /* write remaining stuff */
        fwrite(block,sizeof(DATA),num_read,pDest);
    }
}


bool WritePrivateProfileString(
        const char *pAppName,	// pointer to section name
        const char *pKeyName,	// pointer to key name
        const char *pString,	// pointer to string to add
        const char *pFileName) // pointer to initialization filename
{
    FILE *pFile; /* stream handle */
    FILE *pTempFile; /* stream handle */
    bool status = FALSE; /* return value */
    char line[MAX_LINE_LEN] = {""}; /* line in file */
    char copy_line[MAX_LINE_LEN] = {""}; /* copy of line in file */
    char token[MAX_LINE_LEN] = {""}; /* for tokenizing */
    bool found = FALSE; /* true if key is found */
    bool section = FALSE; /* true if section is found */

    /* flush cache - since we have no cache, just return */
    if (!pAppName && !pKeyName && !pString)
        return (status);

    /* undefined behavior */
    if (!pAppName || !pFileName)
        return (status);

    pFile = fopen(pFileName,"r+");
    if (!pFile)
    {
        if (pString && pKeyName)
        {
            /* new file */
            pFile = fopen(pFileName,"w");
            if (pFile)
            {
                fprintf(pFile,"[%s]\n",pAppName);
                fprintf(pFile,"%s=%s\n",pKeyName,pString);
                fclose(pFile);
                status = TRUE;
            }
        }
    }
    else
    {
        /* open a temp file to use as a buffer */
        pTempFile = tmpfile();
        /* process! */
        if (pTempFile)
        {
            while (fgets(line,sizeof(line),pFile) != NULL)
            {
                /* remove leading and trailing white space */
                rmtrail(line);
                rmlead(line);
                /* comment */
                if ((line[0] == ';') ||
                        (found))
                {
                    /* write line to new file */
                    fprintf(pTempFile,"%s\n",line);
                }
                /* inside my section */
                else if (section)
                {
                    /* comments */
                    if (line[0] == ';')
                    {
                        /* write line to new file */
                        fprintf(pTempFile,"%s\n",line);
                    }
                    /* new section */
                    else if (line[0] == '[')
                    {
                        /* write out data since we didn't find it */
                        if (pKeyName && pString)
                            fprintf(pTempFile,"%s=%s\n",pKeyName,pString);
                        found = TRUE;
                        status = TRUE;
                        /* write out this line, too! */
                        fprintf(pTempFile,"%s\n",line);
                    }
                    /* delete section by not writing line to temp file */
                    else if (!pKeyName)
                    {
                        /* do nothing */
                    }
                    /* finish processing the file */
                    else if (found)
                    {
                        /* write line to new file */
                        fprintf(pTempFile,"%s\n",line);
                    }
                    /* search */
                    else
                    {
                        strcpy(copy_line,line);
                        (void)stptok(copy_line,token,sizeof(token),"=");
                        rmtrail(token);
                        if (strcmpi(token,pKeyName) == 0)
                        {
                            status = TRUE;
                            found = TRUE;
                            /* overrite the line that was read in or remove it by
                 not writing it out if pString is NULL */
                            if (pString)
                                fprintf(pTempFile,"%s=%s\n",pKeyName,pString);
                        }
                        else
                        {
                            /* write the un-matched key line out to the temp file */
                            fprintf(pTempFile,"%s\n",line);
                        }
                    }
                }
                /* look for section */
                else if (line[0] == '[')
                {
                    strcpy(copy_line,line);
                    if (rmbrackets(copy_line))
                    {
                        // it's my section!
                        if (strcmpi(pAppName,copy_line) == 0)
                        {
                            section = TRUE;
                            // delete the section name if the KEY is NULL.
                            if (pKeyName)
                                fprintf(pTempFile,"%s\n",line);
                        }
                        else
                            fprintf(pTempFile,"%s\n",line);
                    }
                }
                else
                {
                    /* write line to new file */
                    fprintf(pTempFile,"%s\n",line);
                }
            }
            /* append to the end of the temp file */
            if (!section && pKeyName)
            {
                fprintf(pTempFile,"[%s]\n",pAppName);
            }
            if (!found && pKeyName && pString)
            {
                fprintf(pTempFile,"%s=%s\n",pKeyName,pString);
                status = TRUE;
            }
            /* finished! */
            fclose(pFile);
            /* copy the temp file data over the existing file */
            pFile = fopen(pFileName,"wb");
            if (pFile)
            {
                rewind(pTempFile);
                filecopy(pFile,pTempFile);
                fclose(pFile);
            }
            fclose(pTempFile);
        }
        /* unable to open temp file */
        else
        {
            fclose(pFile);
        }
    }
    return (status);
}


size_t GetPrivateProfileString(
        const char *pAppName,	// points to section name
        const char *pKeyName,	// points to key name
        const char *pDefault,	// points to default string
        char *pReturnedString,	// points to destination buffer
        size_t nSize,	// size of destination buffer
        const char *pFileName)	// points to initialization filename
{
    memset(pReturnedString,0,nSize);
    size_t count = 0; /* number of characters placed into return string */
    size_t len = 0; /* length of string */
    FILE *pFile = NULL; /* stream handle */
    bool use_default = FALSE; /* TRUE if we need to copy default string */
    char line[MAX_LINE_LEN] = {""}; /* line in file */
    char token[MAX_LINE_LEN] = {""}; /* for tokenizing */
    char *pLine = NULL; /* points to line in file */
    bool section = FALSE; /* true if section is found */
    bool found_in_section = FALSE; /* true if key is found in section */

    if (!pReturnedString || !pFileName)
        return (count);

    /* initialize the return string */
    pReturnedString[0] = '\0';

    pFile = fopen(pFileName,"r");
    if (pFile)
    {
        /* load all section names to ReturnString */
        if (!pAppName)
        {
            while (fgets(line,sizeof(line),pFile) != NULL)
            {
                /* remove leading and trailing white space */
                rmtrail(line);
                rmlead(line);
                if (line[0] == '[')
                {
                    if (rmbrackets(line))
                    {
                        len = strlen(line);
                        if ((len + count + 2) < nSize)
                        {
                            strcpy(pReturnedString,line);
                            len++; /* add null */
                            pReturnedString += len;
                            count += len;
                        }
                        /* copy as much as we can, then truncate */
                        else
                        {
                            len = nSize - 2 - count;
                            strncpy(pReturnedString,line,len);
                            len++; /* add null */
                            pReturnedString += len;
                            count += len;
                            break;
                        }
                    }
                }
            }
        }
        /* find section name */
        else
        {
            while (fgets(line,sizeof(line),pFile) != NULL)
            {
                /* remove leading and trailing white space */
                rmtrail(line);
                rmlead(line);
                /* comment */
                if (line[0] == ';')
                {
                    /* do nothing */
                }
                /* inside my section */
                else if (section)
                {
                    /* comments */
                    if (line[0] == ';')
                    {
                        /* do nothing */
                    }
                    /* new section */
                    else if (line[0] == '[')
                    {
                        /* we must not have found our key name */
                        if (pKeyName)
                            use_default = TRUE;
                        /* stop reading the file */
                        break;
                    }
                    /* search */
                    else if (pKeyName)
                    {
                        (void)stptok(line,token,sizeof(token),"=");
                        rmtrail(token);
                        /* found? */
                        if (strcmpi(token,pKeyName) == 0)
                        {
                            pLine = strchr(line,'=');
                            if (pLine)
                            {
                                found_in_section = TRUE;
                                pLine++;
                                /* cleanup return string */
                                rmtrail(pLine);
                                rmlead(pLine);
                                (void)rmquotes(pLine);
                                /* count what's left */
                                len = strlen(pLine);
                                if (len < nSize)
                                {
                                    strcpy(pReturnedString,pLine);
                                }
                                /* copy as much as we can, then truncate */
                                else
                                {
                                    len = nSize - 1; /* less the null */
                                    strncpy(pReturnedString,pLine,len);
                                }
                                count = len;
                            }
                            /* stop reading the file */
                            break;
                        }
                    }
                    /* load return string with key names */
                    else
                    {
                        found_in_section = TRUE;
                        (void)stptok(line,token,sizeof(token),"=");
                        rmtrail(token);
                        len = strlen(token);
                        if ((len + count + 2) < nSize)
                        {
                            strcpy(pReturnedString,token);
                            len++; /* add null */
                            pReturnedString += len;
                            count += len;
                        }
                        /* copy as much as we can, then truncate */
                        else
                        {
                            len = nSize - 2 - count;
                            strncpy(pReturnedString,token,len);
                            len++; /* add null */
                            pReturnedString += len;
                            count += len;
                            break;
                        }
                    }
                }
                /* look for section */
                else if (line[0] == '[')
                {
                    if (rmbrackets(line))
                    {
                        if (strcmpi(pAppName,line) == 0)
                            section = TRUE;
                    }
                }
            }
            if (!section)
                use_default = TRUE;
        }
        if (!pKeyName || !pAppName)
        {
            /* count doesn't include last 2 nulls */
            if (count)
                count--;
            /* this pointer should be pointing to the start of next string */
            pReturnedString[0] = '\0';
        }
        fclose(pFile);
    }
    // if the file does not exist, return the default
    else
        use_default = TRUE;

    // key not found in section - return default
    if (section && !found_in_section)
        use_default = TRUE;

    if (use_default && pDefault)
    {
        (void)strncpy(pReturnedString,pDefault,nSize);
        pReturnedString[nSize-1] = '\0';
        /* cleanup return string */
        rmtrail(pReturnedString);
        rmlead(pReturnedString);
        (void)rmquotes(pReturnedString);
        /* count what's left */
        count = strlen(pReturnedString);
    }

    return (count);
}


/*******************************/
/******************************************************************
* NAME:         rmlead
* DESCRIPTION:  Remove leading whitespace
* PARAMETERS:   none
* GLOBALS:      none
* RETURN:       none
* ALGORITHM:    none
* NOTES:        none
******************************************************************/
void rmlead(char *str)
{
    char *obuf;

    if (str)
    {
        for (obuf = str; *obuf && isspace(*obuf); ++obuf) {}
        if (str != obuf)
            memmove(str,obuf,strlen(obuf)+1);
    }
    return;
}

/******************************************************************
* NAME:         rmtrail
* DESCRIPTION:  Remove the trailing white space from a string
* PARAMETERS:   none
* GLOBALS:      none
* RETURN:       none
* ALGORITHM:    none
* NOTES:        none
******************************************************************/
void rmtrail(char *str)
{
    size_t i;

    if (str && 0 != (i = strlen(str)))
    {
        /* start at end, not at 0 */
        --i;
        do
        {
            if (!isspace(str[i]))
                break;
        } while (--i);
        str[++i] = '\0';
    }
    return;
}

/******************************************************************
* NAME:         rmquotes
* DESCRIPTION:  Remove single or double quotation marks
*               enclosing the string
* PARAMETERS:   str (IO) string containing quotes
* GLOBALS:      none
* RETURN:       0 if unsuccessful
* ALGORITHM:    none
* NOTES:        none
******************************************************************/
int rmquotes(char *str)
{
    size_t len;
    int status = 0;

    if (str)
    {
        len = strlen(str);
        if (len > 1)
        {
            if (((str[0]     == '\'') &&
                 (str[len-1] == '\'')) ||
                    ((str[0]     == '\"') &&
                     (str[len-1] == '\"')))
            {
                str[len-1] = '\0';
                memmove(str,&str[1],len);
                status = 1;
            }
        }
    }
    return (status);
}

/******************************************************************
* NAME:         rmbrackets
* DESCRIPTION:  Remove brackets enclosing the string
* PARAMETERS:   str (IO) string containing brackets
* GLOBALS:      none
* RETURN:       0 if unsuccessful
* ALGORITHM:    none
* NOTES:        none
******************************************************************/
int rmbrackets(char *str)
{
    size_t len;
    int status = 0;

    if (str)
    {
        len = strlen(str);
        if (len > 1)
        {
            if ((str[0]     == '[') &&
                    (str[len-1] == ']'))
            {
                str[len-1] = '\0';
                memmove(str,&str[1],len);
                status = 1;
            }
        }
    }
    return (status);
}

/***********************************************/
/******************************************************************
* DESCRIPTION:  You pass this function a string to parse,
*               a buffer to receive the "token" that gets scanned,
*               the length of the buffer, and a string of "break"
*               characters that stop the scan.
*               It will copy the string into the buffer up to
*               any of the break characters, or until the buffer
*               is full, and will always leave the buffer
*               null-terminated.  It will return a pointer to the
*               first non-breaking character after the one that
*               stopped the scan.
* RETURN:       It will return a pointer to the
*               first non-breaking character after the one that
*               stopped the scan or NULL on error or end of string.
* ALGORITHM:    none
******************************************************************/
char *stptok(
        const char *s,/* string to parse */
        char *tok, /* buffer that receives the "token" that gets scanned */
        size_t toklen,/* length of the buffer */
        const char *brk)/* string of break characters that will stop the scan */
{
    char *lim; /* limit of token */
    const char *b; /* current break character */


    /* check for invalid pointers */
    if (!s || !tok || !brk)
        return NULL;

    /* check for empty string */
    if (!*s)
        return NULL;

    lim = tok + toklen - 1;
    while ( *s && tok < lim )
    {
        for ( b = brk; *b; b++ )
        {
            if ( *s == *b )
            {
                *tok = 0;
                for (++s, b = brk; *s && *b; ++b)
                {
                    if (*s == *b)
                    {
                        ++s;
                        b = brk;
                    }
                }
                if (!*s)
                    return NULL;
                return (char *)s;
            }
        }
        *tok++ = *s++;
    }
    *tok = 0;

    if (!*s)
        return NULL;
    return (char *)s;
}

