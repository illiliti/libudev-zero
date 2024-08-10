#!/bin/sh -e

if [ "$SUBSYSTEM" = "sound" ]
then
    if printf '%s' "$SYSNAME" | grep '^card' > /dev/null
    then
        printf 'SOUND_INITIALIZED=1\n'
    fi

    if printf '%s' "$SYSNAME" | grep '^pcmC.*D2c$' > /dev/null \
        && grep '^AppleJ' "$SYSPATH/device/id" 2>/dev/null
    then
        printf 'ACP_IGNORE=1\n'
    fi
fi
