/*
 * GNU GENERAL PUBLIC LICENSE Version 3 (GNU GPL v3)
 * Copyright (c) 2019 Biswapriyo Nath
 * This file is part of wslbridge2 project
 */

#ifndef TERMINALSTATE_HPP
#define TERMINALSTATE_HPP

class TerminalState
{
private:
    std::mutex mutex_;
    bool inRawMode_ = false;
    bool modeValid_[2] = {false, false};
    struct termios mode_[2] = {};

public:
    void enterRawMode();

private:
    void leaveRawMode(const std::lock_guard<std::mutex> &lock);

public:
    void fatal(const char *fmt, ...)
        __attribute__((noreturn))
        __attribute__((format(printf, 2, 3)));
    void fatalv(const char *fmt, va_list ap) __attribute__((noreturn));
    void exitCleanly(int exitStatus);
};

#endif /* TERMINALSTATE_HPP */
