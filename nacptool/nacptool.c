#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <locale.h>
#include <wchar.h>
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

typedef struct {
    char name[0x200];
    char author[0x100];
} NacpLanguageEntry;

typedef struct {
    NacpLanguageEntry lang[16]; // 包含所有16种语言

    u8  x3000_unk[0x24];////Normally all-zero?
    u32 x3024_unk;
    u32 x3028_unk;
    u32 x302C_unk;
    u32 x3030_unk;
    u32 x3034_unk;
    u64 titleid0;

    u8 x3040_unk[0x20];
    char version[0x10];

    u64 titleid_dlcbase;
    u64 titleid1;

    u32 x3080_unk;
    u32 x3084_unk;
    u32 x3088_unk;
    u8 x308C_unk[0x24];//zeros?

    u64 titleid2;
    u64 titleids[7];//"Array of application titleIDs, normally the same as the above app-titleIDs. Only set for game-updates?"

    u32 x30F0_unk;
    u32 x30F4_unk;

    u64 titleid3;//"Application titleID. Only set for game-updates?"

    char bcat_passphrase[0x40];
    u8 x3140_unk[0xEC0];//Normally all-zero?
} NacpStruct;

#ifdef _WIN32
// Windows环境下使用wprintf输出UTF-8内容
void print_nacp_info(NacpStruct *nacp) {
    wprintf(L"NACP File Information:\n");
    wprintf(L"----------------------------------------\n");
    
    // 打印所有语言的名称和作者
    wprintf(L"Application Name and Author (All Languages):\n");
    for (int i = 0; i < 16; i++) {
        if (nacp->lang[i].name[0] != '\0') {
            wprintf(L"Language %d:\n", i);
            wprintf(L"  Name: %hs\n", nacp->lang[i].name);
            wprintf(L"  Author: %hs\n", nacp->lang[i].author);
        }
    }
    
    wprintf(L"----------------------------------------\n");
    wprintf(L"Version: %hs\n", nacp->version);
    
    // 打印TitleID信息
    if (nacp->titleid0) {
        wprintf(L"TitleID: 0x%016" PRIx64 L"\n", nacp->titleid0);
        wprintf(L"DLC Base TitleID: 0x%016" PRIx64 L"\n", nacp->titleid_dlcbase);
    }
}
#else
// 非Windows环境下使用标准printf
void print_nacp_info(NacpStruct *nacp) {
    printf("NACP File Information:\n");
    printf("----------------------------------------\n");
    
    // 打印所有语言的名称和作者
    printf("Application Name and Author (All Languages):\n");
    for (int i = 0; i < 16; i++) {
        if (nacp->lang[i].name[0] != '\0') {
            printf("Language %d:\n", i);
            printf("  Name: %s\n", nacp->lang[i].name);
            printf("  Author: %s\n", nacp->lang[i].author);
        }
    }
    
    printf("----------------------------------------\n");
    printf("Version: %s\n", nacp->version);
    
    // 打印TitleID信息
    if (nacp->titleid0) {
        printf("TitleID: 0x%016" PRIx64 "\n", nacp->titleid0);
        printf("DLC Base TitleID: 0x%016" PRIx64 "\n", nacp->titleid_dlcbase);
    }
}
#endif

#ifdef _WIN32
// 通用的GBK到UTF-8转换函数
char *convert_gbk_to_utf8(const char *input) {
    // 检测是否为空
    if (!input || input[0] == '\0') {
        return strdup("");
    }
    
    // 获取需要的宽字符缓冲区大小
    int wide_size = MultiByteToWideChar(936, 0, input, -1, NULL, 0);
    if (wide_size <= 0) {
        return strdup(input); // 转换失败，返回原始输入
    }
    
    // 分配宽字符缓冲区
    wchar_t *wide_str = (wchar_t*)malloc(wide_size * sizeof(wchar_t));
    if (!wide_str) {
        return strdup(input); // 内存分配失败，返回原始输入
    }
    
    // 转换为宽字符
    if (MultiByteToWideChar(936, 0, input, -1, wide_str, wide_size) <= 0) {
        free(wide_str);
        return strdup(input); // 转换失败，返回原始输入
    }
    
    // 获取UTF-8缓冲区大小
    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, NULL, 0, NULL, NULL);
    if (utf8_size <= 0) {
        free(wide_str);
        return strdup(input); // 转换失败，返回原始输入
    }
    
    // 分配UTF-8缓冲区
    char *utf8_str = (char*)malloc(utf8_size);
    if (!utf8_str) {
        free(wide_str);
        return strdup(input); // 内存分配失败，返回原始输入
    }
    
    // 转换为UTF-8
    if (WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, utf8_str, utf8_size, NULL, NULL) <= 0) {
        free(wide_str);
        free(utf8_str);
        return strdup(input); // 转换失败，返回原始输入
    }
    
    // 释放宽字符缓冲区
    free(wide_str);
    
    return utf8_str;
}
#endif

int main(int argc, char* argv[]) {
    // 设置本地化环境，支持UTF-8
    setlocale(LC_ALL, "");
    
    #ifdef _WIN32
    // 设置Windows控制台为UTF-8模式
    SetConsoleOutputCP(65001);
    // 启用控制台的Unicode处理
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
    #endif
    
    // 检查参数
    if (argc < 2) {
#ifdef _WIN32
        fwprintf(stderr, L"用法:\n");
        fwprintf(stderr, L"%hs --create <name> <author> <version> <outfile> [options]\n", argv[0]);
        fwprintf(stderr, L"%hs --read <nacpfile>\n\n", argv[0]);
        fwprintf(stderr, L"FLAGS:\n");
        fwprintf(stderr, L"--create : 创建用于Switch自制应用的control.nacp文件\n");
        fwprintf(stderr, L"--read : 读取并显示NACP文件的内容\n");
        fwprintf(stderr, L"Options:\n");
        fwprintf(stderr, L"--titleid=<titleID> 设置应用程序的titleID\n");
#else
        fprintf(stderr, "用法:\n");
        fprintf(stderr, "%s --create <name> <author> <version> <outfile> [options]\n", argv[0]);
        fprintf(stderr, "%s --read <nacpfile>\n\n", argv[0]);
        fprintf(stderr, "FLAGS:\n");
        fprintf(stderr, "--create : 创建用于Switch自制应用的control.nacp文件\n");
        fprintf(stderr, "--read : 读取并显示NACP文件的内容\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "--titleid=<titleID> 设置应用程序的titleID\n");
#endif
        return EXIT_FAILURE;
    }
    
    // 处理--read选项
    if (strncmp(argv[1], "--read", 6) == 0) {
        if (argc < 3) {
#ifdef _WIN32
            fwprintf(stderr, L"错误: 缺少NACP文件路径\n");
            fwprintf(stderr, L"用法: %hs --read <nacpfile>\n", argv[0]);
#else
            fprintf(stderr, "错误: 缺少NACP文件路径\n");
            fprintf(stderr, "用法: %s --read <nacpfile>\n", argv[0]);
#endif
            return EXIT_FAILURE;
        }
        
        // 打开NACP文件
        FILE* in = fopen(argv[2], "rb");
        if (in == NULL) {
#ifdef _WIN32
            fwprintf(stderr, L"错误: 无法打开NACP文件 '%hs'\n", argv[2]);
#else
            fprintf(stderr, "错误: 无法打开NACP文件 '%s'\n", argv[2]);
#endif
            return EXIT_FAILURE;
        }
        
        // 读取NACP结构
        NacpStruct nacp;
        size_t read = fread(&nacp, 1, sizeof(nacp), in);
        fclose(in);
        
        if (read != sizeof(nacp)) {
#ifdef _WIN32
            fwprintf(stderr, L"错误: 读取NACP文件失败或文件格式不正确\n");
#else
            fprintf(stderr, "错误: 读取NACP文件失败或文件格式不正确\n");
#endif
            return EXIT_FAILURE;
        }
        
        // 打印NACP信息
        print_nacp_info(&nacp);
        return EXIT_SUCCESS;
    }
    
    // 处理--create选项
    if (argc < 6 || strncmp(argv[1], "--create", 8)!=0) {
#ifdef _WIN32
        fwprintf(stderr, L"%hs --create <name> <author> <version> <outfile> [options]\n\n", argv[0]);
        fwprintf(stderr, L"FLAGS:\n");
        fwprintf(stderr, L"--create : 创建用于Switch自制应用的control.nacp文件\n");
        fwprintf(stderr, L"Options:\n");
        fwprintf(stderr, L"--titleid=<titleID> 设置应用程序的titleID\n");
#else
        fprintf(stderr, "%s --create <name> <author> <version> <outfile> [options]\n\n", argv[0]);
        fprintf(stderr, "FLAGS:\n");
        fprintf(stderr, "--create : 创建用于Switch自制应用的control.nacp文件\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "--titleid=<titleID> 设置应用程序的titleID\n");
#endif
        return EXIT_FAILURE;
    }

    NacpStruct nacp;
    memset(&nacp, 0, sizeof(nacp));

    if (sizeof(NacpStruct) != 0x4000) {
        fprintf(stderr, "Bad compile environment!\n");
        return EXIT_FAILURE;
    }

    char *name_input = argv[2];
    char *author_input = argv[3];
    
    // 打印接收到的参数的十六进制值，用于调试编码
    printf("===DEBUG INFO===\n");
    printf("接收到的name参数: ");
    for (int j = 0; name_input[j] != '\0'; j++) {
        printf("%02X ", (unsigned char)name_input[j]);
    }
    printf("\n");
    
    printf("接收到的author参数: ");
    for (int j = 0; author_input[j] != '\0'; j++) {
        printf("%02X ", (unsigned char)author_input[j]);
    }
    printf("\n");
    
    // 由于在msys2环境中参数已经是UTF-8编码，我们直接使用原始输入
    char *name = name_input;
    char *author = author_input;
    
    printf("使用原始UTF-8编码参数\n");
    
    printf("===DEBUG END===\n");
    
    printf("===Handling input parameters===\n");

    int i;
    
    // 清零整个nacp.lang数组
    memset(nacp.lang, 0, sizeof(nacp.lang));
    
    // 为所有语言设置名称和作者
    for (i = 0; i < 16; i++) {
        strncpy(nacp.lang[i].name, name, sizeof(nacp.lang[i].name) - 1);
        strncpy(nacp.lang[i].author, author, sizeof(nacp.lang[i].author) - 1);
    }
    
    // 确保所有字符串都以null字符结尾
    for (i=0; i<16; i++) {
        nacp.lang[i].name[sizeof(nacp.lang[i].name)-1] = '\0';
        nacp.lang[i].author[sizeof(nacp.lang[i].author)-1] = '\0';
    }

    strncpy(nacp.version, argv[4], sizeof(nacp.version)-1);

    int argi;
    u64 titleid=0;
    for (argi=6; argi<argc; argi++) {
        if (strncmp(argv[argi], "--titleid=", 10)==0) sscanf(&argv[argi][10], "%016" SCNx64, &titleid);
    }

    if (titleid) {
        nacp.titleid0 = titleid;
        nacp.titleid_dlcbase = titleid+0x1000;
        nacp.titleid1 = titleid;
        nacp.titleid2 = titleid;
    }

    u8 unk_data[0x20] = {0x0C, 0xFF, 0xFF, 0x0A, 0xFF, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    nacp.x3024_unk = 0x100;
    nacp.x302C_unk = 0xbff;
    nacp.x3034_unk = 0x10000;

    memcpy(nacp.x3040_unk, unk_data, sizeof(unk_data));

    nacp.x3080_unk = 0x3e00000;
    nacp.x3088_unk = 0x180000;
    nacp.x30F0_unk = 0x102;

    FILE* out = fopen(argv[5], "wb");

    if (out == NULL) {
        fprintf(stderr, "Failed to open output file!\n");
        return EXIT_FAILURE;
    }

    fwrite(&nacp, sizeof(nacp), 1, out);

    fclose(out);
    
    #ifdef _WIN32
    // 释放内存
    free(name);
    free(author);
    #endif

    return EXIT_SUCCESS;
}
