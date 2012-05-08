/*-
 * Copyright (c) 2012 Darran Hunt (darran [at] hunt dot net dot nz)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * @file WiFly RN-XV ftp methods 
 */

#include "WiFlyHQ.h"

/** Restore the default ftp settings. These
 * are the settings needed to upgrade the firmware.
 * Is this needed? Why not just use factory defaults?
 */
boolean WiFly::setFtpDefaults(void)
{
    setFtpAddress("208.109.78.34");
    setFtpDirectory("public");
    setFtpUser("roving");
    setFtpPassword("Pass123");
    setFtpFilename("wifly-EZX.img");
    setFtpTimer(10000);
    setFtpPort(21);
    setFtpMode(0);

    return true;
}

boolean WiFly::setFtpAddress(const char *addr)
{
    return setopt(PSTR("set ftp address"), addr);
}

boolean WiFly::setFtpPort(uint16_t port)
{
    return setopt(PSTR("set ftp remote"),port);
}

boolean WiFly::setFtpDirectory(const char *dir)
{
    return setopt(PSTR("set ftp dir"), dir);
}

boolean WiFly::setFtpUser(const char *user)
{
    return setopt(PSTR("set ftp user"), user);
}

boolean WiFly::setFtpPassword(const char *password)
{
    return setopt(PSTR("set ftp password"), password);
}

boolean WiFly::setFtpFilename(const char *filename)
{
    return setopt(PSTR("set ftp filename"), filename);
}

boolean WiFly::setFtpTimer(uint16_t msecs)
{
    return setopt(PSTR("set ftp timer"), msecs / 125);
}

boolean WiFly::setFtpMode(uint8_t mode)
{
    return setopt(PSTR("set ftp mode"), mode, HEX);
}

boolean WiFly::ftpGet(
    const char *addr,
    const char *dir,
    const char *user,
    const char *password,
    const char *filename)
{
    setFtpAddress(addr);
    setFtpDirectory(dir);
    setFtpUser(user);
    setFtpPassword(password);

    send_P(PSTR("ftp get "));
    println(filename);

    /* Wait for *OPEN* */
    /* XXX TBD */

    return true;
}

