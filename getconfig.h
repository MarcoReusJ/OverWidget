#ifndef PROFILE_H
#define PROFILE_H

#if !defined(_INC_WINDOWS)
  #include <stdio.h> // for size_t


  #ifndef FALSE
    #define FALSE 0
  #endif
  #ifndef TRUE
    #define TRUE 1
  #endif

  #ifndef MAX_LINE_LEN
  #define MAX_LINE_LEN 20000
  #endif

  #ifdef __cplusplus
  extern "C" {
  #endif /* __cplusplus */

  bool WritePrivateProfileString(
    const char *pAppName,	//  // INI文件中的一个字段名[节名]可以有很多个节名
    const char *pKeyName,	// lpAppName 下的一个键名，也就是里面具体的变量名
    const char *pString,	// 键值,也就是数据
    const char *pFileName); 	// INI文件的路径

  size_t GetPrivateProfileString(
    const char *pAppName,	//  INI文件中的一个字段名[节名]可以有很多个节名
    const char *pKeyName,	// lpAppName 下的一个键名，也就是里面具体的变量名
    const char *pDefault,	// 如果lpReturnedString为空,则把个变量赋给lpReturnedString
    char *pReturnedString,	// 存放键值的指针变量,用于接收INI文件中键值(数据)的接收缓冲区
    size_t nSize,	// lpReturnedString的缓冲区大小
    const char *pFileName); 	//  INI文件的路径

  extern void rmlead(char *str);
  extern void rmtrail(char *str);
  extern int rmquotes(char *str);
  extern int rmbrackets(char *str);

  char *stptok(const char *s, char *tok, size_t toklen, const char *brk);

  #ifdef __cplusplus
  }
  #endif /* __cplusplus */


#endif // !Windows

#endif
