#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define MAX_COMMAND_LENGTH 100
#define NUM_COMMANDS 6
#define MAX_FILE_CONTENT 10240

// 사용자로부터 한 문자를 입력받는 함수 (실시간 입력 처리)
char getch() {
  struct termios oldt, newt;
  char ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO); // 입력을 하나씩 받도록 설정
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}

// 파일에서 한 문자를 삭제하는 함수
void deleteCharacter(FILE *file, long position) {
  fseek(file, position, SEEK_SET); // 해당 위치로 이동
  fputc(' ', file);                // 그 자리에 공백을 덮어쓰기
  fflush(file);
}

// 파일을 읽고 내용을 화면에 출력하는 함수
void displayFileContent(FILE *file) {
  char line[MAX_FILE_CONTENT];
  while (fgets(line, sizeof(line), file) != NULL) {
    printf("%s", line);
  }
}

// 입력 모드에서 파일을 편집하는 함수
void enterInsertMode(FILE *file, char *filename) {
  char buffer[MAX_FILE_CONTENT];
  int index = 0;
  char ch;

  printf("Entering insert mode. Type 'ESC' to exit insert mode, ':w' to save, "
         "':q' to quit.\n");

  // 기존 파일 내용을 읽어오고 화면에 출력
  fseek(file, 0, SEEK_SET); // 파일 처음으로 이동
  displayFileContent(file);

  while (1) {
    ch = getch(); // 한 문자씩 입력 받음

    // ESC키를 눌렀을 때 입력 모드 종료
    if (ch == 27) { // ESC 키
      printf("\nExiting insert mode.\n");
      break;
    }

    // 입력한 문자를 화면에 출력하고 버퍼에 추가
    if (ch == 8 || ch == 127) { // 백스페이스 (8은 일반 백스페이스, 127은
                                // 윈도우에서의 백스페이스)
      if (index > 0) {
        index--;
        printf("\b \b"); // 화면에서 지우기
      }
    } else {
      buffer[index++] = ch;
      buffer[index] = '\0';
      printf("%c", ch);
    }
  }

  // 파일에 변경 사항 저장
  printf("\nSaving file...\n");
  file = fopen(filename, "a+"); // 파일을 추가 모드로 열기 (덧붙이기)
  if (file == NULL) {
    perror("Unable to open file for saving");
    return;
  }
  fputs(buffer, file); // 기존 내용에 새로운 내용 덧붙이기
  fclose(file);
}

// 명령어 모드에서 명령어 처리하는 함수
void processCommand(FILE *file, const char *command, char *filename) {
  if (strcmp(command, ":w") == 0) {
    // 파일 저장
    printf("\nSaving file...\n");
    fclose(file);
    file = fopen(filename, "a+"); // 파일을 추가 모드로 열기
    if (file == NULL) {
      perror("Unable to open file for saving");
      return;
    }
    fprintf(file, "File saved.\n");
    fclose(file);
  } else if (strcmp(command, ":q") == 0) {
    // 종료
    printf("\nQuitting editor. File contents are saved.\n");
    fclose(file);
    exit(0);
  } else if (strcmp(command, ":wq") == 0) {
    // 저장하고 종료
    printf("\nSaving and quitting...\n");
    fclose(file);
    file = fopen(filename, "a+"); // 파일을 추가 모드로 열기
    if (file == NULL) {
      perror("Unable to open file for saving");
      return;
    }
    fclose(file);
    exit(0);
  } else if (strcmp(command, ":i") == 0) {
    // 입력 모드로 전환
    enterInsertMode(file, filename);
  } else {
    printf("\nUnknown command: %s\n", command);
  }
}

// 기본 명령어 처리 함수
void executeCommand(const char *command) { system(command); }

void makeDirectory(const char *dir) {
  if (mkdir(dir, 0777) != 0) {
    perror("mkdir");
  }
}

void removeDirectory(const char *dir) {
  if (rmdir(dir) != 0) {
    perror("rmdir");
  }
}

void clearScreen() { system("clear"); }

void createFile(const char *filename) {
  FILE *file = fopen(filename, "w");
  if (file == NULL) {
    perror("touch");
  } else {
    fclose(file);
  }
}

int getCurrentDirectory(char *buf, size_t size) {
  if (getcwd(buf, size) == NULL) {
    perror("getcwd");
    return -1;
  }
  return 0;
}

int changeDirectory(const char *path) {
  if (chdir(path) != 0) {
    perror("chdir");
    return -1;
  }
  return 0;
}

// 파일이 존재하는지 확인하는 함수
int fileExists(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file) {
    fclose(file);
    return 1; // 파일이 존재
  }
  return 0; // 파일이 존재하지 않음
}

typedef struct {
  char *name;
  void (*func)(const char *);
} Command;

void processCommandForShell(const Command commands[], const char *input) {
  char command[MAX_COMMAND_LENGTH];
  char argument[MAX_COMMAND_LENGTH];

  // 명령어와 인자를 분리하여 추출
  sscanf(input, "%s %s", command, argument);

  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (strcmp(commands[i].name, command) == 0) {
      commands[i].func(argument);
      return;
    }
  }
  executeCommand(input);
}

// 텍스트 파일 열기
void openFileForEditing(char *filename) {
  FILE *file;
  if (fileExists(filename)) {
    file = fopen(filename, "a+"); // 파일이 존재하면 추가 모드로 열기
  } else {
    printf("File not found. Creating a new file: %s\n", filename);
    file = fopen(filename, "w+"); // 파일이 없으면 새로 생성
  }

  char command[MAX_COMMAND_LENGTH];
  printf("File opened: %s\n", filename);

  // 기본 명령어 안내
  printf("Commands:\n");
  printf(":i  - Enter insert mode\n");
  printf(":w  - Save the file\n");
  printf(":q  - Quit the editor\n");
  printf(":wq - Save and quit\n\n");

  while (1) {
    fgets(command, sizeof(command), stdin);

    // 개행 문자 제거
    command[strlen(command) - 1] = '\0';

    if (command[0] == ':') {
      processCommand(file, command, filename);
    } else {
      printf("Unknown command: %s\n", command);
    }
  }

  fclose(file);
}

int main() {
  Command commands[NUM_COMMANDS] = {
      {"cd", changeDirectory},    {"mkdir", makeDirectory},
      {"rmdir", removeDirectory}, {"clear", clearScreen},
      {"touch", createFile},      {"cd ..", changeDirectory}};

  char command[MAX_COMMAND_LENGTH];
  char currentDirectory[MAX_COMMAND_LENGTH];

  while (1) {
    // 현재 작업 디렉토리를 표시합니다.
    if (getCurrentDirectory(currentDirectory, sizeof(currentDirectory)) == 0) {
      printf("%s $ ", currentDirectory);
    }
    // 사용자로부터 명령어를 입력 받습니다.
    fgets(command, MAX_COMMAND_LENGTH, stdin);

    // 개행 문자 제거
    if (command[strlen(command) - 1] == '\n')
      command[strlen(command) - 1] = '\0';

    if (strncmp(command, "mi ", 3) == 0) {
      openFileForEditing(command + 3); // mi 명령어로 텍스트 편집 시작
    } else {
      processCommandForShell(commands, command);
    }
  }

  return 0;
}
