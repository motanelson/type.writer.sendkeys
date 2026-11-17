

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DELAY 19
#define MAX_FILE_SIZE 10000000

// Set black text and white background
void SetupConsole(void)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(h,
                BACKGROUND_RED |
        BACKGROUND_GREEN |
        BACKGROUND_BLUE |
        BACKGROUND_INTENSITY);

    system("cls");
}

// Send Ctrl+V
void PasteText(void)
{
    INPUT in[4] = {0};

    // CTRL down
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_CONTROL;

    // V down
    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = 'V';

    // V up
    in[2].type = INPUT_KEYBOARD;
    in[2].ki.wVk = 'V';
    in[2].ki.dwFlags = KEYEVENTF_KEYUP;

    // CTRL up
    in[3].type = INPUT_KEYBOARD;
    in[3].ki.wVk = VK_CONTROL;
    in[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, in, sizeof(INPUT));
}

// Send ENTER
void SendEnter(void)
{
    INPUT in[2] = {0};

    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_RETURN;

    in[1] = in[0];
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, in, sizeof(INPUT));
}

// Copy Unicode text to Windows clipboard
int CopyToClipboard(const WCHAR *text, int length)
{
    HGLOBAL memory;
    WCHAR *destination;

    memory = GlobalAlloc(
        GMEM_MOVEABLE,
        ((SIZE_T)length + 1) * sizeof(WCHAR)
    );

    if (!memory)
        return 0;

    destination = (WCHAR *)GlobalLock(memory);

    if (!destination)
    {
        GlobalFree(memory);
        return 0;
    }

    memcpy(destination, text, (SIZE_T)length * sizeof(WCHAR));
    destination[length] = L'\0';

    GlobalUnlock(memory);

    if (!OpenClipboard(NULL))
    {
        GlobalFree(memory);
        return 0;
    }

    EmptyClipboard();

    if (!SetClipboardData(CF_UNICODETEXT, memory))
    {
        CloseClipboard();
        GlobalFree(memory);
        return 0;
    }

    CloseClipboard();

    // Windows now owns the memory
    return 1;
}

int main(void)
{
    char filename[1024];
    int delay;

    FILE *file;
    long filesize;
    size_t bytesRead;

    unsigned char *buffer;
    WCHAR *unicode;
    int unicodeLength;

    SetupConsole();

    printf("====================================\n");
    printf("          TEXT AUTO SENDER\n");
    printf("====================================\n\n");

    printf("File to read: ");
    fflush(stdout);

    if (!fgets(filename, sizeof(filename), stdin))
        return 1;

    filename[strcspn(filename, "\r\n")] = '\0';

    file = fopen(filename, "rb");

    if (!file)
    {
        printf("\nCannot open file!\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    rewind(file);

    if (filesize < 0 || filesize > MAX_FILE_SIZE)
    {
        printf("Invalid file size!\n");
        fclose(file);
        return 1;
    }

    buffer = malloc((size_t)filesize + 1);

    if (!buffer)
    {
        fclose(file);
        return 1;
    }

    bytesRead = fread(buffer, 1, (size_t)filesize, file);
    fclose(file);

    buffer[bytesRead] = '\0';

    // Skip UTF-8 BOM
    size_t offset = 0;

    if (bytesRead >= 3 &&
        buffer[0] == 0xEF &&
        buffer[1] == 0xBB &&
        buffer[2] == 0xBF)
    {
        offset = 3;
    }

    // Convert UTF-8 to UTF-16
    unicodeLength = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        (char *)buffer + offset,
        (int)(bytesRead - offset),
        NULL,
        0
    );

    if (unicodeLength <= 0)
    {
        printf("Invalid UTF-8 file!\n");
        free(buffer);
        return 1;
    }

    unicode = malloc(
        ((SIZE_T)unicodeLength + 1) * sizeof(WCHAR)
    );

    if (!unicode)
    {
        free(buffer);
        return 1;
    }

    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        (char *)buffer + offset,
        (int)(bytesRead - offset),
        unicode,
        unicodeLength
    );

    unicode[unicodeLength] = L'\0';

    free(buffer);

    printf("Delay in seconds (0-19): ");

    if (scanf("%d", &delay) != 1 ||
        delay < 0 || delay > MAX_DELAY)
    {
        printf("Invalid delay!\n");
        free(unicode);
        return 1;
    }

    printf("\nSelect the destination application!\n\n");

    while (delay > 0)
    {
        printf("%d...\n", delay);
        Sleep(1000);
        delay--;
    }

    // Copy complete text to clipboard
    if (!CopyToClipboard(unicode, unicodeLength))
    {
        printf("Clipboard error!\n");
        free(unicode);
        return 1;
    }

    free(unicode);

    // Paste into the application with focus
    PasteText();

    // Allow paste operation to complete
    Sleep(500);

    // Send final ENTER
    SendEnter();

    printf("\nSending completed!\n");

    return 0;
}
