// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifdef _WIN32
#include "Windows.h"
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#include <cstdio>
#include <string>

int main(int argc, char **argv)
{
#ifdef _WIN32
  std::string args;
  bool needSpace = false;
  for (int i = 1; i < argc; ++i) {
    if (needSpace) {
      args += ' ';
    }
    args += argv[i];
    needSpace = true;
  }
  STARTUPINFO sInfo;
  GetStartupInfo(&sInfo);
  PROCESS_INFORMATION pInfo;
  if (!CreateProcess(NULL, (LPSTR)args.c_str(), NULL, NULL, FALSE, 0, NULL,
		     NULL, &sInfo, &pInfo)) {
    std::fprintf(stderr, "CreateProcess failed\n");
    return 1;
  }
  WaitForSingleObject(pInfo.hProcess, INFINITE);
  DWORD status;
  GetExitCodeProcess(pInfo.hProcess, &status);
  CloseHandle(pInfo.hProcess);
  CloseHandle(pInfo.hThread);
  return status ? 0 : 1;
#else
  pid_t child_pid = fork();
  switch (child_pid) {
  case -1:
    std::perror("fork failed");
    return 1;
  case 0:
    // Child
    execvp(argv[1], &argv[1]);
    std::perror("execvp failed");
    return 1;
  default:
    // Parent
    int status = 0;
    if (waitpid(child_pid, &status, 0) != child_pid) {
      std::perror("waitpid failed");
    }
    return status ? 0 : 1;
  }
#endif
}
